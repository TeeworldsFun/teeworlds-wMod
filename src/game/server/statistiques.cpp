#include <new>
#include <algorithm>
#include "statistiques.h"

CStatistiques::CStatistiques(CGameContext *GameServer)
{
	m_pGameServer = GameServer;

    m_errors = 0;
	m_last_write = time_timestamp();
	std::ifstream fichier("Statistiques.txt");
	Stats stats;
	if(!fichier.is_open() || fichier.eof())
    {
        fichier.close();

        std::ofstream fichierOut("Statistiques.txt", std::ios::out);
        if ( !fichierOut.is_open() )
            m_write = false;
        else
            m_write = true;

        fichier.open("Statistiques.bak", std::ifstream::in);
        if (!fichier.is_open())
            return;
    }

    for ( int i = 0; !fichier.eof(); i++ )
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
		fichier >> m_statistiques[i].m_last_connect;
		fichier >> m_statistiques[i].m_lock;
		fichier >> m_statistiques[i].m_upgrade.m_weapon;
		fichier >> m_statistiques[i].m_upgrade.m_life;
		fichier >> m_statistiques[i].m_upgrade.m_move;
		fichier >> m_statistiques[i].m_upgrade.m_hook;
		m_statistiques[i].m_start_time = 0;
		m_statistiques[i].m_actual_kill = 0;
		UpdateStat(i);
		UpdateUpgrade(i);
	}

	m_write = true;

	UpdateRank();
}

CStatistiques::~CStatistiques()
{

}

void CStatistiques::Tick()
{
	if ( difftime(time_timestamp(), m_last_write) >= 150 )
	{
		m_last_write = time_timestamp();
		WriteStat();
		UpdateRank();
		//if ( m_errors >= 5 )
            //m_write = false;
	}
}

void CStatistiques::SetInfo(long id, const char name[], const char clan[], const int country)
{
	m_statistiques[id].m_name = str_quickhash(name);
	m_statistiques[id].m_clan = str_quickhash(clan);
	m_statistiques[id].m_country = country;
}

