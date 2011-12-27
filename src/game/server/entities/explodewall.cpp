/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/event.h>
#include "explodewall.h"

CExplodeWall::CExplodeWall(CGameWorld *pGameWorld, vec2 StartPos, int Owner)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_EXPLODEWALL)
{
    m_From = StartPos;
    m_Pos = m_From;
    m_Owner = Owner;
    m_Health = 2000;
    m_StartTick = 0;
    m_Destroy = false;
    GameWorld()->InsertEntity(this);
}

void CExplodeWall::Tick()
{
    if ( !m_StartTick || Server()->Tick()-m_LastTick < 100 * Server()->TickSpeed() / 1000.0f )
        return;

    if (distance(m_From, m_Pos) && GameServer()->Collision()->IntersectLine(m_From, m_Pos, 0, 0))
    {
        m_Destroy = true;
        return;
    }

    m_LastTick = Server()->Tick();

    vec2 Direction = normalize(m_Pos - m_From);
    int Num = floor(distance(m_From, m_Pos) / 67.5f) + 1;

    for (int i = 0; i < Num; i++)
    {
        GameServer()->CreateExplosion(m_From + (Direction * 67.5f * i), m_Owner, WEAPON_RIFLE, false, false);
    }
}

bool CExplodeWall::TakeDamage(int Dmg, int From, int Weapon, bool Instagib)
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
        FromRace = (rand() % ((MINER + 1) - WARRIOR)) + WARRIOR;
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
        m_Health -= Dmg * 10;

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

void CExplodeWall::Snap(int SnappingClient)
{
    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
    if(!pObj)
        return;

    pObj->m_X = (int)m_Pos.x;
    pObj->m_Y = (int)m_Pos.y;
    pObj->m_FromX = (int)m_From.x;
    pObj->m_FromY = (int)m_From.y;
    pObj->m_StartTick = Server()->Tick() + 1;
}

