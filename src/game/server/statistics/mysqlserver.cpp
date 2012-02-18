#if defined(CONF_SQL)

#include "mysqlserver.h"

#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include <cppconn/statement.h>
#include <cppconn/exception.h>

using namespace sql;

CSqlServer::CSqlServer(CGameContext *pGameServer) : IStatsServer(pGameServer)
{   

}

void CSqlServer::OnInit()
{
    if (m_Init)
        return;

    // create connection
    if(Connect())
    {
        try
        {
            // create tables
            char aBuf[] = 
"CREATE TABLE IF NOT EXISTS `Players_Stats` (\
`Id` smallint unsigned NOT NULL AUTO_INCREMENT,\
`Ip` varchar(42) NOT NULL DEFAULT '0.0.0.0',\
`Pseudo` varchar(16) DEFAULT NULL,\
`Clan` varchar(12) DEFAULT NULL,\
`Country` smallint NOT NULL DEFAULT -1,\
`Name` varchar(16) DEFAULT NULL,\
`Password` BIGINT unsigned DEFAULT NULL,\
`Last_Connect` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',\
`Level` mediumint unsigned NOT NULL DEFAULT 0,\
`Score` mediumint unsigned NOT NULL DEFAULT 0,\
`Killed` mediumint unsigned NOT NULL DEFAULT 0,\
`Dead` mediumint unsigned NOT NULL DEFAULT 0,\
`Suicide` mediumint unsigned NOT NULL DEFAULT 0,\
`Rapport` float unsigned NOT NULL DEFAULT 0,\
`Log_In` mediumint unsigned NOT NULL DEFAULT 0,\
`Fire` mediumint unsigned NOT NULL DEFAULT 0,\
`Pickup_Weapon` mediumint unsigned NOT NULL DEFAULT 0,\
`Pickup_Ninja` mediumint unsigned NOT NULL DEFAULT 0,\
`Change_Weapon` mediumint unsigned NOT NULL DEFAULT 0,\
`Time_Play` time NOT NULL DEFAULT 0,\
`Message` mediumint unsigned NOT NULL DEFAULT 0,\
`Killing_Spree` mediumint unsigned NOT NULL DEFAULT 0,\
`Max_Killing_Spree` mediumint unsigned NOT NULL DEFAULT 0,\
`Flag_Capture` mediumint unsigned NOT NULL DEFAULT 0,\
`Bonus_XP` mediumint unsigned NOT NULL DEFAULT 0,\
`Upgrade_Weapon` tinyint unsigned NOT NULL DEFAULT 0,\
`Upgrade_Life` tinyint unsigned NOT NULL DEFAULT 0,\
`Upgrade_Move` tinyint unsigned NOT NULL DEFAULT 0,\
`Upgrade_Hook` tinyint unsigned NOT NULL DEFAULT 0,\
`Info_Heal_Killer` BOOLEAN NOT NULL DEFAULT 1,\
`Info_XP` BOOLEAN NOT NULL DEFAULT 1,\
`Info_Level_Up` BOOLEAN NOT NULL DEFAULT 1,\
`Info_Killing_Spree` BOOLEAN NOT NULL DEFAULT 1,\
`Info_Race` BOOLEAN NOT NULL DEFAULT 1,\
`Info_Ammo` BOOLEAN NOT NULL DEFAULT 1,\
`Show_Voter` BOOLEAN NOT NULL DEFAULT 1,\
`Ammo_Absolute` BOOLEAN NOT NULL DEFAULT 1,\
`Life_Absolute` BOOLEAN NOT NULL DEFAULT 1,\
`Lock` BOOLEAN NOT NULL DEFAULT 0,\
`Race_Hammer` tinyint NOT NULL DEFAULT 0,\
`Race_Gun` tinyint NOT NULL DEFAULT 0,\
`Race_Shotgun` tinyint NOT NULL DEFAULT 0,\
`Race_Grenade` tinyint NOT NULL DEFAULT 0,\
`Race_Rifle` tinyint NOT NULL DEFAULT 0,\
PRIMARY KEY (`Id`),\
UNIQUE KEY `Name` (`Name`)\
) ENGINE=MyISAM DEFAULT CHARSET=utf8;";

            m_pStatement->execute(aBuf);

            dbg_msg("SQL", "Tables were created successfully");
        }
        catch (sql::SQLException &e)
        {
            char aBuf[256];
            str_format(aBuf, sizeof(aBuf), "MySQL Error: %s", e.what());
            dbg_msg("SQL", aBuf);
            dbg_msg("SQL", "ERROR: Tables were NOT created");
        }

        // disconnect from database
        Disconnect();
    }
    else
        std::terminate();
    
    m_Init = true;
}

