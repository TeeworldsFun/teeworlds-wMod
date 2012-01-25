#include "statistiques.h"
#include <game/server/gamecontext.h>

CStats::CStats(CPlayer* pPlayer)
{
    m_pPlayer = pPlayer;
    m_pGameServer = pPlayer->GameServer();
}

void CStats::SetInfo(const char Name[], const char Clan[], const int Country, const char Ip[MAX_IP_LENGTH])
{
    if (str_comp(Ip, "") != 0)
        str_copy(m_player.m_ip, Ip, MAX_IP_LENGTH);

    str_copy(m_player.m_pseudo, Name, MAX_NAME_LENGTH);
    str_copy(m_player.m_clan, Clan, MAX_CLAN_LENGTH);
    m_player.m_country = country;
}


void CStats::UpdateStat()
{
    if ( m_stats.m_dead != 0 )
        m_stats.m_rapport = (double)m_stats.m_kill / (double)m_stats.m_dead;
    else
        m_stats.m_rapport = 1;

    if ((m_stats.m_kill * m_stats.m_rapport) > m_stats.m_suicide)
        m_stats.m_score = ((m_stats.m_kill * m_stats.m_rapport) - m_stats.m_suicide) + m_stats.m_killing_spree + (m_stats.m_flag_capture * 5);
    else
        m_stats.m_score = 0;

    m_stats.m_level = floor(((-1)+sqrt(1+(8*(m_stats.m_kill + m_stats.m_killing_spree + (m_stats.m_flag_capture * 5) + m_stats.m_bonus_xp))))/2);
    m_stats.m_xp = (m_stats.m_kill + m_stats.m_killing_spree + (m_stats.m_flag_capture * 5) + m_stats.m_bonus_xp) - ((m_stats.m_level * (m_stats.m_level + 1))/2);

    if (m_stats.m_start_time != 0 && !m_conf.m_Lock)
    {
        m_stats.m_time_play += difftime(time_timestamp(), m_stats.m_start_time);
        m_stats.m_start_time = time_timestamp();
    }
}

void CStats::UpdateUpgrade()
{
    m_upgr.m_money = m_level - (m_upgr.m_weapon + m_upgr.m_life + m_upgr.m_move + m_upgr.m_hook);

    if ( m_upgr.m_weapon > 40 )
        m_upgr.m_weapon = 40;
    if ( m_upgr.m_life > 40 )
        m_upgr.m_life = 40;
    if ( m_upgr.m_move > 40 )
        m_upgr.m_move = 40;
    if ( m_upgr.m_hook > 40 )
        m_upgr.m_hook = 40;

    m_upgr.m_stat_weapon.m_auto_gun = m_upgr.m_weapon >= 1 ? true : false;
    m_upgr.m_stat_weapon.m_auto_hammer = m_upgr.m_weapon >= 2 ? true : false;
    m_upgr.m_stat_weapon.m_auto_ninja = m_upgr.m_weapon >= 3 ? true : false;
    if ( m_upgr.m_weapon <= 0 )
    {
        m_upgr.m_stat_weapon.m_speed = 1;
        m_upgr.m_stat_weapon.m_regeneration = 0;
        m_upgr.m_stat_weapon.m_stockage = 10;
    }
    else if ( m_upgr.m_weapon <= 36 )
    {
        m_upgr.m_stat_weapon.m_speed = 1 + ((int)(m_upgr.m_weapon/3) * 0.5f);
        m_upgr.m_stat_weapon.m_regeneration = (int)((m_upgr.m_weapon - 1)/3);
        m_upgr.m_stat_weapon.m_stockage = 10 + (int)(((m_upgr.m_weapon - 2)/3) * 5);
    }
    else if ( m_upgr.m_weapon > 36 )
    {
        m_upgr.m_stat_weapon.m_speed = 1 + ((int)(m_upgr.m_weapon/3) * 0.5f);
        m_upgr.m_stat_weapon.m_regeneration = -1;
        m_upgr.m_stat_weapon.m_stockage = -1;
    }
    if ( m_upgr.m_weapon > 38 )
        m_upgr.m_stat_weapon.m_all_weapon = true;
    else
        m_upgr.m_stat_weapon.m_all_weapon = false;
    if ( m_upgr.m_weapon > 39 )
        m_upgr.m_stat_weapon.m_bounce = 1;
    else
        m_upgr.m_stat_weapon.m_bounce = 0;

    m_upgr.m_stat_life.m_protection = 1 + ((int)((m_upgr.m_life + 3)/4) / 5.0f);
    m_upgr.m_stat_life.m_start_armor = (int)(m_upgr.m_life + 2)/4;
    m_upgr.m_stat_life.m_regeneration = (int)(m_upgr.m_life + 1)/4;
    if ( m_upgr.m_life <= 20 )
    {
        m_upgr.m_stat_life.m_stockage[0] = 10 + (int)(m_upgr.m_life / 2);
        m_upgr.m_stat_life.m_stockage[1] = 10;
    }
    else
    {
        m_upgr.m_stat_life.m_stockage[0] = 20;
        m_upgr.m_stat_life.m_stockage[1] = (int)(m_upgr.m_life / 2);
    }
    
    m_upgr.m_stat_move.m_rate_speed = 1 + ((int)((m_upgr.m_move + 3)/ 4) * 0.2f);
    m_upgr.m_stat_move.m_rate_accel = 1 + ((int)((m_upgr.m_move + 2)/ 4) * 0.1f);
    m_upgr.m_stat_move.m_rate_high_jump = 1 + ((int)((m_upgr.m_move + 1)/ 4) * 0.05f);
    m_upgr.m_stat_move.m_num_jump = 1 + (int)(m_upgr.m_move / 4);
    if ( m_upgr.m_move == 40 )
        m_upgr.m_stat_move.m_num_jump = -1;
    
    m_upgr.m_stat_hook.m_rate_length = 1 + ((int)((m_upgr.m_hook + 2)/ 3) * (2.0f/13.0f));
    m_upgr.m_stat_hook.m_rate_time = 1 + ((int)((m_upgr.m_hook + 1)/ 3) * (7.0f/13.0f));
    m_upgr.m_stat_hook.m_rate_speed = 1 + ((int)(m_upgr.m_hook/ 3) * (2.0f/13.0f));
    if ( m_upgr.m_hook == 40 )
        m_upgr.m_stat_hook.m_hook_damage = true;
    else
        m_upgr.m_stat_hook.m_hook_damage = false;
}

