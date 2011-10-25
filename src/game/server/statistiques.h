#ifndef GAME_SERVER_STATISTIQUES_H
#define GAME_SERVER_STATISTIQUES_H

#include <game/server/gamecontext.h>
#include <fstream>
#include <vector>

enum { MAX_IP_LENGTH = 45 };

struct StatWeapon
{
    bool m_auto_gun;
    bool m_auto_hammer;
    bool m_auto_ninja;
    float m_speed;
    int m_regeneration;
    int m_stockage;
    bool m_all_weapon;
    int m_bounce;
};

struct StatLife
{
    float m_protection;
    int m_start_armor;
    int m_regeneration;
    int m_stockage[2];
};

struct Upgrade
{
    unsigned long m_money;
    unsigned long m_weapon;
    unsigned long m_life;
    unsigned long m_move;
    unsigned long m_hook;
    StatWeapon m_stat_weapon;
    StatLife m_stat_life;
};

struct Rank
{
    unsigned long m_level;
    unsigned long m_score;
    unsigned long m_kill;
    unsigned long m_rapport;
    unsigned long m_log_in;
    unsigned long m_fire;
    unsigned long m_pickup_weapon;
    unsigned long m_pickup_ninja;
    unsigned long m_change_weapon;
    unsigned long m_time_play;
    unsigned long m_message;
    unsigned long m_killing_spree;
    unsigned long m_max_killing_spree;
    unsigned long m_flag_capture;
};

struct Stats
{
    Stats()
    {
        m_id = 0;
        m_name = 0;
        m_clan = 0;
        m_country = -1;
        m_level = 0;
        m_xp = 0;
        m_score = 0;
        m_kill = 0;
        m_dead = 0;
        m_rapport = 0.0;
        m_suicide = 0;
        m_log_in = 0;
        m_fire = 0;
        m_pickup_weapon = 0;
        m_pickup_ninja = 0;
        m_change_weapon = 0;
        m_time_play = 0;
        m_message = 0;
        m_killing_spree = 0;
        m_max_killing_spree = 0;
        m_flag_capture = 0;
        m_start_time = 0;
        m_actual_kill = 0;
        m_last_connect = 0;
        m_lock = false;
        for ( int i = 0; i < NUM_WEAPONS; i++ )
            m_Weapon[i] = WARRIOR;

        m_upgrade.m_money = 0;
        m_upgrade.m_weapon = 0;
        m_upgrade.m_life = 0;
        m_upgrade.m_move = 0;
        m_upgrade.m_hook = 0;

        m_rank.m_level = 0;
        m_rank.m_score = 0;
        m_rank.m_kill = 0;
        m_rank.m_rapport = 0;
        m_rank.m_log_in = 0;
        m_rank.m_fire = 0;
        m_rank.m_pickup_weapon = 0;
        m_rank.m_pickup_ninja = 0;
        m_rank.m_change_weapon = 0;
        m_rank.m_time_play = 0;
        m_rank.m_message = 0;
        m_rank.m_killing_spree = 0;
        m_rank.m_max_killing_spree = 0;
        m_rank.m_flag_capture = 0;
    }

    long m_id;
    char m_ip[MAX_IP_LENGTH];
    unsigned long m_name;
    unsigned long m_clan;
    int m_country;
    unsigned long m_level;
    unsigned long m_xp;
    unsigned long m_score;
    Upgrade m_upgrade;
    Rank m_rank;
    unsigned long m_kill;
    unsigned long m_dead;
    unsigned long m_suicide;
    float m_rapport;
    unsigned long m_log_in;
    unsigned long m_fire;
    unsigned long m_pickup_weapon;
    unsigned long m_pickup_ninja;
    unsigned long m_change_weapon;
    unsigned long m_time_play;
    unsigned long m_message;
    unsigned long m_killing_spree;
    unsigned long m_max_killing_spree;
    unsigned long m_flag_capture;
    unsigned long m_last_connect;
    bool m_lock;
    int m_Weapon[NUM_WEAPONS];

    unsigned long m_start_time;
    unsigned long m_actual_kill;
};

class CStatistiques
{
public:
    CStatistiques(CGameContext *GameServer);
    ~CStatistiques();

    void Tick();

    void SetInfo(long id, const char name[], const char clan[], const int country);

    long GetId(const char ip[MAX_IP_LENGTH], const char name[], const char clan[], const int country);
    inline unsigned long GetLevel(long id)
    {
        UpdateStat(id);
        return m_statistiques[id].m_level;
    }
    inline unsigned long GetXp(long id)
    {
        UpdateStat(id);
        return m_statistiques[id].m_xp;
    }
    inline unsigned long GetScore(long id)
    {
        UpdateStat(id);
        return m_statistiques[id].m_score;
    }
    inline unsigned long GetActualKill(long id)
    {
        return m_statistiques[id].m_actual_kill;
    }
    inline StatWeapon GetStatWeapon(long id)
    {
        return m_statistiques[id].m_upgrade.m_stat_weapon;
    }
    inline StatLife GetStatLife(long id)
    {
        return m_statistiques[id].m_upgrade.m_stat_life;
    }