// anti SQL injection

void CSqlServer::AntiInjection(const char From[], char To[], const int TailleFinal)
{
	int Pos = 0;

	for(int i = 0; i < str_length(From) && Pos + 2 < TailleFinal; i++)
	{
		if(From[i] == '\\')
		{
			To[Pos++] = '\\';
			To[Pos++] = '\\';
		}
		else if(From[i] == '\'')
		{
			To[Pos++] = '\\';
			To[Pos++] = '\'';
		}
		else if(From[i] == '"')
		{
			To[Pos++] = '\\';
			To[Pos++] = '"';
		}
		else if(From[i] == '`')
		{
			To[Pos++] = '\\';
			To[Pos++] = '`';
		}
		else
		{
			To[Pos++] = From[i];
		}
	}

	To[Pos] = '\0';
}

bool CSqlServer::Connect()
{
    try
    {
        m_pDriver = get_driver_instance();
        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "tcp://%s:%d", g_Config.m_SvSqlAddr, g_Config.m_SvSqlPort);
        m_pConnection = m_pDriver->connect(aBuf, g_Config.m_SvSqlUser, g_Config.m_SvSqlPass);
        m_pStatement = m_pConnection->createStatement();

        // Create database if not exists
        str_format(aBuf, sizeof(aBuf), "CREATE DATABASE IF NOT EXISTS %s", g_Config.m_SvSqlDb);
        m_pStatement->execute(aBuf);

        // Connect to specific database
        m_pConnection->setSchema(g_Config.m_SvSqlDb);
        dbg_msg("SQL", "SQL connection established");
        return true;
    }
    catch (sql::SQLException &e)
    {
        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "MySQL Error: %s", e.what());
        dbg_msg("SQL", aBuf);

        dbg_msg("SQL", "ERROR: SQL connection failed");
        return false;
    }
    catch (const std::exception& ex)
    {
        // ...
        dbg_msg("SQL", "1 %s",ex.what());

    }
    catch (const std::string& ex)
    {
        // ...
        dbg_msg("SQL", "2 %s",ex.c_str());
    }
    catch( int )
    {
        dbg_msg("SQL", "3 %s");
    }
    catch( float )
    {
        dbg_msg("SQL", "4 %s");
    }

    catch( char[] )
    {
        dbg_msg("SQL", "5 %s");
    }

    catch( char )
    {
        dbg_msg("SQL", "6 %s");
    }
    catch (...)
    {
        dbg_msg("SQL", "Unknown Error cause by the MySQL/C++ Connector, my advice compile server_debug and use it");

        dbg_msg("SQL", "ERROR: SQL connection failed");
        return false;
    }
    return false;
}

void CSqlServer::Disconnect()
{
    try
    {
        delete m_pStatement;
        delete m_pConnection;
        dbg_msg("SQL", "SQL connection disconnected");
    }
    catch (sql::SQLException &e)
    {
        dbg_msg("SQL", "ERROR: No SQL connection");
    }
}

