/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "teleporter.h"
#include "plasma.h"
#include "monster.h"

CTeleporter::CTeleporter(CGameWorld *pGameWorld, vec2 Pos, int Owner, CTeleporter *Next)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_TELEPORTER)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_StartTick = Server()->Tick();
	m_Next = Next;

	GameWorld()->InsertEntity(this);
}

void CTeleporter::Tick()
{
	if (!m_Next)
		return;

	CCharacter *apEntCharacters[MAX_CLIENTS] = {0};
	int Num = GameServer()->m_World.FindEntities(m_Pos, 6.0f, (CEntity**)apEntCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
	for(int i = 0; i < Num; i++)
	{
		if(!apEntCharacters[i])
			continue;

		Teleport(apEntCharacters[i], true);
	}

	CMonster *apEntMonters[MAX_MONSTERS] = {0};
	Num = GameServer()->m_World.FindEntities(m_Pos, 6.0f, (CEntity**)apEntMonters, MAX_MONSTERS, CGameWorld::ENTTYPE_MONSTER);
	for(int i = 0; i < Num; i++)
	{
		if(!apEntMonters[i])
			continue;

		Teleport(apEntMonters[i], false);
	}
}

void CTeleporter::Teleport(CEntity *pEnt, bool isCharacter)
{
	vec2 alea(0,0);
	vec2 To(0,0);
	do
	{
		alea.x = (rand() % 201) - 100;
		alea.y = (rand() % 201) - 100;
		To.x = m_Next->m_Pos.x + alea.x;
		To.y = m_Next->m_Pos.y + alea.y;
	}
	while (GameServer()->Collision()->TestBox(To, vec2(pEnt->m_ProximityRadius, pEnt->m_ProximityRadius)) || ((alea.x > 0 && alea.x < pEnt->m_ProximityRadius+6.0f) || (alea.x < 0 && alea.x > -(pEnt->m_ProximityRadius+6.0f))) || GameServer()->Collision()->IntersectLine(m_Next->m_Pos, To, 0, 0));

	if (isCharacter)
		reinterpret_cast<CCharacter*>(pEnt)->SetPos(To);
	else
		pEnt->m_Pos = To;
}

void CTeleporter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(!pProj)
		return;

	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(0);
	pProj->m_VelY = (int)(0);
	pProj->m_StartTick = Server()->Tick();
	pProj->m_Type = WEAPON_GRENADE;
}
