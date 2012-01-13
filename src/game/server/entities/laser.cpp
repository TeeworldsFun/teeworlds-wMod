/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/event.h>
#include "laser.h"
#include "turret.h"
#include "teleporter.h"
#include "explodewall.h"

CLaser::CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
    m_Pos = Pos;
    m_Owner = Owner;
    m_Energy = StartEnergy;
    m_Dir = Direction;
    m_Bounces = 0;
    m_EvalTick = 0;
    GameWorld()->InsertEntity(this);
    DoBounce();
}


bool CLaser::HitCharacter(vec2 From, vec2 To)
{
    vec2 At;
    vec2 TempPos;
    
    float ClosestLen = -1;

    CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
    CCharacter *pHit = GameServer()->m_World.IntersectCharacter(From, To, 0.0f, TempPos, pOwnerChar);
    if (pHit)
    {
        ClosestLen = distance(From, TempPos);
        At = TempPos;
    }

    CTurret *pHitTurret = (CTurret*) GameServer()->m_World.IntersectEntity(From, To, 0.0f, TempPos, CGameWorld::ENTTYPE_TURRET);
    if ( pHitTurret && ClosestLen > distance(From, TempPos) && (ClosestLen > distance(From, TempPos) || ClosestLen == -1) )
    {
        ClosestLen = distance(From, TempPos);
        At = TempPos;
        pHit = 0;
    }
    else if ( pHitTurret )
        pHitTurret = 0;

    CExplodeWall *pHitExplodeWall = 0;

    {
        CExplodeWall *p = (CExplodeWall *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_EXPLODEWALL);
        for(; p; p = (CExplodeWall *)p->TypeNext())
        {
            // Store the values for fast access and easy
            // equations-to-code conversion
            float x1 = From.x, x2 = To.x, x3 = p->m_From.x, x4 = p->m_Pos.x;
            float y1 = From.y, y2 = To.y, y3 = p->m_From.y, y4 = p->m_Pos.y;

            float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
            // If d is zero, there is no intersection
            if (d == 0)
                continue;

            // Get the x and y
            float pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
            float x = ( pre * (x3 - x4) - (x1 - x2) * post ) / d;
            float y = ( pre * (y3 - y4) - (y1 - y2) * post ) / d;

            // Check if the x and y coordinates are within both lines
            if ( x < min(x1, x2) || x > max(x1, x2) || x < min(x3, x4) || x > max(x3, x4) )
                continue;
            if ( y < min(y1, y2) || y > max(y1, y2) || y < min(y3, y4) || y > max(y3, y4) )
                continue;

            if ( ClosestLen > distance(From, vec2(x, y)) || ClosestLen == -1 )
            {
                ClosestLen = distance(From, vec2(x, y));
                At.x = x;
                At.y = y;
                pHit = 0;
                pHitTurret = 0;
                pHitExplodeWall = p;
                break;
            }
        }
    }

    if(!pHit && !pHitTurret && !pHitExplodeWall)
        return false;

    m_From = From;
    m_Pos = At;
    m_Energy = -1;

    if ( pHit && (!GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) || m_Bounces > 0 || GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING)) )
        pHit->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE, false);
    else if ( pHitTurret && (!GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) || m_Bounces > 0 || GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING)) )
        pHitTurret->TakeDamage(GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE, false);
    else if ( pHitExplodeWall && (!GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) || m_Bounces > 0 || GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING)) )
        pHitExplodeWall->TakeDamage(GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE, false);

    return true;
}

void CLaser::DoBounce()
{
    m_EvalTick = Server()->Tick();

    if(m_Energy < 0)
    {
        GameServer()->m_World.DestroyEntity(this);
        return;
    }

    vec2 To = m_Pos + m_Dir * m_Energy;

    vec2 At;
    CTeleporter *pTeleporter = (CTeleporter *)GameServer()->m_World.IntersectEntity(m_Pos, To, 12.0f, At, CGameWorld::ENTTYPE_TELEPORTER);
    if (pTeleporter && pTeleporter->GetNext() && m_Pos == pTeleporter->m_Pos)
    {
        m_Pos = pTeleporter->GetNext()->m_Pos + m_Dir;
        To = m_Pos + m_Dir * m_Energy;
    }
    else if (pTeleporter && pTeleporter->GetNext())
    {
        if(!HitCharacter(m_Pos, pTeleporter->m_Pos))
        {
            m_From = m_Pos;
            m_Pos = pTeleporter->m_Pos;
            m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
            return;
        }
    } 

    if(!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) && !GameServer()->m_pEventsGame->IsActualEvent(BULLET_GLUE) && GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
    {
        if(!HitCharacter(m_Pos, To))
        {
            // intersected
            m_From = m_Pos;
            m_Pos = To;
            
            vec2 TempPos = m_Pos;
            vec2 TempDir = m_Dir * 4.0f;

            GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
            m_Pos = TempPos;
            m_Dir = normalize(TempDir);

            m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
            m_Bounces++;

            if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
                m_Energy = -1;

            GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
            GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false);
        }
    }
    else if (GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING))
    {
        if(!HitCharacter(m_Pos, To))
        {        
            m_From = m_Pos;
            m_Pos = To;
            m_Energy = -1;
            
            GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
            GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false);
        }
    }
    else if (GameServer()->m_pEventsGame->IsActualEvent(BULLET_GLUE) && GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
    {
        if(!HitCharacter(m_Pos, To))
        {
            m_From = m_Pos;
            m_Pos = To;
            m_Energy -= (m_Energy / 2) + 2;
            GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
            GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false);
        }
    }
    else
    {
        if(!HitCharacter(m_Pos, To))
        {
            m_From = m_Pos;
            m_Pos = To;
            m_Energy = -1;
            GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
            GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false);
        }
    }
}

void CLaser::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
}

void CLaser::Tick()
{
    if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
        DoBounce();
}

void CLaser::TickPaused()
{
	++m_EvalTick;
}

void CLaser::Snap(int SnappingClient)
{
    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
    if(!pObj)
        return;

    pObj->m_X = (int)m_Pos.x;
    pObj->m_Y = (int)m_Pos.y;
    pObj->m_FromX = (int)m_From.x;
    pObj->m_FromY = (int)m_From.y;
    pObj->m_StartTick = m_EvalTick;
}