int CSqlServer::CreateId(const int ClientID, const char* Name, const char* Password)
{
    if (!Connect())
        return -2;

    char sName[MAX_NAME_LENGTH*2+1];
    AntiInjection(Name, sName, MAX_NAME_LENGTH*2+1);

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "SELECT Id FROM Players_Stats WHERE Name='%s';", sName);
    m_pResult = m_pStatement->executeQuery(aBuf);

    if(m_pResult->rowsCount() > 0)
    {
        delete m_pResult;
        Disconnect();
        return -1;
    }
    
    delete m_pResult;

    char Ip[MAX_IP_LENGTH] = "";
    Server()->GetClientAddr(ClientID, Ip, MAX_IP_LENGTH);
    bool cut = true;
    for ( int i = 0; i < MAX_IP_LENGTH; i++ )
    {
        if ( Ip[i] == '[' )
            cut = false;
        else if ( Ip[i] == ':' && cut == true )
        {
            Ip[i] = '\0';
            break;
        }
        else if ( Ip[i] == ']' )
            cut = true;
    }
    Ip[MAX_IP_LENGTH - 1] = '\0';

    char sPseudo[MAX_NAME_LENGTH*2+1];
    AntiInjection(Server()->ClientName(ClientID), sPseudo, MAX_NAME_LENGTH*2+1);
    char sClan[MAX_CLAN_LENGTH*2+1];
    AntiInjection(Server()->ClientClan(ClientID), sClan, MAX_CLAN_LENGTH*2+1);

    str_format(aBuf, sizeof(aBuf), "INSERT INTO Players_Stats (Ip, Pseudo, Clan, Country, Name, Password) VALUES ('%s', '%s', '%s', %d,'%s', %u);", Ip, sPseudo, sClan, Server()->ClientCountry(ClientID), sName, str_quickhash(Password));
    m_pStatement->execute(aBuf);

    str_format(aBuf, sizeof(aBuf), "SELECT Id FROM Players_Stats WHERE Name='%s';", sName);
    m_pResult = m_pStatement->executeQuery(aBuf);
    m_pResult->next();
    int Id = m_pResult->getInt(1);
    delete m_pResult;
    Disconnect();
    return Id;
}

int CSqlServer::GetId(const char* Name, const char* Password)
{
    if (!Connect())
        return -2;

    char sName[MAX_NAME_LENGTH*2+1];
    AntiInjection(Name, sName, MAX_NAME_LENGTH*2+1);

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "SELECT Id FROM Players_Stats WHERE Name='%s' AND Password=%u;", sName, str_quickhash(Password));
    m_pResult = m_pStatement->executeQuery(aBuf);

    int Id = -1;  
    if(m_pResult->rowsCount())
    {
        m_pResult->next();
        Id = m_pResult->getInt(1);
    }

    delete m_pResult;
    Disconnect();
    return Id;
}

Player CSqlServer::GetPlayer(int id)
{
    if (id < 1)
    {
        Player e;
        e.m_id = -1;
        return e;
    }

    if (!Connect())
    {
        Player e;
        e.m_id = -2;
        return e;
    }

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "SELECT Ip, Pseudo, Clan, Country, Name, Password, UNIX_TIMESTAMP(Last_Connect) FROM Players_Stats WHERE Id=%d;", id);
    m_pResult = m_pStatement->executeQuery(aBuf);

    if(m_pResult->rowsCount() == 0)
    {
        delete m_pResult;
        Disconnect();
        Player e;
        e.m_id = -1;
        return e;
    }

    m_pResult->next();

    Player player;
    player.m_id = id;
    str_copy(player.m_ip, m_pResult->getString(1).c_str(), MAX_IP_LENGTH);
    str_copy(player.m_pseudo, m_pResult->getString(2).c_str(), MAX_NAME_LENGTH);
    str_copy(player.m_clan, m_pResult->getString(3).c_str(), MAX_CLAN_LENGTH);
    player.m_country = m_pResult->getInt(4);
    str_copy(player.m_name, m_pResult->getString(5).c_str(), MAX_NAME_LENGTH);
    player.m_password = m_pResult->getUInt(6);
    player.m_last_connect = m_pResult->getUInt(7);
    delete m_pResult;
    Disconnect();
    return player;
}

