/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#include <new>
#include <engine/shared/config.h>
#include <game/server/statistics/statistiques.h>
#include <game/server/statistics/stats_server.h>
#include <game/server/event.h>
#include "player.h"

MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	if ( m_pGameServer->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE && Team != TEAM_SPECTATORS )
		Team = TEAM_RED;
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();
	m_TeamChangeTick = Server()->Tick();
	m_level = 0;
	m_EndBroadcastTick = 0;
	m_Race = HUMAN;
	for (int i = 0; i < NUM_WEAPONS; i++)
		m_WeaponType[i] = HUMAN;
	m_Mine = 0;
	m_pStats = 0;
	m_pStats = new CStats(this, m_pGameServer);
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
	m_pStats->SetStopPlay();
	m_pStats->WriteAll();
	delete m_pStats;
	m_pStats = 0;
}

void CPlayer::Tick()
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	Server()->SetClientScore(m_ClientID, m_Score);

	// do latency stuff
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(!GameServer()->m_World.m_Paused)
	{
		if(!m_pCharacter && m_Team == TEAM_SPECTATORS && m_SpectatorID == SPEC_FREEVIEW)
			m_ViewPos -= vec2(clamp(m_ViewPos.x-m_LatestActivity.m_TargetX, -500.0f, 500.0f), clamp(m_ViewPos.y-m_LatestActivity.m_TargetY, -400.0f, 400.0f));

		if(!m_pCharacter && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
			m_Spawning = true;

		if(m_pCharacter)
		{
			if(m_pCharacter->IsAlive())
			{
				m_ViewPos = m_pCharacter->m_Pos;
			}
			else
			{
				delete m_pCharacter;
				m_pCharacter = 0;
			}
		}
		else if(m_Spawning && m_RespawnTick <= Server()->Tick())
			TryRespawn();
	}
	else
	{
		++m_RespawnTick;
		++m_DieTick;
		++m_ScoreStartTick;
		++m_LastActionTick;
		++m_TeamChangeTick;
 	}
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS && m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[m_SpectatorID])
		m_ViewPos = GameServer()->m_apPlayers[m_SpectatorID]->m_ViewPos;
}

void CPlayer::Snap(int SnappingClient)
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, m_ClientID, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;
	if ( GameServer()->m_pEventsGame->GetActualEventTeam() == ANONYMOUS && SnappingClient != m_ClientID )
		StrToInts(&pClientInfo->m_Name0, 4, "");
	else
	{
		char Prefix[10] = "";
		if (Server()->IsAuthed(m_ClientID) == AUTHED_ADMIN)
			str_append(Prefix, "A", 10);
		else if (Server()->IsAuthed(m_ClientID) == AUTHED_MOD)
			str_append(Prefix, "M", 10);
		if (GameServer()->m_pEventsGame->GetActualEventTeam() == STEAL_TEE && (GameServer()->m_pController->m_Captain[TEAM_RED] == m_ClientID || GameServer()->m_pController->m_Captain[TEAM_BLUE] == m_ClientID) )
			str_append(Prefix, "C", 10);

		char Name[MAX_NAME_LENGTH + 10] = "";
		if (m_pStats->GetId() < 0)
			str_format(Name, MAX_NAME_LENGTH + 10, "[%sX]%s", Prefix, Server()->ClientName(m_ClientID));
		else
			str_format(Name, MAX_NAME_LENGTH + 10, "[%s%ld]%s", Prefix, m_pStats->GetLevel(), Server()->ClientName(m_ClientID));
		StrToInts(&pClientInfo->m_Name0, 4, Name);
	}
	StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, m_ClientID, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = m_ClientID;
	pPlayerInfo->m_Score = m_Score;
	if ( GameServer()->m_pEventsGame->GetActualEventTeam() == ANONYMOUS && SnappingClient != m_ClientID )
		pPlayerInfo->m_Team = TEAM_SPECTATORS;
	else
	pPlayerInfo->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;

	if(m_ClientID == SnappingClient && m_Team == TEAM_SPECTATORS)
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game =S (%s)", Server()->ClientName(m_ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game =(", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING)
			return;

		// reset input
		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
 		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		m_pCharacter->Die(m_ClientID, Weapon);
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::Respawn()
{
	if(m_Team != TEAM_SPECTATORS)
		m_Spawning = true;
}

void CPlayer::SetTeam(int Team, bool DoChatMsg)
{
	if(m_pStats->GetId() < 0)
		return;

	// clamp the team
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team)
		return;

	char aBuf[512];
	if(DoChatMsg)
	{
		if ( GameServer()->m_pEventsGame->GetActualEventTeam() == ANONYMOUS )
		{
			str_format(aBuf, sizeof(aBuf), "'%s' joined the game", Server()->ClientName(m_ClientID));
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		}
		else if ( GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE )
		{
			str_format(aBuf, sizeof(aBuf), "'%s' joined the zombies", Server()->ClientName(m_ClientID));
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(m_ClientID), GameServer()->m_pController->GetTeamName(Team));
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		}
	}

	KillCharacter();

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	m_SpectatorID = SPEC_FREEVIEW;
	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

void CPlayer::SetCaptureTeam(int Team, int Killer)
{
	// clamp the team
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team || (GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE && Team != TEAM_RED) || (m_Team != TEAM_SPECTATORS && m_ClientID == GameServer()->m_pController->m_Captain[m_Team] && GameServer()->m_pController->GetTeamScore(m_Team) > 1) )
		return;

	char aBuf[512];
	if ( GameServer()->m_pController->IsTeamplay() && GameServer()->m_pEventsGame->GetActualEventTeam() >= STEAL_TEE )
	{
		char NameTeam[512];
		if (GameServer()->m_pEventsGame->GetActualEventTeam() == STEAL_TEE)
			str_copy(NameTeam, GameServer()->m_pController->GetTeamName(Team), 512);
		else if (GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE)
			str_copy(NameTeam, "zombies", 512);
		str_format(aBuf, sizeof(aBuf), "'%s' was captured by the %s !", Server()->ClientName(m_ClientID), NameTeam);
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->m_SpectatorID = Killer;
		}

		m_SpectatorID = Killer;
	}

	if (GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE)
		GameServer()->SendChatTarget(GetCID(), "You're a zombie ! Eat some brains !");
}

int CPlayer::GetSID() const
{
	return m_pStats->GetId();
};

bool CPlayer::SetSID(long id)
{
	if(m_pStats->GetId() == id)
		return true;

	if (m_pStats->GetId() > 0)
	{
		m_pStats->SetStopPlay();
		m_pStats->WriteAll();
	}

	delete m_pStats;
	m_pStats = new CStats(this, m_pGameServer);

	if ( id > 0 )
	{
		m_pStats->Set(GameServer()->m_pStatsServer->GetAll(id));
		if (m_pStats->GetId() < 0)
			return false;
	}
	else if ( id == 0 )
		m_pStats->ConnectAnonymous();
	else
		return false;

	KillCharacter();
	m_pStats->SetStartPlay();
	m_pStats->UpdateInfo();
	m_pStats->UpdateStat();
	m_pStats->UpdateUpgrade();
	GameServer()->SendTuningParams(m_ClientID);
	return true;
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	if(!GameServer()->m_pController->CanSpawn(m_Team, &SpawnPos))
		return;

	m_Score = m_pStats->GetScore();

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(SpawnPos);
}
