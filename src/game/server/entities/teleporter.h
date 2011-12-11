/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_TELEPORTER_H
#define GAME_SERVER_ENTITIES_TELEPORTER_H

#include <game/server/entity.h>

class CTeleporter : public CEntity
{
public:
    CTeleporter(CGameWorld *pGameWorld, vec2 Pos, int Owner, CTeleporter *Next = 0);

    virtual void Tick();
    virtual void Snap(int SnappingClient);

    void SetNext(CTeleporter *Next) { m_Next = Next; };
    int GetStartTick() { return m_StartTick; };
    void ResetStartTick() { m_StartTick = Server()->Tick(); };
private:
    int m_Owner;
    int m_StartTick;
    CTeleporter *m_Next;
};

#endif