long CStatistiques::GetId(const char ip[MAX_IP_LENGTH], const char a[], const char b[], const int country)
{
	unsigned long Name = str_quickhash(a);
	unsigned long Clan = str_quickhash(b);

	std::vector<Stats>::iterator first = m_statistiques.begin();
	const std::vector<Stats>::iterator end = m_statistiques.end();

	unsigned long occurences = 0;
	std::vector<unsigned long> id;

	for ( ; first != end; first++ )
	{
		if ( str_comp(ip, first->m_ip) == 0 )
		{
			id.push_back(first->m_id);
			occurences++;
		}
	}

	if ( occurences == 1 && m_statistiques[id[0]].m_start_time == 0 )
	{
		m_statistiques[id[0]].m_name = Name;
		m_statistiques[id[0]].m_clan = Clan;
		m_statistiques[id[0]].m_country = country;
		return id[0];
	}
	else if ( occurences > 1 )
	{
		for ( unsigned long i = 0; i < occurences; i++ )
		{
			if ( m_statistiques[id[i]].m_start_time == 0 && m_statistiques[id[i]].m_name == Name && m_statistiques[id[i]].m_clan == Clan && m_statistiques[id[i]].m_country == country)
			{
				return id[i];
			}
		}

		for ( unsigned long i = 0; i < occurences; i++ )
		{
			if ( m_statistiques[id[i]].m_start_time == 0 && m_statistiques[id[i]].m_name == Name && m_statistiques[id[i]].m_clan == Clan )
			{
				m_statistiques[id[i]].m_country = country;
				return id[i];
			}
		}

		for ( unsigned long i = 0; i < occurences; i++ )
		{
			if ( m_statistiques[id[i]].m_start_time == 0 && m_statistiques[id[i]].m_name == Name && m_statistiques[id[i]].m_country == country)
			{
				m_statistiques[id[i]].m_clan = Clan;
				return id[i];
			}
		}

		for ( unsigned long i = 0; i < occurences; i++ )
		{
			if ( m_statistiques[id[i]].m_start_time == 0 && m_statistiques[id[i]].m_name == Name )
			{
				m_statistiques[id[i]].m_clan = Clan;
				m_statistiques[id[i]].m_country = country;
				return id[i];
			}
		}
	}
	else
	{
		occurences = 0;
		id.resize(0);
		first = m_statistiques.begin();

		for ( ; first != end; first++ )
		{
			if ( first->m_name == Name )
			{
				id.push_back(first->m_id);
				occurences++;
			}
		}

		if ( occurences == 1 && m_statistiques[id[0]].m_start_time == 0 )
		{
			str_copy(m_statistiques[id[0]].m_ip, ip, MAX_IP_LENGTH);
			m_statistiques[id[0]].m_clan = Clan;
			m_statistiques[id[0]].m_country = country;
			return id[0];
		}
		else if ( occurences > 1 )
		{
			for ( unsigned long i = 0; i < occurences; i++ )
			{
				if ( m_statistiques[id[i]].m_start_time == 0 && m_statistiques[id[i]].m_clan == Clan && m_statistiques[id[i]].m_country == country )
				{
					str_copy(m_statistiques[id[i]].m_ip, ip, MAX_IP_LENGTH);
					return id[i];
				}
			}

			for ( unsigned long i = 0; i < occurences; i++ )
			{
				if ( m_statistiques[id[i]].m_start_time == 0 && m_statistiques[id[i]].m_clan == Clan )
				{
					str_copy(m_statistiques[id[i]].m_ip, ip, MAX_IP_LENGTH);
					m_statistiques[id[i]].m_country = country;
					return id[i];
				}
			}

			for ( unsigned long i = 0; i < occurences; i++ )
			{
				if ( m_statistiques[id[i]].m_start_time == 0 && m_statistiques[id[i]].m_country == country )
				{
					str_copy(m_statistiques[id[i]].m_ip, ip, MAX_IP_LENGTH);
					m_statistiques[id[i]].m_clan = Clan;
					return id[i];
				}
			}

			for ( unsigned long i = 0; i < occurences; i++ )
			{
				if ( m_statistiques[id[i]].m_start_time == 0 )
				{
					str_copy(m_statistiques[id[i]].m_ip, ip, MAX_IP_LENGTH);
					m_statistiques[id[i]].m_clan = Clan;
					m_statistiques[id[i]].m_country = country;
					return id[i];
				}
			}
		}
	}

	Stats Statistiques;
	unsigned long new_id = m_statistiques.size();
	m_statistiques.push_back(Statistiques);
	m_statistiques[new_id].m_id = new_id;
	str_copy(m_statistiques[new_id].m_ip, ip, MAX_IP_LENGTH);
	m_statistiques[new_id].m_name = Name;
	m_statistiques[new_id].m_clan = Clan;
	m_statistiques[new_id].m_country = country;
	m_statistiques[new_id].m_lock = false;
	UpdateStat(new_id);
	UpdateRank();
	return new_id;
}

void CStatistiques::UpdateStat(long id)
{
	if ( m_statistiques[id].m_dead != 0 )
		m_statistiques[id].m_rapport = (float)m_statistiques[id].m_kill / (float)m_statistiques[id].m_dead;
	else
		m_statistiques[id].m_rapport = 1;

	if ((m_statistiques[id].m_kill * m_statistiques[id].m_rapport) > m_statistiques[id].m_suicide)
		m_statistiques[id].m_score = ((m_statistiques[id].m_kill * m_statistiques[id].m_rapport) - m_statistiques[id].m_suicide) + m_statistiques[id].m_killing_spree + (m_statistiques[id].m_flag_capture * 5);
	else
		m_statistiques[id].m_score = 0;

	m_statistiques[id].m_level = floor(((-1)+sqrt(1+(8*(m_statistiques[id].m_kill + m_statistiques[id].m_killing_spree + (m_statistiques[id].m_flag_capture * 5)))))/2);
	m_statistiques[id].m_xp = (m_statistiques[id].m_kill + m_statistiques[id].m_killing_spree + (m_statistiques[id].m_flag_capture * 5)) - (m_statistiques[id].m_level * (m_statistiques[id].m_level + 1))/2;

	if ( m_statistiques[id].m_start_time != 0 && !m_statistiques[id].m_lock)
	{
		m_statistiques[id].m_time_play += difftime(time_timestamp(), m_statistiques[id].m_start_time);
		m_statistiques[id].m_start_time = time_timestamp();
	}
}

