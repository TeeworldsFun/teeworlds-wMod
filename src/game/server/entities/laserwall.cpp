/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/event.h>
#include "laserwall.h"

CLaserWall::CLaserWall(CGameWorld *pGameWorld, vec2 StartPos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_From = StartPos;
	m_Pos = m_From;
	m_Owner = Owner;
	m_StartTick = 0;
	GameServer()->m_apPlayers[m_Owner]->m_NumLaserWall++;
	GameWorld()->InsertEntity(this);
}

void CLaserWall::Reset()
{
	if(GameServer()->m_apPlayers[m_Owner])
		GameServer()->m_apPlayers[m_Owner]->m_NumLaserWall--;
	GameServer()->m_World.DestroyEntity(this);
}

void CLaserWall::Tick()
{
	if( m_Killed >= 5 || GameServer()->Collision()->IntersectLine(m_From, m_Pos, 0x0, 0x0) || (m_StartTick && Server()->Tick() >= m_StartTick+Server()->TickSpeed()*30) || !GameServer()->m_apPlayers[m_Owner] || !GameServer()->m_apPlayers[m_Owner]->GetCharacter() )
	{
		Reset();
		return;
	}

	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, m_From, 0.f, At, pOwnerChar);
	if(!pHit)
		return;

	pHit->Die(m_Owner, WEAPON_HAMMER);
	m_Killed++;
	if(pOwnerChar)
		pOwnerChar->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
}

void CLaserWall::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = Server()->Tick() + 1;
}

