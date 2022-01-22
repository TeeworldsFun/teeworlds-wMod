/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include "gamecontext.h"
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include "gamemodes/dm.h"
#include "gamemodes/tdm.h"
#include "gamemodes/ctf.h"
#include "gamemodes/mod.h"
#include "entities/projectile.h"
#include "entities/turret.h"
#include "entities/explodewall.h"
#include "entities/monster.h"
#include "statistics/statistiques.h"
#include "statistics/mysqlserver.h"
#include "event.h"

enum
{
	RESET,
	NO_RESET
};

void CGameContext::Construct(int Resetting)
{
	m_Resetting = 0;
	m_pServer = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0;

	for(int i = 0; i < MAX_MONSTERS; i++)
		m_apMonsters[i] = 0;

	m_pController = 0;
	m_VoteCloseTime = 0;
	m_pVoteOptionFirst = 0;
	m_pVoteOptionLast = 0;
	m_NumVoteOptions = 0;
	m_LockTeams = 0;

	m_pEventsGame = new CEvent(this);
	if(Resetting==NO_RESET)
	{
#if defined(CONF_SQL)
		m_pStatsServer = new CSqlServer(this);
#else
		m_pStatsServer = 0;
#endif
		m_pVoteOptionHeap = new CHeap();
	}
}

CGameContext::CGameContext(int Resetting)
{
	Construct(Resetting);
}

CGameContext::CGameContext()
{
	Construct(NO_RESET);
}

CGameContext::~CGameContext()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		delete m_apPlayers[i];
	for(int i = 0; i < MAX_MONSTERS; i++)
		delete m_apMonsters[i];
	delete m_pEventsGame;
	if(!m_Resetting)
	{
		delete m_pVoteOptionHeap;
#if defined(CONF_SQL)
		delete m_pStatsServer;
#endif
	}
}

void CGameContext::Clear()
{
	CHeap *pVoteOptionHeap = m_pVoteOptionHeap;
	CVoteOptionServer *pVoteOptionFirst = m_pVoteOptionFirst;
	CVoteOptionServer *pVoteOptionLast = m_pVoteOptionLast;
	int NumVoteOptions = m_NumVoteOptions;
	CTuningParams Tuning = m_Tuning;
	IStatsServer *pStatsServer = m_pStatsServer;

	m_Resetting = true;
	this->~CGameContext();
	mem_zero(this, sizeof(*this));
	new (this) CGameContext(RESET);

	m_pVoteOptionHeap = pVoteOptionHeap;
	m_pVoteOptionFirst = pVoteOptionFirst;
	m_pVoteOptionLast = pVoteOptionLast;
	m_NumVoteOptions = NumVoteOptions;
	m_Tuning = Tuning;
	m_pStatsServer = pStatsServer;
}


class CCharacter *CGameContext::GetPlayerChar(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return 0;
	return m_apPlayers[ClientID]->GetCharacter();
}

void CGameContext::CreateDamageInd(vec2 Pos, float Angle, int Amount)
{
	float a = 3 * 3.14159f / 2 + Angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+2));
		CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd));
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_Angle = (int)(f*256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(vec2 Pos)
{
	// create the event
	CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)m_Events.Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}


void CGameContext::CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, bool Smoke, bool FromMonster)
{
	// create the event
	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}

	if (!NoDamage)
	{
		// deal damage
		IEntityDamageable *apEnts[MAX_ENTITIES_DAMAGEABLE];
		float Radius = 135.0f;
		float InnerRadius = 48.0f;
		int Num = m_World.FindEntitiesDamageable(Pos, Radius, apEnts, MAX_ENTITIES_DAMAGEABLE);
		for(int i = 0; i < Num; i++)
		{
			if (!FromMonster && Smoke && apEnts[i]->GetType() == CGameWorld::ENTTYPE_CHARACTER && reinterpret_cast<CCharacter*>(apEnts[i])->GetPlayer()->GetCID() == Owner)
				continue;

			vec2 Diff = apEnts[i]->m_Pos - Pos;
			if (apEnts[i]->GetType() == CGameWorld::ENTTYPE_EXPLODEWALL)
				Diff = closest_point_on_line(reinterpret_cast<CExplodeWall*>(apEnts[i])->m_From, apEnts[i]->m_Pos, Pos) - Pos;

			vec2 ForceDir(0,1);
			float l = length(Diff);
			if(l)
				ForceDir = normalize(Diff);
			l = 1-clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);

			int ExtraDmg = 0;
			if(FromMonster)
			{
				if(GetValidMonster(Owner)) // Security
					ExtraDmg += GetValidMonster(Owner)->GetDifficulty() - 1;
			}
			float Dmg = (6 + ExtraDmg) * l;

			if((int)Dmg)
				apEnts[i]->TakeDamage(ForceDir*Dmg*2, m_pEventsGame->IsActualEvent(WALLSHOT) ? 0 : (int)Dmg, Owner, Weapon, false, FromMonster);
		}
	}
}

/*
void create_smoke(vec2 Pos)
{
	// create the event
	EV_EXPLOSION *pEvent = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(pEvent)
	{
		pEvent->x = (int)Pos.x;
		pEvent->y = (int)Pos.y;
	}
}*/

void CGameContext::CreatePlayerSpawn(vec2 Pos)
{
	// create the event
	CNetEvent_Spawn *ev = (CNetEvent_Spawn *)m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn));
	if(ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateDeath(vec2 Pos, int ClientID)
{
	// create the event
	CNetEvent_Death *pEvent = (CNetEvent_Death *)m_Events.Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ClientID = ClientID;
	}
}

void CGameContext::CreateSound(vec2 Pos, int Sound, int Mask)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)m_Events.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), Mask);
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

void CGameContext::CreateSoundGlobal(int Sound, int Target)
{
	if (Sound < 0)
		return;

	CNetMsg_Sv_SoundGlobal Msg;
	Msg.m_SoundID = Sound;
	if(Target == -2)
		Server()->SendPackMsg(&Msg, MSGFLAG_NOSEND, -1);
	else
	{
		int Flag = MSGFLAG_VITAL;
		if(Target != -1)
			Flag |= MSGFLAG_NORECORD;
		Server()->SendPackMsg(&Msg, Flag, Target);
	}
}


void CGameContext::SendChatTarget(int To, const char *pText, int Type)
{
	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_ClientID = -1;
	Msg.m_pMessage = pText;
	if (Type == CHAT_INFO)
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, To);
	else
	{
		if (To != -1)
		{
			if ( (Type == CHAT_INFO_HEAL_KILLER && m_apPlayers[To]->m_pStats->GetConf().m_InfoHealKiller) ||
				 (Type == CHAT_INFO_XP && m_apPlayers[To]->m_pStats->GetConf().m_InfoXP) ||
				 (Type == CHAT_INFO_LEVELUP && m_apPlayers[To]->m_pStats->GetConf().m_InfoLevelUp) ||
				 (Type == CHAT_INFO_KILLING_SPREE && m_apPlayers[To]->m_pStats->GetConf().m_InfoKillingSpree) ||
				 (Type == CHAT_INFO_RACE && m_apPlayers[To]->m_pStats->GetConf().m_InfoRace) ||
				 (Type == CHAT_INFO_AMMO && m_apPlayers[To]->m_pStats->GetConf().m_InfoAmmo) ||
				 (Type == CHAT_INFO_VOTER && m_apPlayers[To]->m_pStats->GetConf().m_ShowVoter))
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, To);
		}
		else
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (m_apPlayers[i])
				{
					if ( (Type == CHAT_INFO_HEAL_KILLER && m_apPlayers[i]->m_pStats->GetConf().m_InfoHealKiller) ||
						 (Type == CHAT_INFO_XP && m_apPlayers[i]->m_pStats->GetConf().m_InfoXP) ||
						 (Type == CHAT_INFO_LEVELUP && m_apPlayers[i]->m_pStats->GetConf().m_InfoLevelUp) ||
						 (Type == CHAT_INFO_KILLING_SPREE && m_apPlayers[i]->m_pStats->GetConf().m_InfoKillingSpree) ||
						 (Type == CHAT_INFO_RACE && m_apPlayers[i]->m_pStats->GetConf().m_InfoRace) ||
						 (Type == CHAT_INFO_AMMO && m_apPlayers[i]->m_pStats->GetConf().m_InfoAmmo) ||
						 (Type == CHAT_INFO_VOTER && m_apPlayers[i]->m_pStats->GetConf().m_ShowVoter))
						Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
				}
			}
		}
	}
}


void CGameContext::SendChat(int ChatterClientID, int Team, const char *pText)
{
	char aBuf[256];
	if(ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
		str_format(aBuf, sizeof(aBuf), "%d:%d:%s: %s", ChatterClientID, Team, Server()->ClientName(ChatterClientID), pText);
	else
		str_format(aBuf, sizeof(aBuf), "*** %s", pText);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, Team!=CHAT_ALL?"teamchat":"chat", aBuf);

	if(Team == CHAT_ALL)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	else
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;

		// pack one for the recording only
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NOSEND, -1);

		// send to the clients
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_apPlayers[i] && m_apPlayers[i]->GetTeam() == Team)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
		}
	}
}

void CGameContext::SendEmoticon(int ClientID, int Emoticon)
{
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_ClientID = ClientID;
	Msg.m_Emoticon = Emoticon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
}

void CGameContext::SendWeaponPickup(int ClientID, int Weapon)
{
	CNetMsg_Sv_WeaponPickup Msg;
	Msg.m_Weapon = Weapon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}


void CGameContext::SendBroadcast(const char *pText, int ClientID, int LifeTime)
{
	if (ClientID != -1)
	{
		if (m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_EndBroadcastTick <= Server()->Tick())
		{
			CNetMsg_Sv_Broadcast Msg;
			Msg.m_pMessage = pText;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
			m_apPlayers[ClientID]->m_EndBroadcastTick = Server()->Tick() + Server()->TickSpeed()*LifeTime;
		}
	}
	else
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
			SendBroadcast(pText, i, LifeTime);
	}
}

//
void CGameContext::StartVote(const char *pDesc, const char *pCommand, const char *pReason)
{
	// check if a vote is already running
	if(m_VoteCloseTime)
		return;

	// reset votes
	m_VoteEnforce = VOTE_ENFORCE_UNKNOWN;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
		{
			m_apPlayers[i]->m_Vote = 0;
			m_apPlayers[i]->m_VotePos = 0;
		}
	}

	// start vote
	m_VoteCloseTime = time_get() + time_freq()*25;
	str_copy(m_aVoteDescription, pDesc, sizeof(m_aVoteDescription));
	str_copy(m_aVoteCommand, pCommand, sizeof(m_aVoteCommand));
	str_copy(m_aVoteReason, pReason, sizeof(m_aVoteReason));
	SendVoteSet(-1);
	m_VoteUpdate = true;
}


void CGameContext::EndVote()
{
	m_VoteCloseTime = 0;
	SendVoteSet(-1);
}

void CGameContext::SendVoteSet(int ClientID)
{
	CNetMsg_Sv_VoteSet Msg;
	if(m_VoteCloseTime)
	{
		Msg.m_Timeout = (m_VoteCloseTime-time_get())/time_freq();
		Msg.m_pDescription = m_aVoteDescription;
		Msg.m_pReason = m_aVoteReason;
	}
	else
	{
		Msg.m_Timeout = 0;
		Msg.m_pDescription = "";
		Msg.m_pReason = "";
	}
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SendVoteStatus(int ClientID, int Total, int Yes, int No)
{
	CNetMsg_Sv_VoteStatus Msg = {0};
	Msg.m_Total = Total;
	Msg.m_Yes = Yes;
	Msg.m_No = No;
	Msg.m_Pass = Total - (Yes+No);

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);

}

