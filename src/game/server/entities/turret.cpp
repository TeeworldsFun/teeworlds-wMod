/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "turret.h"
#include "plasma.h"

CTurret::CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
    m_Pos = Pos;
    m_Owner = Owner;
    m_StartTick = Server()->Tick();
    m_LastTick = m_StartTick;
    GameWorld()->InsertEntity(this);
}

void CTurret::Tick()
{
    if ((Server()->Tick()-m_LastTick) < 150 * Server()->TickSpeed() / 1000.f)
        return;

    m_LastTick = Server()->Tick();

    CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
    CCharacter *TargetChr = 0;

    CCharacter *apEnts[MAX_CLIENTS] = {0};
    int Num = GameServer()->m_World.FindEntities(m_Pos, 450.0f, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
    for(int i = 0; i < Num; i++)
    {
        if ( OwnerChar != apEnts[i] && (!GameServer()->m_pController->IsTeamplay() || OwnerChar->GetPlayer()->GetTeam() != apEnts[i]->GetPlayer()->GetTeam())
            && !GameServer()->Collision()->IntersectLine(m_Pos, apEnts[i]->m_Pos, 0, 0) )
        {
            TargetChr = apEnts[i];
            break;
        }
    }
    
    if(!TargetChr)
        return;

    vec2 Direction = normalize(TargetChr->m_Pos - m_Pos);
    new CPlasma(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_Owner);
}

void CTurret::Snap(int SnappingClient)
{
    if(NetworkClipped(SnappingClient))
        return;

    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
    if(!pObj)
        return;

    pObj->m_X = (int)m_Pos.x;
    pObj->m_Y = (int)m_Pos.y;
    pObj->m_FromX = (int)m_Pos.x;
    pObj->m_FromY = (int)m_Pos.y;
    pObj->m_StartTick = Server()->Tick();
}
