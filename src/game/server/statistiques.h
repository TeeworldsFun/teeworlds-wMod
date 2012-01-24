#ifndef GAME_SERVER_STATISTIQUES_H
#define GAME_SERVER_STATISTIQUES_H

#include "player.h"

struct StatWeapon
{
    StatWeapon()
    {
        m_auto_gun = false;
        m_auto_hammer = false;
        m_auto_ninja = false;
        m_speed = 1.0f;
        m_regeneration = 0;
        m_stockage = 10;
        m_all_weapon = false;
        m_bounce = 0;
    }

    bool m_auto_gun;
    bool m_auto_hammer;
    bool m_auto_ninja;
    float m_speed;
    short int m_regeneration;
    short int m_stockage;
    bool m_all_weapon;
    short int m_bounce;
};

struct StatLife
{
    StatLife()
    {
        m_protection = 1.0f;
        m_start_armor = 0;
        m_regeneration = 0;
        m_stockage[0] = 10;
        m_stockage[1] = 10;
    }
    float m_protection;
    short int m_start_armor;
    short int m_regeneration;
    short int m_stockage[2];
};

struct StatMove
{
    StatMove()
    {
        m_rate_speed = 1.0f;
        m_rate_accel = 1.0f;
        m_rate_high_jump = 1.0f;
        m_num_jump = 1;
    }
    float m_rate_speed;
    float m_rate_accel;
    float m_rate_high_jump;
    short int m_num_jump;
};

struct StatHook
{
    StatHook()
    {
        m_rate_length = 1.0f;
        m_rate_time = 1.0f;
        m_rate_speed = 1.0f;
        m_hook_damage = false;
    }
    float m_rate_length;
    float m_rate_time;
    float m_rate_speed;
    bool m_hook_damage;
};

struct Conf
    {
        Conf()
        {
            m_InfoHealKiller = true;
            m_InfoXP = true;
            m_InfoLevelUp = true;
            m_InfoKillingSpree = true;
            m_InfoRace = true;
            m_InfoAmmo = true;
            m_ShowVoter = true;
            m_AmmoAbsolute = true;
            m_LifeAbsolute = true;
            m_Lock = false;
        }
        bool m_InfoHealKiller;
        bool m_InfoXP;
        bool m_InfoLevelUp;
        bool m_InfoKillingSpree;
        bool m_InfoRace;
        bool m_InfoAmmo;
        bool m_ShowVoter;
        bool m_AmmoAbsolute;
        bool m_LifeAbsolute;
        bool m_Lock;
        int m_Weapon[NUM_WEAPONS];
};

class CStats
{
public:
    enum {MAX_IP_LENGTH = 42};

    CStats(CPlayer* pPlayer);
    void SetInfo(const char Name[], const char Clan[], const int Country, const char Ip[MAX_IP_LENGTH] = "");

    inline unsigned int GetLevel() { UpdateStat(); return m_level; };
    inline unsigned int GetXp() { UpdateStat(); return m_xp; };
    inline unsigned int GetScore() { UpdateStat(); return m_score; };
    inline unsigned int GetActualKill() { return m_actual_kill; };
    inline StatWeapon GetStatWeapon() { return m_upgr.m_stat_weapon; };
    inline StatLife GetStatLife() { return m_upgr.m_stat_life; };
    inline StatMove GetStatMove() { return m_upgr.m_stat_move; };
    inline StatHook GetStatHook() { return m_upgr.m_stat_hook; };
    inline Conf GetConf() { return m_conf; };

    void UpdateStat();
    void UpdateUpgrade();
    void DisplayStat();
    void DisplayPlayer();
    void ResetPartialStat();
    void ResetAllStat();
    void ResetUpgr();
    
