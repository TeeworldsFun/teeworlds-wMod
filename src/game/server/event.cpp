#include <new>
#include <engine/shared/config.h>
#include "event.h"

CEvent::CEvent(CGameContext *GameServer)
{
    m_pGameServer = GameServer;
    m_StartEvent[0] = 0;
    m_StartEvent[1] = 0;
    m_StartEventTeam = 0;
    m_StartEventRound = 0;
    m_ActualEvent[0] = -1;
    m_ActualEvent[1] = -1;
    m_ActualEventTeam = -1;
    m_LastSend = 0;
}

CEvent::~CEvent()
{

}

void CEvent::Tick()
{
    if ( g_Config.m_SvTwoEvent && Controller()->IsTeamplay() )
        g_Config.m_SvTwoEvent = false;

    int Elapsed[2] = {0, 0};
    char Text[256] = "";

    for ( int i = 0; i < 2; i++ )
    {
        if ( i == 1 && !g_Config.m_SvTwoEvent )
            break;

        Elapsed[i] = Server()->Tick() - m_StartEvent[i];

        if ( Elapsed[i] >= 150 * Server()->TickSpeed() || m_ActualEvent[i] == -1 )
        {
            int NewEvent = 0;
            if ( m_ActualEvent[0] == SURVIVOR )
            {
                GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
                Controller()->EndRound();
            }

            while (!CanBeUsed(NewEvent = (rand() % ((END - 1) - NOTHING + 1)) + NOTHING, i));

            m_ActualEvent[i] = NewEvent;
            m_StartEvent[i] = Server()->Tick();
            SetTune();
            m_LastSend = Server()->Tick() - Server()->TickSpeed()*2;
        }

        char aBuf[256] = "";
        char Prefix[10] = "";

        if ( i == 0 )
            str_copy(Prefix, "Event", 10);
        else
            str_copy(Prefix, "\nEvent 2", 10);

        switch(m_ActualEvent[i])
        {
        case NOTHING:
            str_format(aBuf, 256, "%s : Nothing. | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case HAMMER:
            str_format(aBuf, 256, "%s : All Get Hammer ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case GUN:
            str_format(aBuf, 256, "%s : All Get Gun ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case SHOTGUN:
            str_format(aBuf, 256, "%s : All Get Shotgun ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case GRENADE:
            str_format(aBuf, 256, "%s : All Get Grenade ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case RIFLE:
            str_format(aBuf, 256, "%s : All Get Rifle ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case KATANA:
            str_format(aBuf, 256, "%s : All Get Katana ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case UNLIMITED_AMMO:
            str_format(aBuf, 256, "%s : All Get Unlimited Ammo ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case LIFE_ARMOR_CRAZY:
            str_format(aBuf, 256, "%s : All Get Life and Armor Crazy ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case SURVIVOR:
            str_format(aBuf, 256, "%s : Mode Survivor ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case PROTECT_X2:
            str_format(aBuf, 256, "%s : All Get Protect X2 ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case INSTAGIB:
            str_format(aBuf, 256, "%s : Mode Instagib ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case WALLSHOT:
            str_format(aBuf, 256, "%s : Mode WallShot ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case BULLET_PIERCING:
            str_format(aBuf, 256, "%s : All Bullets can pierce Tee and Tiles ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case BULLET_BOUNCE:
            str_format(aBuf, 256, "%s : All Bullets can bounce ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case BULLET_GLUE:
            str_format(aBuf, 256, "%s : All Bullets are glue ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case HAVE_ALL_WEAPON:
            str_format(aBuf, 256, "%s : All get all weapons when respawn ! | Remaining %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case GRAVITY_0_25:
            str_format(aBuf, 256, "%s : Gravity modified to 0.25 ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case GRAVITY_0:
            str_format(aBuf, 256, "%s : Gravity modified to 0 ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case GRAVITY_M0_25:
            str_format(aBuf, 256, "%s : Gravity modified to -0.25 ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case GRAVITY_M0_5:
            str_format(aBuf, 256, "%s : Gravity modified to -0.5 ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case BOUNCE_10:
            str_format(aBuf, 256, "%s : Laser Bounce Num modified to 10 ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case HOOK_VERY_LONG:
            str_format(aBuf, 256, "%s : Hook Length is now very long ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case SPEED_X10:
            str_format(aBuf, 256, "%s : All Get Speed X10 ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case WEAPON_SLOW:
            str_format(aBuf, 256, "%s : All bullets are slow ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case RACE_WARRIOR:
            str_format(aBuf, 256, "%s : All be race Warrior ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case RACE_ENGINEER:
            str_format(aBuf, 256, "%s : All be race Engineer ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case RACE_ORC:
            str_format(aBuf, 256, "%s : All be race Orc ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case RACE_MINER:
            str_format(aBuf, 256, "%s : All be race Miner ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case RACE_RANDOM:
            str_format(aBuf, 256, "%s : All have race Random ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case JUMP_UNLIMITED:
            str_format(aBuf, 256, "%s : All can always jump ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case JUMP_X1_5:
            str_format(aBuf, 256, "%s : All can jump higher ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        case ALL:
            str_format(aBuf, 256, "%s : All events are active ! | Remaining : %02d:%02d", Prefix, (150 - Elapsed[i]/Server()->TickSpeed()) / 60, (150 - Elapsed[i]/Server()->TickSpeed()) % 60);
            break;
        }

        str_append(Text, aBuf, 256);
    }

    if ( Controller()->IsTeamplay() )
    {
        int ElapsedTeam = Server()->Tick() - m_StartEventTeam;
        if ( ElapsedTeam >= 300 * Server()->TickSpeed() || m_ActualEventTeam == -1 )
        {
            if ( m_ActualEventTeam >= T_SURVIVOR )
            {
                GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
                Controller()->EndRound();
            }
            int NewEvent;
            while (!CanBeUsed(NewEvent = (rand() % ((T_END - 1) - T_NOTHING + 1)) + T_NOTHING, 3));

            if ( m_ActualEventTeam >= STEAL_TEE )
                Controller()->EndRound();

            m_ActualEventTeam = NewEvent;
            m_StartEventTeam = Server()->Tick();
            if ( NewEvent == TEE_VS_ZOMBIE )
                Controller()->EndRound();

            m_LastSend = Server()->Tick() - Server()->TickSpeed()*2;
        }
        char Temp[256] = "";
        switch (m_ActualEventTeam)
        {
        case T_NOTHING:
            str_format(Temp, 256, "\nEvent Team : Nothing. | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case HAMMER_HEAL:
            str_format(Temp, 256, "\nEvent Team : Hammer Heal Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case GUN_HEAL:
            str_format(Temp, 256, "\nEvent Team : Gun Heal Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case SHOTGUN_HEAL:
            str_format(Temp, 256, "\nEvent Team : Shotgun Heal Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case GRENADE_HEAL:
            str_format(Temp, 256, "\nEvent Team : Grenade Heal Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case RIFLE_HEAL:
            str_format(Temp, 256, "\nEvent Team : Rifle Heal Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case KATANA_HEAL:
            str_format(Temp, 256, "\nEvent Team : Katana Heal Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case CAN_HEAL:
            str_format(Temp, 256, "\nEvent Team : Can heal teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case HAMMER_KILL:
            str_format(Temp, 256, "\nEvent Team : Hammer Kill Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case GUN_KILL:
            str_format(Temp, 256, "\nEvent Team : Gun Kill Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case SHOTGUN_KILL:
            str_format(Temp, 256, "\nEvent Team : Shotgun Kill Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case GRENADE_KILL:
            str_format(Temp, 256, "\nEvent Team : Grenade Kill Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case RIFLE_KILL:
            str_format(Temp, 256, "\nEvent Team : Rifle Kill Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case KATANA_KILL:
            str_format(Temp, 256, "\nEvent Team : Katana Kill Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case CAN_KILL:
            str_format(Temp, 256, "\nEvent Team : Can kill teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case ANONYMOUS:
            str_format(Temp, 256, "\nEvent Team : All are anonymous ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case T_SURVIVOR:
            str_format(Temp, 256, "\nEvent Team : Survivor ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case STEAL_TEE:
            str_format(Temp, 256, "\nEvent Team : Steal Tee ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        case TEE_VS_ZOMBIE:
            str_format(Temp, 256, "\nEvent Team : Tee vs Zombies ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
            break;
        }

        str_append(Text, Temp, 256);
    }

    if (Server()->Tick() >= m_LastSend+Server()->TickSpeed()*1)
    {
        for ( int i = 0; i < MAX_CLIENTS; i++ )
        {
            if ( GameServer()->m_apPlayers[i] )
            {
                if ( GameServer()->m_apPlayers[i]->m_BroadcastTick == 0 )
                    GameServer()->SendBroadcast(Text, i);
                else if ( Server()->Tick() > GameServer()->m_apPlayers[i]->m_BroadcastTick+Server()->TickSpeed()*5 )
                {
                    GameServer()->m_apPlayers[i]->m_BroadcastTick = 0;
                    GameServer()->SendBroadcast(Text, i);
                }
            }
        }
        m_LastSend = Server()->Tick();
    }
}

bool CEvent::CanBeUsed(int NewEvent, int Type, bool Canuseactual)
{
    if ( Type == 0 || Type == 1 )
    {
        if (NewEvent == m_ActualEvent[0] && !Canuseactual)
            return false;
        else if (g_Config.m_SvTwoEvent && NewEvent == m_ActualEvent[1] && !Canuseactual)
            return false;
        else if (g_Config.m_SvTwoEvent && NewEvent >= GUN && NewEvent <= KATANA && m_ActualEvent[1] >= GUN && m_ActualEvent[1] <= KATANA)
            return false;
        else if (g_Config.m_SvTwoEvent && NewEvent >= RACE_WARRIOR && NewEvent <= RACE_RANDOM && m_ActualEvent[1] >= RACE_WARRIOR && m_ActualEvent[1] <= RACE_RANDOM)
            return false;
        else if (NewEvent == SURVIVOR && Controller()->IsTeamplay())
            return false;

        else if (NewEvent == HAMMER && !Controller()->IsWeaponEntity(WEAPON_HAMMER))
            return false;
        else if (NewEvent == GUN && !Controller()->IsWeaponEntity(WEAPON_GUN))
            return false;
        else if (NewEvent == SHOTGUN && !Controller()->IsWeaponEntity(WEAPON_SHOTGUN))
            return false;
        else if (NewEvent == GRENADE && !Controller()->IsWeaponEntity(WEAPON_GRENADE))
            return false;
        else if (NewEvent == RIFLE && !Controller()->IsWeaponEntity(WEAPON_RIFLE))
            return false;
        else if (NewEvent == WALLSHOT && !Controller()->IsWeaponEntity(WEAPON_RIFLE))
            return false;
        else if (NewEvent == BOUNCE_10 && !Controller()->IsWeaponEntity(WEAPON_RIFLE))
            return false;

        if ( Type == 1 )
        {
            if ((NewEvent >= GUN && NewEvent <= KATANA) && (m_ActualEvent[0] >= GUN && m_ActualEvent[0] <= KATANA))
                return false;
            else if ((NewEvent >= RACE_WARRIOR && NewEvent <= RACE_RANDOM) && (m_ActualEvent[0] >= RACE_WARRIOR && m_ActualEvent[0] <= RACE_RANDOM))
                return false;
            else if (NewEvent == WALLSHOT || NewEvent == SURVIVOR)
                return false;
            else if (m_ActualEvent[0] == GRAVITY_0_25 && (NewEvent == GRAVITY_0 || NewEvent == GRAVITY_M0_25 || NewEvent == GRAVITY_M0_5))
                return false;
            else if (m_ActualEvent[0] == GRAVITY_0 && (NewEvent == GRAVITY_0_25 || NewEvent == GRAVITY_M0_25 || NewEvent == GRAVITY_M0_5))
                return false;
            else if (m_ActualEvent[0] == GRAVITY_M0_25 && (NewEvent == GRAVITY_0_25 || NewEvent == GRAVITY_0 || NewEvent == GRAVITY_M0_5))
                return false;
            else if (m_ActualEvent[0] == GRAVITY_M0_5 && (NewEvent == GRAVITY_0_25 || NewEvent == GRAVITY_0 || NewEvent == GRAVITY_M0_25))
                return false;
            else if (m_ActualEvent[0] == BULLET_BOUNCE && (NewEvent == BULLET_PIERCING || NewEvent == BULLET_GLUE))
                return false;
            else if (m_ActualEvent[0] == BULLET_PIERCING && (NewEvent == BULLET_BOUNCE || NewEvent == BULLET_GLUE))
                return false;
            else if (m_ActualEvent[0] == BULLET_GLUE && (NewEvent == BULLET_PIERCING || NewEvent == BULLET_BOUNCE))
                return false;
            else if (NewEvent == ALL)
                return false;
        }
    }
    else if ( Type == 3 )
    {
        if (NewEvent == m_ActualEventTeam && !Canuseactual)
            return false;
        else if (NewEvent >= T_SURVIVOR && str_comp(g_Config.m_SvGametype, "ctf") == 0)
            return false;

        else if ((NewEvent == HAMMER_HEAL || NewEvent == HAMMER_KILL) && !Controller()->IsWeaponEntity(WEAPON_HAMMER))
            return false;
        else if ((NewEvent == GUN_HEAL || NewEvent == GUN_KILL) && !Controller()->IsWeaponEntity(WEAPON_GUN))
            return false;
        else if ((NewEvent == SHOTGUN_HEAL || NewEvent == SHOTGUN_KILL) && !Controller()->IsWeaponEntity(WEAPON_SHOTGUN))
            return false;
        else if ((NewEvent == GRENADE_HEAL || NewEvent == GRENADE_KILL) && !Controller()->IsWeaponEntity(WEAPON_GRENADE))
            return false;
        else if ((NewEvent == RIFLE_HEAL || NewEvent == RIFLE_KILL) && !Controller()->IsWeaponEntity(WEAPON_RIFLE))
            return false;
        else if ((NewEvent == KATANA_HEAL || NewEvent == KATANA_KILL) && !Controller()->IsWeaponEntity(WEAPON_NINJA) && !IsActualEvent(KATANA))
            return false;
    }
    else
        return false;

    return true;
}

void CEvent::SetTune()
{
    ResetTune();
    if ( IsActualEvent(GRAVITY_0_25) )
    {
        GameServer()->Tuning()->Set("gravity", static_cast<float>(0.25));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(GRAVITY_0) )
    {
        GameServer()->Tuning()->Set("gravity", static_cast<float>(0.0));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(GRAVITY_M0_25) )
    {
        GameServer()->Tuning()->Set("gravity", static_cast<float>(-0.25));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(GRAVITY_M0_5) )
    {
        GameServer()->Tuning()->Set("gravity", static_cast<float>(-0.5));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(BOUNCE_10) )
    {
        GameServer()->Tuning()->Set("laser_bounce_num", static_cast<float>(10.0));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(HOOK_VERY_LONG) )
    {
        GameServer()->Tuning()->Set("hook_length", static_cast<float>(10000.0));
        GameServer()->Tuning()->Set("hook_fire_speed", static_cast<float>(300.0));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(SPEED_X10) )
    {
        GameServer()->Tuning()->Set("ground_control_speed", static_cast<float>(100.0));
        GameServer()->Tuning()->Set("air_control_speed", static_cast<float>(50.0));
        GameServer()->Tuning()->Set("hook_fire_speed", static_cast<float>(300.0));
        GameServer()->Tuning()->Set("hook_drag_speed", static_cast<float>(150.0));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(JUMP_X1_5) )
    {
        GameServer()->Tuning()->Set("ground_jump_impulse", static_cast<float>(13.2f) * static_cast<float>(1.5f));
        GameServer()->Tuning()->Set("air_jump_impulse", static_cast<float>(12.0f)  * static_cast<float>(1.5f));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(WEAPON_SLOW) )
    {
        GameServer()->Tuning()->Set("gun_speed", static_cast<float>(110.0));
        GameServer()->Tuning()->Set("gun_lifetime", static_cast<float>(40.0));
        GameServer()->Tuning()->Set("shotgun_speed", static_cast<float>(137.5));
        GameServer()->Tuning()->Set("shotgun_lifetime", static_cast<float>(4.0));
        GameServer()->Tuning()->Set("grenade_speed", static_cast<float>(50.0));
        GameServer()->Tuning()->Set("grenade_lifetime", static_cast<float>(40.0));
        GameServer()->Tuning()->Set("laser_bounce_delay", static_cast<float>(1500.0));
        GameServer()->SendTuningParams(-1);
    }
    if ( IsActualEvent(ALL) )
    {
        GameServer()->Tuning()->Set("gravity", static_cast<float>(0.0));
        GameServer()->Tuning()->Set("laser_bounce_num", static_cast<float>(10.0));
        GameServer()->Tuning()->Set("hook_length", static_cast<float>(10000.0));
        GameServer()->Tuning()->Set("hook_fire_speed", static_cast<float>(300.0));
        GameServer()->Tuning()->Set("gun_speed", static_cast<float>(110.0));
        GameServer()->Tuning()->Set("gun_lifetime", static_cast<float>(40.0));
        GameServer()->Tuning()->Set("shotgun_speed", static_cast<float>(137.5));
        GameServer()->Tuning()->Set("shotgun_lifetime", static_cast<float>(4.0));
        GameServer()->Tuning()->Set("grenade_speed", static_cast<float>(50.0));
        GameServer()->Tuning()->Set("grenade_lifetime", static_cast<float>(40.0));
        GameServer()->Tuning()->Set("laser_bounce_delay", static_cast<float>(1500.0));
        GameServer()->SendTuningParams(-1);
    }
}
void CEvent::ResetTune()
{
    GameServer()->Tuning()->Set("gravity", static_cast<float>(0.5));
    GameServer()->Tuning()->Set("laser_bounce_num", static_cast<float>(1.0));
    GameServer()->Tuning()->Set("hook_length", static_cast<float>(450.0));

    GameServer()->Tuning()->Set("ground_control_speed", static_cast<float>(10.0));
    GameServer()->Tuning()->Set("air_control_speed", static_cast<float>(5.0));
    GameServer()->Tuning()->Set("hook_fire_speed", static_cast<float>(80.0));
    GameServer()->Tuning()->Set("hook_drag_speed", static_cast<float>(15.0));
    GameServer()->Tuning()->Set("ground_jump_impulse", static_cast<float>(13.2f));
    GameServer()->Tuning()->Set("air_jump_impulse", static_cast<float>(12.0f));

    GameServer()->Tuning()->Set("gun_speed", static_cast<float>(2200.0));
    GameServer()->Tuning()->Set("gun_lifetime", static_cast<float>(2.0));
    GameServer()->Tuning()->Set("shotgun_speed", static_cast<float>(2750.0));
    GameServer()->Tuning()->Set("shotgun_lifetime", static_cast<float>(0.20));
    GameServer()->Tuning()->Set("grenade_speed", static_cast<float>(1000.0));
    GameServer()->Tuning()->Set("grenade_lifetime", static_cast<float>(2.0));
    GameServer()->Tuning()->Set("laser_bounce_delay", static_cast<float>(150.0));

    GameServer()->SendTuningParams(-1);
}

void CEvent::NextEvent()
{
    if ( m_ActualEvent[0] == SURVIVOR )
    {
        GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
        Controller()->EndRound();
    }

    for ( int i = 1; true; i++ )
    {
        if (m_ActualEvent[0] + i < END && CanBeUsed(m_ActualEvent[0] + i, 0))
        {
            m_ActualEvent[0] += i;
            break;
        }
        else if (m_ActualEvent[0] + i >= END && CanBeUsed((m_ActualEvent[0] + i) % END, 0))
        {
            m_ActualEvent[0] = (m_ActualEvent[0] + i) % END;
            break;
        }
    }

    m_StartEvent[0] = Server()->Tick();
    SetTune();
}

void CEvent::NextRandomEvent()
{
    if ( m_ActualEvent[0] == SURVIVOR )
    {
        GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
        Controller()->EndRound();
    }

    int NewEvent;
    while (!CanBeUsed(NewEvent = (rand() % ((END - 1) - NOTHING + 1)) + NOTHING, 0));
    m_ActualEvent[0] = NewEvent;
    m_StartEvent[0] = Server()->Tick();
    SetTune();
}

bool CEvent::SetEvent(int event)
{
    if ( event >= 0 && event < END )
    {
        if ( m_ActualEvent[0] == SURVIVOR )
        {
            GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
            Controller()->EndRound();
        }

        if (!CanBeUsed(event, 0, true))
        {
            GameServer()->SendChatTarget(-1, "Can't use this event with this config.");
            return false;
        }

        m_ActualEvent[0] = event;
        m_StartEvent[0] = Server()->Tick();
        SetTune();
        return true;
    }
    else
        return false;

    return false;
}

bool CEvent::AddTime(long secondes)
{
    if ( secondes > 0 )
    {
        m_StartEvent[0] += secondes * Server()->TickSpeed();
        return true;
    }
    else
        return false;

    return false;
}

void CEvent::NextEventTeam()
{
    if ( m_ActualEventTeam >= T_SURVIVOR )
    {
        if ( m_ActualEventTeam == T_SURVIVOR )
            GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
        Controller()->EndRound();
    }

    for ( int i = 1; true; i++ )
    {
        if (m_ActualEventTeam + i < END && CanBeUsed(m_ActualEventTeam + i, 0))
        {
            m_ActualEventTeam += i;
            break;
        }
        else if (m_ActualEventTeam + i > END && CanBeUsed((m_ActualEventTeam + i) % END, 0))
        {
            m_ActualEventTeam = (m_ActualEventTeam + i) % END;
            break;
        }
    }

    if ( m_ActualEventTeam == TEE_VS_ZOMBIE )
        Controller()->EndRound();

    m_StartEventTeam = Server()->Tick();
}

void CEvent::NextRandomEventTeam()
{
    if ( m_ActualEventTeam >= T_SURVIVOR )
    {
        if ( m_ActualEventTeam == T_SURVIVOR )
            GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
        Controller()->EndRound();
    }

    int NewEvent;
    while (!CanBeUsed(NewEvent = (rand() % ((T_END - 1) - T_NOTHING + 1)) + T_NOTHING, 3));
    m_ActualEventTeam = NewEvent;
    m_StartEventTeam = Server()->Tick();
    if ( m_ActualEventTeam == TEE_VS_ZOMBIE )
        Controller()->EndRound();
}

bool CEvent::SetEventTeam(int event)
{
    if ( event >= 0 && event < T_END )
    {
        if ( m_ActualEventTeam >= T_SURVIVOR )
        {
            if ( m_ActualEventTeam == T_SURVIVOR )
                GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
            Controller()->EndRound();
        }

        if (!CanBeUsed(event, 3, true))
        {
            GameServer()->SendChatTarget(-1, "Can't use this event with this config.");
            return false;
        }

        m_ActualEventTeam = event;
        m_StartEventTeam = Server()->Tick();
        if ( m_ActualEventTeam == TEE_VS_ZOMBIE )
            Controller()->EndRound();
        return true;
    }
    else
        return false;

    return false;
}

bool CEvent::AddTimeTeam(long secondes)
{
    if ( secondes > 0 )
    {
        m_StartEventTeam += secondes * Server()->TickSpeed();
        return true;
    }
    else
        return false;

    return false;
}

