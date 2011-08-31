#ifndef GAME_SERVER_BOT_H
#define GAME_SERVER_BOT_H

#include <game/server/player.h>

class CBot : public CPlayer
{
public:
	CBot(CGameContext *pGameServer, int ClientID, int Team);
private:
};

#endif