    inline void AddKill(unsigned int level_victim)
    {
        m_actual_kill++;
        if (m_conf.m_Lock)
            return;
        m_kill++;
        if (!m_level)
            m_level = 1;
        if (static_cast<int>(level_victim/m_level) - 1 > 0)
            m_bonus_xp += (level_victim/m_level) - 1;
    }
    inline void AddDead()
    {
        if ( m_conf.m_Lock )
        {
            m_actual_kill = 0;
            return;
        }
        m_dead++;
        AddKillingSpree();
        m_actual_kill = 0;
    }
    inline void AddSuicide() { m_suicide += m_conf.m_Lock ? 0 : 1; };
    inline void AddFire() { m_fire += m_conf.m_Lock ? 0 : 1; };
    inline void AddPickUpWeapon() { m_pickup_weapon += m_conf.m_Lock ? 0 : 1; };
    inline void AddPickUpNinja() { m_pickup_ninja += m_conf.m_Lock ? 0 : 1; };
    inline void AddChangeWeapon() { m_change_weapon += m_conf.m_Lock ? 0 : 1; };
    inline void SetStartPlay()
    {
        for (int i = 0; i < NUM_WEAPONS; i++)
            m_pPlayer->m_WeaponType[i] = m_conf.m_Weapon[i];
        if ( m_conf.m_Lock )
            return;
        m_start_time = time_timestamp();
        m_log_in++;
    }
    inline void SetStopPlay()
    {
        for (int i = 0; i < NUM_WEAPONS; i++)
            m_conf.m_Weapon[i] = m_pPlayer->m_WeaponType[i];
        m_player.m_last_connect = time_timestamp();
        if ( m_conf.m_Lock )
            return;
        AddKillingSpree();
        UpdateStat();
        m_start_time = 0;
    }
    inline void AddMessage() { m_message += m_conf.m_Lock ? 0 : 1; };
    inline void AddFlagCapture() { m_flag_capture += m_conf.m_Lock ? 0 : 1; };
    inline bool InfoHealKiller() { return m_conf.m_InfoHealKiller = m_conf.m_InfoHealKiller ? false : true; };
    inline bool InfoXP() { return m_conf.m_InfoXP = m_conf.m_InfoXP ? false : true; };
    inline bool InfoLevelUp() { return m_conf.m_InfoLevelUp = m_conf.m_InfoLevelUp ? false : true; };
    inline bool InfoKillingSpree() { return m_conf.m_InfoKillingSpree = m_conf.m_InfoKillingSpree ? false : true; };
    inline bool InfoRace() { return m_conf.m_InfoRace = m_conf.m_InfoRace ? false : true; };
    inline bool InfoAmmo() { return m_conf.m_InfoAmmo = m_conf.m_InfoAmmo ? false : true; };
    inline bool ShowVoter() { return m_conf.m_ShowVoter = m_conf.m_ShowVoter ? false : true; };
    inline bool AmmoAbsolute() { return m_conf.m_AmmoAbsolute = m_conf.m_AmmoAbsolute ? false : true; };
    inline bool LifeAbsolute() { return m_conf.m_LifeAbsolute = m_conf.m_LifeAbsolute ? false : true; };
    inline void EnableAllInfo()
    {
        m_conf.m_InfoHealKiller = true;
        m_conf.m_InfoXP = true;
        m_conf.m_InfoLevelUp = true;
        m_conf.m_InfoKillingSpree = true;
        m_conf.m_InfoRace = true;
        m_conf.m_InfoAmmo = true;
        m_conf.m_ShowVoter = true;
    }
    inline void DisableAllInfo()
    {
        m_conf.m_InfoHealKiller = false;
        m_conf.m_InfoXP = false;
        m_conf.m_InfoLevelUp = false;
        m_conf.m_InfoKillingSpree = false;
        m_conf.m_InfoRace = false;
        m_conf.m_InfoAmmo = false;
        m_conf.m_ShowVoter = false;
    }
    inline bool Lock() { return m_conf.m_Lock = m_conf.m_Lock ? false : true; };

    inline int UpgradeWeapon()
    {
        UpdateUpgrade();
        if (m_upgr.m_money)
            return 1;
        if (m_conf.m_Lock)
            return 2;
        if (m_upgr.m_weapon >= 40)
            return 3;
        m_upgr.m_weapon++;
        UpdateUpgrade();
        return 0;
    };
    inline int UpgradeLife(long id)
    {
        UpdateUpgrade();
        if (m_upgr.m_money)
            return 1;
        if (m_conf.m_Lock)
            return 2;
        if (m_upgr.m_life >= 40)
            return 3;
        m_upgr.m_life++;
        UpdateUpgrade();
        return 0;
    };
    inline int UpgradeMove(long id)
    {
        UpdateUpgrade();
        if (m_upgr.m_money)
            return 1;
        if (m_conf.m_Lock)
            return 2;
        if (m_upgr.m_move >= 40)
            return 3;
        m_upgr.m_move++;
        UpdateUpgrade();
        return 0;
    };
    inline int UpgradeHook(long id)
    {
        UpdateUpgrade();
        if (m_upgr.m_money)
            return 1;
        if (m_conf.m_Lock)
            return 2;
        if (m_upgr.m_hook >= 40)
            return 3;
        m_upgr.m_hook++;
        UpdateUpgrade();
        return 0;
    };
private:
    CPlayer* m_pPlayer;

    struct Player
    {
        unsigned int m_id;
        char m_ip[MAX_IP_LENGTH];
        char m_name[MAX_NAME_LENGTH];
        char m_clan[MAX_CLAN_LENGTH];
        int m_country;
        unsigned int m_last_connect;
    } m_player;

    unsigned int m_level;
    unsigned int m_xp;
    unsigned int m_score;
    unsigned int m_kill;
    unsigned int m_dead;
    unsigned int m_suicide;
    double m_rapport;
    unsigned int m_log_in;
    unsigned int m_fire;
    unsigned int m_pickup_weapon;
    unsigned int m_pickup_ninja;
    unsigned int m_change_weapon;
    unsigned int m_time_play;
    unsigned int m_message;
    unsigned int m_killing_spree;
    unsigned int m_max_killing_spree;
    unsigned int m_flag_capture;
    unsigned int m_bonus_xp;
    unsigned int m_start_time;
    unsigned short int m_actual_kill;

    struct Upgrade
    {
        int m_money;
        short int m_weapon;
        short int m_life;
        short int m_move;
        short int m_hook;
        StatWeapon m_stat_weapon;
        StatLife m_stat_life;
        StatMove m_stat_move;
        StatHook m_stat_hook;
    } m_upgr;

    Conf m_conf;
    
    CGameContext *GameServer() const { return m_pGameServer; };
    IServer *Server() const { return m_pGameServer->Server(); };
    IGameController *Controller() const { return m_pGameServer->m_pController; };

    inline void AddKillingSpree()
    {
        if ( m_actual_kill > 5 )
        {
            m_killing_spree += m_actual_kill;
            if ( m_actual_kill > m_max_killing_spree )
                m_max_killing_spree = m_actual_kill;
        }
    }
    
    CGameContext *m_pGameServer;
};

#endif