void CGameContext::AbortVoteKickOnDisconnect(int ClientID)
{
	if(m_VoteCloseTime && ((!str_comp_num(m_aVoteCommand, "kick ", 5) && str_toint(&m_aVoteCommand[5]) == ClientID) ||
						   (!str_comp_num(m_aVoteCommand, "set_team ", 9) && str_toint(&m_aVoteCommand[9]) == ClientID)))
		m_VoteCloseTime = -1;
}


void CGameContext::CheckPureTuning()
{
	// might not be created yet during start up
	if(!m_pController)
		return;

	g_Config.m_SvScorelimit = 0;
}

void CGameContext::SendTuningParams(int ClientID)
{
	CheckPureTuning();

	if (ClientID >= 0)
	{
		CTuningParams User = m_Tuning;

		if (m_apPlayers[ClientID]->GetSID() >= 0)
		{
			float RateSpeed = 1.0f;
			float RateAccel = 1.0f;
			float RateHighJump = 1.0f;
			float RateLengthHook = 1.0f;
			float RateSpeedHook = 1.0f;
			if (!m_pEventsGame->IsActualEvent(SPEED_X10))
			{
				RateSpeed = m_apPlayers[ClientID]->m_pStats->GetStatMove().m_rate_speed;
				RateAccel = m_apPlayers[ClientID]->m_pStats->GetStatMove().m_rate_accel;
				RateSpeedHook = m_apPlayers[ClientID]->m_pStats->GetStatHook().m_rate_speed;
			}
			if (m_apPlayers[ClientID]->m_pStats->GetActualKill() >= 5)
			{
				RateSpeed *= 1.5f;
				RateAccel *= 1.5f;
			}
			if (!m_pEventsGame->IsActualEvent(JUMP_X1_5))
				RateHighJump = m_apPlayers[ClientID]->m_pStats->GetStatMove().m_rate_high_jump;
			if (!m_pEventsGame->IsActualEvent(HOOK_VERY_LONG))
				RateLengthHook = m_apPlayers[ClientID]->m_pStats->GetStatHook().m_rate_length;

			User.Set("ground_control_speed", m_Tuning.m_GroundControlSpeed * RateSpeed);
			User.Set("air_control_speed", m_Tuning.m_AirControlSpeed * RateSpeed);
			User.Set("ground_control_accel", m_Tuning.m_GroundControlAccel * RateAccel);
			User.Set("air_control_accel", m_Tuning.m_AirControlAccel * RateAccel);
			User.Set("ground_jump_impulse", m_Tuning.m_GroundJumpImpulse * RateHighJump);
			User.Set("air_jump_impulse", m_Tuning.m_AirJumpImpulse * RateHighJump);
			User.Set("hook_length", m_Tuning.m_HookLength * RateLengthHook);
			User.Set("hook_fire_speed", m_Tuning.m_HookFireSpeed * RateSpeedHook);
			User.Set("hook_drag_speed", m_Tuning.m_HookDragSpeed * RateSpeedHook);
		}


		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&User;
		for(unsigned i = 0; i < sizeof(User)/sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
	}
	else
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!m_apPlayers[i])
				continue;

			SendTuningParams(i);
		}
	}
}

void CGameContext::SwapTeams()
{
	if(!m_pController->IsTeamplay())
		return;
	
	SendChat(-1, CGameContext::CHAT_ALL, "Teams were swapped");

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			m_apPlayers[i]->SetTeam(m_apPlayers[i]->GetTeam()^1, false);
	}

	(void)m_pController->CheckTeamBalance();
}

void CGameContext::OnTick()
{
	// check tuning
	CheckPureTuning();

	// copy tuning
	m_World.m_Core.m_Tuning = m_Tuning;
	m_World.Tick();

	m_pEventsGame->Tick();
	//if(world.paused) // make sure that the game object always updates
	m_pController->Tick();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
		{
			m_apPlayers[i]->Tick();
			m_apPlayers[i]->PostTick();
		}
	}

	for(int i = 0; i < MAX_MONSTERS; i++)
	{
		if (GetValidMonster(i) && !m_apMonsters[i]->IsAlive())
			OnMonsterDeath(i);
	}

	int AliveMonsters = 0;
	for(int i = 0; i < MAX_MONSTERS; i++)
	{
		if (GetValidMonster(i))
		{
			if (AliveMonsters == g_Config.m_SvNumMonster)
				OnMonsterDeath(i);
			else
				AliveMonsters++;
		}
	}

	while (AliveMonsters < g_Config.m_SvNumMonster && AliveMonsters <= MAX_MONSTERS)
	{
		NewMonster();
		AliveMonsters++;
	}

	//m_pStatsServer->Tick();
	// update voting
	if(m_VoteCloseTime)
	{
		// abort the kick-vote on player-leave
		if(m_VoteCloseTime == -1)
		{
			SendChat(-1, CGameContext::CHAT_ALL, "投票通过：平票");
			EndVote();
		}
		else
		{
			int Total = 0, Yes = 0, No = 0;
			if(m_VoteUpdate)
			{
				// count votes
				char aaBuf[MAX_CLIENTS][NETADDR_MAXSTRSIZE] = {{0}};
				for(int i = 0; i < MAX_CLIENTS; i++)
					if(m_apPlayers[i])
						Server()->GetClientAddr(i, aaBuf[i], NETADDR_MAXSTRSIZE);
				bool aVoteChecked[MAX_CLIENTS] = {0};
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(!m_apPlayers[i] || (m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && (!m_pController->IsTeamplay() || m_pEventsGame->GetActualEventTeam() != T_SURVIVOR) && !m_pEventsGame->IsActualEvent(SURVIVOR)) || aVoteChecked[i])	// don't count in votes by spectators
						continue;

					int ActVote = m_apPlayers[i]->m_Vote;
					int ActVotePos = m_apPlayers[i]->m_VotePos;

					// check for more players with the same ip (only use the vote of the one who voted first)
					for(int j = i+1; j < MAX_CLIENTS; ++j)
					{
						if(!m_apPlayers[j] || aVoteChecked[j] || str_comp(aaBuf[j], aaBuf[i]))
							continue;

						aVoteChecked[j] = true;
						if(m_apPlayers[j]->m_Vote && (!ActVote || ActVotePos > m_apPlayers[j]->m_VotePos))
						{
							ActVote = m_apPlayers[j]->m_Vote;
							ActVotePos = m_apPlayers[j]->m_VotePos;
						}
					}

					Total++;
					if(ActVote > 0)
						Yes++;
					else if(ActVote < 0)
						No++;
				}

				if(Yes >= Total/2+1)
					m_VoteEnforce = VOTE_ENFORCE_YES;
				else if(No >= (Total+1)/2)
					m_VoteEnforce = VOTE_ENFORCE_NO;
			}

			if(m_VoteEnforce == VOTE_ENFORCE_YES || (time_get() > m_VoteCloseTime && Yes >= Total/2) || (time_get() > m_VoteCloseTime && No == 0))
			{
				Server()->SetRconCID(IServer::RCON_CID_VOTE);
				Console()->ExecuteLine(m_aVoteCommand);
				Server()->SetRconCID(IServer::RCON_CID_SERV);
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "投票结果：通过");

				if(m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_LastVoteCall = 0;
			}
			else if(m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "投票结果：不通过");
			}
			else if(m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
	}


#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		for(int i = 0; i < g_Config.m_DbgDummies ; i++)
		{
			CNetObj_PlayerInput Input = {0};
			Input.m_Direction = (i&1)?-1:1;
			m_apPlayers[MAX_CLIENTS-i-1]->OnPredictedInput(&Input);
		}
	}
#endif
}

// Server hooks
void CGameContext::OnClientDirectInput(int ClientID, void *pInput)
{
	if(!m_World.m_Paused)
		m_apPlayers[ClientID]->OnDirectInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientPredictedInput(int ClientID, void *pInput)
{
	if(!m_World.m_Paused)
		m_apPlayers[ClientID]->OnPredictedInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientEnter(int ClientID)
{
	//world.insert_entity(&players[client_id]);
	m_apPlayers[ClientID]->Respawn();
	char aBuf[512];
	if ( m_pEventsGame->GetActualEventTeam() != ANONYMOUS )
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s =D", Server()->ClientName(ClientID), m_pController->GetTeamName(m_apPlayers[ClientID]->GetTeam()));
	else
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the game =D", Server()->ClientName(ClientID));
	SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), m_apPlayers[ClientID]->GetTeam());
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	m_VoteUpdate = true;
	SendChatTarget(ClientID, "*** 欢迎来到极端武器怪物猎人模组 2.5版本***");
	SendChatTarget(ClientID, "** 作者 PJK **");
	SendChatTarget(ClientID, "* AI作者 Neox76 *");
	SendChatTarget(ClientID, "* 这是一个有趣的mod，有很多爆炸性的东西和很多有趣的修改！ *");
	SendChatTarget(ClientID, "** 获取更多帮助(聊天栏输入) : /info , /cmdlist , /upgr , /race , /stats and /ranks **");
	SendChatTarget(ClientID, "*** 感谢你选择这个服务器来玩， 玩的开心~ ;D ! ***");
}

void CGameContext::OnClientConnected(int ClientID)
{
	// Check which team the player should be on
	int StartTeam = TEAM_SPECTATORS;

	m_apPlayers[ClientID] = new(ClientID) CPlayer(this, ClientID, StartTeam);
	//players[client_id].init(client_id);
	//players[client_id].client_id = client_id;

	(void)m_pController->CheckTeamBalance();

#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		if(ClientID >= MAX_CLIENTS-g_Config.m_DbgDummies)
			return;
	}
#endif

	// send active vote
	if(m_VoteCloseTime)
		SendVoteSet(ClientID);

	// send motd
	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = g_Config.m_SvMotd;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::OnClientDrop(int ClientID, const char *pReason)
{
	if ( ClientID == m_pController->m_Captain[TEAM_RED] )
		m_pController->m_Captain[TEAM_RED] = -1;
	else if ( ClientID == m_pController->m_Captain[TEAM_BLUE] )
		m_pController->m_Captain[TEAM_BLUE] = -1;
	AbortVoteKickOnDisconnect(ClientID);
	m_apPlayers[ClientID]->m_pStats->SetStopPlay();
	m_apPlayers[ClientID]->OnDisconnect(pReason);
	delete m_apPlayers[ClientID];
	m_apPlayers[ClientID] = 0;

	(void)m_pController->CheckTeamBalance();
	m_VoteUpdate = true;

	// update spectator modes
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_apPlayers[i] && m_apPlayers[i]->m_SpectatorID == ClientID)
			m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
	}
}

void CGameContext::ParseArguments(const char *Message, int nb_result, char Result[][256])
{
	for (int i = 0; i < nb_result; i++)
		Result[i][0] = 0;

	int Actual = 0;
	int Pos = 0;

	for (int i = 0; i < str_length(Message); i++)
	{
		if (Message[i] == ' ')
		{
			Result[Actual][Pos] = 0;
			Pos = 0;
			if (Message[i + 1] != ' ')
				Actual++;
			if (Actual >= nb_result)
				break;
		}
		else if (Pos < 255)
		{
			Result[Actual][Pos] = Message[i];
			Pos++;
		}
	}

	Result[Actual][Pos] = 0;
}