Stats CSqlServer::GetStats(int id)
{
    if (id < 1)
    {
        Stats e;
        e.m_actual_kill = -1;
        return e;
    }

    if (!Connect())
    {
        Stats e;
        e.m_actual_kill = -2;
        return e;
    }

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "SELECT Level, Score, Killed, Dead, Suicide, Rapport,\
Log_In, Fire, Pickup_Weapon, Pickup_Ninja, Change_Weapon, TIME_TO_SEC(Time_Play), Message,\
Killing_Spree, Max_Killing_Spree, Flag_Capture, Bonus_XP FROM Players_Stats WHERE Id=%d;", id);
    m_pResult = m_pStatement->executeQuery(aBuf);

    if(m_pResult->rowsCount() == 0)
    {
        delete m_pResult;
        Disconnect();
        Stats e;
        e.m_actual_kill = -1;
        return e;
    }

    m_pResult->next();

    Stats stats;
    stats.m_level = m_pResult->getUInt(1);
    stats.m_score = m_pResult->getUInt(2);
    stats.m_kill = m_pResult->getUInt(3);
    stats.m_dead = m_pResult->getUInt(4);
    stats.m_suicide = m_pResult->getUInt(5);
    stats.m_rapport = m_pResult->getDouble(6);
    stats.m_log_in = m_pResult->getUInt(7);
    stats.m_fire = m_pResult->getUInt(8);
    stats.m_pickup_weapon = m_pResult->getUInt(9);
    stats.m_pickup_ninja = m_pResult->getUInt(10);
    stats.m_change_weapon = m_pResult->getUInt(11);
    stats.m_time_play = m_pResult->getUInt(12);
    stats.m_message = m_pResult->getUInt(13);
    stats.m_killing_spree = m_pResult->getUInt(14);
    stats.m_max_killing_spree = m_pResult->getUInt(15);
    stats.m_flag_capture = m_pResult->getUInt(16);
    stats.m_bonus_xp = m_pResult->getUInt(17);

    delete m_pResult;
    Disconnect();
    return stats;
}

Upgrade CSqlServer::GetUpgrade(int id)
{
    if (id < 1)
    {
        Upgrade e;
        e.m_money = -1;
        return e;
    }

    if (!Connect())
    {
        Upgrade e;
        e.m_money = -2;
        return e;
    }

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "SELECT Upgrade_Weapon, Upgrade_Life, Upgrade_Move, Upgrade_Hook FROM Players_Stats WHERE Id=%d;", id);
    m_pResult = m_pStatement->executeQuery(aBuf);

    if(m_pResult->rowsCount() == 0)
    {
        delete m_pResult;
        Disconnect();
        Upgrade e;
        e.m_money = -1;
        return e;
    }

    m_pResult->next();

    Upgrade upgr;
    upgr.m_weapon = m_pResult->getUInt(1);
    upgr.m_life = m_pResult->getUInt(2);
    upgr.m_move = m_pResult->getUInt(3);
    upgr.m_hook = m_pResult->getUInt(4);

    delete m_pResult;
    Disconnect();
    return upgr;
}

Conf CSqlServer::GetConf(int id)
{
    if (id < 1)
    {
        Conf e;
        e.m_Weapon[0] = -1;
        return e;
    }

    if (!Connect())
    {
        Conf e;
        e.m_Weapon[0] = -2;
        return e;
    }

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "SELECT Info_Heal_Killer, Info_XP, Info_Level_Up, Info_Killing_Spree,\
Info_Race, Info_Ammo, Show_Voter, Ammo_Absolute, Life_Absolute, `Lock`,\
Race_Hammer, Race_Gun, Race_Shotgun, Race_Grenade, Race_Rifle FROM Players_Stats WHERE Id=%d;", id);
    m_pResult = m_pStatement->executeQuery(aBuf);

    if(m_pResult->rowsCount() == 0)
    {
        delete m_pResult;
        Disconnect();
        Conf e;
        e.m_Weapon[0] = -1;
        return e;
    }

    m_pResult->next();

    Conf conf;
    conf.m_InfoHealKiller = m_pResult->getBoolean(1);
    conf.m_InfoXP = m_pResult->getBoolean(2);
    conf.m_InfoLevelUp = m_pResult->getBoolean(3);
    conf.m_InfoKillingSpree = m_pResult->getBoolean(4);
    conf.m_InfoRace = m_pResult->getBoolean(5);
    conf.m_InfoAmmo = m_pResult->getBoolean(6);
    conf.m_ShowVoter = m_pResult->getBoolean(7);
    conf.m_AmmoAbsolute = m_pResult->getBoolean(8);
    conf.m_LifeAbsolute = m_pResult->getBoolean(9);
    conf.m_Lock = m_pResult->getBoolean(10);
    conf.m_Weapon[WEAPON_HAMMER] = m_pResult->getInt(11);
    conf.m_Weapon[WEAPON_GUN] = m_pResult->getInt(12);
    conf.m_Weapon[WEAPON_SHOTGUN] = m_pResult->getInt(13);
    conf.m_Weapon[WEAPON_GRENADE] = m_pResult->getInt(14);
    conf.m_Weapon[WEAPON_RIFLE] = m_pResult->getInt(15);

    delete m_pResult;
    Disconnect();
    return conf;
}

