/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#ifndef GAME_SERVER_ENTITIES_EXPLODEWALL_H
#define GAME_SERVER_ENTITIES_EXPLODEWALL_H

#include <game/server/entity_damageable.h>

class CExplodeWall : public IEntityDamageable
{
public:
	CExplodeWall(CGameWorld *pGameWorld, vec2 StartPos, int Owner);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	int GetOwner() { return m_Owner; };
	bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon, bool Instagib, bool FromMonster = false);
	int m_StartTick;
	vec2 m_From;
	bool m_Destroy;
private:
	int m_Owner;
	int m_LastTick;
	int m_Health;
	int m_DamageTaken;
	int m_DamageTakenTick;
	CExplodeWall *m_pDouble;
};

#endif
