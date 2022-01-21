#ifndef GAME_SERVER_ENTITIES_LOOT_H
#define GAME_SERVER_ENTITIES_LOOT_H
#include "pickup.h"

class CLoot : public CPickup
{
public:
	CLoot(CGameWorld *pGameWorld, int Type, int SubType = 0);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

	vec2 m_Vel;
	int m_StartTick;
};

#endif