void CStatistiques::UpdateUpgrade(long id)
{
	m_statistiques[id].m_upgrade.m_money = m_statistiques[id].m_level - (m_statistiques[id].m_upgrade.m_weapon + m_statistiques[id].m_upgrade.m_life + m_statistiques[id].m_upgrade.m_move + m_statistiques[id].m_upgrade.m_hook);

	if ( m_statistiques[id].m_upgrade.m_weapon >= 1 )
		m_statistiques[id].m_upgrade.m_stat_weapon.m_auto_gun = true;
	if ( m_statistiques[id].m_upgrade.m_weapon >= 2 )
		m_statistiques[id].m_upgrade.m_stat_weapon.m_auto_hammer = true;
	if ( m_statistiques[id].m_upgrade.m_weapon >= 36 )
	{
		m_statistiques[id].m_upgrade.m_stat_weapon.m_speed = 1 + ((int)(m_statistiques[id].m_upgrade.m_weapon/3) * 0.5f);
		m_statistiques[id].m_upgrade.m_stat_weapon.m_regeneration = (int)((m_statistiques[id].m_upgrade.m_weapon - 1)/3);
		m_statistiques[id].m_upgrade.m_stat_weapon.m_stockage = 10 + (int)(((m_statistiques[id].m_upgrade.m_weapon - 1)/3) * 5);
	}
	else if ( m_statistiques[id].m_upgrade.m_weapon > 36 )
	{
		m_statistiques[id].m_upgrade.m_stat_weapon.m_speed = 1 + ((int)(m_statistiques[id].m_upgrade.m_weapon/3) * 0.5f);
		m_statistiques[id].m_upgrade.m_stat_weapon.m_regeneration = -1;
		m_statistiques[id].m_upgrade.m_stat_weapon.m_stockage = -1;
		if ( m_statistiques[id].m_upgrade.m_weapon > 38 )
			m_statistiques[id].m_upgrade.m_stat_weapon.m_all_weapon = true;
		if ( m_statistiques[id].m_upgrade.m_weapon > 39 )
			m_statistiques[id].m_upgrade.m_stat_weapon.m_bounce = true;
	}
}

class Trier
{
public:
	enum {LEVEL, SCORE, KILL, RAPPORT, LOG_IN, FIRE, PICKUP_WEAPON, PICKUP_NINJA, CHANGE_WEAPON, TIME_PLAY, MESSAGE, KILLING_SPREE, MAX_KILLING_SPREE, FLAG_CAPTURE};

	Trier(int type)
	{
		m_type = type;
	}

	bool operator()(const Stats *a, const Stats *b)
	{
		switch (m_type)
		{
			case LEVEL:
				return a->m_level > b->m_level;
				break;
			case SCORE:
				return a->m_score > b->m_score;
				break;
			case KILL:
				return a->m_kill > b->m_kill;
				break;
			case RAPPORT:
				return a->m_rapport > b->m_rapport;
				break;
			case LOG_IN:
				return a->m_log_in > b->m_log_in;
				break;
			case FIRE:
				return a->m_fire > b->m_fire;
				break;
			case PICKUP_WEAPON:
				return a->m_pickup_weapon > b->m_pickup_weapon;
				break;
			case PICKUP_NINJA:
				return a->m_pickup_ninja > b->m_pickup_ninja;
				break;
			case CHANGE_WEAPON:
				return a->m_change_weapon > b->m_change_weapon;
				break;
			case TIME_PLAY:
				return a->m_time_play > b->m_time_play;
				break;
			case MESSAGE:
				return a->m_message > b->m_message;
				break;
			case KILLING_SPREE:
				return a->m_killing_spree > b->m_killing_spree;
				break;
			case MAX_KILLING_SPREE:
				return a->m_max_killing_spree > b->m_max_killing_spree;
				break;
			case FLAG_CAPTURE:
				return a->m_flag_capture > b->m_flag_capture;
				break;
		}

		return a->m_score > b->m_score;
	}

