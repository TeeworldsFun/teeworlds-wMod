#ifndef GAME_SERVER_EVENT_H
#define GAME_SERVER_EVENT_H

#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

enum {NOTHING, HAMMER, GUN, SHOTGUN, GRENADE, RIFLE, KATANA, UNLIMITED_AMMO, LIFE_ARMOR_CRAZY, SURVIVOR, PROTECT_X2, INSTAGIB, WALLSHOT, BULLET_PIERCING, BULLET_BOUNCE, BULLET_GLUE, HAVE_ALL_WEAPON, GRAVITY_0_25, GRAVITY_0, GRAVITY_M0_25, GRAVITY_M0_5, BOUNCE_10, HOOK_VERY_LONG, SPEED_X10,  WEAPON_SLOW, RACE_WARRIOR, RACE_ENGINEER, RACE_ORC, RACE_MINER, RACE_RANDOM, JUMP_UNLIMITED, JUMP_X1_5, CURVATURE_0, ALL, END};

enum {T_NOTHING, HAMMER_HEAL, GUN_HEAL, SHOTGUN_HEAL, GRENADE_HEAL, RIFLE_HEAL, KATANA_HEAL, CAN_HEAL, HAMMER_KILL, GUN_KILL, SHOTGUN_KILL, GRENADE_KILL, RIFLE_KILL, KATANA_KILL, CAN_KILL, ANONYMOUS, T_SURVIVOR, STEAL_TEE, TEE_VS_ZOMBIE, T_END};

class CEvent
{
public:
    CEvent(CGameContext *GameServer);
    ~CEvent();
    void Tick();

    inline bool IsActualEvent(int Event)
    {
        if (Event == m_ActualEvent[0] || (g_Config.m_SvTwoEvent && Event == m_ActualEvent[1]) || (m_ActualEvent[0] == ALL && (Event > KATANA || Event < SHOTGUN) && Event != WALLSHOT && Event != SURVIVOR && Event != GRAVITY_0 && Event != GRAVITY_M0_25 && Event != GRAVITY_M0_5 && Event != BULLET_PIERCING && Event != BULLET_GLUE && (Event <= RACE_WARRIOR || Event > RACE_RANDOM)))
        {
            return true;
        }
        return false;
    };
    inline int GetActualEventTeam()
    {
        if (!Controller()->IsTeamplay())
        {
            return 0;
        }
        return m_ActualEventTeam;
    };

    void NextEvent();
    void NextRandomEvent();
    bool SetEvent(int event);
    bool AddTime(long secondes = 150);

    void NextEventTeam();
    void NextRandomEventTeam();
    bool SetEventTeam(int event);
    bool AddTimeTeam(long secondes = 150);

    int m_StartEventRound;

private:
    CGameContext *GameServer() const
    {
        return m_pGameServer;
    }
    IServer *Server() const
    {
        return m_pGameServer->Server();
    }
    IGameController *Controller() const
    {
        return m_pGameServer->m_pController;
    }

    bool CanBeUsed(int NewEvent, int Type = 0, bool Canuseactual = false);
    void SetTune();
    void ResetTune();
    int m_ActualEvent[2];
    int m_ActualEventTeam;
    int m_StartEvent[2];
    int m_StartEventTeam;
    int m_LastSend;

    CGameContext *m_pGameServer;
};
#endif
