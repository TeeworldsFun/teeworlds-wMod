/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/

#include "gameworld.h"
#include "entity.h"
#include "entity_damageable.h"
#include "gamecontext.h"
#include "entities/explodewall.h"
#include "entities/monster.h"

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
CGameWorld::CGameWorld()
{
	m_pGameServer = 0x0;
	m_pServer = 0x0;

	m_Paused = false;
	m_ResetRequested = false;
	for(int i = 0; i < NUM_ENTTYPES; i++)
		m_apFirstEntityTypes[i] = 0;
}

CGameWorld::~CGameWorld()
{
	// delete all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		while(m_apFirstEntityTypes[i])
			delete m_apFirstEntityTypes[i];
}

void CGameWorld::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
}

CEntity *CGameWorld::FindFirst(int Type)
{
	return Type < 0 || Type >= NUM_ENTTYPES ? 0 : m_apFirstEntityTypes[Type];
}

int CGameWorld::FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type)
{
	if(Type < 0 || Type >= NUM_ENTTYPES)
		return 0;

	int Num = 0;
	if (Type != ENTTYPE_EXPLODEWALL)
	{
		for(CEntity *pEnt = m_apFirstEntityTypes[Type];	pEnt; pEnt = pEnt->m_pNextTypeEntity)
		{
			if(distance(pEnt->m_Pos, Pos) < Radius+pEnt->m_ProximityRadius)
			{
				if(ppEnts)
					ppEnts[Num] = pEnt;
				Num++;
				if(Num == Max)
					break;
			}
		}
	}
	else
	{
		for(CEntity *pEnt = m_apFirstEntityTypes[Type];	pEnt; pEnt = pEnt->m_pNextTypeEntity)
		{
            if(distance(closest_point_on_line(reinterpret_cast<CExplodeWall*>(pEnt)->m_From, pEnt->m_Pos, Pos), Pos) < Radius+pEnt->m_ProximityRadius)
			{
				if(ppEnts)
					ppEnts[Num] = pEnt;
				Num++;
				if(Num == Max)
					break;
			}
		}
	}

	return Num;
}

int CGameWorld::FindEntitiesDamageable(vec2 Pos, float Radius, IEntityDamageable **ppEnts, int Max)
{
	int Num = 0;
	for (int i = ENTTYPE_CHARACTER; i <= ENTTYPE_EXPLODEWALL; i++)
	{
		CEntity *ppEntsTemp[Max];
		int NumTemp = FindEntities(Pos, Radius, ppEntsTemp, Max, i);
		for (int j = 0; j < NumTemp; j++)
		{
			if (Num == Max)
				return Num;

			ppEnts[Num] = reinterpret_cast<IEntityDamageable*>(ppEntsTemp[j]);
			Num++;
		}
	}

	return Num;
}

void CGameWorld::InsertEntity(CEntity *pEnt)
{
#ifdef CONF_DEBUG
	for(CEntity *pCur = m_apFirstEntityTypes[pEnt->m_ObjType]; pCur; pCur = pCur->m_pNextTypeEntity)
		dbg_assert(pCur != pEnt, "err");
#endif

	// insert it
	if(m_apFirstEntityTypes[pEnt->m_ObjType])
		m_apFirstEntityTypes[pEnt->m_ObjType]->m_pPrevTypeEntity = pEnt;
	pEnt->m_pNextTypeEntity = m_apFirstEntityTypes[pEnt->m_ObjType];
	pEnt->m_pPrevTypeEntity = 0x0;
	m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt;
}

void CGameWorld::DestroyEntity(CEntity *pEnt)
{
	pEnt->m_MarkedForDestroy = true;
}

void CGameWorld::RemoveEntity(CEntity *pEnt)
{
	// not in the list
	if(!pEnt->m_pNextTypeEntity && !pEnt->m_pPrevTypeEntity && m_apFirstEntityTypes[pEnt->m_ObjType] != pEnt)
		return;

	// remove
	if(pEnt->m_pPrevTypeEntity)
		pEnt->m_pPrevTypeEntity->m_pNextTypeEntity = pEnt->m_pNextTypeEntity;
	else
		m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt->m_pNextTypeEntity;
	if(pEnt->m_pNextTypeEntity)
		pEnt->m_pNextTypeEntity->m_pPrevTypeEntity = pEnt->m_pPrevTypeEntity;

	// keep list traversing valid
	if(m_pNextTraverseEntity == pEnt)
		m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;

	pEnt->m_pNextTypeEntity = 0;
	pEnt->m_pPrevTypeEntity = 0;
}

//
void CGameWorld::Snap(int SnappingClient)
{
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Snap(SnappingClient);
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Reset()
{
	// reset all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Reset();
			pEnt = m_pNextTraverseEntity;
		}
	RemoveEntities();

	GameServer()->m_pController->PostReset();
	RemoveEntities();

	m_ResetRequested = false;
}

void CGameWorld::RemoveEntities()
{
	// destroy objects marked for destruction
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			if(pEnt->m_MarkedForDestroy)
			{
				RemoveEntity(pEnt);
				pEnt->Destroy();
			}
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Tick()
{
	if(m_ResetRequested)
		Reset();

	if(!m_Paused)
	{
		if(GameServer()->m_pController->IsForceBalanced())
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Teams have been balanced");
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->Tick();
				pEnt = m_pNextTraverseEntity;
			}

		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickDefered();
				pEnt = m_pNextTraverseEntity;
			}
	}
	else
	{
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickPaused();
				pEnt = m_pNextTraverseEntity;
			}
	}

	RemoveEntities();
}


