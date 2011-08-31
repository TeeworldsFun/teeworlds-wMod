/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUP_H
#define GAME_SERVER_ENTITIES_PICKUP_H

#include <game/server/entity.h>

const int PickupPhysSize = 14;

class CProtect : public CEntity
{
public:
	CProtect(CGameWorld *pGameWorld, CCharacter *Character, float StartDegres, int Type);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	CCharacter *m_pCharacter;
	int m_Type;
	float m_degres;
};

#endif
