/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_TURRET_H
#define GAME_SERVER_ENTITIES_TURRET_H

#include <game/server/entity.h>

class CTurret : public CEntity
{
public:
    CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner);

    virtual void Tick();
    virtual void Snap(int SnappingClient);

    int GetStartTick() { return m_StartTick; };
private:
    int m_Owner;
    int m_StartTick;
    int m_LastTick;
};

#endif