	int m_type;
};


void CStatistiques::UpdateRank()
{
	std::vector<Stats*> tri;

	for ( unsigned long i = 0; i < m_statistiques.size(); i++ )
	{
		tri.push_back(&m_statistiques[i]);
	}

	Trier FoncteurTri(0);
	for ( int i = 0; i < Trier::FLAG_CAPTURE + 1; i++ )
	{
		FoncteurTri.m_type = i;
		sort(tri.begin(), tri.end(), FoncteurTri);

		switch (i)
		{
			case Trier::LEVEL:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_level = j + 1;
				}
				break;
			case Trier::SCORE:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_score = j + 1;
				}
				break;
			case Trier::KILL:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_kill = j + 1;
				}
				break;
			case Trier::RAPPORT:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_rapport = j + 1;
				}
				break;
			case Trier::LOG_IN:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_log_in = j + 1;
				}
				break;
			case Trier::FIRE:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_fire = j + 1;
				}
				break;
			case Trier::PICKUP_WEAPON:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_pickup_weapon = j + 1;
				}
				break;
			case Trier::PICKUP_NINJA:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_pickup_ninja = j + 1;
				}
				break;
			case Trier::CHANGE_WEAPON:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_change_weapon = j + 1;
				}
				break;
			case Trier::TIME_PLAY:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_time_play = j + 1;
				}
				break;
			case Trier::MESSAGE:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_message = j + 1;
				}
				break;
			case Trier::KILLING_SPREE:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_killing_spree = j + 1;
				}
				break;
			case Trier::MAX_KILLING_SPREE:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_max_killing_spree = j + 1;
				}
				break;
			case Trier::FLAG_CAPTURE:
				for ( unsigned long j = 0; j < tri.size(); j++ )
				{
					tri[j]->m_rank.m_flag_capture = j + 1;
				}
				break;
		}
	}
}

void CStatistiques::DisplayStat(long id, const char* Name)
{
	UpdateStat(id);

	char a[256] = "";
	char stats[18][50];

	str_format(stats[0], 50, "Name : %s", Name);
	str_format(stats[1], 50, "Level : %ld", m_statistiques[id].m_level);
	str_format(stats[2], 50, "Rank : %ld.", m_statistiques[id].m_rank.m_score);

	str_format(stats[3], 50, "Score : %ld", m_statistiques[id].m_score);
	str_format(stats[4], 50, "Killed : %ld", m_statistiques[id].m_kill);
	str_format(stats[5], 50, "Dead : %ld", m_statistiques[id].m_dead);

	str_format(stats[6], 50, "Rapport K/D : %lf", m_statistiques[id].m_rapport);
	str_format(stats[7], 50, "Suicide : %ld", m_statistiques[id].m_suicide);
	str_format(stats[8], 50, "Log-in : %ld", m_statistiques[id].m_log_in);

	str_format(stats[9], 50, "Fire : %ld", m_statistiques[id].m_fire);
	str_format(stats[10], 50, "Pick-Up Weapon : %ld", m_statistiques[id].m_pickup_weapon);
	str_format(stats[11], 50, "Pick-Up Ninja : %ld", m_statistiques[id].m_pickup_ninja);

	str_format(stats[12], 50, "Switch Weapon : %ld", m_statistiques[id].m_change_weapon);
	str_format(stats[13], 50, "Time Play : %ld min", m_statistiques[id].m_time_play / 60);
	str_format(stats[14], 50, "Msg Sent : %ld", m_statistiques[id].m_message);

	str_format(stats[15], 50, "Total Killing Spree : %ld", m_statistiques[id].m_killing_spree);
	str_format(stats[16], 50, "Max Killing Spree : %ld", m_statistiques[id].m_max_killing_spree);
	str_format(stats[17], 50, "Flag Capture : %ld", m_statistiques[id].m_flag_capture);

	for ( int i = 0; i < 6; i++ )
	{
		str_format(a, 256, "%s | %s | %s", stats[i * 3], stats[(i * 3) + 1], stats[(i * 3) + 2]);
		GameServer()->SendChatTarget(-1, a);
	}
}