void CGameContext::CommandOnChat(const char *Message, const int ClientID, const int Team)
{
	char Arguments[3][256];
	ParseArguments(Message, 3, Arguments);
	if (str_comp_nocase(Arguments[0], "/cmdlist") == 0 || str_comp_nocase(Arguments[0], "/help") == 0 )
	{
		SendChatTarget(ClientID, "*** 指令列表 : ***");
		SendChatTarget(ClientID, "/cmdlist 或 /help : 获取指令列表（废话）.");
		SendChatTarget(ClientID, "/info 或 /credits : 获取关于这个模式的信息.");
		SendChatTarget(ClientID, "/ammo : 获取你当前武器的子弹.");
		SendChatTarget(ClientID, "/race : 选择一个种族.");
		SendChatTarget(ClientID, "/stats 或 /ranks : 获取你的成绩，或排名.");
		SendChatTarget(ClientID, "/player 或 /upgr : 获取你的等级，或升级你的等级.");
		SendChatTarget(ClientID, "/conf : 修改你的账号设定.");
		SendChatTarget(ClientID, "/reset_stats 或 /reset_all_stats 又或 /reset_upgr: 重置你的成绩，或重置你的所有成绩，又或重置你的等级");
	}
	else if(str_comp_nocase(Arguments[0], "/info") == 0)
	{
		SendChatTarget(ClientID, "*** 欢迎来到极端武器怪物猎人模组 2.5版本***");
		SendChatTarget(ClientID, "** 作者 PJK **");
		SendChatTarget(ClientID, "* AI作者 Neox76 *");
		SendChatTarget(ClientID, "* 这是一个有趣的mod，有很多爆炸性的东西和很多有趣的修改！ *");
		SendChatTarget(ClientID, "** 获取更多帮助(聊天栏输入) : /info , /cmdlist , /upgr , /race , /stats and /ranks **");
		SendChatTarget(ClientID, "*** 感谢你选择这个服务器来玩， 玩的开心~ ;D ! ***");
	}
	else if(str_comp_nocase(Arguments[0], "/reg") == 0)
	{
		if(Arguments[1][0] == 0)
		{
			SendChatTarget(ClientID, "使用方式 : /reg <用户名> <密码>");
			SendChatTarget(ClientID, "描述 : 使用这个指令来在这个服务器里创建账号. (允许多个账号 :D)");
			return;
		}
		else if(str_length(Arguments[1]) >= MAX_NAME_LENGTH)
		{
			SendChatTarget(ClientID, "你的用户名太长了. (中文按拼音数量计算字数)");
			return;
		}
		else if(Arguments[2][0] == 0)
		{
			SendChatTarget(ClientID, "你必须要给出一个密码 !");
			return;
		}

#if defined(CONF_SQL)
		int Error = m_pStatsServer->CreateId(ClientID, Arguments[1], Arguments[2]);
		if (Error == -1)
		{
			SendChatTarget(ClientID, "这个用户名已经被使用了 !");
			return;
		}
		else if (Error == -2)
		{
			SendChatTarget(ClientID, "错误 ：无法连接至数据库 ！请再次尝试（如果多次尝试不成功请咨询管理员 管理员QQ：1421709710）.");
			return;
		}
		else if (Error >= 0)
		{
			SendChatTarget(ClientID, "您成功注册了一个账号！ 请使用 /login 登录账号!");
			return;
		}
		else
		{
			SendChatTarget(ClientID, "错误 : 未知 ! 请咨询管理员（管理员QQ：1421709710）.");
			return;
		}
#else
		SendChatTarget(ClientID, "This is a version-compilated without support SQL so no account :(");
#endif
	}
	else if(str_comp_nocase(Arguments[0], "/login") == 0)
	{
		if(Arguments[1][0] == 0)
		{
			SendChatTarget(ClientID, "使用方式 : /login <用户名> <密码>");
			SendChatTarget(ClientID, "描述 : 使用这个指令登录你的账号.");
			SendChatTarget(ClientID, "信息 : 你可以使用用户名 \"陌生人\" 来脱离存档游玩.");
			return;
		}
		else if(str_comp(Arguments[1], "陌生人") == 0)
		{
			//Connection Anonymous
			if (m_apPlayers[ClientID]->GetSID() == 0)
			{
				SendChatTarget(ClientID, "你已经在作为陌生人游玩了!!");
				return;
			}

			m_apPlayers[ClientID]->SetSID(0);
			if(m_pController->IsTeamplay())
				SendChatTarget(ClientID, "你作为一个陌生人连接进了TWS ! 你可以现在加入游戏 !");
			else
			{
				SendChatTarget(ClientID, "你作为一个陌生人加入了游戏 ! 玩的开心 !");
				m_apPlayers[ClientID]->SetTeam(0);
			}
			return;
		}
		else if(Arguments[2][0] == 0)
		{
			SendChatTarget(ClientID, "兄嘚，密码捏？");
			return;
		}

#if defined(CONF_SQL)
		int Id = m_pStatsServer->GetId(Arguments[1], Arguments[2]);
		if (Id == -1)
		{
			SendChatTarget(ClientID, "账户用户名/密码错误 !");
			return;
		}
		else if (Id == -2)
		{
			SendChatTarget(ClientID, "数据库错误.");
			return;
		}
		else if (Id > 0)
		{
			if(m_apPlayers[ClientID]->GetSID() == Id)
			{
				SendChatTarget(ClientID, "您已经登录了账户了 !!");
				return;
			}

			if(!m_apPlayers[ClientID]->SetSID(Id))
				SendChatTarget(ClientID, "错误");
			else if(m_pController->IsTeamplay())
				SendChatTarget(ClientID, "连接成功！你可以现在加入游戏！");
			else
			{
				SendChatTarget(ClientID, "连接成功");
				m_apPlayers[ClientID]->SetTeam(0);
			}
			return;
		}
		else
		{
			SendChatTarget(ClientID, "错误 : 未知 ! 请联系管理员 管理员QQ：1421709710.");
			return;
		}
#else
		SendChatTarget(ClientID, "This is a version-compilated without support SQL so no account :(");
#endif
	}
	else if(str_comp_nocase(Arguments[0], "/credits") == 0)
	{
		SendChat(ClientID, Team, Arguments[0]);
		SendChatTarget(-1, "*** 怪物猎人模式 v2.5 ***");
		SendChatTarget(-1, "** 作者 [FR] PJK **");
		SendChatTarget(-1, "* AI作者 Neox76 *");
	}
	else if(m_apPlayers[ClientID]->GetSID() < 0)
	{
		char error[256] = "";
		str_format(error, 256, "未被识别的指令 : %s. 为了获取更多指令的使用全, 请登录账号！", Arguments[0]);
		SendChatTarget(ClientID, error);
		return;
	}
	else if(str_comp_nocase(Arguments[0], "/ammo") == 0)
	{
		CCharacter *pChr = m_apPlayers[ClientID]->GetCharacter();
		if ( pChr )
		{
			char aBuf[256] = "";
			str_format(aBuf, 256, "您当前武器的剩余子弹量 : %d/%d.", pChr->GetAmmoActiveWeapon(), g_pData->m_Weapons.m_aId[pChr->GetActiveWeapon()].m_Maxammo);
			SendChatTarget(ClientID, aBuf);
		}
		else
			SendChatTarget(ClientID, "你死去了 或 你是一个旁观者");
	}
	else if(str_comp_nocase(Arguments[0], "/race") == 0)
	{
		if(str_comp_nocase(Arguments[1], "human") == 0)
		{
			if(m_apPlayers[ClientID]->m_Race == HUMAN)
				return;
			char aBuf[256] = "";
			str_format(aBuf, 256, "%s 的血脉溅起一层层波纹！人类的赞歌就是勇气的赞歌！汤姆逊波纹疾走！", Server()->ClientName(ClientID));
			SendChatTarget(-1, aBuf, CHAT_INFO_RACE);
			m_apPlayers[ClientID]->KillCharacter();
			m_apPlayers[ClientID]->m_Race = HUMAN;
			for (int i = 0; i < NUM_WEAPONS; i++)
				m_apPlayers[ClientID]->m_WeaponType[i] = HUMAN;
		}
		else if(str_comp_nocase(Arguments[1], "gnome") == 0)
		{
			if(m_apPlayers[ClientID]->m_Race == GNOME)
				return;
			char aBuf[256] = "";
			str_format(aBuf, 256, "%s 成为了一名日本人(侏儒)", Server()->ClientName(ClientID));
			SendChatTarget(-1, aBuf, CHAT_INFO_RACE);
			m_apPlayers[ClientID]->KillCharacter();
			m_apPlayers[ClientID]->m_Race = GNOME;
			for (int i = 0; i < NUM_WEAPONS; i++)
				m_apPlayers[ClientID]->m_WeaponType[i] = GNOME;
		}
		else if(str_comp_nocase(Arguments[1], "orc") == 0)
		{
			if(m_apPlayers[ClientID]->m_Race == ORC)
				return;
			char aBuf[256] = "";
			str_format(aBuf, 256, "%s 的野性爆发了出来！是兽人的血脉！", Server()->ClientName(ClientID));
			SendChatTarget(-1, aBuf, CHAT_INFO_RACE);
			m_apPlayers[ClientID]->KillCharacter();
			m_apPlayers[ClientID]->m_Race = ORC;
			for (int i = 0; i < NUM_WEAPONS; i++)
				m_apPlayers[ClientID]->m_WeaponType[i] = ORC;
		}
		else if(str_comp_nocase(Arguments[1], "elf") == 0)
		{
			if(m_apPlayers[ClientID]->m_Race == ELF)
				return;
			char aBuf[256] = "";
			str_format(aBuf, 256, "米西米西，滑不垃圾，如果你不拉稀，我就不能米西。 %s 继承了灰太狼(精灵密咒)的血脉 !", Server()->ClientName(ClientID));
			SendChatTarget(-1, aBuf, CHAT_INFO_RACE);
			m_apPlayers[ClientID]->KillCharacter();
			m_apPlayers[ClientID]->m_Race = ELF;
			for (int i = 0; i < NUM_WEAPONS; i++)
				m_apPlayers[ClientID]->m_WeaponType[i] = ELF;
		}
		else if(str_comp_nocase(Arguments[1], "custom") == 0)
		{
			if(m_apPlayers[ClientID]->m_Race != CUSTOM)
			{
				char aBuf[256] = "";
				str_format(aBuf, 256, "孤独的游行者，%s", Server()->ClientName(ClientID));
				SendChatTarget(-1, aBuf, CHAT_INFO_RACE);
				m_apPlayers[ClientID]->KillCharacter();
			}

			SendChatTarget(ClientID, "使用指令 /hammer | /gun | /shotgun | /grenade | /rifle 来自定义你的武器 !");
			m_apPlayers[ClientID]->m_Race = CUSTOM;
		}
		else if(Arguments[1][0] == 0 || Arguments[1][0] == ' ')
		{
			SendChatTarget(ClientID, "使用方式 : /race <race>");
			SendChatTarget(ClientID, "种族列表 : Human(人类)， Gnome(日本人)，Orc(兽人)， Elf(灰太狼)，Custom(游行者)");
			SendChatTarget(ClientID, "描述：请替换种族为种族列表内的英文，示例: '/race Human'");
		}
		else
		{
			SendChatTarget(ClientID, "不存在这个种族 !");
		}
	}
	else if (str_comp_nocase(Arguments[0], "/hammer") == 0 ||
			 str_comp_nocase(Arguments[0], "/gun") == 0 ||
			 str_comp_nocase(Arguments[0], "/shotgun") == 0 ||
			 str_comp_nocase(Arguments[0], "/grenade") == 0 ||
			 str_comp_nocase(Arguments[0], "/rifle") == 0)
	{
		if (str_comp_nocase(Arguments[1], "human") == 0 ||
				str_comp_nocase(Arguments[1], "gnome") == 0 ||
				str_comp_nocase(Arguments[1], "orc") == 0 ||
				str_comp_nocase(Arguments[1], "elf") == 0)
		{
			if (m_apPlayers[ClientID]->m_Race != CUSTOM)
			{
				char aBuf[256] = "";
				str_format(aBuf, 256, "孤独的游行者，%s", Server()->ClientName(ClientID));
				SendChatTarget(-1, aBuf, CHAT_INFO_RACE);
				m_apPlayers[ClientID]->m_Race = CUSTOM;
			}

			m_apPlayers[ClientID]->KillCharacter();
		}
		else if (Arguments[1][0] == 0 || Arguments[1][0] == ' ')
		{
			char aBuf[256] = "";
			str_format(aBuf, 256, "使用方式 : %s <种族>", Arguments[0]);
			SendChatTarget(ClientID, aBuf);
			SendChatTarget(ClientID, "种族 : Human or Gnome or Orc or Elf");
		}
		else
		{
			SendChatTarget(ClientID, "Human(人类)， Gnome(日本人)，Orc(兽人)， Elf(灰太狼)，Custom(游行者)");
			return;
		}

		int Weapon = 0;
		int Race = 0;

		if (str_comp_nocase(Arguments[0], "/hammer") == 0)
			Weapon = WEAPON_HAMMER;
		else if (str_comp_nocase(Arguments[0], "/gun") == 0)
			Weapon = WEAPON_GUN;
		else if (str_comp_nocase(Arguments[0], "/shotgun") == 0)
			Weapon = WEAPON_SHOTGUN;
		else if (str_comp_nocase(Arguments[0], "/grenade") == 0)
			Weapon = WEAPON_GRENADE;
		else if (str_comp_nocase(Arguments[0], "/rifle") == 0)
			Weapon = WEAPON_RIFLE;

		if (str_comp_nocase(Arguments[1], "human") == 0)
			Race = HUMAN;
		else if (str_comp_nocase(Arguments[1], "gnome") == 0)
			Race = GNOME;
		else if (str_comp_nocase(Arguments[1], "orc") == 0)
			Race = ORC;
		else if (str_comp_nocase(Arguments[1], "elf") == 0)
			Race = ELF;

		m_apPlayers[ClientID]->m_WeaponType[Weapon] = Race;

		char aBuf[256] = "";
		str_format(aBuf, 256, "The type of your %s is %s now !", Arguments[0], Arguments[1]);
		SendChatTarget(ClientID, aBuf, CHAT_INFO_RACE);
	}
	else if(str_comp_nocase(Arguments[0], "/stats") == 0)
	{
		SendChat(ClientID, Team, Arguments[0]);
		m_apPlayers[ClientID]->m_pStats->DisplayStat();
	}
	else if(str_comp_nocase(Arguments[0], "/ranks") == 0)
	{
		SendChat(ClientID, Team, Arguments[0]);
#if defined(CONF_SQL)
		m_pStatsServer->DisplayRank(m_apPlayers[ClientID]->GetSID());
#else
		SendChatTarget(ClientID, "This is a version-compilated without support SQL so no ranks :(");
#endif
	}
	else if(str_comp_nocase(Arguments[0], "/bestof") == 0)
	{
		SendChat(ClientID, Team, Arguments[0]);
#if defined(CONF_SQL)
		m_pStatsServer->DisplayBestOf();
#else
		SendChatTarget(ClientID, "This is a version-compilated without support SQL so no bestof :(");
#endif
	}
	else if(str_comp_nocase(Arguments[0], "/player") == 0)
		m_apPlayers[ClientID]->m_pStats->DisplayPlayer();
	else if(str_comp_nocase(Arguments[0], "/upgr") == 0)
	{

		int Code = 4;
		if (Arguments[1][0] == 0 || Arguments[1][0] == ' ')
		{
			SendChatTarget(ClientID, "使用方式 : /upgr <type> [count]");
			SendChatTarget(ClientID, "类型 : Weapon(武器)， Life(血量)， Move(移动)， Hook(钩子)");
			return;
		}
		else if (str_comp_nocase(Arguments[1], "weapon") != 0 &&
				 str_comp_nocase(Arguments[1], "life") != 0 &&
				 str_comp_nocase(Arguments[1], "move") != 0 &&
				 str_comp_nocase(Arguments[1], "hook") != 0)
		{
			SendChatTarget(ClientID, "This upgrade doesn't exist !");
			return;
		}

		unsigned int count = max(1, str_toint(Arguments[2]));
		if (str_comp_nocase(Arguments[1], "weapon") == 0)
			Code = m_apPlayers[ClientID]->m_pStats->UpgradeWeapon(count);
		else if (str_comp_nocase(Arguments[1], "life") == 0)
			Code = m_apPlayers[ClientID]->m_pStats->UpgradeLife(count);
		else if (str_comp_nocase(Arguments[1], "move") == 0)
			Code = m_apPlayers[ClientID]->m_pStats->UpgradeMove(count);
		else if (str_comp_nocase(Arguments[1], "hook") == 0)
			Code = m_apPlayers[ClientID]->m_pStats->UpgradeHook(count);

		switch (Code)
		{
		case 0:
			SendChatTarget(ClientID, "你的账号升级了 !");
			break;

		case 1:
			SendChatTarget(ClientID, "你没有足够的钱");
			break;

		case 2:
			SendChatTarget(ClientID, "你锁定了你的账号 !");
			break;

		case 3:
			SendChatTarget(ClientID, "你无法继续升级了 ! 最高级别 : 40");
			break;
		default:
			SendChatTarget(ClientID, "呃呃呃，未知错误 !");
			break;
		}
	}
	else if(str_comp_nocase(Arguments[0], "/conf") == 0)
	{
		if(str_comp_nocase(Arguments[1], "InfoHealKiller") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->InfoHealKiller();
			char a[256] = "";
			str_format(a, 256, "显示杀手信息 : %s. ", statut ? "开启" : "关闭");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "InfoXP") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->InfoXP();
			char a[256] = "";
			str_format(a, 256, "显示XP信息 : %s. ", statut ? "开启" : "关闭");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "InfoLevelUp") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->InfoLevelUp();
			char a[256] = "";
			str_format(a, 256, "显示升级信息 : %s. ", statut ? "开启" : "关闭");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "InfoKillingSpree") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->InfoKillingSpree();
			char a[256] = "";
			str_format(a, 256, "连续击杀信息 : %s. ", statut ? "开启" : "关闭");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "InfoRace") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->InfoRace();
			char a[256] = "";
			str_format(a, 256, "种族信息 : %s. ", statut ? "开启" : "关闭");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "InfoAmmo") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->InfoAmmo();
			char a[256] = "";
			str_format(a, 256, "子弹信息 : %s. ", statut ? "开启" : "关闭");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "InfoVoter") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->ShowVoter();
			char a[256] = "";
			str_format(a, 256, "投票者信息 : %s. ", statut ? "开启" : "关闭");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "EnableAllInfo") == 0)
		{
			m_apPlayers[ClientID]->m_pStats->EnableAllInfo();
			SendChatTarget(ClientID, "已开启所有提示信息");
		}
		else if(str_comp_nocase(Arguments[1], "DisableAllInfo") == 0)
		{
			m_apPlayers[ClientID]->m_pStats->DisableAllInfo();
			SendChatTarget(ClientID, "已关闭所有提示信息");
		}
		else if(str_comp_nocase(Arguments[1], "AmmoAbsolute") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->AmmoAbsolute();
			char a[256] = "";
			str_format(a, 256, "Information of Ammo is now : %s. ", statut ? "Absolute" : "Relative");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "LifeAbsolute") == 0)
		{
			bool statut = m_apPlayers[ClientID]->m_pStats->LifeAbsolute();
			char a[256] = "";
			str_format(a, 256, "Information of Life is now : %s. ", statut ? "Absolute" : "Relative");
			SendChatTarget(ClientID, a);
		}
		else if(str_comp_nocase(Arguments[1], "Lock") == 0)
		{
			bool lock = m_apPlayers[ClientID]->m_pStats->Lock();
			char a[256] = "";
			str_format(a, 256, "你的所有等级 : %s. ", lock ? "锁定" : "取消锁定");
			SendChatTarget(ClientID, a);
		}
		else
		{
			SendChatTarget(ClientID, "使用方式 : /conf <选项>");
			SendChatTarget(ClientID, "选项 : InfoHealKiller（显示杀手信息），InfoXP（XP信息），InfoLevelUp（等级提升信息）， InfoKillingSpree（击杀信息），InfoRace（种族信息），InfoAmmo（子弹信息）， InfoVoter（投票者信息）， EnableAllInfo（开启所有信息）， DisableAllInfo（关闭所有信息） or AmmoAbsolute or LifeAbsolute or Lock");
			SendChatTarget(ClientID, "细节 : 开启或关闭.");
		}
	}
	else if(str_comp_nocase(Arguments[0], "/reset_stats") == 0)
	{
		m_apPlayers[ClientID]->m_pStats->ResetPartialStats();
		SendChatTarget(ClientID, "您成功重置了等级 !");
	}
	else if(str_comp_nocase(Arguments[0], "/reset_all_stats") == 0)
	{
		m_apPlayers[ClientID]->m_pStats->ResetAllStats();
		SendChatTarget(ClientID, "您，您成功重置了所有等级 !");
	}
	else if(str_comp_nocase(Arguments[0], "/reset_upgr") == 0)
	{
		m_apPlayers[ClientID]->m_pStats->ResetUpgr();
		SendChatTarget(ClientID, "您成功重置了所有技能 !");
	}
	else
	{
		char error[256] = "";
		str_format(error, 256, "无法识别: %s. 为了获取可使用的指令, 聊天框输入 /cmdlist", Arguments[0]);
		SendChatTarget(ClientID, error);
	}
}
void CGameContext::OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgID, pUnpacker);
	CPlayer *pPlayer = m_apPlayers[ClientID];

	if(!pRawMsg)
	{
		if(g_Config.m_Debug)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgID), MsgID, m_NetObjHandler.FailedMsgOn());
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}
		return;
	}

	if(Server()->ClientIngame(ClientID))
	{
		if(MsgID == NETMSGTYPE_CL_SAY)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat+Server()->TickSpeed() > Server()->Tick() && ((CNetMsg_Cl_Say *)pRawMsg)->m_pMessage[0] != '/')
				return;

			CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
			int Team = pMsg->m_Team ? pPlayer->GetTeam() : CGameContext::CHAT_ALL;
			
			// trim right and set maximum length to 128 utf8-characters
			int Length = 0;
			const char *p = pMsg->m_pMessage;
			const char *pEnd = 0;
			while(*p)
			{
				const char *pStrOld = p;
				int Code = str_utf8_decode(&p);

				// check if unicode is not empty
				if(Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
						(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
						Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
				{
					pEnd = 0;
				}
				else if(pEnd == 0)
					pEnd = pStrOld;

				if(++Length >= 127)
				{
					*(const_cast<char *>(p)) = 0;
					break;
				}
			}
			if(pEnd != 0)
				*(const_cast<char *>(pEnd)) = 0;

			// drop empty and autocreated spam messages (more than 16 characters per second)
			if(Length == 0 || (g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat+Server()->TickSpeed()*((15+Length)/16) > Server()->Tick()))
				return;

			pPlayer->m_LastChat = Server()->Tick();

			if(pMsg->m_pMessage[0]=='/')
			{
				CommandOnChat(pMsg->m_pMessage, ClientID, Team);
			}
			else
			{
				SendChat(ClientID, Team, pMsg->m_pMessage);
				m_apPlayers[ClientID]->m_pStats->AddMessage();
			}
		}
		else if(MsgID == NETMSGTYPE_CL_CALLVOTE)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastVoteTry && pPlayer->m_LastVoteTry+Server()->TickSpeed()*3 > Server()->Tick())
				return;

			int64 Now = Server()->Tick();
			pPlayer->m_LastVoteTry = Now;
			if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			{
				SendChatTarget(ClientID, "Spectators aren't allowed to start a vote.");
				return;
			}

			if(m_VoteCloseTime)
			{
				SendChatTarget(ClientID, "Wait for current vote to end before calling a new one.");
				return;
			}

			int Timeleft = pPlayer->m_LastVoteCall + Server()->TickSpeed()*60 - Now;
			if(pPlayer->m_LastVoteCall && Timeleft > 0)
			{
				char aChatmsg[512] = {0};
				str_format(aChatmsg, sizeof(aChatmsg), "You must wait %d seconds before making another vote", (Timeleft/Server()->TickSpeed())+1);
				SendChatTarget(ClientID, aChatmsg);
				return;
			}

			char aChatmsg[512] = {0};
			char aDesc[VOTE_DESC_LENGTH] = {0};
			char aCmd[VOTE_CMD_LENGTH] = {0};
			CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;
			const char *pReason = pMsg->m_Reason[0] ? pMsg->m_Reason : "No reason given";

			if(str_comp_nocase(pMsg->m_Type, "option") == 0)
			{
				CVoteOptionServer *pOption = m_pVoteOptionFirst;
				while(pOption)
				{
					if(str_comp_nocase(pMsg->m_Value, pOption->m_aDescription) == 0)
					{
						str_format(aChatmsg, sizeof(aChatmsg), "'%s' called vote to change server option '%s' (%s)", Server()->ClientName(ClientID),
								   pOption->m_aDescription, pReason);
						str_format(aDesc, sizeof(aDesc), "%s", pOption->m_aDescription);
						str_format(aCmd, sizeof(aCmd), "%s", pOption->m_aCommand);
						break;
					}

					pOption = pOption->m_pNext;
				}

				if(!pOption)
				{
					str_format(aChatmsg, sizeof(aChatmsg), "'%s' isn't an option on this server", pMsg->m_Value);
					SendChatTarget(ClientID, aChatmsg);
					return;
				}
			}
			else if(str_comp_nocase(pMsg->m_Type, "kick") == 0)
			{
				if(!g_Config.m_SvVoteKick)
				{
					SendChatTarget(ClientID, "Server does not allow voting to kick players");
					return;
				}

				if(g_Config.m_SvForceVoteReason && (str_comp_nocase(pReason, "No reason given") == 0 || str_length(pReason) < 3))
				{
					SendChatTarget(ClientID, "Server does not allow voting to kick players without reason");
					return;
				}
				else if(g_Config.m_SvForceVoteReason && (
							str_comp_nocase(pReason, "lol") == 0 ||
							str_comp_nocase(pReason, "noob") == 0 ||
							str_comp_nocase(pReason, "pro") == 0 ||
							str_comp_nocase(pReason, "gay") == 0 ||
							str_comp_nocase(pReason, "cheat") == 0))
				{
					SendChatTarget(ClientID, "Server does not allow voting to kick players without REAL reason !");
					return;
				}
				if(g_Config.m_SvVoteKickMin)
				{
					int PlayerNum = 0;
					for(int i = 0; i < MAX_CLIENTS; ++i)
						if(m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
							++PlayerNum;

					if(PlayerNum < g_Config.m_SvVoteKickMin)
					{
						str_format(aChatmsg, sizeof(aChatmsg), "Kick voting requires %d players on the server", g_Config.m_SvVoteKickMin);
						SendChatTarget(ClientID, aChatmsg);
						return;
					}
				}

				int KickID = str_toint(pMsg->m_Value);
				if(KickID < 0 || KickID >= MAX_CLIENTS || !m_apPlayers[KickID])
				{
					SendChatTarget(ClientID, "Invalid client id to kick");
					return;
				}
				if(KickID == ClientID)
				{
					SendChatTarget(ClientID, "You can't kick yourself");
					return;
				}
				if(Server()->IsAuthed(KickID))
				{
					SendChatTarget(ClientID, "You can't kick admins");
					char aBufKick[128];
					str_format(aBufKick, sizeof(aBufKick), "'%s' called for vote to kick you (Reason : %s)", Server()->ClientName(ClientID), pReason);
					SendChatTarget(KickID, aBufKick);
					return;
				}

				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to kick '%s' (%s)", Server()->ClientName(ClientID), Server()->ClientName(KickID), pReason);
				str_format(aDesc, sizeof(aDesc), "Kick '%s'", Server()->ClientName(KickID));
				if (!g_Config.m_SvVoteKickBantime)
					str_format(aCmd, sizeof(aCmd), "kick %d Kicked by vote", KickID);
				else
				{
					char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
					Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
					str_format(aCmd, sizeof(aCmd), "ban %s %d Banned by vote", aAddrStr, g_Config.m_SvVoteKickBantime);
				}
			}
			else if(str_comp_nocase(pMsg->m_Type, "spectate") == 0)
			{
				if(!g_Config.m_SvVoteSpectate)
				{
					SendChatTarget(ClientID, "Server does not allow voting to move players to spectators");
					return;
				}

				if(g_Config.m_SvForceVoteReason && ((str_comp_nocase(pReason, "No reason given") == 0 || str_length(pReason) < 3)))
				{
					SendChatTarget(ClientID, "Server does not allow voting to move players to spectators without reason");
					return;
				}
				else if(g_Config.m_SvForceVoteReason && (
							str_comp_nocase(pReason, "lol") == 0 ||
							str_comp_nocase(pReason, "noob") == 0 ||
							str_comp_nocase(pReason, "pro") == 0 ||
							str_comp_nocase(pReason, "gay") == 0 ||
							str_comp_nocase(pReason, "菜") ==0 ||
							str_comp_nocase(pReason, "太强") == 0 ||
							str_comp_nocase(pReason, "傻逼") == 0 ||
							str_comp_nocase(pReason, "开挂") == 0 ||
							str_comp_nocase(pReason, "蠢") == 0 ||
							str_comp_nocase(pReason, "gay") == 0 ||
							str_comp_nocase(pReason, "cheat") == 0))
				{
					SendChatTarget(ClientID, "人不行，别怪路不平 !");
					return;
				}
				int SpectateID = str_toint(pMsg->m_Value);
				if(SpectateID < 0 || SpectateID >= MAX_CLIENTS || !m_apPlayers[SpectateID] || m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
				{
					SendChatTarget(ClientID, "Invalid client id to move");
					return;
				}
				if(SpectateID == ClientID)
				{
					SendChatTarget(ClientID, "You can't move yourself");
					return;
				}

				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to move '%s' to spectators (%s)", Server()->ClientName(ClientID), Server()->ClientName(SpectateID), pReason);
				str_format(aDesc, sizeof(aDesc), "move '%s' to spectators", Server()->ClientName(SpectateID));
				str_format(aCmd, sizeof(aCmd), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
			}

			if(aCmd[0])
			{
				SendChat(-1, CGameContext::CHAT_ALL, aChatmsg);
				StartVote(aDesc, aCmd, pReason);
				pPlayer->m_Vote = 1;
				pPlayer->m_VotePos = m_VotePos = 1;
				m_VoteCreator = ClientID;
				pPlayer->m_LastVoteCall = Now;
			}
		}
		else if(MsgID == NETMSGTYPE_CL_VOTE)
		{
			if(!m_VoteCloseTime)
				return;

			if(pPlayer->m_Vote == 0)
			{
				CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
				if(!pMsg->m_Vote)
					return;

				if(g_Config.m_SvShowPersonalVote)
				{
					char aBuf[128];

					if(pMsg->m_Vote < 0) //no
						str_format(aBuf, sizeof(aBuf), "%s voted no", Server()->ClientName(pPlayer->GetCID()));
					else
						str_format(aBuf, sizeof(aBuf), "%s voted yes", Server()->ClientName(pPlayer->GetCID()));

					SendChatTarget(-1, aBuf, CHAT_INFO_VOTER);
				}
				pPlayer->m_Vote = pMsg->m_Vote;
				pPlayer->m_VotePos = ++m_VotePos;
				m_VoteUpdate = true;
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETTEAM && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)pRawMsg;

			if(pPlayer->GetTeam() == pMsg->m_Team || (g_Config.m_SvSpamprotection && pPlayer->m_LastSetTeam && pPlayer->m_LastSetTeam+Server()->TickSpeed()*3 > Server()->Tick()))
				return;

			if(pMsg->m_Team != TEAM_SPECTATORS && m_LockTeams)
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				SendBroadcast("Teams are locked", ClientID);
				return;
			}

			if(m_pEventsGame->GetActualEventTeam() >= STEAL_TEE && pMsg->m_Team != TEAM_SPECTATORS)
			{
				SendBroadcast("You can't join other team with this event, wait a team capture all players", ClientID);
				return;
			}

			if(pMsg->m_Team != TEAM_SPECTATORS &&
					((m_pEventsGame->IsActualEvent(SURVIVOR) && m_pController->GetNumPlayer(0) > 0)
					 || (m_pController->IsTeamplay() && m_pEventsGame->GetActualEventTeam() == T_SURVIVOR && m_pController->GetNumPlayer(0) > 0 && m_pController->GetNumPlayer(1) > 0)))
			{
				SendBroadcast("You can't join other team with this event, wait a winner", ClientID);
				return;
			}
			if(pPlayer->m_TeamChangeTick > Server()->Tick())
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				int TimeLeft = (pPlayer->m_TeamChangeTick - Server()->Tick())/Server()->TickSpeed();
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Time to wait before changing team: %02d:%02d", TimeLeft/60, TimeLeft%60);
				SendBroadcast(aBuf, ClientID);
				return;
			}

			// Switch team on given client and kill/respawn him
			if(m_pController->CanJoinTeam(pMsg->m_Team, ClientID))
			{
				if(m_pController->CanChangeTeam(pPlayer, pMsg->m_Team))
				{
					if(m_apPlayers[ClientID]->GetSID() < 0)
					{
						SendBroadcast("你作为陌生人登录，不会保存你的任何记录 !", ClientID);
						SendChatTarget(ClientID, "你作为陌生人登录，不会保存你的任何记录 ! 如果你想保持你的账号数据, 请输入/reg注册一个账号，或使用/login登录你的账号 ! 玩的开心 ;)", ClientID);
						m_apPlayers[ClientID]->SetSID(0);
					}
					pPlayer->m_LastSetTeam = Server()->Tick();
					if(pPlayer->GetTeam() == TEAM_SPECTATORS || pMsg->m_Team == TEAM_SPECTATORS)
						m_VoteUpdate = true;
					pPlayer->SetTeam(pMsg->m_Team);
					(void)m_pController->CheckTeamBalance();
					pPlayer->m_TeamChangeTick = Server()->Tick();
				}
				else
					SendBroadcast("Teams must be balanced, please join other team", ClientID);
			}
			else
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Only %d active players are allowed", Server()->MaxClients()-g_Config.m_SvSpectatorSlots);
				SendBroadcast(aBuf, ClientID);
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETSPECTATORMODE && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetSpectatorMode *pMsg = (CNetMsg_Cl_SetSpectatorMode *)pRawMsg;

			if(pPlayer->GetTeam() != TEAM_SPECTATORS || pPlayer->m_SpectatorID == pMsg->m_SpectatorID || ClientID == pMsg->m_SpectatorID ||
					(g_Config.m_SvSpamprotection && pPlayer->m_LastSetSpectatorMode && pPlayer->m_LastSetSpectatorMode+Server()->TickSpeed()*3 > Server()->Tick()))
				return;

			pPlayer->m_LastSetSpectatorMode = Server()->Tick();
			if(pMsg->m_SpectatorID != SPEC_FREEVIEW && (!m_apPlayers[pMsg->m_SpectatorID] || m_apPlayers[pMsg->m_SpectatorID]->GetTeam() == TEAM_SPECTATORS))
				SendChatTarget(ClientID, "Invalid spectator id used");
			else
				pPlayer->m_SpectatorID = pMsg->m_SpectatorID;
		}
		else if (MsgID == NETMSGTYPE_CL_CHANGEINFO)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastChangeInfo && pPlayer->m_LastChangeInfo+Server()->TickSpeed()*5 > Server()->Tick())
				return;

			CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set infos
			char aOldName[MAX_NAME_LENGTH];
			str_copy(aOldName, Server()->ClientName(ClientID), sizeof(aOldName));
			Server()->SetClientName(ClientID, pMsg->m_pName);
			if(str_comp(aOldName, Server()->ClientName(ClientID)) != 0)
			{
				char aChatText[256];
				str_format(aChatText, sizeof(aChatText), "'%s' changed name to '%s'", aOldName, Server()->ClientName(ClientID));
				SendChat(-1, CGameContext::CHAT_ALL, aChatText);
			}
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);
			m_apPlayers[ClientID]->m_pStats->UpdateInfo();
		}
		else if (MsgID == NETMSGTYPE_CL_EMOTICON && !m_World.m_Paused)
		{
			CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;

			if(g_Config.m_SvSpamprotection && pPlayer->m_LastEmote && pPlayer->m_LastEmote+Server()->TickSpeed()*3 > Server()->Tick())
				return;

			pPlayer->m_LastEmote = Server()->Tick();

			SendEmoticon(ClientID, pMsg->m_Emoticon);
		}
		else if (MsgID == NETMSGTYPE_CL_KILL && !m_World.m_Paused)
		{
			if(pPlayer->m_LastKill && pPlayer->m_LastKill+Server()->TickSpeed()*3 > Server()->Tick())
				return;

			pPlayer->m_LastKill = Server()->Tick();
			pPlayer->KillCharacter(WEAPON_SELF);
		}
	}
	else
	{
		if(MsgID == NETMSGTYPE_CL_STARTINFO)
		{
			if(pPlayer->m_IsReady)
				return;

			CNetMsg_Cl_StartInfo *pMsg = (CNetMsg_Cl_StartInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set start infos
			Server()->SetClientName(ClientID, pMsg->m_pName);
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);

			// send vote options
			CNetMsg_Sv_VoteClearOptions ClearMsg;
			Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);

			CNetMsg_Sv_VoteOptionListAdd OptionMsg;
			int NumOptions = 0;
			OptionMsg.m_pDescription0 = "";
			OptionMsg.m_pDescription1 = "";
			OptionMsg.m_pDescription2 = "";
			OptionMsg.m_pDescription3 = "";
			OptionMsg.m_pDescription4 = "";
			OptionMsg.m_pDescription5 = "";
			OptionMsg.m_pDescription6 = "";
			OptionMsg.m_pDescription7 = "";
			OptionMsg.m_pDescription8 = "";
			OptionMsg.m_pDescription9 = "";
			OptionMsg.m_pDescription10 = "";
			OptionMsg.m_pDescription11 = "";
			OptionMsg.m_pDescription12 = "";
			OptionMsg.m_pDescription13 = "";
			OptionMsg.m_pDescription14 = "";
			CVoteOptionServer *pCurrent = m_pVoteOptionFirst;
			while(pCurrent)
			{
				switch(NumOptions++)
				{
				case 0: OptionMsg.m_pDescription0 = pCurrent->m_aDescription; break;
				case 1: OptionMsg.m_pDescription1 = pCurrent->m_aDescription; break;
				case 2: OptionMsg.m_pDescription2 = pCurrent->m_aDescription; break;
				case 3: OptionMsg.m_pDescription3 = pCurrent->m_aDescription; break;
				case 4: OptionMsg.m_pDescription4 = pCurrent->m_aDescription; break;
				case 5: OptionMsg.m_pDescription5 = pCurrent->m_aDescription; break;
				case 6: OptionMsg.m_pDescription6 = pCurrent->m_aDescription; break;
				case 7: OptionMsg.m_pDescription7 = pCurrent->m_aDescription; break;
				case 8: OptionMsg.m_pDescription8 = pCurrent->m_aDescription; break;
				case 9: OptionMsg.m_pDescription9 = pCurrent->m_aDescription; break;
				case 10: OptionMsg.m_pDescription10 = pCurrent->m_aDescription; break;
				case 11: OptionMsg.m_pDescription11 = pCurrent->m_aDescription; break;
				case 12: OptionMsg.m_pDescription12 = pCurrent->m_aDescription; break;
				case 13: OptionMsg.m_pDescription13 = pCurrent->m_aDescription; break;
				case 14:
				{
					OptionMsg.m_pDescription14 = pCurrent->m_aDescription;
					OptionMsg.m_NumOptions = NumOptions;
					Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
					OptionMsg = CNetMsg_Sv_VoteOptionListAdd();
					NumOptions = 0;
					OptionMsg.m_pDescription1 = "";
					OptionMsg.m_pDescription2 = "";
					OptionMsg.m_pDescription3 = "";
					OptionMsg.m_pDescription4 = "";
					OptionMsg.m_pDescription5 = "";
					OptionMsg.m_pDescription6 = "";
					OptionMsg.m_pDescription7 = "";
					OptionMsg.m_pDescription8 = "";
					OptionMsg.m_pDescription9 = "";
					OptionMsg.m_pDescription10 = "";
					OptionMsg.m_pDescription11 = "";
					OptionMsg.m_pDescription12 = "";
					OptionMsg.m_pDescription13 = "";
					OptionMsg.m_pDescription14 = "";
				}
				}
				pCurrent = pCurrent->m_pNext;
			}
			if(NumOptions > 0)
			{
				OptionMsg.m_NumOptions = NumOptions;
				Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
			}

			// send tuning parameters to client
			SendTuningParams(ClientID);

			// client is ready to enter
			pPlayer->m_IsReady = true;
			CNetMsg_Sv_ReadyToEnter m;
			Server()->SendPackMsg(&m, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);
		}
	}
}

void CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if(pSelf->Tuning()->Set(pParamName, NewValue))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s changed to %.2f", pParamName, NewValue);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
		pSelf->SendTuningParams(-1);
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "No such tuning parameter");
}

void CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams TuningParams;
	*pSelf->Tuning() = TuningParams;
	pSelf->SendTuningParams(-1);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "Tuning reset");
}

void CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	for(int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		str_format(aBuf, sizeof(aBuf), "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
	}
}

void CGameContext::ConPause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pSelf->m_pController->IsGameOver())
		return;

	pSelf->m_World.m_Paused ^= 1;
}

void CGameContext::ConChangeMap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->ChangeMap(pResult->NumArguments() ? pResult->GetString(0) : "");
}

void CGameContext::ConRestart(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments())
		pSelf->m_pController->DoWarmup(pResult->GetInteger(0));
	else
		pSelf->m_pController->StartRound();
}

void CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendBroadcast(pResult->GetString(0), -1);
}

void CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, pResult->GetString(0));
}

void CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	int Delay = pResult->NumArguments()>2 ? pResult->GetInteger(2) : 0;
	if(!pSelf->m_apPlayers[ClientID])
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "moved client %d to team %d", ClientID, Team);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	pSelf->m_apPlayers[ClientID]->m_TeamChangeTick = pSelf->Server()->Tick()+pSelf->Server()->TickSpeed()*Delay*60;
	pSelf->m_apPlayers[ClientID]->SetTeam(Team);
	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConSetTeamAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Team = clamp(pResult->GetInteger(0), -1, 1);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "All players were moved to the %s", pSelf->m_pController->GetTeamName(Team));
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(pSelf->m_apPlayers[i])
			pSelf->m_apPlayers[i]->SetTeam(Team, false);

	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConSwapTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SwapTeams();
}