AllStats CSqlServer::GetAll(int id)
{
    return AllStats(GetPlayer(id), GetStats(id), GetUpgrade(id), GetConf(id));
}

void CSqlServer::WriteStats(int id, AllStats container)
{
    if (id < 1)
        return;

    if (!Connect())
        return;

    Player player = container.m_player;
    Stats stats = container.m_stats;
    Upgrade upgr = container.m_upgr;
    Conf conf = container.m_conf;

    char sPseudo[MAX_NAME_LENGTH*2+1];
    AntiInjection(player.m_pseudo, sPseudo, MAX_NAME_LENGTH*2+1);
    char sClan[MAX_CLAN_LENGTH*2+1];
    AntiInjection(player.m_clan, sClan, MAX_CLAN_LENGTH*2+1);

    char aBuf[900];
    str_format(aBuf, sizeof(aBuf), "UPDATE Players_Stats SET Ip='%s', Pseudo='%s', Clan='%s', Country=%d,\
Last_Connect=FROM_UNIXTIME(%u), Level=%u, Score=%u, Killed=%u, Dead=%u, Suicide=%u,\
Rapport=%lf, Log_In=%u, Fire=%u, Pickup_Weapon=%u, Pickup_Ninja=%u, Change_Weapon=%u, Time_Play=SEC_TO_TIME(%u),\
Message=%u, Killing_Spree=%u, Max_Killing_Spree=%u, Flag_Capture=%u, Bonus_XP=%u, Upgrade_Weapon=%u,\
Upgrade_Life=%u, Upgrade_Move=%u, Upgrade_Hook=%u, Info_Heal_Killer=%d, Info_XP=%d, Info_Level_Up=%d,\
Info_Killing_Spree=%d, Info_Race=%d, Info_Ammo=%d, Show_Voter=%d, Ammo_Absolute=%d, Life_Absolute=%d,\
`Lock`=%d, Race_Hammer=%d, Race_Gun=%d, Race_Shotgun=%d, Race_Grenade=%d, Race_Rifle=%d WHERE Id=%d;",
player.m_ip, player.m_pseudo, player.m_clan, player.m_country, player.m_last_connect, stats.m_level, stats.m_score,
stats.m_kill, stats.m_dead, stats.m_suicide, stats.m_rapport, stats.m_log_in, stats.m_fire, stats.m_pickup_weapon,
stats.m_pickup_ninja, stats.m_change_weapon, stats.m_time_play, stats.m_message, stats.m_killing_spree, stats.m_max_killing_spree,
stats.m_flag_capture, stats.m_bonus_xp, upgr.m_weapon, upgr.m_life, upgr.m_move, upgr.m_hook, conf.m_InfoHealKiller,
conf.m_InfoXP, conf.m_InfoLevelUp, conf.m_InfoKillingSpree, conf.m_InfoRace, conf.m_InfoAmmo, conf.m_ShowVoter,
conf.m_AmmoAbsolute, conf.m_LifeAbsolute, conf.m_Lock, conf.m_Weapon[WEAPON_HAMMER], conf.m_Weapon[WEAPON_GUN],
conf.m_Weapon[WEAPON_SHOTGUN], conf.m_Weapon[WEAPON_GRENADE], conf.m_Weapon[WEAPON_RIFLE], id);

    m_pStatement->execute(aBuf);

    Disconnect();
}

int CSqlServer::GetRank(int id)
{
    if (id < 1 || !Connect())
    {
        return -1;
    }

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "SELECT Id FROM Players_Stats WHERE Id=%d;", id);
    m_pResult = m_pStatement->executeQuery(aBuf);

    if(m_pResult->rowsCount() == 0)
    {
        delete m_pResult;
        Disconnect();
        return -1;
    }

    delete m_pResult;

    str_format(aBuf, sizeof(aBuf), "SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Score > (SELECT Score FROM Players_Stats t2 WHERE Id=%d);", id);
    m_pResult = m_pStatement->executeQuery(aBuf);
    m_pResult->next();

    int Rank = m_pResult->getUInt(1);
    delete m_pResult;
    Disconnect();
    return Rank;
}
    
