/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/
#ifndef GAME_SERVER_ENTITY_DAMAGEABLE_H
#define GAME_SERVER_ENTITY_DAMAGEABLE_H

#include <new>
#include <game/server/entity.h>

const int MAX_ENTITIES_DAMAGEABLE = MAX_CLIENTS + MAX_MONSTERS + MAX_CLIENTS*5 + MAX_CLIENTS*3;

class IEntityDamageable : public CEntity
{
public:
	IEntityDamageable(CGameWorld *pGameWorld, int Objtype) : CEntity(pGameWorld, Objtype) {}
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon, bool Instagib, bool FromMonster = false) = 0;
};

#endif