void CGameContext::ConShuffleTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController->IsTeamplay())
		return;

	int CounterRed = 0;
	int CounterBlue = 0;
	int PlayerTeam = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			++PlayerTeam;
	PlayerTeam = (PlayerTeam+1)/2;
	
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were shuffled");

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			if(CounterRed == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_BLUE, false);
			else if(CounterBlue == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_RED, false);
			else
			{
				if(rand() % 2)
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_BLUE, false);
					++CounterBlue;
				}
				else
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_RED, false);
					++CounterRed;
				}
			}
		}
	}

	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConLockTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_LockTeams ^= 1;
	if(pSelf->m_LockTeams)
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were locked");
	else
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were unlocked");
}

void CGameContext::ConAddVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);
	const char *pCommand = pResult->GetString(1);

	if(pSelf->m_NumVoteOptions == MAX_VOTE_OPTIONS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "maximum number of vote options reached");
		return;
	}

	// check for valid option
	if(!pSelf->Console()->LineIsValid(pCommand) || str_length(pCommand) >= VOTE_CMD_LENGTH)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid command '%s'", pCommand);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}
	while(*pDescription && *pDescription == ' ')
		pDescription++;
	if(str_length(pDescription) >= VOTE_DESC_LENGTH || *pDescription == 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid option '%s'", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// check for duplicate entry
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while(pOption)
	{
		if(str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "option '%s' already exists", pDescription);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
		pOption = pOption->m_pNext;
	}

	// add the option
	++pSelf->m_NumVoteOptions;
	int Len = str_length(pCommand);

	pOption = (CVoteOptionServer *)pSelf->m_pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
	pOption->m_pNext = 0;
	pOption->m_pPrev = pSelf->m_pVoteOptionLast;
	if(pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	pSelf->m_pVoteOptionLast = pOption;
	if(!pSelf->m_pVoteOptionFirst)
		pSelf->m_pVoteOptionFirst = pOption;

	str_copy(pOption->m_aDescription, pDescription, sizeof(pOption->m_aDescription));
	mem_copy(pOption->m_aCommand, pCommand, Len+1);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "added option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConRemoveVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);

	// check for valid option
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while(pOption)
	{
		if(str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
			break;
		pOption = pOption->m_pNext;
	}
	if(!pOption)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "option '%s' does not exist", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// inform clients about removed option
	CNetMsg_Sv_VoteOptionRemove OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);

	// TODO: improve this
	// remove the option
	--pSelf->m_NumVoteOptions;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "removed option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	CHeap *pVoteOptionHeap = new CHeap();
	CVoteOptionServer *pVoteOptionFirst = 0;
	CVoteOptionServer *pVoteOptionLast = 0;
	int NumVoteOptions = pSelf->m_NumVoteOptions;
	for(CVoteOptionServer *pSrc = pSelf->m_pVoteOptionFirst; pSrc; pSrc = pSrc->m_pNext)
	{
		if(pSrc == pOption)
			continue;

		// copy option
		int Len = str_length(pSrc->m_aCommand);
		CVoteOptionServer *pDst = (CVoteOptionServer *)pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
		pDst->m_pNext = 0;
		pDst->m_pPrev = pVoteOptionLast;
		if(pDst->m_pPrev)
			pDst->m_pPrev->m_pNext = pDst;
		pVoteOptionLast = pDst;
		if(!pVoteOptionFirst)
			pVoteOptionFirst = pDst;

		str_copy(pDst->m_aDescription, pSrc->m_aDescription, sizeof(pDst->m_aDescription));
		mem_copy(pDst->m_aCommand, pSrc->m_aCommand, Len+1);
	}

	// clean up
	delete pSelf->m_pVoteOptionHeap;
	pSelf->m_pVoteOptionHeap = pVoteOptionHeap;
	pSelf->m_pVoteOptionFirst = pVoteOptionFirst;
	pSelf->m_pVoteOptionLast = pVoteOptionLast;
	pSelf->m_NumVoteOptions = NumVoteOptions;
}

void CGameContext::ConForceVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pType = pResult->GetString(0);
	const char *pValue = pResult->GetString(1);
	const char *pReason = pResult->NumArguments() > 2 && pResult->GetString(2)[0] ? pResult->GetString(2) : "No reason given";
	char aBuf[128] = {0};

	if(str_comp_nocase(pType, "option") == 0)
	{
		CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
		while(pOption)
		{
			if(str_comp_nocase(pValue, pOption->m_aDescription) == 0)
			{
				str_format(aBuf, sizeof(aBuf), "admin forced server option '%s' (%s)", pValue, pReason);
				pSelf->SendChatTarget(-1, aBuf);
				pSelf->Console()->ExecuteLine(pOption->m_aCommand);
				break;
			}

			pOption = pOption->m_pNext;
		}

		if(!pOption)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' isn't an option on this server", pValue);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
	}
	else if(str_comp_nocase(pType, "kick") == 0)
	{
		int KickID = str_toint(pValue);
		if(KickID < 0 || KickID >= MAX_CLIENTS || !pSelf->m_apPlayers[KickID])
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to kick");
			return;
		}

		if (!g_Config.m_SvVoteKickBantime)
		{
			str_format(aBuf, sizeof(aBuf), "kick %d %s", KickID, pReason);
			pSelf->Console()->ExecuteLine(aBuf);
		}
		else
		{
			char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
			pSelf->Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
			str_format(aBuf, sizeof(aBuf), "ban %s %d %s", aAddrStr, g_Config.m_SvVoteKickBantime, pReason);
			pSelf->Console()->ExecuteLine(aBuf);
		}
	}
	else if(str_comp_nocase(pType, "spectate") == 0)
	{
		int SpectateID = str_toint(pValue);
		if(SpectateID < 0 || SpectateID >= MAX_CLIENTS || !pSelf->m_apPlayers[SpectateID] || pSelf->m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to move");
			return;
		}

		str_format(aBuf, sizeof(aBuf), "admin moved '%s' to spectator (%s)", pSelf->Server()->ClientName(SpectateID), pReason);
		pSelf->SendChatTarget(-1, aBuf);
		str_format(aBuf, sizeof(aBuf), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
		pSelf->Console()->ExecuteLine(aBuf);
	}
}

