/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#include <engine/shared/config.h>
#include <game/mapitems.h>

#include <game/generated/protocol.h>

#include <game/gamecore.h>

#include "gamecontroller.h"
#include "gamecontext.h"
#include "entities/pickup.h"
#include "entities/loot.h"
#include "entities/explodewall.h"
#include "statistics/statistiques.h"
#include "event.h"

IGameController::IGameController(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
	m_pGameType = "unknown";

	//
	DoWarmup(g_Config.m_SvWarmup);
	m_GameOverTick = -1;
	m_SuddenDeath = 0;
	m_RoundStartTick = Server()->Tick();
	m_RoundCount = 0;
	m_GameFlags = 0;
	m_aTeamscore[TEAM_RED] = 0;
	m_aTeamscore[TEAM_BLUE] = 0;
	m_aMapWish[0] = 0;

	m_UnbalancedTick = -1;
	m_ForceBalanced = false;
	m_ForceDoBalance = false;

	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;

	m_GameWeapon[WEAPON_HAMMER] = true;
	m_GameWeapon[WEAPON_GUN] = true;
	m_GameWeapon[WEAPON_SHOTGUN] = false;
	m_GameWeapon[WEAPON_GRENADE] = false;
	m_GameWeapon[WEAPON_RIFLE] = false;
	m_GameWeapon[WEAPON_NINJA] = false;

	m_Captain[0] = -1;
	m_Captain[1] = -1;
}

IGameController::~IGameController()
{
}

float IGameController::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos)
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for(; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if(pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		float d = distance(Pos, pC->m_Pos);
		Score += Scoremod * (d == 0 ? 1000000000.0f : 1.0f/d);
	}

	return Score;
}

void IGameController::EvaluateSpawnType(CSpawnEval *pEval, int Type)
{
	// get spawn point
	for(int i = 0; i < m_aNumSpawnPoints[Type]; i++)
	{
		// check if the position is occupado
		IEntityDamageable *aEnts[MAX_ENTITIES_DAMAGEABLE];
		int Num = GameServer()->m_World.FindEntitiesDamageable(m_aaSpawnPoints[Type][i], 64, aEnts, MAX_ENTITIES_DAMAGEABLE);
		vec2 Positions[5] = { vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f) };	// start, left, up, right, down
		int Result = -1;
		for(int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for(int c = 0; c < Num; ++c)
			{
				vec2 SpawnPos = m_aaSpawnPoints[Type][i]+Positions[Index];
				vec2 EntPos = aEnts[c]->m_Pos;
				if (aEnts[c]->GetType() == CGameWorld::ENTTYPE_EXPLODEWALL)
					EntPos = closest_point_on_line(reinterpret_cast<CExplodeWall*>(aEnts[c])->m_From, aEnts[c]->m_Pos, SpawnPos);

				if(GameServer()->Collision()->CheckPoint(m_aaSpawnPoints[Type][i]+Positions[Index]) ||
					distance(EntPos, SpawnPos) <= aEnts[c]->m_ProximityRadius)
				{
					Result = -1;
					break;
				}
			}
		}
		if(Result == -1)
			continue;	// try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i]+Positions[Result];
		float S = EvaluateSpawnPos(pEval, P);
		if(!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

bool IGameController::CanSpawn(int Team, vec2 *pOutPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;

	if(IsTeamplay())
	{
		Eval.m_FriendlyTeam = Team;

		// first try own team spawn, then normal spawn and then enemy
		EvaluateSpawnType(&Eval, 1+(Team&1));
		if(!Eval.m_Got)
		{
			EvaluateSpawnType(&Eval, 0);
			if(!Eval.m_Got)
				EvaluateSpawnType(&Eval, 1+((Team+1)&1));
		}
	}
	else
	{
		EvaluateSpawnType(&Eval, 0);
		EvaluateSpawnType(&Eval, 1);
		EvaluateSpawnType(&Eval, 2);
	}

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}


bool IGameController::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;

	if(Index == ENTITY_SPAWN)
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
	else if(Index == ENTITY_SPAWN_RED)
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
	else if(Index == ENTITY_SPAWN_BLUE)
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
	else if(Index == ENTITY_ARMOR_1)
		Type = POWERUP_ARMOR;
	else if(Index == ENTITY_HEALTH_1)
		Type = POWERUP_HEALTH;
	else if(Index == ENTITY_WEAPON_SHOTGUN)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
		m_GameWeapon[WEAPON_SHOTGUN] = true;
	}
	else if(Index == ENTITY_WEAPON_GRENADE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
		m_GameWeapon[WEAPON_GRENADE] = true;
	}
	else if(Index == ENTITY_WEAPON_RIFLE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_RIFLE;
		m_GameWeapon[WEAPON_RIFLE] = true;
	}
	else if(Index == ENTITY_POWERUP_NINJA && g_Config.m_SvPowerups)
	{
		Type = POWERUP_NINJA;
		SubType = WEAPON_NINJA;
		m_GameWeapon[WEAPON_NINJA] = true;
	}

	if(Type != -1)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, SubType);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