const char* Suffix(unsigned long number)
{
    if ( number != 11 && (number - 1) % 10 == 0)
        return "st";
    else if ( number != 12 && (number - 2) % 10 == 0)
        return "nd";
    else if ( number != 13 && (number - 3) % 10 == 0)
        return "rd";
    else
        return "th";
}

void CSqlServer::DisplayRank(int id)
{
    if (id < 1)
    {
        GameServer()->SendChatTarget(-1, "The anonymous account hasn't got ranks !");
        return;
    }

    if (!Connect())
    {
        GameServer()->SendChatTarget(-1, "Error while connecting to the database, try again !");
        return;
    }

    char Name[MAX_NAME_LENGTH] = "";
    char aBuf[2100];
    str_format(aBuf, sizeof(aBuf), "SELECT Pseudo FROM Players_Stats WHERE Id=%d;", id);
    m_pResult = m_pStatement->executeQuery(aBuf);

    if(m_pResult->rowsCount() == 0)
    {
        GameServer()->SendChatTarget(-1, "Error while getting ranks ! Try again !");
        delete m_pResult;
        Disconnect();
        return;
    }
    
    m_pResult->next();
    str_copy(Name, m_pResult->getString(1).c_str(), MAX_NAME_LENGTH);
    delete m_pResult;

    str_format(aBuf, sizeof(aBuf), "SELECT (SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Level > (SELECT Level FROM Players_Stats t2 WHERE Id=%d)) AS Level, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Score > (SELECT Score FROM Players_Stats t2 WHERE Id=%d)) AS Score, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Killed > (SELECT Killed FROM Players_Stats t2 WHERE Id=%d)) AS Killed, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Rapport > (SELECT Rapport FROM Players_Stats t2 WHERE Id=%d)) AS Rapport, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Log_In > (SELECT Log_In FROM Players_Stats t2 WHERE Id=%d)) AS Log_In, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Fire > (SELECT Fire FROM Players_Stats t2 WHERE Id=%d)) AS Fire, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Pickup_Weapon > (SELECT Pickup_Weapon FROM Players_Stats t2 WHERE Id=%d)) AS Pickup_Weapon, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Pickup_Ninja > (SELECT Pickup_Ninja FROM Players_Stats t2 WHERE Id=%d)) AS Pickup_Ninja, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Change_Weapon > (SELECT Change_Weapon FROM Players_Stats t2 WHERE Id=%d)) AS Change_Weapon, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Time_Play > (SELECT Time_Play FROM Players_Stats t2 WHERE Id=%d)) AS Time_Play, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Message > (SELECT Message FROM Players_Stats t2 WHERE Id=%d)) AS Message, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Killing_Spree > (SELECT Killing_Spree FROM Players_Stats t2 WHERE Id=%d)) AS Killing_Spree, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Max_Killing_Spree > (SELECT Max_Killing_Spree FROM Players_Stats t2 WHERE Id=%d)) AS Max_Killing_Spree, \
(SELECT COUNT(*) + 1 AS Position FROM Players_Stats t1 WHERE t1.Flag_Capture > (SELECT Flag_Capture FROM Players_Stats t2 WHERE Id=%d)) AS Flag_Capture;",
id, id, id, id, id, id, id, id, id, id, id, id, id, id);
    m_pResult = m_pStatement->executeQuery(aBuf);
    m_pResult->next();

    char a[256] = "";
    char ranks[15][50];

    str_format(ranks[0], 50, "Name : %s", Name);
    str_format(ranks[1], 50, "Level : %ld%s.", m_pResult->getUInt(1), Suffix(m_pResult->getUInt(1)));
    str_format(ranks[2], 50, "Score : %ld%s.", m_pResult->getUInt(2), Suffix(m_pResult->getUInt(2)));

    str_format(ranks[3], 50, "Killed : %ld%s.", m_pResult->getUInt(3), Suffix(m_pResult->getUInt(3)));
    str_format(ranks[4], 50, "Rapport K/D : %ld%s.", m_pResult->getUInt(4), Suffix(m_pResult->getUInt(4)));
    str_format(ranks[5], 50, "Log-in : %ld%s.", m_pResult->getUInt(5), Suffix(m_pResult->getUInt(5)));

    str_format(ranks[6], 50, "Fire : %ld%s.", m_pResult->getUInt(6), Suffix(m_pResult->getUInt(6)));
    str_format(ranks[7], 50, "Pick-Up Weapon : %ld%s.", m_pResult->getUInt(7), Suffix(m_pResult->getUInt(7)));
    str_format(ranks[8], 50, "Pick-Up Ninja : %ld%s.", m_pResult->getUInt(8), Suffix(m_pResult->getUInt(8)));

    str_format(ranks[9], 50, "Switch Weapon : %ld%s.", m_pResult->getUInt(9), Suffix(m_pResult->getUInt(9)));
    str_format(ranks[10], 50, "Time Play : %ld%s.", m_pResult->getUInt(10), Suffix(m_pResult->getUInt(10)));
    str_format(ranks[11], 50, "Msg Sent : %ld%s.", m_pResult->getUInt(11), Suffix(m_pResult->getUInt(11)));

    str_format(ranks[12], 50, "Total Killing Spree : %ld%s.", m_pResult->getUInt(12), Suffix(m_pResult->getUInt(12)));
    str_format(ranks[13], 50, "Max Killing Spree : %ld%s.", m_pResult->getUInt(13), Suffix(m_pResult->getUInt(13)));
    str_format(ranks[14], 50, "Flag Capture : %ld%s.", m_pResult->getUInt(14), Suffix(m_pResult->getUInt(14)));

    for ( int i = 0; i < 5; i++ )
    {
        str_format(a, 256, "%s | %s | %s", ranks[i * 3], ranks[(i * 3) + 1], ranks[(i * 3) + 2]);
        GameServer()->SendChatTarget(-1, a);
    }

    delete m_pResult;
    Disconnect();
}

