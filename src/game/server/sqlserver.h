#ifndef GAME_SERVER_SQLSERVER_H
#define GAME_SERVER_SQLSERVER_H

#include <game/server/gamecontext.h>

#include <mysql_connection.h>

#include <cppconn/driver.h>
/*#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>*/

class CSqlServer
{
public:
    CSqlServer(CGameContext *pGameServer);
private:
    sql::Driver *m_pDriver;
    sql::Connection *m_pConnection;
    sql::Statement *m_pStatement;

    CGameContext *GameServer() const { return m_pGameServer; };
    IServer *Server() const { return m_pGameServer->Server(); };
    IGameController *Controller() const { return m_pGameServer->m_pController; };
    
    CGameContext* m_pGameServer;
};

#endif