void CGameContext::ConClearVotes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "cleared votes");
	CNetMsg_Sv_VoteClearOptions VoteClearOptionsMsg;
	pSelf->Server()->SendPackMsg(&VoteClearOptionsMsg, MSGFLAG_VITAL, -1);
	pSelf->m_pVoteOptionHeap->Reset();
	pSelf->m_pVoteOptionFirst = 0;
	pSelf->m_pVoteOptionLast = 0;
	pSelf->m_NumVoteOptions = 0;
}

void CGameContext::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	// check if there is a vote running
	if(!pSelf->m_VoteCloseTime)
		return;

	if(str_comp_nocase(pResult->GetString(0), "yes") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_YES;
	else if(str_comp_nocase(pResult->GetString(0), "no") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "admin forced vote %s", pResult->GetString(0));
	pSelf->SendChatTarget(-1, aBuf);
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pResult->GetString(0));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameContext::ConNextEvent(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pEventsGame->NextEvent();
}

void CGameContext::ConNextRandomEvent(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pEventsGame->NextRandomEvent();
}

void CGameContext::ConSetEvent(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!pSelf->m_pEventsGame->SetEvent(pResult->GetInteger(0)))
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid event id to set");
}

void CGameContext::ConAddTimeEvent(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (pResult->NumArguments())
	{
		if (!pSelf->m_pEventsGame->AddTime(pResult->GetInteger(0)))
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid time to add");
	}
	else
	{
		if (!pSelf->m_pEventsGame->AddTime())
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Error unknown to add time");
	}
}