const char* Suffix(unsigned long number)
{
	if ( number == 1 )
		return "st";
	else if ( number == 2 )
		return "nd";
	else if ( number == 3 )
		return "rd";
	else
		return "th";
}

void CStatistiques::DisplayRank(long id, const char* Name)
{
	char a[256] = "";
	char ranks[15][50];

	str_format(ranks[0], 50, "Name : %s", Name);
	str_format(ranks[1], 50, "Level : %ld%s.", m_statistiques[id].m_rank.m_level, Suffix(m_statistiques[id].m_rank.m_level));
	str_format(ranks[2], 50, "Score : %ld%s.", m_statistiques[id].m_rank.m_score, Suffix(m_statistiques[id].m_rank.m_score));

	str_format(ranks[3], 50, "Killed : %ld%s.", m_statistiques[id].m_rank.m_kill, Suffix(m_statistiques[id].m_rank.m_kill));
	str_format(ranks[4], 50, "Rapport K/D : %ld%s.", m_statistiques[id].m_rank.m_rapport, Suffix(m_statistiques[id].m_rank.m_rapport));
	str_format(ranks[5], 50, "Log-in : %ld%s.", m_statistiques[id].m_rank.m_log_in, Suffix(m_statistiques[id].m_rank.m_log_in));

	str_format(ranks[6], 50, "Fire : %ld%s.", m_statistiques[id].m_rank.m_fire, Suffix(m_statistiques[id].m_rank.m_fire));
	str_format(ranks[7], 50, "Pick-Up Weapon : %ld%s.", m_statistiques[id].m_rank.m_pickup_weapon, Suffix(m_statistiques[id].m_rank.m_pickup_weapon));
	str_format(ranks[8], 50, "Pick-Up Ninja : %ld%s.", m_statistiques[id].m_rank.m_pickup_ninja, Suffix(m_statistiques[id].m_rank.m_pickup_ninja));

	str_format(ranks[9], 50, "Switch Weapon : %ld%s.", m_statistiques[id].m_rank.m_change_weapon, Suffix(m_statistiques[id].m_rank.m_change_weapon));
	str_format(ranks[10], 50, "Time Play : %ld%s.", m_statistiques[id].m_rank.m_time_play, Suffix(m_statistiques[id].m_rank.m_time_play));
	str_format(ranks[11], 50, "Msg Sent : %ld%s.", m_statistiques[id].m_rank.m_message, Suffix(m_statistiques[id].m_rank.m_message));

	str_format(ranks[12], 50, "Total Killing Spree : %ld%s.", m_statistiques[id].m_rank.m_killing_spree, Suffix(m_statistiques[id].m_rank.m_killing_spree));
	str_format(ranks[13], 50, "Max Killing Spree : %ld%s.", m_statistiques[id].m_rank.m_max_killing_spree, Suffix(m_statistiques[id].m_rank.m_max_killing_spree));
	str_format(ranks[14], 50, "Flag Capture : %ld%s.", m_statistiques[id].m_rank.m_flag_capture, Suffix(m_statistiques[id].m_rank.m_flag_capture));

	for ( int i = 0; i < 5; i++ )
	{
		str_format(a, 256, "%s | %s | %s", ranks[i * 3], ranks[(i * 3) + 1], ranks[(i * 3) + 2]);
		GameServer()->SendChatTarget(-1, a);
	}
}

void CStatistiques::DisplayPlayer(long id, int ClientID)
{
	UpdateUpgrade(id);
	char upgr[7][50];

	str_format(upgr[0], 50, "Name : %s | Level : %ld | Score : %ld", GameServer()->m_apPlayers[ClientID]->GetRealName(), m_statistiques[id].m_level, m_statistiques[id].m_score);
	str_format(upgr[1], 50, "Upgrade :");
	str_format(upgr[2], 50, "Money : %ld", m_statistiques[id].m_upgrade.m_money);
	str_format(upgr[3], 50, "Weapon: %ld", m_statistiques[id].m_upgrade.m_weapon);
	str_format(upgr[4], 50, "Life : %ld", m_statistiques[id].m_upgrade.m_life);
	str_format(upgr[5], 50, "Move : %ld", m_statistiques[id].m_upgrade.m_move);
	str_format(upgr[6], 50, "Hook : %ld", m_statistiques[id].m_upgrade.m_hook);


	for ( int i = 0; i < 7; i++ )
	{
		GameServer()->SendChatTarget(ClientID, upgr[i]);
	}
}