void IGameController::EndRound()
{
	if(m_Warmup) // game can't end when we are running warmup
		return;

	GameServer()->m_World.m_Paused = true;
	m_GameOverTick = Server()->Tick();
	m_SuddenDeath = 0;
}

void IGameController::ResetGame()
{
	GameServer()->m_World.m_ResetRequested = true;
}

const char *IGameController::GetTeamName(int Team)
{
	if(IsTeamplay())
	{
		if(Team == TEAM_RED)
			return "red team";
		else if(Team == TEAM_BLUE)
			return "blue team";
	}
	else
	{
		if(Team == 0)
			return "game";
	}

	return "spectators";
}

static bool IsSeparator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void IGameController::StartRound()
{
	ResetGame();

	if (IsNormalEnd())
	m_RoundStartTick = Server()->Tick();
	if ( GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE )
		GameServer()->m_pEventsGame->m_StartEventRound = Server()->Tick();
	m_SuddenDeath = 0;
	m_GameOverTick = -1;
	GameServer()->m_World.m_Paused = false;
	m_aTeamscore[TEAM_RED] = 0;
	m_aTeamscore[TEAM_BLUE] = 0;
	m_ForceBalanced = false;
	m_ForceDoBalance = true;
	m_Captain[0] = -1;
	m_Captain[1] = -1;
	Server()->DemoRecorder_HandleAutoStart();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "start round type='%s' teamplay='%d'", m_pGameType, m_GameFlags&GAMEFLAG_TEAMS);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	GameServer()->SendChatTarget(-1, "--------------------");
}

void IGameController::ChangeMap(const char *pToMap)
{
	str_copy(m_aMapWish, pToMap, sizeof(m_aMapWish));
	EndRound();
}

void IGameController::CycleMap()
{
	if(m_aMapWish[0] != 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "rotating map to %s", m_aMapWish);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		str_copy(g_Config.m_SvMap, m_aMapWish, sizeof(g_Config.m_SvMap));
		m_aMapWish[0] = 0;
		m_RoundCount = 0;
		return;
	}
	if(!str_length(g_Config.m_SvMaprotation))
		return;

	if(m_RoundCount < g_Config.m_SvRoundsPerMap-1)
	{
		if(g_Config.m_SvRoundSwap)
			GameServer()->SwapTeams();
		return;
	}

	if (!IsNormalEnd())
		return;

	// handle maprotation
	const char *pMapRotation = g_Config.m_SvMaprotation;
	const char *pCurrentMap = g_Config.m_SvMap;

	int CurrentMapLen = str_length(pCurrentMap);
	const char *pNextMap = pMapRotation;
	while(*pNextMap)
	{
		int WordLen = 0;
		while(pNextMap[WordLen] && !IsSeparator(pNextMap[WordLen]))
			WordLen++;

		if(WordLen == CurrentMapLen && str_comp_num(pNextMap, pCurrentMap, CurrentMapLen) == 0)
		{
			// map found
			pNextMap += CurrentMapLen;
			while(*pNextMap && IsSeparator(*pNextMap))
				pNextMap++;

			break;
		}

		pNextMap++;
	}

	// restart rotation
	if(pNextMap[0] == 0)
		pNextMap = pMapRotation;

	// cut out the next map
	char aBuf[512] = {0};
	for(int i = 0; i < 511; i++)
	{
		aBuf[i] = pNextMap[i];
		if(IsSeparator(pNextMap[i]) || pNextMap[i] == 0)
		{
			aBuf[i] = 0;
			break;
		}
	}

	// skip spaces
	int i = 0;
	while(IsSeparator(aBuf[i]))
		i++;

	m_RoundCount = 0;

	char aBufMsg[256];
	str_format(aBufMsg, sizeof(aBufMsg), "rotating map to %s", &aBuf[i]);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	str_copy(g_Config.m_SvMap, &aBuf[i], sizeof(g_Config.m_SvMap));
}

void IGameController::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_Score = 0;
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
		}
	}
}

void IGameController::OnPlayerInfoChange(class CPlayer *pP)
{
	const int aTeamColors[2] = {65387, 10223467};
	if(IsTeamplay())
	{
		pP->m_TeeInfos.m_UseCustomColor = 1;
		if(pP->GetTeam() >= TEAM_RED && pP->GetTeam() <= TEAM_BLUE)
		{
			pP->m_TeeInfos.m_ColorBody = aTeamColors[pP->GetTeam()];
			pP->m_TeeInfos.m_ColorFeet = aTeamColors[pP->GetTeam()];
		}
		else
		{
			pP->m_TeeInfos.m_ColorBody = 12895054;
			pP->m_TeeInfos.m_ColorFeet = 12895054;
		}
	}
}


