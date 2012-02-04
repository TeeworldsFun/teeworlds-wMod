#include <iostream>
#include <ctime>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

#define DB_VERSION 1

enum { MAX_NAME_LENGHT = 16, MAX_CLAN_LENGTH = 12, MAX_IP_LENGTH = 45 };

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
    int m_regeneration;
    int m_stockage;
    bool m_all_weapon;
    int m_bounce;
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
    int m_start_armor;
    int m_regeneration;
    int m_stockage[2];
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
    int m_num_jump;
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

struct Upgrade
{
    unsigned long m_money;
    unsigned long m_weapon;
    unsigned long m_life;
    unsigned long m_move;
    unsigned long m_hook;
    StatWeapon m_stat_weapon;
    StatLife m_stat_life;
    StatMove m_stat_move;
    StatHook m_stat_hook;
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
    int m_Weapon[4];
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
        m_bonus_xp = 0;
        m_start_time = 0;
        m_actual_kill = 0;
        m_last_connect = 0;

        m_conf.m_InfoHealKiller = true;
        m_conf.m_InfoXP = true;
        m_conf.m_InfoLevelUp = true;
        m_conf.m_InfoKillingSpree = true;
        m_conf.m_InfoRace = true;
        m_conf.m_InfoAmmo = true;
        m_conf.m_ShowVoter = true;
        m_conf.m_AmmoAbsolute = true;
        m_conf.m_LifeAbsolute = true;
        m_conf.m_Lock = false;
        for ( int i = 0; i < 4; i++ )
            m_conf.m_Weapon[i] = 0;

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
        
        m_to_remove = false;
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
    unsigned long m_bonus_xp;
    unsigned long m_last_connect;

    Conf m_conf;

    unsigned long m_start_time;
    unsigned long m_actual_kill;
    bool m_to_remove;
};


std::vector<Stats> m_statistiques;

void str_time(char *buffer, int buffer_size, time_t time_data)
{
	struct tm *time_info;
	time_info = localtime(&time_data);
	strftime(buffer, buffer_size, "%Y-%m-%d %H-%M-%S", time_info);
	buffer[buffer_size-1] = 0;	/* assure null termination */
}

void WriteStat()
{
    std::ofstream fichier("Statistiques.db", std::ios::out | std::ios::trunc);
    if(fichier.is_open())
    {
        char Time[256] = "";
        int Heures = 0;
        int Minutes = 0;
        int Secondes = 0;

        for ( unsigned long i = 0; i < m_statistiques.size(); i++ )
        {
            Heures = m_statistiques[i].m_time_play / 3600;
            Minutes = (m_statistiques[i].m_time_play / 60) % 60;
            Secondes = m_statistiques[i].m_time_play % 60;

            fichier << i + 1 << ";";
            fichier << m_statistiques[i].m_ip << ";";
            fichier << m_statistiques[i].m_name << ";";
            fichier << m_statistiques[i].m_clan << ";";
            fichier << m_statistiques[i].m_country << ";";
            str_time(Time, 256, m_statistiques[i].m_last_connect);
            fichier << Time << ";";
            fichier << m_statistiques[i].m_kill << ";";
            fichier << m_statistiques[i].m_dead << ";";
            fichier << m_statistiques[i].m_suicide << ";";
            fichier << m_statistiques[i].m_log_in << ";";
            fichier << m_statistiques[i].m_fire << ";";
            fichier << m_statistiques[i].m_pickup_weapon << ";";
            fichier << m_statistiques[i].m_pickup_ninja << ";";
            fichier << m_statistiques[i].m_change_weapon << ";";
            fichier << Heures << ":" << Minutes << ":" << Secondes << ";";
            fichier << m_statistiques[i].m_message << ";";
            fichier << m_statistiques[i].m_killing_spree << ";";
            fichier << m_statistiques[i].m_max_killing_spree << ";";
            fichier << m_statistiques[i].m_flag_capture << ";";
            fichier << m_statistiques[i].m_bonus_xp << ";";
            fichier << m_statistiques[i].m_upgrade.m_weapon << ";";
            fichier << m_statistiques[i].m_upgrade.m_life << ";";
            fichier << m_statistiques[i].m_upgrade.m_move << ";";
            fichier << m_statistiques[i].m_upgrade.m_hook << ";";
            fichier << m_statistiques[i].m_conf.m_InfoHealKiller << ";";
            fichier << m_statistiques[i].m_conf.m_InfoXP << ";";
            fichier << m_statistiques[i].m_conf.m_InfoLevelUp << ";";
            fichier << m_statistiques[i].m_conf.m_InfoKillingSpree << ";";
            fichier << m_statistiques[i].m_conf.m_InfoRace << ";";
            fichier << m_statistiques[i].m_conf.m_InfoAmmo << ";";
            fichier << m_statistiques[i].m_conf.m_AmmoAbsolute << ";";
            fichier << m_statistiques[i].m_conf.m_LifeAbsolute << ";";
            fichier << m_statistiques[i].m_conf.m_ShowVoter << ";";
            fichier << m_statistiques[i].m_conf.m_Lock << ";";
            for (int j = 0; j < 4; j++)
                fichier << 0 << ";";
            fichier << std::endl;
        }
        fichier.close();
    }
}