void CStatistiques::WriteStat()
{
	if ( m_write == false )
		return;

	std::ofstream fichier("Statistiques.new", std::ios::out | std::ios::trunc);
	if(fichier.is_open())
	{
		for ( unsigned long i = 0; i < m_statistiques.size(); i++ )
		{
			UpdateStat(i);
			if ( m_statistiques[i].m_level > 0 )
			{
				fichier << m_statistiques[i].m_ip << " ";
				fichier << m_statistiques[i].m_name << " ";
				fichier << m_statistiques[i].m_clan << " ";
				fichier << m_statistiques[i].m_country << " ";
				fichier << m_statistiques[i].m_kill << " ";
				fichier << m_statistiques[i].m_dead << " ";
				fichier << m_statistiques[i].m_suicide << " ";
				fichier << m_statistiques[i].m_log_in << " ";
				fichier << m_statistiques[i].m_fire << " ";
				fichier << m_statistiques[i].m_pickup_weapon << " ";
				fichier << m_statistiques[i].m_pickup_ninja << " ";
				fichier << m_statistiques[i].m_change_weapon << " ";
				fichier << m_statistiques[i].m_time_play << " ";
				fichier << m_statistiques[i].m_message << " ";
				fichier << m_statistiques[i].m_killing_spree << " ";
				fichier << m_statistiques[i].m_max_killing_spree << " ";
				fichier << m_statistiques[i].m_flag_capture << " ";
				fichier << m_statistiques[i].m_last_connect << " ";
				fichier << m_statistiques[i].m_lock << " ";
				fichier << m_statistiques[i].m_upgrade.m_weapon << " ";
				fichier << m_statistiques[i].m_upgrade.m_life << " ";
				fichier << m_statistiques[i].m_upgrade.m_move << " ";
				fichier << m_statistiques[i].m_upgrade.m_hook << " ";
			}
		}

		fichier.close();

		std::ifstream test("Statistiques.bak");
		if ( test.is_open() )
		{
			test.close();
			if (fs_remove("Statistiques.bak"))
            {
                m_errors++;
				return;
            }
		}

		if ( fs_rename("Statistiques.txt", "Statistiques.bak") == 0 )
		{
			fs_rename("Statistiques.new", "Statistiques.txt");
		}
        else
            m_errors++;
	}
    else
        m_errors++;
}

void CStatistiques::ResetPartialStat(long id)
{
	m_statistiques[id].m_kill = 0;
	m_statistiques[id].m_dead = 0;
	m_statistiques[id].m_suicide = 0;
	m_statistiques[id].m_fire = 0;
	m_statistiques[id].m_killing_spree = 0;
	m_statistiques[id].m_max_killing_spree = 0;
	m_statistiques[id].m_flag_capture = 0;
	UpdateStat(id);
}

void CStatistiques::ResetAllStat(long id)
{
	m_statistiques[id].m_kill = 0;
	m_statistiques[id].m_dead = 0;
	m_statistiques[id].m_suicide = 0;
	m_statistiques[id].m_log_in = 0;
	m_statistiques[id].m_fire = 0;
	m_statistiques[id].m_pickup_weapon = 0;
	m_statistiques[id].m_pickup_ninja = 0;
	m_statistiques[id].m_change_weapon = 0;
	m_statistiques[id].m_time_play = 0;
	m_statistiques[id].m_message = 0;
	m_statistiques[id].m_killing_spree = 0;
	m_statistiques[id].m_max_killing_spree = 0;
	m_statistiques[id].m_flag_capture = 0;
	UpdateStat(id);
}