void IGameController::OnCharacterKill(class CPlayer *pKiller, class CPlayer *pVictim)
{
	if (pVictim)
		pKiller->m_pStats->AddKill(pVictim->m_pStats->GetLevel());
	else
		pKiller->m_pStats->AddKill(0, true);

	pKiller->m_Score = pKiller->m_pStats->GetScore();

	if( pKiller->m_pStats->GetLevel() > pKiller->m_level )
	{
		pKiller->m_level = pKiller->m_pStats->GetLevel();
		char Text[256] = "";
		str_format(Text, 256, "XP : %d/%d", pKiller->m_level, pKiller->m_level);
		m_pGameServer->SendChatTarget(pKiller->GetCID(), Text, CGameContext::CHAT_INFO_XP);
		str_format(Text, 256, "%s has a levelup ! He is level %ld now ! Good Game ;) !", Server()->ClientName(pKiller->GetCID()), pKiller->m_level);
		m_pGameServer->SendChatTarget(-1, Text, CGameContext::CHAT_INFO_LEVELUP);
		m_pGameServer->SendChatTarget(pKiller->GetCID(), "You've get a levelup ! Don't forget put /upgr on the chat to upgr your statistics ! Good Game ;)");
	}
	else
	{
		char Text[256] = "";
		str_format(Text, 256, "XP : %d/%d", pKiller->m_pStats->GetXp(), pKiller->m_level + 1);
		m_pGameServer->SendChatTarget(pKiller->GetCID(), Text, CGameContext::CHAT_INFO_XP);
	}
}

