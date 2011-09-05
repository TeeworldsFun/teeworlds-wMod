#ifndef GAME_SERVER_EVENT_H
#define GAME_SERVER_EVENT_H

#include <game/server/gamecontext.h>

enum {NOTHING, HAMMER, GUN, SHOTGUN, GRENADE, RIFLE, KATANA, UNLIMITED_AMMO, LIFE_ARMOR_CRAZY, SURVIVOR, PROTECT_X2, INSTAGIB, WALLSHOT, BULLET_PIERCING, BULLET_BOUNCE, HAVE_ALL_WEAPON, GRAVITY_0, GRAVITY_M0_5, BOUNCE_10, HOOK_VERY_LONG, SPEED_X10,  WEAPON_SLOW, END};

enum {T_NOTHING, HAMMER_HEAL, RIFLE_HEAL, CAN_KILL, STEAL_TEE, TEE_VS_ZOMBIE, T_END};

class CEvent
{
public:
	CEvent(CGameContext *GameServer);
	~CEvent();
	void Tick();

	inline int GetActualEvent() { return m_ActualEvent; };
	inline int GetActualEventTeam() { return m_ActualEventTeam; };

	void NextEvent();
	void NextRandomEvent();
	bool SetEvent(int event);
	bool AddTime(long secondes = 150);

	void NextEventTeam();
	void NextRandomEventTeam();
	bool SetEventTeam(int event);
	bool AddTimeTeam(long secondes = 150);

	time_t m_StartEventRound;

private:
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
	IGameController *Controller() const { return m_pController; }

	void SetTune();
	void ResetTune();
	int m_ActualEvent;
	int m_ActualEventTeam;
	time_t m_StartEvent;
	time_t m_StartEventTeam;
	time_t m_LastSend;

	CGameContext *m_pGameServer;
	IServer *m_pServer;
	IGameController *m_pController;
};
#endif
