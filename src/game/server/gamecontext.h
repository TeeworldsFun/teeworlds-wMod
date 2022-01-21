/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <game/layers.h>
#include <game/voting.h>

#include "eventhandler.h"
#include "gameworld.h"
#include "gamecontroller.h"
#include "player.h"

/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/

class CGameContext : public IGameServer
{
	IServer *m_pServer;
	class IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;

	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConPause(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConLockTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static void ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	static void ConNextEvent(IConsole::IResult *pResult, void *pUserData);
	static void ConNextRandomEvent(IConsole::IResult *pResult, void *pUserData);
	static void ConSetEvent(IConsole::IResult *pResult, void *pUserData);
	static void ConAddTimeEvent(IConsole::IResult *pResult, void *pUserData);
	static void ConNextEventTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConNextRandomEventTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSetEventTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConAddTimeEventTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConListPlayer(IConsole::IResult *pResult, void *pUserData);
	static void ConGiveShotgun(IConsole::IResult *pResult, void *pUserData);
	static void ConGiveGrenade(IConsole::IResult *pResult, void *pUserData);
	static void ConGiveRifle(IConsole::IResult *pResult, void *pUserData);
	static void ConGiveKatana(IConsole::IResult *pResult, void *pUserData);
	static void ConGiveProtect(IConsole::IResult *pResult, void *pUserData);
	static void ConGiveInvisibility(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveShotgun(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveGrenade(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveRifle(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveKatana(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveProtect(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveInvisibility(IConsole::IResult *pResult, void *pUserData);
	static void ConSetSid(IConsole::IResult *pResult, void *pUserData);
    static void ConGiveXP(IConsole::IResult *pResult, void *pUserData);
	static void ConSpawnMonster(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	CGameContext(int Resetting);
	void Construct(int Resetting);

	bool m_Resetting;
public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }

	CGameContext();
	~CGameContext();

	void Clear();
	class IStatsServer *m_pStatsServer;
	class CEvent *m_pEventsGame;

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	IGameController *m_pController;
	CGameWorld m_World;

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);

	int m_LockTeams;

	// voting
	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientID);

	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_NumVoteOptions;
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, bool Smoke, bool FromMonster = false);
	void CreateHammerHit(vec2 Pos);
	void CreatePlayerSpawn(vec2 Pos);
	void CreateDeath(vec2 Pos, int Who);
	void CreateSound(vec2 Pos, int Sound, int Mask=-1);
	void CreateSoundGlobal(int Sound, int Target=-1);


	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	enum
	{
		CHAT_INFO,
		CHAT_INFO_HEAL_KILLER,
		CHAT_INFO_XP,
		CHAT_INFO_LEVELUP,
		CHAT_INFO_KILLING_SPREE,
		CHAT_INFO_RACE,
		CHAT_INFO_AMMO,
		CHAT_INFO_VOTER
	};
	// network
	void SendChatTarget(int To, const char *pText, int Type = CHAT_INFO);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(const char *pText, int ClientID, int LifeTime = 3);


	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);

	//
	void SwapTeams();

	void ParseArguments(const char *Message, int nb_result, char Result[][256]);
	void CommandOnChat(const char *Message, const int ClientID, const int Team);

	// engine events
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnShutdown();

	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientID);
	virtual void OnPostSnap();

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	virtual void OnClientConnected(int ClientID);
	virtual void OnClientEnter(int ClientID);
	virtual void OnClientDrop(int ClientID, const char *pReason);
	virtual void OnClientDirectInput(int ClientID, void *pInput);
	virtual void OnClientPredictedInput(int ClientID, void *pInput);

	virtual bool IsClientReady(int ClientID);
	virtual bool IsClientPlayer(int ClientID);
	virtual bool IsValidPlayer(int ClientID);

	// Monster
	class CMonster *m_apMonsters[MAX_MONSTERS];
	class CMonster *GetValidMonster(int MonsterID) const;
	void NewMonster();
	void OnMonsterDeath(int MonsterID);

	virtual const char *GameType();
	virtual const char *Version();
	virtual const char *NetVersion();
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientID) { return 1<<ClientID; }
inline int CmaskAllExceptOne(int ClientID) { return 0x7fffffff^CmaskOne(ClientID); }
inline bool CmaskIsSet(int Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }
#endif