int IGameController::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon, bool FromMonster)
{
	if (!FromMonster)
	{
		if(!pKiller || Weapon == WEAPON_GAME)
			return 0;
		if(pKiller == pVictim->GetPlayer())
			pKiller->m_pStats->AddSuicide();
		else
		{
			if(!(IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam()))
				OnCharacterKill(pKiller, pVictim->GetPlayer());

			if ( pKiller->m_pStats->GetActualKill() > 0 && (pKiller->m_pStats->GetActualKill() % 5) == 0 )
			{
				char spree_note[6][32] = { "is on a killing spree", "is on a rampage", "is dominating", "is unstoppable", "is Godlike", "is Wicked SICK" };
				if( pKiller->m_pStats->GetActualKill() <= 30 )
				{
					char buf[512];
					str_format(buf, sizeof(buf), "%s %s with %ld kills !", Server()->ClientName(pKiller->GetCID()), spree_note[pKiller->m_pStats->GetActualKill()/5-1], pKiller->m_pStats->GetActualKill());
					GameServer()->SendChatTarget(-1, buf, CGameContext::CHAT_INFO_KILLING_SPREE);
				}
				else
				{
					char Text[256] = "";
					str_format(Text, 256, "WARNING : %s must be stopped !!! %ld kills !!! ;)", Server()->ClientName(pKiller->GetCID()), pKiller->m_pStats->GetActualKill());
					GameServer()->SendChatTarget(-1, Text, CGameContext::CHAT_INFO_KILLING_SPREE);
				}
				if ((pKiller->m_pStats->GetActualKill() / 5) < 5)
				{
					char bonus_note[4][100] = {"handle x1.5 and fly unlimited", "all weapons", "invisibility", "protect for 30 sec but loses invisibility"};
					char Text[256] = "";
					str_format(Text, 256, "%s gets %s !", Server()->ClientName(pKiller->GetCID()), bonus_note[(pKiller->m_pStats->GetActualKill() / 5) - 1]);
					GameServer()->SendChatTarget(-1, Text, CGameContext::CHAT_INFO_KILLING_SPREE);

					str_format(Text, 256, "You've get %s by killing spree !", bonus_note[(pKiller->m_pStats->GetActualKill() / 5) - 1]);
					GameServer()->SendChatTarget(pKiller->GetCID(), Text);

					if ((pKiller->m_pStats->GetActualKill() / 5) == 1)
						GameServer()->SendTuningParams(pKiller->GetCID());
					else if ((pKiller->m_pStats->GetActualKill() / 5) == 2 && pKiller->GetCharacter())
					{
						pKiller->GetCharacter()->GiveWeapon(WEAPON_GUN, -2);
						pKiller->GetCharacter()->GiveWeapon(WEAPON_SHOTGUN, -2);
						pKiller->GetCharacter()->GiveWeapon(WEAPON_GRENADE, -2);
						pKiller->GetCharacter()->GiveWeapon(WEAPON_RIFLE, -2);
						pKiller->GetCharacter()->GiveNinja();
					}
					else if ((pKiller->m_pStats->GetActualKill() / 5) == 4 && pKiller->GetCharacter())
					{
						pKiller->GetCharacter()->m_Protect = Server()->Tick() + Server()->TickSpeed() * 30;
					}
				}
			}

			if ( pVictim->GetPlayer()->m_PlayerFlags & PLAYERFLAG_CHATTING )
			{
				char Text[256] = "";
				str_format(Text, 256, "%s made a chatkill to %s!", Server()->ClientName(pKiller->GetCID()), Server()->ClientName(pVictim->GetPlayer()->GetCID()));
				GameServer()->SendChatTarget(-1, Text);
			}

			if ( pKiller->GetCharacter() )
			{
				char Text[256] = "";
				str_format(Text, 256, "%s had %d%% health and %d%% armor.", Server()->ClientName(pKiller->GetCID()), pKiller->GetCharacter()->GetPercentHealth(), pKiller->GetCharacter()->GetPercentArmor());
				GameServer()->SendBroadcast(Text, pVictim->GetPlayer()->GetCID(), 2);
			}
			else
			{
				char Text[256] = "";
				str_format(Text, 256, "%s had 0%% health and 0%% armor.", Server()->ClientName(pKiller->GetCID()));
				GameServer()->SendBroadcast(Text, pVictim->GetPlayer()->GetCID(), 2);
			}
		}
	}

	if (pVictim->GetPlayer()->m_pStats->GetActualKill() >= 5)
	{
		char Text[256] = "";
		str_format(Text, 256, "%s %ld-kills killing spree was ended by %s.", Server()->ClientName(pVictim->GetPlayer()->GetCID()), pVictim->GetPlayer()->m_pStats->GetActualKill(), !FromMonster ? Server()->ClientName(pKiller->GetCID()) : "a monster");
		GameServer()->CreateExplosion(pVictim->m_Pos, pVictim->GetPlayer()->GetCID(), WEAPON_GAME, true, false);
		GameServer()->CreateSound(pVictim->m_Pos, SOUND_GRENADE_EXPLODE);
		GameServer()->SendChatTarget(-1, Text, CGameContext::CHAT_INFO_KILLING_SPREE);
	}

	pVictim->GetPlayer()->m_pStats->AddDead();
	GameServer()->SendTuningParams(pVictim->GetPlayer()->GetCID());

	if(Weapon == WEAPON_SELF)
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;

	if(GameServer()->m_pEventsGame->IsActualEvent(SURVIVOR) || GameServer()->m_pEventsGame->GetActualEventTeam() == T_SURVIVOR)
	{
		pVictim->GetPlayer()->SetCaptureTeam(TEAM_SPECTATORS, !FromMonster ? pKiller->GetCID() : SPEC_FREEVIEW);
		char Text[256] = "";
		str_format(Text, 256, "%s is killed by %s ! Try Again ;)", Server()->ClientName(pVictim->GetPlayer()->GetCID()), !FromMonster ? Server()->ClientName(pKiller->GetCID()) : "a monster");
		GameServer()->CreateExplosion(pVictim->m_Pos, pVictim->GetPlayer()->GetCID(), WEAPON_GAME, true, false);
		GameServer()->CreateSound(pVictim->m_Pos, SOUND_GRENADE_EXPLODE);
		GameServer()->SendChatTarget(-1, Text);
	}

	vec2 alea(0,0);

	CLoot *loot = new CLoot(&GameServer()->m_World, POWERUP_HEALTH, 0);
	do
	{
		alea.x = (rand() % 201) - 100;
		alea.y = (rand() % 101);
		loot->m_Pos.x = pVictim->m_Pos.x + alea.x;
		loot->m_Pos.y = pVictim->m_Pos.y - alea.y;
	}
	while (GameServer()->Collision()->TestBox(loot->m_Pos, vec2(PickupPhysSize, PickupPhysSize)));

	loot = new CLoot(&GameServer()->m_World, POWERUP_ARMOR, 0);
	do
	{
		alea.x = (rand() % 201) - 100;
		alea.y = (rand() % 101);
		loot->m_Pos.x = pVictim->m_Pos.x + alea.x;
		loot->m_Pos.y = pVictim->m_Pos.y - alea.y;
	}
	 while (GameServer()->Collision()->TestBox(loot->m_Pos, vec2(PickupPhysSize, PickupPhysSize)));

	if (pVictim->GetActiveWeapon() != WEAPON_GUN && pVictim->GetActiveWeapon() != WEAPON_HAMMER &&
		 !GameServer()->m_pEventsGame->IsActualEvent(KATANA))
	{
		loot = new CLoot(&GameServer()->m_World, pVictim->GetActiveWeapon() == WEAPON_NINJA ? POWERUP_NINJA : POWERUP_WEAPON, pVictim->GetActiveWeapon());
		do
		{
			alea.x = (rand() % 201) - 100;
			alea.y = (rand() % 101);
			loot->m_Pos.x = pVictim->m_Pos.x + alea.x;
			loot->m_Pos.y = pVictim->m_Pos.y - alea.y;
		}
		 while (GameServer()->Collision()->TestBox(loot->m_Pos, vec2(PickupPhysSize, PickupPhysSize)));
	}

	return 0;
}

