#if defined(CONF_SQL)

#include "mysqlserver.h"

#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include <cppconn/statement.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>

#include <vector>

/*#include <driver.h>
#include <connection.h>

#include <prepared_statement.h>

#include <metadata.h>

#include <warning.h>
    
#define NUMOFFSET 100
#define COLNAME 200*/

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
`Killed` mediumint unsigned NOT NULL DEFAULT 0,\
`Dead` mediumint unsigned NOT NULL DEFAULT 0,\
`Suicide` mediumint unsigned NOT NULL DEFAULT 0,\
`Score` float unsigned NOT NULL DEFAULT 0,\
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

            // delete statement
            delete m_pStatement;
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
    ResultSet *pResults(m_pStatement->executeQuery(aBuf));

    if(pResults->rowsCount() > 0)
    {
        delete pResults;
        Disconnect();
        return -1;
    }
    
    delete pResults;

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
    pResults = m_pStatement->executeQuery(aBuf);
    pResults->next();
    int Id = pResults->getInt(1);
    delete pResults;
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
    ResultSet *pResults(m_pStatement->executeQuery(aBuf));

    int Id = -1;  
    if(pResults->rowsCount())
    {
        pResults->next();
        Id = pResults->getInt(1);
    }

    delete pResults;
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
    str_format(aBuf, sizeof(aBuf), "SELECT Ip, Pseudo, Clan, Country, Name, Password, UNIX_TIMESTAMP(Last_Connect) FROM Players_Stats WHERE Id=%ld;", id);
    ResultSet *pResults(m_pStatement->executeQuery(aBuf));

    if(pResults->rowsCount() == 0)
    {
        delete pResults;
        Disconnect();
        Player e;
        e.m_id = -1;
        return e;
    }

    pResults->next();

    Player player;
    player.m_id = id;
    str_copy(player.m_ip, pResults->getString(1).c_str(), MAX_IP_LENGTH);
    str_copy(player.m_pseudo, pResults->getString(2).c_str(), MAX_NAME_LENGTH);
    str_copy(player.m_clan, pResults->getString(3).c_str(), MAX_CLAN_LENGTH);
    player.m_country = pResults->getInt(4);
    str_copy(player.m_name, pResults->getString(5).c_str(), MAX_NAME_LENGTH);
    player.m_password = pResults->getUInt(6);
    player.m_last_connect = pResults->getUInt(7);
    delete pResults;
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
Killing_Spree, Max_Killing_Spree, Flag_Capture, Bonus_XP FROM Players_Stats WHERE Id=%ld;", id);
    ResultSet *pResults(m_pStatement->executeQuery(aBuf));

    if(pResults->rowsCount() == 0)
    {
        delete pResults;
        Disconnect();
        Stats e;
        e.m_actual_kill = -1;
        return e;
    }

    pResults->next();

    Stats stats;
    stats.m_level = pResults->getUInt(1);
    stats.m_score = pResults->getUInt(2);
    stats.m_kill = pResults->getUInt(3);
    stats.m_dead = pResults->getUInt(4);
    stats.m_suicide = pResults->getUInt(5);
    stats.m_rapport = pResults->getDouble(6);
    stats.m_log_in = pResults->getUInt(7);
    stats.m_fire = pResults->getUInt(8);
    stats.m_pickup_weapon = pResults->getUInt(9);
    stats.m_pickup_ninja = pResults->getUInt(10);
    stats.m_change_weapon = pResults->getUInt(11);
    stats.m_time_play = pResults->getUInt(12);
    stats.m_message = pResults->getUInt(13);
    stats.m_killing_spree = pResults->getUInt(14);
    stats.m_max_killing_spree = pResults->getUInt(15);
    stats.m_flag_capture = pResults->getUInt(16);
    stats.m_bonus_xp = pResults->getUInt(17);

    delete pResults;
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
    str_format(aBuf, sizeof(aBuf), "SELECT Upgrade_Weapon, Upgrade_Life, Upgrade_Move, Upgrade_Hook FROM Players_Stats WHERE Id=%ld;", id);
    ResultSet *pResults(m_pStatement->executeQuery(aBuf));

    if(pResults->rowsCount() == 0)
    {
        delete pResults;
        Disconnect();
        Upgrade e;
        e.m_money = -1;
        return e;
    }

    pResults->next();

    Upgrade upgr;
    upgr.m_weapon = pResults->getUInt(1);
    upgr.m_life = pResults->getUInt(2);
    upgr.m_move = pResults->getUInt(3);
    upgr.m_hook = pResults->getUInt(4);

    delete pResults;
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
Race_Hammer, Race_Gun, Race_Shotgun, Race_Grenade, Race_Rifle FROM Players_Stats WHERE Id=%ld;", id);
    ResultSet *pResults(m_pStatement->executeQuery(aBuf));

    if(pResults->rowsCount() == 0)
    {
        delete pResults;
        Disconnect();
        Conf e;
        e.m_Weapon[0] = -1;
        return e;
    }

    pResults->next();

    Conf conf;
    conf.m_InfoHealKiller = pResults->getBoolean(1);
    conf.m_InfoXP = pResults->getBoolean(2);
    conf.m_InfoLevelUp = pResults->getBoolean(3);
    conf.m_InfoKillingSpree = pResults->getBoolean(4);
    conf.m_InfoRace = pResults->getBoolean(5);
    conf.m_InfoAmmo = pResults->getBoolean(6);
    conf.m_ShowVoter = pResults->getBoolean(7);
    conf.m_AmmoAbsolute = pResults->getBoolean(8);
    conf.m_LifeAbsolute = pResults->getBoolean(9);
    conf.m_Lock = pResults->getBoolean(10);
    conf.m_Weapon[WEAPON_HAMMER] = pResults->getInt(11);
    conf.m_Weapon[WEAPON_GUN] = pResults->getInt(12);
    conf.m_Weapon[WEAPON_SHOTGUN] = pResults->getInt(13);
    conf.m_Weapon[WEAPON_GRENADE] = pResults->getInt(14);
    conf.m_Weapon[WEAPON_RIFLE] = pResults->getInt(15);

    delete pResults;
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

    dbg_msg("SQL", aBuf);
    m_pStatement->execute(aBuf);

    Disconnect();
}

void CSqlServer::DisplayRank(int id)
{

}

void CSqlServer::DisplayBestOf()
{

}

#endif
