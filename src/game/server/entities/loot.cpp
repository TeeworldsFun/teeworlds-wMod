/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include "loot.h"

CLoot::CLoot(CGameWorld *pGameWorld, int Type, int SubType)
: CPickup(pGameWorld, Type, SubType)
{
	m_SpawnTick = -1;
	m_Vel = vec2(0,0);
}

void CLoot::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CLoot::Tick()
{
	CPickup::Tick();

	if(m_SpawnTick > 0)
	{
		GameWorld()->DestroyEntity(this);
	}
	else 
	{
		m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
		GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(PickupPhysSize, PickupPhysSize), 0.5f);
	}

	if(GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameLayerClipped(m_Pos))
	{
		Reset();
	}
}

void CLoot::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y - 32;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}