void IGameController::OnCharacterSpawn(class CCharacter *pChr)
{
	// default health
	pChr->IncreaseHealth(10);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GiveWeapon(WEAPON_GUN, 10);
}

void IGameController::DoWarmup(int Seconds)
{
	if(Seconds < 0)
		m_Warmup = 0;
	else
		m_Warmup = Seconds*Server()->TickSpeed();
}

bool IGameController::IsFriendlyFire(int ClientID1, int ClientID2, int Weapon)
{
	if(ClientID1 == ClientID2)
		return false;

	if(IsTeamplay())
	{
		if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
			return false;

		if (GameServer()->m_pEventsGame->GetActualEventTeam() == CAN_KILL)
			return false;

		if((GameServer()->m_pEventsGame->GetActualEventTeam() == HAMMER_KILL && Weapon == WEAPON_HAMMER) ||
				(GameServer()->m_pEventsGame->GetActualEventTeam() == GUN_KILL && Weapon == WEAPON_GUN) ||
				(GameServer()->m_pEventsGame->GetActualEventTeam() == SHOTGUN_KILL && Weapon == WEAPON_SHOTGUN) ||
				(GameServer()->m_pEventsGame->GetActualEventTeam() == GRENADE_KILL && Weapon == WEAPON_GRENADE) ||
				(GameServer()->m_pEventsGame->GetActualEventTeam() == RIFLE_KILL && Weapon == WEAPON_RIFLE) ||
				(GameServer()->m_pEventsGame->GetActualEventTeam() == KATANA_KILL && Weapon == WEAPON_NINJA)
		  )
			return false;

		if(GameServer()->m_apPlayers[ClientID1]->GetTeam() == GameServer()->m_apPlayers[ClientID2]->GetTeam())
			return true;
	}

	return false;
}

bool IGameController::IsForceBalanced()
{
	if(m_ForceBalanced)
	{
		m_ForceBalanced = false;
		return true;
	}
	else
		return false;
}

bool IGameController::IsNormalEnd()
{
	if (IsTeamplay() && ((g_Config.m_SvScorelimit > 0 && (m_aTeamscore[TEAM_RED] >= g_Config.m_SvScorelimit || m_aTeamscore[TEAM_BLUE] >= g_Config.m_SvScorelimit)) || (g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60)))
		return true;
	else if (!IsTeamplay())
	{
		// gather some stats
		int Topscore = 0;
		int TopscoreCount = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				if(GameServer()->m_apPlayers[i]->m_Score > Topscore)
				{
					Topscore = GameServer()->m_apPlayers[i]->m_Score;
					TopscoreCount = 1;
				}
				else if(GameServer()->m_apPlayers[i]->m_Score == Topscore)
					TopscoreCount++;
			}
		}
		// check score win condition
		if((g_Config.m_SvScorelimit > 0 && Topscore >= g_Config.m_SvScorelimit) || (g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
			return true;
	}

	return false;
}

bool IGameController::CanBeMovedOnBalance(int ClientID)
{
	return true;
}

