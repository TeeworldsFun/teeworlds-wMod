/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUP_H
#define GAME_SERVER_ENTITIES_PICKUP_H

#include <game/server/entity.h>

const int PickupPhysSize = 14;

class CAura : public CEntity
{
public:
	CAura(CGameWorld *pGameWorld, int Owner, float StartDegres, int Distance, int Type);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
	int m_Type;
	float m_Degres;
	int m_Distance;
};

#endif
