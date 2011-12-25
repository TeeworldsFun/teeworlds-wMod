/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "teleporter.h"
#include "plasma.h"

CTeleporter::CTeleporter(CGameWorld *pGameWorld, vec2 Pos, int Owner, CTeleporter *Next)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_TELEPORTER)
{
    m_Pos = Pos;
    m_Owner = Owner;
    m_StartTick = Server()->Tick();
    m_Next = Next;

    GameWorld()->InsertEntity(this);
}

void CTeleporter::Tick()
{
    if (!m_Next)
        return;

    CCharacter *apEnts[MAX_CLIENTS] = {0};
    int Num = GameServer()->m_World.FindEntities(m_Pos, 6.0f, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
    for(int i = 0; i < Num; i++)
    {
        if(!apEnts[i])
            continue;

        vec2 alea(0,0);
        vec2 To(0,0);
        do
        {
            alea.x = (rand() % 201) - 100;
            alea.y = (rand() % 201) - 100;
            To.x = m_Next->m_Pos.x + alea.x;
            To.y = m_Next->m_Pos.y + alea.y;
        }
        while (GameServer()->Collision()->TestBox(To, vec2(CCharacter::ms_PhysSize, CCharacter::ms_PhysSize)) || ((alea.x > 0 && alea.x < CCharacter::ms_PhysSize+6.0f) || (alea.x < 0 && alea.x > -(CCharacter::ms_PhysSize+6.0f))) || GameServer()->Collision()->IntersectLine(m_Next->m_Pos, To, 0, 0));

        apEnts[i]->SetPos(To);
    }
}

void CTeleporter::Snap(int SnappingClient)
{
    if(NetworkClipped(SnappingClient))
        return;

    CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
    if(!pProj)
        return;

    pProj->m_X = (int)m_Pos.x;
    pProj->m_Y = (int)m_Pos.y;
    pProj->m_VelX = (int)(0);
    pProj->m_VelY = (int)(0);
    pProj->m_StartTick = Server()->Tick();
    pProj->m_Type = WEAPON_GRENADE;
}
