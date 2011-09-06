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
	GameWorld()->RemoveEntity(this);
}

void CLoot::Tick()
{
	CPickup::Tick();

	if(m_SpawnTick > 0)
	{
		GameWorld()->RemoveEntity(this);
	}
	else 
	{
		m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
		GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(PickupPhysSize, PickupPhysSize), 0.5f);
	}
}