void IGameController::Tick()
{
	// do warmup
	if(m_Warmup)
	{
		m_Warmup--;
		if(!m_Warmup)
			StartRound();
	}

	if(m_GameOverTick != -1)
	{
		// game over.. wait for restart
		if ( (!IsNormalEnd() && Server()->Tick() > m_GameOverTick+Server()->TickSpeed()*2) || Server()->Tick() > m_GameOverTick+Server()->TickSpeed()*10)

		{
			CycleMap();
			StartRound();
			m_RoundCount++;
		}
	}

	// do team-balancing
	if(IsTeamplay() && ((GameServer()->m_pEventsGame->GetActualEventTeam() < STEAL_TEE && m_UnbalancedTick != -1 && Server()->Tick() > m_UnbalancedTick+g_Config.m_SvTeambalanceTime*Server()->TickSpeed()*60)|| (GameServer()->m_pEventsGame->GetActualEventTeam() != TEE_VS_ZOMBIE && m_ForceDoBalance == true)))
	{
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "Balancing teams");


		int aT[2] = {0,0};
		float aTScore[2] = {0,0};
		float aPScore[MAX_CLIENTS] = {0.0f};
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			{
				aT[GameServer()->m_apPlayers[i]->GetTeam()]++;
				aPScore[i] = GameServer()->m_apPlayers[i]->m_Score*Server()->TickSpeed()*60.0f/
					(Server()->Tick()-GameServer()->m_apPlayers[i]->m_ScoreStartTick);
				aTScore[GameServer()->m_apPlayers[i]->GetTeam()] += aPScore[i];
			}
		}

		// are teams unbalanced?
		if(absolute(aT[0]-aT[1]) >= 2)
		{
			int M = (aT[0] > aT[1]) ? 0 : 1;
			int NumBalance = absolute(aT[0]-aT[1]) / 2;

			do
			{
				CPlayer *pP = 0;
				float PD = aTScore[M];
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(!GameServer()->m_apPlayers[i] || !CanBeMovedOnBalance(i))
						continue;
					// remember the player who would cause lowest score-difference
					if(GameServer()->m_apPlayers[i]->GetTeam() == M && (!pP || absolute((aTScore[M^1]+aPScore[i]) - (aTScore[M]-aPScore[i])) < PD))
					{
						pP = GameServer()->m_apPlayers[i];
						PD = absolute((aTScore[M^1]+aPScore[i]) - (aTScore[M]-aPScore[i]));
					}
				}

				// move the player to the other team
				int Temp = pP->m_LastActionTick;
				if ( m_ForceDoBalance )
					pP->SetTeam(M^1, false);
				else
					pP->SetTeam(M^1, true);

				pP->m_LastActionTick = Temp;

				pP->Respawn();

				if ( !m_ForceDoBalance )
				pP->m_ForceBalanced = true;
			} while (--NumBalance);

			if ( !m_ForceDoBalance )
			m_ForceBalanced = true;
		}
		m_UnbalancedTick = -1;

		if ( m_ForceDoBalance )
			m_ForceDoBalance = false;
	}
	else if (IsTeamplay() && GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE && m_ForceDoBalance == true )
	{
		m_ForceDoBalance = false;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i] || !CanBeMovedOnBalance(i))
				continue;

			GameServer()->m_apPlayers[i]->SetTeam(TEAM_BLUE, false);
		}
	}
	else if (!IsTeamplay() && m_ForceDoBalance == true)
	{
		for ( int i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( GameServer()->m_apPlayers[i] )
				GameServer()->m_apPlayers[i]->SetTeam(0, false);
		}

		m_ForceDoBalance = false;
	}

	// check for inactive players
	if(g_Config.m_SvInactiveKickTime > 0)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
		#ifdef CONF_DEBUG
			if(g_Config.m_DbgDummies)
			{
				if(i >= MAX_CLIENTS-g_Config.m_DbgDummies)
					break;
			}
		#endif
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && !Server()->IsAuthed(i))
			{
				if(Server()->Tick() > GameServer()->m_apPlayers[i]->m_LastActionTick+g_Config.m_SvInactiveKickTime*Server()->TickSpeed()*60)
				{
					switch(g_Config.m_SvInactiveKick)
					{
					case 0:
						{
							// move player to spectator
							GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
						}
						break;
					case 1:
						{
							// move player to spectator if the reserved slots aren't filled yet, kick him otherwise
							int Spectators = 0;
							for(int j = 0; j < MAX_CLIENTS; ++j)
								if(GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() == TEAM_SPECTATORS)
									++Spectators;
							if(Spectators >= g_Config.m_SvSpectatorSlots)
								Server()->Kick(i, "Kicked for inactivity");
							else
								GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
						}
						break;
					case 2:
						{
							// kick the player
							Server()->Kick(i, "Kicked for inactivity");
						}
					}
				}
			}
		}
	}

	DoWincheck();
}


bool IGameController::IsTeamplay() const
{
	return m_GameFlags&GAMEFLAG_TEAMS;
}

void IGameController::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = (CNetObj_GameInfo *)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if(m_GameOverTick != -1)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
	if(m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;
	if(GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
	if (GameServer()->m_pEventsGame->GetActualEventTeam() != TEE_VS_ZOMBIE)
	{
	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
		pGameInfoObj->m_TimeLimit = g_Config.m_SvTimelimit;
	}
	else
	{
		pGameInfoObj->m_RoundStartTick = GameServer()->m_pEventsGame->m_StartEventRound;
		pGameInfoObj->m_TimeLimit = 35 / 60;
	}

	pGameInfoObj->m_WarmupTimer = m_Warmup;
	pGameInfoObj->m_ScoreLimit = g_Config.m_SvScorelimit;

	pGameInfoObj->m_RoundNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvRoundsPerMap) ? g_Config.m_SvRoundsPerMap : 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount+1;
}

int IGameController::GetAutoTeam(int NotThisID)
{
	// this will force the auto balancer to work overtime aswell
	if(g_Config.m_DbgStress)
		return 0;

	if ( GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE )
		return TEAM_RED;

	int aNumplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	int Team = 0;
	if(IsTeamplay())
		Team = aNumplayers[TEAM_RED] > aNumplayers[TEAM_BLUE] ? TEAM_BLUE : TEAM_RED;

	if(CanJoinTeam(Team, NotThisID))
		return Team;
	return -1;
}

bool IGameController::CanJoinTeam(int Team, int NotThisID)
{
	if(Team == TEAM_SPECTATORS || (GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS))
		return true;

	int aNumplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	return (aNumplayers[0] + aNumplayers[1]) < Server()->MaxClients()-g_Config.m_SvSpectatorSlots;
}

