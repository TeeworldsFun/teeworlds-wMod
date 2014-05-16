/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#ifndef GAME_SERVER_ENTITIES_TURRET_H
#define GAME_SERVER_ENTITIES_TURRET_H

#include <game/server/entity.h>

class CTurret : public CEntity
{
public:
	CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	int GetOwner() { return m_Owner; };
	int GetStartTick() { return m_StartTick; };
	void ResetStartTick() { m_StartTick = Server()->Tick(); m_LastTick = m_StartTick; };
	bool TakeDamage(int Dmg, int From, int Weapon, bool Instagib);

	bool m_Destroy;
private:
	int m_Owner;
	int m_StartTick;
	int m_LastTick;
	int m_Health;
	int m_DamageTaken;
	int m_DamageTakenTick;
};

#endif
