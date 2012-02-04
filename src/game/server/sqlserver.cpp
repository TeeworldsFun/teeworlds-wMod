#include "sqlserver.h"

#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

/*#include <driver.h>
#include <connection.h>
#include <statement.h>
#include <prepared_statement.h>
#include <resultset.h>
#include <metadata.h>
#include <resultset_metadata.h>
#include <exception.h>
#include <warning.h>*/
	
#define NUMOFFSET 100
#define COLNAME 200

using namespace sql;

CSqlServer::CSqlServer(CGameContext *pGameServer)
{
    pGameServer = m_pGameServer;
    
    char aBuf[512] = "";
    str_format(aBuf, 512, "tcp://%s:%d/%d", g_Config.m_SvSqlAddr, g_Config.m_SvSqlPort, g_Config.m_SvSqlDb);

    m_pDriver = get_driver_instance();
    m_pConnection = m_pDriver->connect(aBuf, g_Config.m_SvSqlUser, g_Config.m_SvSqlPass);
    m_pStatement = m_pConnection->createStatement();
}
