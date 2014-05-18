/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/statistics/statistiques.h>
#include <game/server/event.h>
#include "turret.h"
#include "plasma.h"
#include "explodewall.h"

CTurret::CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner)
	: IEntityDamageable(pGameWorld, CGameWorld::ENTTYPE_TURRET)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_StartTick = Server()->Tick();
	m_LastTick = m_StartTick;
	m_Health = 120 * GameServer()->m_apPlayers[m_Owner]->m_pStats->GetStatLife().m_protection;
	m_DamageTaken = 0;
	m_DamageTakenTick = 0;
	m_Destroy = false;
	m_ProximityRadius = 15.0f;
	GameWorld()->InsertEntity(this);
}

void CTurret::Tick()
{
	if (!GameServer()->m_apPlayers[m_Owner] || (Server()->Tick()-m_LastTick) < 1125 * Server()->TickSpeed() / (1000.f * max(1.875f, GameServer()->m_apPlayers[m_Owner]->m_pStats->GetStatWeapon().m_speed)))
		return;

	m_LastTick = Server()->Tick();

	bool IgnoreTeam = !GameServer()->m_pController->IsTeamplay() ||
					  GameServer()->m_pEventsGame->GetActualEventTeam() == GUN_HEAL ||
					  GameServer()->m_pEventsGame->GetActualEventTeam() == CAN_HEAL ||
					  GameServer()->m_pEventsGame->GetActualEventTeam() == GUN_KILL ||
					  GameServer()->m_pEventsGame->GetActualEventTeam() == CAN_KILL;

	float ClosestLen = -1;
	float Len = 0;
	vec2 TargetPos;

	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	IEntityDamageable *apEnts[MAX_ENTITIES_DAMAGEABLE];
	int Num = GameServer()->m_World.FindEntitiesDamageable(m_Pos, 450.0f, apEnts, MAX_ENTITIES_DAMAGEABLE);
	for(int i = 0; i < Num; i++)
	{
		if (OwnerChar == apEnts[i])
			continue;

		if (apEnts[i]->GetType() == CGameWorld::ENTTYPE_TURRET && reinterpret_cast<CTurret*>(apEnts[i])->GetOwner() == m_Owner)
			continue;

		if (apEnts[i]->GetType() == CGameWorld::ENTTYPE_EXPLODEWALL && reinterpret_cast<CExplodeWall*>(apEnts[i])->GetOwner() == m_Owner)
			continue;

		if (apEnts[i]->GetType() == CGameWorld::ENTTYPE_MONSTER || IgnoreTeam ||
		   (apEnts[i]->GetType() == CGameWorld::ENTTYPE_CHARACTER &&
			GameServer()->m_apPlayers[m_Owner]->GetTeam() != reinterpret_cast<CCharacter*>(apEnts[i])->GetPlayer()->GetTeam()))
		{
			vec2 EntPos = apEnts[i]->m_Pos;
			if (apEnts[i]->GetType() == CGameWorld::ENTTYPE_EXPLODEWALL)
				EntPos = closest_point_on_line(reinterpret_cast<CExplodeWall*>(apEnts[i])->m_From, apEnts[i]->m_Pos, m_Pos);

			if ((ClosestLen > (Len = distance(m_Pos, EntPos)) || ClosestLen == -1)
					&& !GameServer()->Collision()->IntersectLine(m_Pos, EntPos, 0, 0))
			{
				ClosestLen = Len;
				TargetPos = EntPos;
			}
		}
	}

	vec2 Direction(0,0);
	if (ClosestLen != -1)
		Direction = normalize(TargetPos - m_Pos);
	else
		return;

	new CPlasma(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_Owner);
}

bool CTurret::TakeDamage(vec2 Force, int Dmg, int From, int Weapon, bool Instagib, bool FromMonster)
{
	if (GameServer()->m_pEventsGame->IsActualEvent(INSTAGIB))
		Instagib = true;

	if (!FromMonster)
	{
		int FromRace = 0;
		if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_HUMAN) )
			FromRace = HUMAN;
		else if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_GNOME) )
			FromRace = GNOME;
		else if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_ORC) )
			FromRace = ORC;
		else if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_ELF) )
			FromRace = ELF;
		else if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_RANDOM) )
			FromRace = (rand() % ((ELF + 1) - HUMAN)) + HUMAN;
		else if ( GameServer()->m_apPlayers[From] )
			FromRace = GameServer()->m_apPlayers[From]->m_WeaponType[Weapon];

		if (FromRace == ORC && Weapon == WEAPON_RIFLE)
			Instagib = true;
	}

	if(!FromMonster && GameServer()->m_pController->IsFriendlyFire(m_Owner, From, Weapon) && !g_Config.m_SvTeamdamage)
		return false;

	if(!FromMonster && From == m_Owner)
		return false;

	if (GameServer()->m_pEventsGame->IsActualEvent(PROTECT_X2))
		Dmg = max(1, Dmg/2);

	if (!Instagib)
		m_Health -= Dmg;
	else
		m_Health -= Dmg * 5;

	m_DamageTaken++;

	// create healthmod indicator
	if(Server()->Tick() < m_DamageTakenTick+25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
	}
	else
	{
		m_DamageTaken = 0;
		GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
	}

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(!FromMonster && From >= 0 && From != m_Owner && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(m_Pos, SOUND_HIT, Mask);
	}

	// check for death
	if(m_Health <= 0)
	{
		m_Destroy = true;

		// set attacker's face to happy (taunt!)
		if (!FromMonster && From >= 0 && From != m_Owner && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
				pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
		}

		return true;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	return true;
}

void CTurret::Snap(int SnappingClient)
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