bool IGameController::CheckTeamBalance()
{
	if(!IsTeamplay() || !g_Config.m_SvTeambalanceTime)
		return true;

	int aT[2] = {0, 0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if(pP && pP->GetTeam() != TEAM_SPECTATORS)
			aT[pP->GetTeam()]++;
	}

	char aBuf[256];
	if(absolute(aT[0]-aT[1]) >= 2)
	{
		str_format(aBuf, sizeof(aBuf), "Teams are NOT balanced (red=%d blue=%d)", aT[0], aT[1]);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		if(GameServer()->m_pController->m_UnbalancedTick == -1)
			GameServer()->m_pController->m_UnbalancedTick = Server()->Tick();
		return false;
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "Teams are balanced (red=%d blue=%d)", aT[0], aT[1]);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		GameServer()->m_pController->m_UnbalancedTick = -1;
		return true;
	}
}

bool IGameController::CanChangeTeam(CPlayer *pPlayer, int JoinTeam)
{
	int aT[2] = {0, 0};

	if (!IsTeamplay() || JoinTeam == TEAM_SPECTATORS || !g_Config.m_SvTeambalanceTime)
		return true;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if(pP && pP->GetTeam() != TEAM_SPECTATORS)
			aT[pP->GetTeam()]++;
	}

	// simulate what would happen if changed team
	aT[JoinTeam]++;
	if (pPlayer->GetTeam() != TEAM_SPECTATORS)
		aT[JoinTeam^1]--;

	// there is a player-difference of at least 2
	if(absolute(aT[0]-aT[1]) >= 2)
	{
		// player wants to join team with less players
		if ((aT[0] < aT[1] && JoinTeam == TEAM_RED) || (aT[0] > aT[1] && JoinTeam == TEAM_BLUE))
			return true;
		else
			return false;
	}
	else
		return true;
}