void CSqlServer::DisplayBestOf()
{
    if (!Connect())
        return;

    m_pResult = m_pStatement->executeQuery("SELECT MAX(Level), MAX(Score), MAX(Killed), MAX(Rapport), MAX(Log_In), \
MAX(Fire), MAX(Pickup_Weapon), MAX(Pickup_Ninja), MAX(Change_Weapon), MAX(TIME_TO_SEC(Time_Play)), MAX(Message), MAX(Killing_Spree), \
MAX(Max_Killing_Spree), MAX(Flag_Capture) FROM Players_Stats;");
    m_pResult->next();

    char a[256] = "";
    char stats[15][50];

    str_format(stats[0], 50, "Best of :");
    str_format(stats[1], 50, "Level : %u", m_pResult->getUInt(1));
    str_format(stats[2], 50, "Score : %u", m_pResult->getUInt(2));

    str_format(stats[3], 50, "Killed : %u", m_pResult->getUInt(3));
    str_format(stats[4], 50, "Rapport K/D : %f", m_pResult->getDouble(4));
    str_format(stats[5], 50, "Log-in : %u", m_pResult->getUInt(5));

    str_format(stats[6], 50, "Fire : %u", m_pResult->getUInt(6));
    str_format(stats[7], 50, "Pick-Up Weapon : %u", m_pResult->getUInt(7));
    str_format(stats[8], 50, "Pick-Up Ninja : %u", m_pResult->getUInt(8));

    str_format(stats[9], 50, "Switch Weapon : %u", m_pResult->getUInt(9));
    str_format(stats[10], 50, "Time Play : %u min", m_pResult->getUInt(10) / 60);
    str_format(stats[11], 50, "Msg Sent : %u", m_pResult->getUInt(11));

    str_format(stats[12], 50, "Total Killing Spree : %u", m_pResult->getUInt(12));
    str_format(stats[13], 50, "Max Killing Spree : %u", m_pResult->getUInt(13));
    str_format(stats[14], 50, "Flag Capture : %u", m_pResult->getUInt(14));

    str_format(a, 256, "%s %s | %s", stats[0], stats[1], stats[2]);
    GameServer()->SendChatTarget(-1, a);

    delete m_pResult;
    Disconnect();

    for ( int i = 1; i < 5; i++ )
    {
        str_format(a, 256, "%s | %s | %s", stats[i * 3], stats[(i * 3) + 1], stats[(i * 3) + 2]);
        GameServer()->SendChatTarget(-1, a);
    }
}

#endif
