/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/event.h>
#include "laserwall.h"
#include "turret.h"
#include "monster.h"

CLaserWall::CLaserWall(CGameWorld *pGameWorld, vec2 StartPos, int Owner, bool Double)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_From = StartPos;
	m_Pos = m_From;
	m_Owner = Owner;
	m_Killed = 0;
	m_StartTick = 0;
	m_Destroy = false;
	m_Double = Double;
	m_pDouble = 0;
	GameWorld()->InsertEntity(this);
}

CLaserWall::~CLaserWall()
{
	if (m_pDouble)
	{
		delete m_pDouble;
		m_pDouble = 0;
	}
}

void CLaserWall::Tick()
{
	if ( m_Double )
		return;

	if( m_Killed >= 5 || (distance(m_From, m_Pos) && GameServer()->Collision()->IntersectLine(m_From, m_Pos, 0, 0)) || (m_StartTick && Server()->Tick() >= m_StartTick+Server()->TickSpeed()*30))
	{
		m_Destroy = true;
		return;
	}

	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, m_From, 0.f, At, pOwnerChar);

	if(pHit && !GameServer()->m_pController->IsFriendlyFire(m_Owner, pHit->GetPlayer()->GetCID(), WEAPON_RIFLE) && pHit->m_Protect != 0 && pHit->TakeDamage(vec2(0,0), 10, m_Owner, WEAPON_HAMMER, true))
		m_Killed++;

	CMonster *pHit2 = GameServer()->m_World.IntersectMonster(m_Pos, m_From, 0.f, At, 0);
	if (pHit2)
		pHit2->TakeDamage(vec2(0,0), 10, m_Owner, WEAPON_HAMMER, true);

	CTurret *pHitTurret = (CTurret *)GameServer()->m_World.IntersectEntity(m_Pos, m_From, 0.f, At, CGameWorld::ENTTYPE_TURRET);
	if (pHitTurret)
		pHitTurret->TakeDamage(vec2(0,0), 10, m_Owner, WEAPON_HAMMER, true);
}

void CLaserWall::CreateDouble()
{
	if (m_Double || m_pDouble)
		return;

	m_pDouble = new CLaserWall(&GameServer()->m_World, m_Pos, m_Owner, true);
	m_pDouble->m_Pos = m_From;
}

void CLaserWall::Snap(int SnappingClient)
{
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = Server()->Tick() + 1;
}

