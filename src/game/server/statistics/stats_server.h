#ifndef GAME_SERVER_STATS_SERVER_H
#define GAME_SERVER_STATS_SERVER_H

#include <game/server/gamecontext.h>
#include "statistiques.h"

class IStatsServer
{
public:
    IStatsServer(CGameContext *pGameServer)
    {
        m_pGameServer = pGameServer;
        m_Init = false;
    };

    virtual void OnInit() = 0;
    virtual int CreateId(const int ClientID, const char* Name, const char* Password) = 0;
    virtual int GetId(const char* Name, const char* Password) = 0;
    virtual Player GetPlayer(int id) = 0;
    virtual Stats GetStats(int id) = 0;
    virtual Upgrade GetUpgrade(int id) = 0;
    virtual Conf GetConf(int id) = 0;
    virtual AllStats GetAll(int id) = 0;
    virtual void WriteStats(int id, AllStats container) = 0;

    virtual void DisplayRank(int id) = 0;
    virtual void DisplayBestOf() = 0;
protected:
    bool m_Init;
    CGameContext *GameServer() const { return m_pGameServer; };
    IServer *Server() const { return m_pGameServer->Server(); };
    IGameController *Controller() const { return m_pGameServer->m_pController; };
private:
    CGameContext* m_pGameServer;
};

#endif
