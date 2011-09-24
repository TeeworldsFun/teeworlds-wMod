#ifndef GAME_SERVER_EVENT_H
#define GAME_SERVER_EVENT_H

#include <game/server/gamecontext.h>

enum {NOTHING, HAMMER, GUN, SHOTGUN, GRENADE, RIFLE, KATANA, UNLIMITED_AMMO, LIFE_ARMOR_CRAZY, SURVIVOR, PROTECT_X2, INSTAGIB, WALLSHOT, BULLET_PIERCING, BULLET_BOUNCE, HAVE_ALL_WEAPON, GRAVITY_0, GRAVITY_M0_5, BOUNCE_10, HOOK_VERY_LONG, SPEED_X10,  WEAPON_SLOW, ALL, END};

enum {T_NOTHING, HAMMER_HEAL, RIFLE_HEAL, CAN_KILL, T_SURVIVOR, STEAL_TEE, TEE_VS_ZOMBIE, T_END};

class CEvent
{
public:
	CEvent(CGameContext *GameServer);
	~CEvent();
	void Tick();

	//ONLY FOR EVENT BEFORE KATANA !!!
	inline int GetActualEvent() { return m_ActualEvent[0]; };

	inline bool IsTwoEvent() { return m_TwoEvent; };
	inline bool IsActualEvent(int Event) { if ( Event == m_ActualEvent[0] || (IsTwoEvent() && Event == m_ActualEvent[1]) || (m_ActualEvent[0] == ALL && Event > KATANA && Event != WALLSHOT && Event != SURVIVOR && Event != GRAVITY_M0_5 && Event != BULLET_PIERCING)) { return true; } return false; };
	inline int GetActualEventTeam() { if (!Controller()->IsTeamplay()) { return 0; } return m_ActualEventTeam; };

	void NextEvent();
	void NextRandomEvent();
	bool SetEvent(int event);
	bool AddTime(long secondes = 150);

	inline void SetTwoEvent() { if (m_TwoEvent) { m_TwoEvent = false; return; } else if ( !Controller()->IsTeamplay() ) { m_TwoEvent = true; return; } };

	void NextEventTeam();
	void NextRandomEventTeam();
	bool SetEventTeam(int event);
	bool AddTimeTeam(long secondes = 150);

	int m_StartEventRound;

private:
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pGameServer->Server(); }
	IGameController *Controller() const { return m_pGameServer->m_pController; }

	void SetTune();
	void ResetTune();
	bool m_TwoEvent;
	int m_ActualEvent[2];
	int m_ActualEventTeam;
	int m_StartEvent[2];
	int m_StartEventTeam;
	int m_LastSend;

	CGameContext *m_pGameServer;
};
#endif