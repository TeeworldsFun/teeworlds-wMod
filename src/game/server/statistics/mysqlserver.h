#ifndef GAME_SERVER_MYSQLSERVER_H
#define GAME_SERVER_MYSQLSERVER_H

#include <game/server/gamecontext.h>
#include "stats_server.h"

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/resultset.h>

class CSqlServer : public IStatsServer
{
public:
    CSqlServer(CGameContext *pGameServer);
    void OnInit();
    void AntiInjection(const char From[], char To[], const int TailleFinal);
    int CreateId(const int ClientID, const char* Name, const char* Password);
    int GetId(const char* Name, const char* Password);
    Player GetPlayer(int id);
    Stats GetStats(int id);
    Upgrade GetUpgrade(int id);
    Conf GetConf(int id);
    AllStats GetAll(int id);
    void WriteStats(int id, AllStats container);

    int GetRank(int id);
    void DisplayRank(int id);
    void DisplayBestOf();
private:
    bool Connect();
    void Disconnect();
    sql::Driver *m_pDriver;
    sql::Connection *m_pConnection;
    sql::Statement *m_pStatement;
    sql::ResultSet *m_pResult;
};

#endif