void CGameContext::ConNextEventTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if ( pSelf->m_pController->IsTeamplay() )
		pSelf->m_pEventsGame->NextEventTeam();
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "The actual gametype is not with teams");
		pSelf->SendChatTarget(-1, "The actual gametype is not with teams !");
	}
}

void CGameContext::ConNextRandomEventTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if ( pSelf->m_pController->IsTeamplay() )
		pSelf->m_pEventsGame->NextRandomEventTeam();
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "The actual gametype is not with teams");
		pSelf->SendChatTarget(-1, "The actual gametype is not with teams !");
	}
}

void CGameContext::ConSetEventTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if ( pSelf->m_pController->IsTeamplay() )
	{
		if (!pSelf->m_pEventsGame->SetEventTeam(pResult->GetInteger(0)))
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid event id to set");
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "The actual gametype is not with teams");
		pSelf->SendChatTarget(-1, "The actual gametype is not with teams !");
	}
}

void CGameContext::ConAddTimeEventTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if ( pSelf->m_pController->IsTeamplay() )
	{
		if (pResult->NumArguments())
		{
			if (!pSelf->m_pEventsGame->AddTimeTeam(pResult->GetInteger(0)))
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid time to add");
		}
		else
		{
			if (!pSelf->m_pEventsGame->AddTimeTeam())
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Error unknown to add time");
		}
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "The actual gametype is not with teams");
}

void CGameContext::ConListPlayer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "List of Players :");

	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( pSelf->m_apPlayers[i] )
		{
			char Text[256] = "";
			str_format(Text, 256, "Client ID : %d. Name : %s. Stats ID : %ld.", i, pSelf->Server()->ClientName(i), pSelf->m_apPlayers[i]->GetSID());
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", Text);
		}
	}
}