    void UpdateStat(long id);
    void UpdateUpgrade(long id);
    void UpdateRank();

    void DisplayStat(long id, const char* Name);
    void DisplayRank(long id, const char* Name);
    void DisplayPlayer(long id, int ClientID);
    void DisplayBestOf();

    void WriteStat();

    void ResetPartialStat(long id);
    void ResetAllStat(long id);

    inline void AddKill(long id)
    {
        m_statistiques[id].m_actual_kill++;
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_kill++;
    }
    inline void AddDead(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            m_statistiques[id].m_actual_kill = 0;
            return;
        }
        m_statistiques[id].m_dead++;
        AddKillingSpree(id);
        m_statistiques[id].m_actual_kill = 0;
    }
    inline void AddSuicide(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_suicide++;
    }
    inline void AddFire(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_fire++;
    }
    inline void AddPickUpWeapon(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_pickup_weapon++;
    }
    inline void AddPickUpNinja(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_pickup_ninja++;
    }
    inline void AddChangeWeapon(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_change_weapon++;
    }
    inline void SetStartPlay(long id, int ClientID)
    {
        for (int i = 0; i < NUM_WEAPONS; i++)
        {
            GameServer()->m_apPlayers[ClientID]->m_WeaponType[i] = m_statistiques[id].m_Weapon[i];
        }
        m_statistiques[id].m_start_time = time(NULL);
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_log_in++;
    }
    inline void SetStopPlay(long id, int ClientID)
    {
        for (int i = 0; i < NUM_WEAPONS; i++)
        {
            m_statistiques[id].m_Weapon[i] = GameServer()->m_apPlayers[ClientID]->m_WeaponType[i];
        }
        m_statistiques[id].m_last_connect = time(NULL);
        if ( m_statistiques[id].m_lock )
        {
            m_statistiques[id].m_start_time = 0;
            return;
        }
        AddKillingSpree(id);
        UpdateStat(id);
        m_statistiques[id].m_start_time = 0;
    }
    inline void AddMessage(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_message++;
    }
    inline void AddFlagCapture(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            return;
        }
        m_statistiques[id].m_flag_capture++;
    }
    inline bool Lock(long id)
    {
        if ( m_statistiques[id].m_lock )
        {
            m_statistiques[id].m_lock = false;
            return false;
        }
        else
        {
            m_statistiques[id].m_lock = true;
            return true;
        }
    }

    inline int UpgradeWeapon(long id)
    {
        UpdateUpgrade(id);
        if (!m_statistiques[id].m_upgrade.m_money)
            return 1;
        if (m_statistiques[id].m_lock)
            return 2;
        if (m_statistiques[id].m_upgrade.m_weapon >= 40)
            return 3;
        m_statistiques[id].m_upgrade.m_weapon++;
        UpdateUpgrade(id);
        return 0;
    };
    inline int UpgradeLife(long id)
    {
        UpdateUpgrade(id);
        if (!m_statistiques[id].m_upgrade.m_money)
            return 1;
        if (m_statistiques[id].m_lock)
            return 2;
        if (m_statistiques[id].m_upgrade.m_life >= 40)
            return 3;
        m_statistiques[id].m_upgrade.m_life++;
        UpdateUpgrade(id);
        return 0;
    };
    inline int UpgradeMove(long id)
    {
        UpdateUpgrade(id);
        if (!m_statistiques[id].m_upgrade.m_money)
            return 1;
        if (m_statistiques[id].m_lock)
            return 2;
        if (m_statistiques[id].m_upgrade.m_move >= 40)
            return 3;
        m_statistiques[id].m_upgrade.m_move++;
        UpdateUpgrade(id);
        return 0;
    };
    inline int UpgradeHook(long id)
    {
        UpdateUpgrade(id);
        if (!m_statistiques[id].m_upgrade.m_money)
            return 1;
        if (m_statistiques[id].m_lock)
            return 2;
        if (m_statistiques[id].m_upgrade.m_hook >= 40)
            return 3;
        m_statistiques[id].m_upgrade.m_hook++;
        UpdateUpgrade(id);
        return 0;
    };

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

    inline void AddKillingSpree(long id)
    {
        if ( m_statistiques[id].m_actual_kill > 5 )
        {
            m_statistiques[id].m_killing_spree += m_statistiques[id].m_actual_kill;
            if ( m_statistiques[id].m_actual_kill > m_statistiques[id].m_max_killing_spree )
            {
                m_statistiques[id].m_max_killing_spree = m_statistiques[id].m_actual_kill;
            }
        }
    }

    std::vector<Stats> m_statistiques;
    std::vector<Stats*> m_tri[14];
    bool m_write;
    int m_last_write;
    int m_errors;

    CGameContext *m_pGameServer;
};

#endif
