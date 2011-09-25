/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/event.h>
#include "plasma.h"

CPlasma::CPlasma(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	GameWorld()->InsertEntity(this);
	Tick();
}


bool CPlasma::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);
	if(!pHit)
		return false;

	m_Pos = At;
	if ( !GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) )
		m_Energy = -1;
	if ( !GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) || m_Bounces > 0 )
		pHit->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE);
	return true;
}

void CPlasma::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CPlasma::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*100)/1000.0f)
		return;

	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	vec2 To = m_Pos + m_Dir;
	m_Energy -= distance(m_Pos, To);

	if(GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
	{
		if(!HitCharacter(m_Pos, To))
		{
			// intersected
			m_Pos = To;

			if ( !GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) )
			{
				vec2 TempPos = m_Pos;
				vec2 TempDir = m_Dir * 4.0f;

				GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
				m_Pos = TempPos;
				m_Dir = normalize(TempDir);

				m_Bounces++;
				m_Energy -= GameServer()->Tuning()->m_LaserBounceCost;

				if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
					m_Energy = -1;

				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
				GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false);
			}
		}
	}
	else
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_Pos = To;
			m_Energy = -1;
		}
	}
}

void CPlasma::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_Pos.x;
	pObj->m_FromY = (int)m_Pos.y;
	pObj->m_StartTick = Server()->Tick();
}