void CGameContext::ConGiveShotgun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				pSelf->m_apPlayers[i]->GetCharacter()->GiveWeapon(WEAPON_SHOTGUN, 10);
				pSelf->SendChatTarget(i, "管理员给了你一把散弹枪 !");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		pSelf->m_apPlayers[id]->GetCharacter()->GiveWeapon(WEAPON_SHOTGUN, 10);
		pSelf->SendChatTarget(id, "管理员给了你一把散弹枪 !");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConGiveGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				if(pSelf->m_apPlayers[i]->GetCharacter()->GiveWeapon(WEAPON_GRENADE, 10))
					pSelf->SendChatTarget(i, "管理员给了你一把榴弹枪 !");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		if(pSelf->m_apPlayers[id]->GetCharacter()->GiveWeapon(WEAPON_GRENADE, 10))
			pSelf->SendChatTarget(id, "管理员给了你一把榴弹枪 !");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConGiveRifle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				if (pSelf->m_apPlayers[i]->GetCharacter()->GiveWeapon(WEAPON_RIFLE, 10))
					pSelf->SendChatTarget(i, "管理员给了你一把激光枪 !");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		if (pSelf->m_apPlayers[id]->GetCharacter()->GiveWeapon(WEAPON_RIFLE, 10))
			pSelf->SendChatTarget(id, "管理员给了你一把激光枪 !");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConGiveKatana(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				if (pSelf->m_apPlayers[i]->GetCharacter()->GiveNinja())
					pSelf->SendChatTarget(i, "管理员给了你忍者 !");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		if (pSelf->m_apPlayers[id]->GetCharacter()->GiveNinja())
			pSelf->SendChatTarget(id, "管理员给了你忍者 !");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConGiveProtect(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				pSelf->m_apPlayers[i]->GetCharacter()->m_Protect = -1;
				pSelf->SendChatTarget(i, "An admin give to you a protection");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		pSelf->m_apPlayers[id]->GetCharacter()->m_Protect = -1;
		pSelf->SendChatTarget(id, "An admin give to you a protection");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConGiveInvisibility(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				pSelf->m_apPlayers[i]->GetCharacter()->m_Invisibility = true;
				pSelf->SendChatTarget(i, "An admin give to you an invisibility");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		pSelf->m_apPlayers[id]->GetCharacter()->m_Invisibility = true;
		pSelf->SendChatTarget(id, "An admin give to you an invisibility");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConRemoveShotgun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				if(pSelf->m_apPlayers[i]->GetCharacter()->RemoveWeapon(WEAPON_SHOTGUN))
					pSelf->SendChatTarget(i, "An admin remove your shotgun !");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		if(pSelf->m_apPlayers[id]->GetCharacter()->RemoveWeapon(WEAPON_SHOTGUN))
			pSelf->SendChatTarget(id, "An admin remove your shotgun !");
	}

	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConRemoveGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				if(pSelf->m_apPlayers[i]->GetCharacter()->RemoveWeapon(WEAPON_GRENADE))
					pSelf->SendChatTarget(i, "An admin remove your launch-grenade !");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		if(pSelf->m_apPlayers[id]->GetCharacter()->RemoveWeapon(WEAPON_GRENADE))
			pSelf->SendChatTarget(id, "An admin remove your launch-grenade !");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConRemoveRifle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				if(pSelf->m_apPlayers[i]->GetCharacter()->RemoveWeapon(WEAPON_RIFLE))
					pSelf->SendChatTarget(i, "An admin remove your rifle !");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		if(pSelf->m_apPlayers[id]->GetCharacter()->RemoveWeapon(WEAPON_RIFLE))
			pSelf->SendChatTarget(id, "An admin remove your rifle !");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConRemoveKatana(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() )
			{
				if(pSelf->m_apPlayers[i]->GetCharacter()->RemoveWeapon(WEAPON_NINJA))
					pSelf->SendChatTarget(i, "An admin remove your katana !");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() )
	{
		if(pSelf->m_apPlayers[id]->GetCharacter()->RemoveWeapon(WEAPON_NINJA))
			pSelf->SendChatTarget(id, "An admin remove your katana !");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConRemoveProtect(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() && pSelf->m_apPlayers[i]->GetCharacter()->m_Protect != 0)
			{
				pSelf->m_apPlayers[i]->GetCharacter()->m_Protect = 0;
				pSelf->SendChatTarget(i, "An admin remove your protection");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() && pSelf->m_apPlayers[id]->GetCharacter()->m_Protect != 0)
	{
		pSelf->m_apPlayers[id]->GetCharacter()->m_Protect = 0;
		pSelf->SendChatTarget(id, "An admin remove your protection");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConRemoveInvisibility(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int id = pResult->GetInteger(0);
	if ( id == -1 )
	{
		for (int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetCharacter() && pSelf->m_apPlayers[i]->GetCharacter()->m_Invisibility == true)
			{
				pSelf->m_apPlayers[i]->GetCharacter()->m_Invisibility = false;
				pSelf->SendChatTarget(i, "An admin remove your invisibility");
			}
		}
	}
	else if ( pSelf->m_apPlayers[id] && pSelf->m_apPlayers[id]->GetCharacter() && pSelf->m_apPlayers[id]->GetCharacter()->m_Invisibility == true)
	{
		pSelf->m_apPlayers[id]->GetCharacter()->m_Invisibility = false;
		pSelf->SendChatTarget(id, "An admin remove your invisibility");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid ID");
}

void CGameContext::ConSetSid(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetInteger(1);
	unsigned int StatID = pResult->GetInteger(0);
	if ( ClientID >= 0 && ClientID < MAX_CLIENTS && pSelf->m_apPlayers[ClientID] )
	{
		if (StatID >= 0 && pSelf->m_apPlayers[ClientID]->SetSID(StatID))
		{
			/*BASIC IMPLANTATION*/
		}
		else
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid Stat ID");
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid Client ID");
}

void CGameContext::ConGiveXP(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetInteger(0);
	int XP = pResult->GetInteger(1);
	if (ClientID >= 0 && ClientID < MAX_CLIENTS && pSelf->m_apPlayers[ClientID])
	{
		pSelf->m_apPlayers[ClientID]->m_pStats->AddXP(XP);
		pSelf->m_apPlayers[ClientID]->m_pStats->UpdateStat();
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid Client ID");
}

void CGameContext::ConSpawnMonster(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int MonsterID = pResult->GetInteger(0);
	if (MonsterID < 0 || MonsterID > MAX_MONSTERS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid Monster ID");
		return;
	}
	else if (pSelf->m_apMonsters[MonsterID])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Monster ID already used !");
		return;
	}

	pSelf->m_apMonsters[MonsterID] = new CMonster(&pSelf->m_World, TYPE_HAMMER, MonsterID, -1, 0, 1);
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		CGameContext *pSelf = (CGameContext *)pUserData;
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(pSelf->m_apPlayers[i])
				pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}
}

void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "Dump tuning");

	Console()->Register("pause", "", CFGFLAG_SERVER, ConPause, this, "Pause/unpause game");
	Console()->Register("change_map", "?r", CFGFLAG_SERVER|CFGFLAG_STORE, ConChangeMap, this, "Change map");
	Console()->Register("restart", "?i", CFGFLAG_SERVER|CFGFLAG_STORE, ConRestart, this, "Restart in x seconds (0 = abort)");
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "Broadcast message");
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "Say in chat");
	Console()->Register("set_team", "ii?i", CFGFLAG_SERVER, ConSetTeam, this, "Set team of player to team");
	Console()->Register("set_team_all", "i", CFGFLAG_SERVER, ConSetTeamAll, this, "Set team of all players to team");
	Console()->Register("swap_teams", "", CFGFLAG_SERVER, ConSwapTeams, this, "Swap the current teams");
	Console()->Register("shuffle_teams", "", CFGFLAG_SERVER, ConShuffleTeams, this, "Shuffle the current teams");
	Console()->Register("lock_teams", "", CFGFLAG_SERVER, ConLockTeams, this, "Lock/unlock teams");

	Console()->Register("add_vote", "sr", CFGFLAG_SERVER, ConAddVote, this, "Add a voting option");
	Console()->Register("remove_vote", "s", CFGFLAG_SERVER, ConRemoveVote, this, "Remove a voting option");
	Console()->Register("force_vote", "ss?r", CFGFLAG_SERVER, ConForceVote, this, "Force a voting option");
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, ConClearVotes, this, "Clears the voting options");
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConVote, this, "Force a vote to yes/no");

	Console()->Register("next_event", "", CFGFLAG_SERVER, ConNextEvent, this, "Next Event for the game.");
	Console()->Register("next_random_event", "", CFGFLAG_SERVER, ConNextRandomEvent, this, "Next Random Event for the game.");
	Console()->Register("set_event", "i", CFGFLAG_SERVER, ConSetEvent, this, "Set Event for the game.");
	Console()->Register("add_time_event", "?i", CFGFLAG_SERVER, ConAddTimeEvent, this, "Add more time to the actual Event.");

	Console()->Register("next_event_team", "", CFGFLAG_SERVER, ConNextEventTeam, this, "Next Event for the game.");
	Console()->Register("next_random_event_team", "", CFGFLAG_SERVER, ConNextRandomEventTeam, this, "Next Random Event for the game.");
	Console()->Register("set_event_team", "i", CFGFLAG_SERVER, ConSetEventTeam, this, "Set Event for the game.");
	Console()->Register("add_time_event_team", "?i", CFGFLAG_SERVER, ConAddTimeEventTeam, this, "Add more time to the actual Event.");

	Console()->Register("list_player", "", CFGFLAG_SERVER, ConListPlayer, this, "List name players, clients id and statistics id.");
	Console()->Register("shotgun", "i", CFGFLAG_SERVER, ConGiveShotgun, this, "Give shotgun to the player with this client id.");
	Console()->Register("grenade", "i", CFGFLAG_SERVER, ConGiveGrenade, this, "Give grenade to the player with this client id.");
	Console()->Register("rifle", "i", CFGFLAG_SERVER, ConGiveRifle, this, "Give rifle to the player with this client id.");
	Console()->Register("katana", "i", CFGFLAG_SERVER, ConGiveKatana, this, "Give katana to the player with this client id.");
	Console()->Register("protect", "i", CFGFLAG_SERVER, ConGiveProtect, this, "Give protection to the player with this client id.");
	Console()->Register("invisibility", "i", CFGFLAG_SERVER, ConGiveInvisibility, this, "Give invisibility to the player with this client id.");
	Console()->Register("unshotgun", "i", CFGFLAG_SERVER, ConRemoveShotgun, this, "Remove shotgun from the player with this client id.");
	Console()->Register("ungrenade", "i", CFGFLAG_SERVER, ConRemoveGrenade, this, "Remove grenade from the player with this client id.");
	Console()->Register("unrifle", "i", CFGFLAG_SERVER, ConRemoveRifle, this, "Remove rifle from the player with this client id.");
	Console()->Register("unkatana", "i", CFGFLAG_SERVER, ConRemoveKatana, this, "Remove katana from the player with this client id.");
	Console()->Register("unprotect", "i", CFGFLAG_SERVER, ConRemoveProtect, this, "Remove protection from the player with this client id.");
	Console()->Register("uninvisibility", "i", CFGFLAG_SERVER, ConRemoveInvisibility, this, "Remove invisibility from the player with this client id.");

	Console()->Register("set_sid", "ii", CFGFLAG_SERVER, ConSetSid, this, "Set the stats_id to the ClientID.");
	Console()->Register("xp", "ii", CFGFLAG_SERVER, ConGiveXP, this, "Give some xp to the ClientID.");
	Console()->Register("spawn", "i", CFGFLAG_SERVER, ConSpawnMonster, this, "Spawn a monster.");
	Console()->Chain("sv_motd", ConchainSpecialMotdupdate, this);
}

void CGameContext::OnInit(/*class IKernel *pKernel*/)
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_World.SetGameServer(this);
	m_Events.SetGameServer(this);

	//if(!data) // only load once
	//data = load_data_from_memory(internal_data);

	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Server()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	m_Layers.Init(Kernel());
	m_Collision.Init(&m_Layers);

	// reset everything here
	//world = new GAMEWORLD;
	//players = new CPlayer[MAX_CLIENTS];

	// select gametype
	if(str_comp(g_Config.m_SvGametype, "mod") == 0)
		m_pController = new CGameControllerMOD(this);
	else if(str_comp(g_Config.m_SvGametype, "ctf") == 0)
		m_pController = new CGameControllerCTF(this);
	else if(str_comp(g_Config.m_SvGametype, "tdm") == 0)
		m_pController = new CGameControllerTDM(this);
	else
		m_pController = new CGameControllerDM(this);

	// setup core world
	//for(int i = 0; i < MAX_CLIENTS; i++)
	//	game.players[i].core.world = &game.world.core;

	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = m_Layers.GameLayer();
	CTile *pTiles = (CTile *)Kernel()->RequestInterface<IMap>()->GetData(pTileMap->m_Data);




	/*
	num_spawn_points[0] = 0;
	num_spawn_points[1] = 0;
	num_spawn_points[2] = 0;
	*/

	for(int y = 0; y < pTileMap->m_Height; y++)
	{
		for(int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width+x].m_Index;

			if(Index >= ENTITY_OFFSET)
			{
				vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);
				m_pController->OnEntity(Index-ENTITY_OFFSET, Pos);
			}
		}
	}

#if defined(CONF_SQL)
	m_pStatsServer->OnInit();
#endif

	for (int i = 0; i < g_Config.m_SvNumMonster; i++)
		NewMonster();

#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		for(int i = 0; i < g_Config.m_DbgDummies ; i++)
		{
			OnClientConnected(MAX_CLIENTS-i-1);
		}
	}
#endif
}

void CGameContext::OnShutdown()
{
	delete m_pController;
	m_pController = 0;
	Clear();
}

void CGameContext::OnSnap(int ClientID)
{
	// add tuning to demo
	CTuningParams StandardTuning;
	if(ClientID == -1 && Server()->DemoRecorder_IsRecording() && mem_comp(&StandardTuning, &m_Tuning, sizeof(CTuningParams)) != 0)
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&m_Tuning;
		for(unsigned i = 0; i < sizeof(m_Tuning)/sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Server()->SendMsg(&Msg, MSGFLAG_RECORD|MSGFLAG_NOSEND, ClientID);
	}

	m_World.Snap(ClientID);
	m_pController->Snap(ClientID);
	m_Events.Snap(ClientID);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
			m_apPlayers[i]->Snap(ClientID);
	}
}
void CGameContext::OnPreSnap() {}
void CGameContext::OnPostSnap()
{
	m_Events.Clear();
}

bool CGameContext::IsClientReady(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_IsReady ? true : false;
}

bool CGameContext::IsClientPlayer(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetTeam() != TEAM_SPECTATORS ? true : false;
}

bool CGameContext::IsValidPlayer(int ClientID)
{
	if(ClientID >= MAX_CLIENTS || ClientID < 0)
		return false;

	if(!m_apPlayers[ClientID])
		return false;

	return true;
}

CMonster *CGameContext::GetValidMonster(int MonsterID) const
{
	if(MonsterID >= MAX_MONSTERS || MonsterID < 0)
		return 0;

	if(!m_apMonsters[MonsterID])
		return 0;

	return m_apMonsters[MonsterID];
}

void CGameContext::NewMonster()
{
	for(int i = 0; i < MAX_MONSTERS; i++)
	{
		if(!GetValidMonster(i))
		{
			m_apMonsters[i] = new CMonster(&m_World, rand()%(NUM_WEAPONS-2) + 2, i, 10, 10, g_Config.m_SvDifficulty);
			break;
		}
	}
}

void CGameContext::OnMonsterDeath(int MonsterID)
{
	if(!GetValidMonster(MonsterID))
		return;

	m_apMonsters[MonsterID]->Destroy();
	delete m_apMonsters[MonsterID];
	m_apMonsters[MonsterID] = 0;
}

const char *CGameContext::GameType() { return m_pController && m_pController->m_pGameType ? m_pController->m_pGameType : ""; }
const char *CGameContext::Version() { return GAME_VERSION; }
const char *CGameContext::NetVersion() { return GAME_NETVERSION; }

IGameServer *CreateGameServer() { return new CGameContext; }