int main(int argc, char *argv[])
{
    std::ifstream fichier("Statistiques.txt");
    Stats stats;

    if(!fichier.is_open())
    {
        std::cout << "Can't open Statistiques.txt !" << std::endl;
        return 1;
    }
    else if (fichier.eof())
    {
        fichier.close();
        std::cout << "Statistiques.txt is empty !" << std::endl;
        return 1;
    }

    int Version = 0;
    fichier >> Version;

    if (Version != 1)
    {
        fichier.close();

        std::cout << "Please upgrade Statistiques.txt to ver. 1 !" << std::endl;
        return 1;
    }

    int i = 0;
    for (i = 0; !fichier.eof(); i++ )
    {
        m_statistiques.push_back(stats);
        m_statistiques[i].m_id = i;
        fichier >> m_statistiques[i].m_ip;
        fichier >> m_statistiques[i].m_name;
        fichier >> m_statistiques[i].m_clan;
        fichier >> m_statistiques[i].m_country;
        fichier >> m_statistiques[i].m_kill;
        fichier >> m_statistiques[i].m_dead;
        fichier >> m_statistiques[i].m_suicide;
        fichier >> m_statistiques[i].m_log_in;
        fichier >> m_statistiques[i].m_fire;
        fichier >> m_statistiques[i].m_pickup_weapon;
        fichier >> m_statistiques[i].m_pickup_ninja;
        fichier >> m_statistiques[i].m_change_weapon;
        fichier >> m_statistiques[i].m_time_play;
        fichier >> m_statistiques[i].m_message;
        fichier >> m_statistiques[i].m_killing_spree;
        fichier >> m_statistiques[i].m_max_killing_spree;
        fichier >> m_statistiques[i].m_flag_capture;
        fichier >> m_statistiques[i].m_bonus_xp;
        fichier >> m_statistiques[i].m_last_connect;
        fichier >> m_statistiques[i].m_conf.m_InfoHealKiller;
        fichier >> m_statistiques[i].m_conf.m_InfoXP;
        fichier >> m_statistiques[i].m_conf.m_InfoLevelUp;
        fichier >> m_statistiques[i].m_conf.m_InfoKillingSpree;
        fichier >> m_statistiques[i].m_conf.m_InfoRace;
        fichier >> m_statistiques[i].m_conf.m_InfoAmmo;
        fichier >> m_statistiques[i].m_conf.m_ShowVoter;
        fichier >> m_statistiques[i].m_conf.m_AmmoAbsolute;
        fichier >> m_statistiques[i].m_conf.m_LifeAbsolute;
        fichier >> m_statistiques[i].m_conf.m_Lock;
        for (int j = 0; j < 4; j++)
            fichier >> m_statistiques[i].m_conf.m_Weapon[j];
        fichier >> m_statistiques[i].m_upgrade.m_weapon;
        fichier >> m_statistiques[i].m_upgrade.m_life;
        fichier >> m_statistiques[i].m_upgrade.m_move;
        fichier >> m_statistiques[i].m_upgrade.m_hook;
        m_statistiques[i].m_start_time = 0;
        m_statistiques[i].m_actual_kill = 0;
    }

    m_statistiques.pop_back();

    WriteStat();    
    std::cout << "Done !" << std::endl;
	return 0;
}

