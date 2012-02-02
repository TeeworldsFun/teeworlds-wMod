#ifndef GAME_SERVER_SQLSERVER_H
#define GAME_SERVER_SQLSERVER_H

#include <game/server/gamecontext.h>

class CSqlServer
{
public:
    CSqlServer(CGameContext *pGameServer);
private:
    class Driver *m_driver;
    class Connection *m_connection;
 
    CGameContext *GameServer() const { return m_pGameServer; };
    IServer *Server() const { return m_pGameServer->Server(); };
    IGameController *Controller() const { return m_pGameServer->m_pController; };
    
    CGameContext* m_pGameServer;
};

#endif
