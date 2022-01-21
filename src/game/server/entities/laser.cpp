/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/event.h>
#include "laser.h"
#include "turret.h"
#include "teleporter.h"
#include "explodewall.h"
#include "monster.h"

CLaser::CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, bool FromMonster)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_FromMonster = FromMonster;
	GameWorld()->InsertEntity(this);
	DoBounce();
}


bool CLaser::HitCharacter(vec2 From, vec2 To)
{
    vec2 At;

    CEntity *pOwnerChar = !m_FromMonster ? (CEntity*)GameServer()->GetPlayerChar(m_Owner) : (CEntity*)GameServer()->GetValidMonster(m_Owner);
    IEntityDamageable *pTarget = GameServer()->m_World.IntersectEntityDamageable(From, To, 0.0f, At, pOwnerChar);

	if(!pTarget)
		return false;

	m_From = From;
	m_Pos = At;
	m_Energy = -1;

	int Damage = 0;
	if(m_FromMonster && GameServer()->GetValidMonster(m_Owner))
		Damage += GameServer()->GetValidMonster(m_Owner)->GetDifficulty() - 1;

	if (!GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) || m_Bounces > 0 || GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING))
		pTarget->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage + Damage, m_Owner, WEAPON_RIFLE, false, m_FromMonster);

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
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false, m_FromMonster);
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
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false, m_FromMonster);
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
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false, m_FromMonster);
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
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false, m_FromMonster);
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