void CStats::DisplayStat()
{
    UpdateStat();

    char a[256] = "";
    char stats[18][50];

    str_format(stats[0], 50, "Name : %s", m_pseudo);
    str_format(stats[1], 50, "Level : %ld", m_level);
//  str_format(stats[2], 50, "Rank : %ld.", m_rank.m_score);

    str_format(stats[3], 50, "Score : %ld", m_score);
    str_format(stats[4], 50, "Killed : %ld", m_kill);
    str_format(stats[5], 50, "Dead : %ld", m_dead);

    str_format(stats[6], 50, "Rapport K/D : %lf", m_rapport);
    str_format(stats[7], 50, "Suicide : %ld", m_suicide);
    str_format(stats[8], 50, "Log-in : %ld", m_log_in);

    str_format(stats[9], 50, "Fire : %ld", m_fire);
    str_format(stats[10], 50, "Pick-Up Weapon : %ld", m_pickup_weapon);
    str_format(stats[11], 50, "Pick-Up Ninja : %ld", m_pickup_ninja);

    str_format(stats[12], 50, "Switch Weapon : %ld", m_change_weapon);
    str_format(stats[13], 50, "Time Play : %ld min", m_time_play / 60);
    str_format(stats[14], 50, "Msg Sent : %ld", m_message);

    str_format(stats[15], 50, "Total Killing Spree : %ld", m_killing_spree);
    str_format(stats[16], 50, "Max Killing Spree : %ld", m_max_killing_spree);
    str_format(stats[17], 50, "Flag Capture : %ld", m_flag_capture);

    for ( int i = 0; i < 6; i++ )
    {
        str_format(a, 256, "%s | %s | %s", stats[i * 3], stats[(i * 3) + 1], stats[(i * 3) + 2]);
        GameServer()->SendChatTarget(-1, a);
    }
}

void CStats::DisplayPlayer()
{
    UpdateUpgrade();

    char upgr[7][50];
    str_format(upgr[0], 50, "Name : %s | Level : %ld | Score : %ld", Server()->ClientName(pPlayer->GetCID()), m_level, m_score);
    str_format(upgr[1], 50, "Upgrade :");
    str_format(upgr[2], 50, "Money : %ld", m_upgr.m_money);
    str_format(upgr[3], 50, "Weapon: %ld", m_upgr.m_weapon);
    str_format(upgr[4], 50, "Life : %ld", m_upgr.m_life);
    str_format(upgr[5], 50, "Move : %ld", m_upgr.m_move);
    str_format(upgr[6], 50, "Hook : %ld", m_upgr.m_hook);


    for ( int i = 0; i < 7; i++ )
    {
        GameServer()->SendChatTarget(ClientID, upgr[i]);
    }
}

void CStats::ResetPartialStat()
{
    m_kill = 0;
    m_dead = 0;
    m_suicide = 0;
    m_fire = 0;
    m_killing_spree = 0;
    m_max_killing_spree = 0;
    m_flag_capture = 0;
    m_bonus_xp = 0;

    m_upgr.m_weapon = 0;
    m_upgr.m_life = 0;
    m_upgr.m_move = 0;
    m_upgr.m_hook = 0;
    UpdateStat();
    UpdateUpgrade();
}

void CStatistiques::ResetAllStat()
{
    m_kill = 0;
    m_dead = 0;
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

    m_upgr.m_weapon = 0;
    m_upgr.m_life = 0;
    m_upgr.m_move = 0;
    m_upgr.m_hook = 0;
    UpdateStat();
    UpdateUpgrade();
}

void CStatistiques::ResetUpgr(long id)
{
    m_upgr.m_weapon = 0;
    m_upgr.m_life = 0;
    m_upgr.m_move = 0;
    m_upgr.m_hook = 0;
    UpdateUpgrade();
}