void IGameController::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested)
	{
		if(IsTeamplay())
		{
			// check score win condition
			if ( m_pGameServer->m_pEventsGame->GetActualEventTeam() == T_SURVIVOR && m_ForceDoBalance == false )
			{
				m_aTeamscore[0] = GetNumPlayer(0);
				m_aTeamscore[1] = GetNumPlayer(1);
				int nb_player = 0;
				for ( int i = 0; i < MAX_CLIENTS; i++ )
				{
					if ( GameServer()->IsClientPlayer(i) )
						nb_player++;
				}

				if ( m_aTeamscore[0] && !m_aTeamscore[1] && nb_player > 1 )
				{
					GameServer()->SendChatTarget(-1, "The red team win !!! Good Game !!!");
					EndRound();
				}
				else if ( m_aTeamscore[1] && !m_aTeamscore[0] && nb_player > 1 )
				{
					GameServer()->SendChatTarget(-1, "The blue team win !!! Good Game !!!");
					EndRound();
				}
				else if ( !m_aTeamscore[0] && !m_aTeamscore[1] && nb_player > 0 )
				{
					GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
					EndRound();
				}
			}
			else if ( m_pGameServer->m_pEventsGame->GetActualEventTeam() >= STEAL_TEE && m_ForceDoBalance == false )
			{
				m_aTeamscore[0] = GetNumPlayer(0);
				m_aTeamscore[1] = GetNumPlayer(1);

				if (GameServer()->m_pEventsGame->GetActualEventTeam() == STEAL_TEE)
				{
					if ( m_Captain[TEAM_RED] < 0 && m_aTeamscore[TEAM_RED] > 0 )
					{
						while (!GameServer()->m_apPlayers[m_Captain[TEAM_RED] = (rand() % (0 - MAX_CLIENTS + 1)) + 0] || GameServer()->m_apPlayers[m_Captain[TEAM_RED]]->GetTeam() != TEAM_RED);
						char Text[256] = "";
						str_format(Text, 256, "%s is the captain of the red team !", Server()->ClientName(m_Captain[TEAM_RED]));
						GameServer()->SendChatTarget(-1, Text);
						GameServer()->SendChatTarget(m_Captain[TEAM_RED], "You're the captain of the red team ! You must keep your teammate and capture all players !");
					}
					if ( m_Captain[TEAM_BLUE] < 0 && m_aTeamscore[TEAM_BLUE] > 0 )
					{
						while (!GameServer()->m_apPlayers[m_Captain[TEAM_BLUE] = (rand() % (0 - MAX_CLIENTS + 1)) + 0] || GameServer()->m_apPlayers[m_Captain[TEAM_BLUE]]->GetTeam() != TEAM_BLUE);
						char Text[256] = "";
						str_format(Text, 256, "%s is the captain of the blue team !", Server()->ClientName(m_Captain[TEAM_BLUE]));
						GameServer()->SendChatTarget(-1, Text);
						GameServer()->SendChatTarget(m_Captain[TEAM_BLUE], "You're the captain of the blue team ! You must keep your teammate and capture all players !");
					}
				}
				else if (IsTeamplay() && GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE && m_aTeamscore[TEAM_BLUE] > 1 && m_Captain[TEAM_RED] < 0 && Server()->Tick() > GameServer()->m_pEventsGame->m_StartEventRound+Server()->TickSpeed()*5 )
				{
					if ( !m_aTeamscore[TEAM_RED] )
					{
						while (!GameServer()->m_apPlayers[m_Captain[TEAM_RED] = (rand() % (0 - MAX_CLIENTS + 1)) + 0] || GameServer()->m_apPlayers[m_Captain[TEAM_RED]]->GetTeam() == TEAM_SPECTATORS);
						GameServer()->m_apPlayers[m_Captain[TEAM_RED]]->SetTeam(TEAM_RED, false);
						char Text[256] = "";
						str_format(Text, 256, "%s is a zombie !!! Flee or be his slaves !!!", Server()->ClientName(m_Captain[TEAM_RED]));
						GameServer()->SendChatTarget(-1, Text);
						GameServer()->SendChatTarget(m_Captain[TEAM_RED], "You're a zombie ! Eat some brains !");
					}
					else
						while (!GameServer()->m_apPlayers[m_Captain[TEAM_RED] = (rand() % (0 - MAX_CLIENTS + 1)) + 0] || GameServer()->m_apPlayers[m_Captain[TEAM_RED]]->GetTeam() != TEAM_RED);
				}

				if ((m_pGameServer->m_pEventsGame->GetActualEventTeam() == STEAL_TEE && ((m_aTeamscore[0] == 0 && m_aTeamscore[1] > 1) || (m_aTeamscore[1] == 0 && m_aTeamscore[0] > 1))) ||
						(m_pGameServer->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE && ((m_aTeamscore[TEAM_BLUE] == 0 && m_aTeamscore[TEAM_RED] > 1) || Server()->Tick() > GameServer()->m_pEventsGame->m_StartEventRound+Server()->TickSpeed()*35)))
				{
					if ( m_pGameServer->m_pEventsGame->GetActualEventTeam() == STEAL_TEE )
					{
						int Team = m_aTeamscore[TEAM_RED] ? TEAM_RED : TEAM_BLUE;
						char Text[256] = "";
						str_format(Text, 256, "The %s of captain %s win !", Team ? "red team" : "blue team", Server()->ClientName(m_Captain[Team]));
						GameServer()->SendChatTarget(-1, Text);
					}
					else if ( m_pGameServer->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE )
					{
						int Team = m_aTeamscore[TEAM_BLUE] ? TEAM_BLUE : TEAM_RED;
						if ( Team == TEAM_BLUE )
							m_aTeamscore[TEAM_BLUE] = 100;
						char Text[256] = "";
						str_format(Text, 256, "The %s win !", Team ? "humans" : "zombies");
						GameServer()->SendChatTarget(-1, Text);
					}

					EndRound();
				}
			}
			else if((g_Config.m_SvScorelimit > 0 && (m_aTeamscore[TEAM_RED] >= g_Config.m_SvScorelimit || m_aTeamscore[TEAM_BLUE] >= g_Config.m_SvScorelimit)) ||
				(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
			{
				if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
					EndRound();
				else
					m_SuddenDeath = 1;
			}
		}
		else
		{
			if (GameServer()->m_pEventsGame->IsActualEvent(SURVIVOR))
			{
				int active_player = 0;
				int nb_player = 0;
				int id = 0;
				for ( int i = 0; i < MAX_CLIENTS; i++ )
				{
					if ( GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetSID() >= 0 )
						nb_player++;
					if ( GameServer()->IsClientPlayer(i) )
					{
						active_player++;
						id = i;
					}
				}

				if ( active_player == 1 && nb_player > 1 )
				{
					char aBuf[256];
					str_format(aBuf, 256, "%s is the winner !!! Good Game !!!",  Server()->ClientName(id));
					GameServer()->SendChatTarget(-1, aBuf);
					EndRound();
				}
				else if ( active_player == 0 && nb_player > 0 )
				{
					GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
					EndRound();
				}
			}

			// gather some stats
			int Topscore = 0;
			int TopscoreCount = 0;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i])
				{
					if(GameServer()->m_apPlayers[i]->m_Score > Topscore)
					{
						Topscore = GameServer()->m_apPlayers[i]->m_Score;
						TopscoreCount = 1;
					}
					else if(GameServer()->m_apPlayers[i]->m_Score == Topscore)
						TopscoreCount++;
				}
			}

			// check score win condition
			if(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60)
					EndRound();
		}
	}
}

int IGameController::ClampTeam(int Team)
{
	if(Team < 0)
		return TEAM_SPECTATORS;
	if(IsTeamplay())
		return Team&1;
	return 0;
}

int IGameController::GetNumPlayer(int Team)
{
	int aT[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			aT[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}
	return aT[Team];
}

