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
`Pseudo_Hash` BIGINT unsigned DEFAULT NULL COMMENT 'Old System',\
`Clan` varchar(12) DEFAULT NULL,\
`Clan_Hash` BIGINT unsigned DEFAULT NULL COMMENT 'Old System',\
`Country` smallint NOT NULL DEFAULT -1,\
`Name` varchar(16) DEFAULT NULL,\
`Password` BIGINT unsigned DEFAULT NULL,\
`Last_Connect` datetime NOT NULL,\
`Killed` mediumint unsigned NOT NULL DEFAULT 0,\
`Dead` mediumint unsigned NOT NULL DEFAULT 0,\
`Suicide` mediumint unsigned NOT NULL DEFAULT 0,\
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
UNIQUE KEY `Name` (`Name`),\
KEY `Pseudo_Hash` (`Pseudo_Hash`)\
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
	
    m_Init = true;
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

int CSqlServer::GetId(const char* Name, const char* Password)
{
    if (!Connect())
        return -1;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "SELECT Id FROM Players_Stats WHERE Name=\"%s\" AND Password=%ld;", Name, str_quickhash(Password));
    ResultSet *pResults(m_pStatement->executeQuery(aBuf));
        
    if(pResults->rowsCount() == 0)
    {
        Disconnect();
        return -1;
    }
    
    int Id = pResults->getInt(0);
    delete pResults;
    Disconnect();
    return Id;
}

int CSqlServer::GetId(const char* Ip, const char* Pseudo, const char* Clan, const int Country)
{
    if (!Connect())
        return -1;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "SELECT * FROM Players_Stats WHERE Ip=\"%s\";", Ip);
    ResultSet *pResults(m_pStatement->executeQuery(aBuf));
        
    if(pResults->rowsCount() == 1)
    {
        int Id = pResults->getInt(0);
        delete pResults;
        Disconnect();
        return Id;
    }
    else if (pResults->rowsCount() > 1)
    {
        std::vector<int> Id;

        while (pResults->next())
        {
            if(static_cast<unsigned int>(pResults->getInt("Pseudo_Hash")) == str_quickhash(Pseudo) &&
               static_cast<unsigned int>(pResults->getInt("Clan_Hash")) == str_quickhash(Clan) &&
               pResults->getInt("Country") == Country)
            {
                Id.push_back(pResults->getInt(0));
            }
        }
        
        if (Id.size() == 1)
        {
            delete pResults;
            Disconnect();
            return Id.at(0);
        }
    }
    else
    {
    }

    delete pResults;
    Disconnect();
    return -1;
}

Player CSqlServer::GetPlayer(int id)
{

}

Stats CSqlServer::GetStats(int id)
{

}

Upgrade CSqlServer::GetUpgrade(int id)
{

}

Conf CSqlServer::GetConf(int id)
{

}

AllStats CSqlServer::GetAll(int id)
{

}

void CSqlServer::WriteStats(int id, AllStats container)
{

}

    
#endif