// TODO: should be more general
CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		float Len = distance(p->m_Pos, IntersectPos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}

CMonster *CGameWorld::IntersectMonster(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other monsters
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CMonster *pClosest = 0;

	CMonster *p = (CMonster *)FindFirst(ENTTYPE_MONSTER);
	for(; p; p = (CMonster *)p->TypeNext())
	{
		if(p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		float Len = distance(p->m_Pos, IntersectPos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}

bool intersectionBetweenTwoSegments(vec2 From1, vec2 To1, vec2 From2, vec2 To2, vec2& At)
{
    // Store the values for fast access and easy
    // equations-to-code conversion
    float x1 = From1.x, x2 = To1.x, x3 = From2.x, x4 = To2.x;
    float y1 = From1.y, y2 = To1.y, y3 = From2.y, y4 = To2.y;

    float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    // If d is zero, there is no intersection
    if (d == 0)
        return false;

    // Get the x and y
    float pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
    float x = ( pre * (x3 - x4) - (x1 - x2) * post ) / d;
    float y = ( pre * (y3 - y4) - (y1 - y2) * post ) / d;

    // Check if the x and y coordinates are within both lines
    if ( x < min(x1, x2) || x > max(x1, x2) || x < min(x3, x4) || x > max(x3, x4) )
        return false;
    if ( y < min(y1, y2) || y > max(y1, y2) || y < min(y3, y4) || y > max(y3, y4) )
        return false;

    At.x = x;
    At.y = y;
    return true;
}

CExplodeWall *CGameWorld::IntersectExplodeWall(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other explodewalls
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
    CExplodeWall *pClosest = 0;
    bool Intersect = false;

    CExplodeWall *p = (CExplodeWall *)FindFirst(CGameWorld::ENTTYPE_EXPLODEWALL);
	for(; p; p = (CExplodeWall *)p->TypeNext())
	{
		if(p == pNotThis)
			continue;

        vec2 IntersectPos;
        if (!intersectionBetweenTwoSegments(Pos0, Pos1, p->m_From, p->m_Pos, IntersectPos) && !Intersect)
        {
            vec2 At[4] = {closest_point_on_line(p->m_From, p->m_Pos, Pos0),
                         closest_point_on_line(p->m_From, p->m_Pos, Pos1),
                         closest_point_on_line(Pos0, Pos1, p->m_From),
                         closest_point_on_line(Pos0, Pos1, p->m_Pos)};

            float Len[4] = {distance(Pos0, At[0]),
                            distance(Pos1, At[1]),
                            distance(p->m_From, At[2]),
                            distance(p->m_Pos, At[3])};

            for (int i = 0; i < 4; i++)
            {
                if(Len[i] < p->m_ProximityRadius+Radius)
                {
                    if (Len[i] < ClosestLen)
                    {
                        NewPos = At[i];
                        ClosestLen = Len[i];
                        pClosest = p;
                    }
                }
            }
        }
        else
        {
            float Len = distance(Pos0, IntersectPos);
            if (!Intersect || Len < ClosestLen)
            {
                NewPos = IntersectPos;
                ClosestLen = Len;
                pClosest = p;
                Intersect = true;
            }
        }
	}

	return pClosest;
}

CEntity *CGameWorld::IntersectEntity(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, int Type, CEntity *pNotThis)
{
	if(Type < 0 || Type >= NUM_ENTTYPES)
		return 0;

	if(Type == ENTTYPE_EXPLODEWALL)
		return IntersectExplodeWall(Pos0, Pos1, Radius, NewPos, pNotThis);

	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CEntity *pClosest = 0;

	CEntity *p = (CEntity *)FindFirst(Type);
	for(; p; p = (CEntity *)p->TypeNext())
	{
		if(p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		float Len = distance(p->m_Pos, IntersectPos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}

IEntityDamageable *CGameWorld::IntersectEntityDamageable(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
    IEntityDamageable *pClosest = 0;
    for (int i = 0; i < 4; i++)
    {
        vec2 At;
        IEntityDamageable *p = (IEntityDamageable*)IntersectEntity(Pos0, Pos1, Radius, At, ENTTYPE_CHARACTER + i, pNotThis);
        if (!p)
            continue;

        float Len = distance(p->m_Pos, At);
        if (i == 3)
            Len = distance(closest_point_on_line(((CExplodeWall*)p)->m_From, p->m_Pos, At), At);

        if (Len < ClosestLen)
        {
            NewPos = At;
            ClosestLen = Len;
            pClosest = p;
        }
    }

	return pClosest;
}

CCharacter *CGameWorld::ClosestCharacter(vec2 Pos, float Radius, CEntity *pNotThis, bool IgnoreCollide)
{
	// Find other players
	float ClosestRange = Radius*2;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		if(!IgnoreCollide)
		{
			if(GameServer()->Collision()->IntersectLine2(Pos, p->m_Pos))
				continue;
		}

		float Len = distance(Pos, p->m_Pos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			if(Len < ClosestRange)
			{
				ClosestRange = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}
