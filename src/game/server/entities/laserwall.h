/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#ifndef GAME_SERVER_ENTITIES_LASERWALL_H
#define GAME_SERVER_ENTITIES_LASERWALL_H

#include <game/server/entity.h>

class CLaserWall : public CEntity
{
public:
	CLaserWall(CGameWorld *pGameWorld, vec2 StartPos, int Owner, bool Double);
	~CLaserWall();

	virtual void Tick();
	virtual void Snap(int SnappingClient);
	virtual void CreateDouble();

	int m_StartTick;
	bool m_Destroy;
private:
	vec2 m_From;
	int m_Owner;
	int m_Killed;
	bool m_Double;
	CLaserWall *m_pDouble;
};

#endif
