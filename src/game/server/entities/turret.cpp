/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/event.h>
#include "turret.h"
#include "plasma.h"

CTurret::CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_TURRET)
{
    m_Pos = Pos;
    m_Owner = Owner;
    m_StartTick = Server()->Tick();
    m_LastTick = m_StartTick;
    m_Health = 100;
    m_DamageTaken = 0;
    m_DamageTakenTick = 0;
    m_Destroy = false;
    m_ProximityRadius = 6.0f;
    GameWorld()->InsertEntity(this);
}

void CTurret::Tick()
{
    if ((Server()->Tick()-m_LastTick) < 150 * Server()->TickSpeed() / 1000.f)
        return;

    m_LastTick = Server()->Tick();

    CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
    CCharacter *TargetChr = 0;
    CTurret *TargetTurret = 0;

    CCharacter *apEnts[MAX_CLIENTS] = {0};
    CTurret *apEntsTurret[MAX_CLIENTS] = {0};
    int Num = GameServer()->m_World.FindEntities(m_Pos, 450.0f, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
    for(int i = 0; i < Num; i++)
    {
        if ( OwnerChar != apEnts[i] && (!GameServer()->m_pController->IsTeamplay() || OwnerChar->GetPlayer()->GetTeam() != apEnts[i]->GetPlayer()->GetTeam() || GameServer()->m_pEventsGame->GetActualEventTeam() == RIFLE_HEAL)
            && !GameServer()->Collision()->IntersectLine(m_Pos, apEnts[i]->m_Pos, 0, 0) )
        {
            TargetChr = apEnts[i];
            break;
        }
    }

    if (!TargetChr)
    {
        Num = GameServer()->m_World.FindEntities(m_Pos, 450.0f, (CEntity**)apEntsTurret, MAX_CLIENTS * 5, CGameWorld::ENTTYPE_TURRET);
        for(int i = 0; i < Num; i++)
        {
            if ( m_Owner != apEntsTurret[i]->GetOwner() && (!GameServer()->m_pController->IsTeamplay() || OwnerChar->GetPlayer()->GetTeam() != GameServer()->m_apPlayers[apEntsTurret[i]->GetOwner()]->GetTeam())
                && !GameServer()->Collision()->IntersectLine(m_Pos, apEntsTurret[i]->m_Pos, 0, 0) )
            {
                TargetTurret = apEntsTurret[i];
                break;
            }
        }
    }

    vec2 Direction(0,0);
    if (TargetChr)
        Direction = normalize(TargetChr->m_Pos - m_Pos);
    else if (TargetTurret)
        Direction = normalize(TargetTurret->m_Pos - m_Pos);
    else
        return;

    new CPlasma(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_Owner);
}

bool CTurret::TakeDamage(int Dmg, int From, int Weapon, bool Instagib)
{
    int FromRace = 0;
    if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_WARRIOR) )
        FromRace = WARRIOR;
    else if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_ENGINEER) )
        FromRace = ENGINEER;
    else if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_ORC) )
        FromRace = ORC;
    else if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_MINER) )
        FromRace = MINER;
    else if ( GameServer()->m_pEventsGame->IsActualEvent(RACE_RANDOM) )
        FromRace = (rand() % (RACE_MINER - RACE_WARRIOR)) + RACE_WARRIOR;
    else if ( GameServer()->m_apPlayers[From] )
        FromRace = GameServer()->m_apPlayers[From]->m_WeaponType[Weapon];

    if (GameServer()->m_pEventsGame->IsActualEvent(INSTAGIB) || (FromRace == ORC && Weapon == WEAPON_RIFLE))
        Instagib = true;

    if(GameServer()->m_pController->IsFriendlyFire(m_Owner, From, Weapon) && !g_Config.m_SvTeamdamage)
        return false;

    if(From == m_Owner)
        return false;

    if ( GameServer()->m_pEventsGame->IsActualEvent(PROTECT_X2) )
        Dmg = max(1, Dmg/2);

    if (!Instagib)
        m_Health -= Dmg;
    else
        m_Health = 0;

    m_DamageTaken++;

    // create healthmod indicator
    if(Server()->Tick() < m_DamageTakenTick+25)
    {
        // make sure that the damage indicators doesn't group together
        GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
    }
    else
    {
        m_DamageTaken = 0;
        GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
    }

    m_DamageTakenTick = Server()->Tick();

    // do damage Hit sound
    if(From >= 0 && From != m_Owner && GameServer()->m_apPlayers[From])
    {
        int Mask = CmaskOne(From);
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
                Mask |= CmaskOne(i);
        }
        GameServer()->CreateSound(m_Pos, SOUND_HIT, Mask);
    }

    // check for death
    if(m_Health <= 0)
    {
        m_Destroy = true;

        // set attacker's face to happy (taunt!)
        if (From >= 0 && From != m_Owner && GameServer()->m_apPlayers[From])
        {
            CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
            if (pChr)
            {
                pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
            }
        }

        return true;
    }

    if (Dmg > 2)
        GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
    else
        GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);
    
    return true;
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
