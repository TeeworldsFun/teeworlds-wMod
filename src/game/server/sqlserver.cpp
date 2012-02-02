#include "sqlserver.h"

#include <game/server/gamecontext.h>

#include <driver.h>
#include <connection.h>
/*#include <statement.h>
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
}
