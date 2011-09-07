#include <new>
#include <ctime>
#include <engine/shared/config.h>
#include "event.h"

CEvent::CEvent(CGameContext *GameServer)
{
	m_pGameServer = GameServer;
	m_pServer = m_pGameServer->Server();
	m_pController = m_pGameServer->m_pController;
	m_StartEvent[0] = 0;
	m_StartEvent[1] = 0;
	m_StartEventTeam = 0;
	m_StartEventRound = 0;
	m_TwoEvent = false;
	m_ActualEvent[0] = -1;
	m_ActualEvent[1] = -1;
	m_ActualEventTeam = 0;
	m_LastSend = 0;
}

CEvent::~CEvent()
{

}

void CEvent::Tick()
{
	if ( m_TwoEvent && Controller()->IsTeamplay() )
		m_TwoEvent = false;

	int Elapsed[2] = {0, 0};
	Elapsed[0] = Server()->Tick() - m_StartEvent[0];
	if ( m_TwoEvent )
		Elapsed[1] = Server()->Tick() - m_StartEvent[1];

	if ( Elapsed[0] >= 150 * Server()->TickSpeed() || m_ActualEvent[0] == -1 )
	{
		if ( m_ActualEvent[0] == SURVIVOR )
		{
			GameServer()->SendChatTarget(-1, "There isn't a winner ... ");
			Controller()->EndRound();
		}

		int NewEvent = (rand() % ((END - 1) - NOTHING + 1)) + NOTHING;
		while ( (NewEvent = (rand() % ((END - 1) - NOTHING + 1)) + NOTHING) == m_ActualEvent[0] || (NewEvent == SURVIVOR && Controller()->IsTeamplay()));

		m_ActualEvent[0] = NewEvent;
		m_StartEvent[0] = Server()->Tick();
		SetTune();
	}
	if ( m_TwoEvent && (Elapsed[1] >= 150 * Server()->TickSpeed() || m_ActualEvent[0] == -1) )
	{
		int NewEvent = (rand() % ((ALL - 1) - NOTHING + 1)) + NOTHING;
		while ( (NewEvent = (rand() % ((END - 1) - NOTHING + 1)) + NOTHING) == m_ActualEvent[1] || NewEvent == m_ActualEvent[0] || (NewEvent > NOTHING && NewEvent <= KATANA) || NewEvent == WALLSHOT || (m_ActualEvent[0] == GRAVITY_0 && NewEvent == GRAVITY_M0_5) || (m_ActualEvent[0] == GRAVITY_M0_5 && NewEvent == GRAVITY_0) || (m_ActualEvent[0] == BULLET_BOUNCE && NewEvent == BULLET_PIERCING) || (m_ActualEvent[0] == BULLET_PIERCING && NewEvent == BULLET_BOUNCE) || NewEvent == SURVIVOR);

		m_ActualEvent[1] = NewEvent;
		m_StartEvent[1] = Server()->Tick();
		SetTune();
	}

	char Text[256] = "";
	switch(m_ActualEvent[0])
	{
		case NOTHING:
			str_format(Text, 256, "Event : Nothing. | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case HAMMER:
			str_format(Text, 256, "Event : All Get Hammer ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case GUN:
			str_format(Text, 256, "Event : All Get Gun ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case SHOTGUN:
			str_format(Text, 256, "Event : All Get Shotgun ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case GRENADE:
			str_format(Text, 256, "Event : All Get Grenade ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case RIFLE:
			str_format(Text, 256, "Event : All Get Rifle ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case KATANA:
			str_format(Text, 256, "Event : All Get Katana ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case UNLIMITED_AMMO:
			str_format(Text, 256, "Event : All Get Unlimited Ammo ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case LIFE_ARMOR_CRAZY:
			str_format(Text, 256, "Event : All Get Life and Armor Crazy ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case SURVIVOR:
			str_format(Text, 256, "Event : Mode Survivor ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case PROTECT_X2:
			str_format(Text, 256, "Event : All Get Protect X2 ! | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case INSTAGIB:
			str_format(Text, 256, "Event : Mode Instagib ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case WALLSHOT:
			str_format(Text, 256, "Event : Mode WallShot ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case BULLET_PIERCING:
			str_format(Text, 256, "Event : All Bullets can pierce Tee and Tiles ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case BULLET_BOUNCE:
			str_format(Text, 256, "Event : All Bullets can bounce ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case HAVE_ALL_WEAPON:
			str_format(Text, 256, "Event : All get all weapons when respawn ! | Remaininig %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case GRAVITY_0:
			str_format(Text, 256, "Event : Gravity modified to 0 ! | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case GRAVITY_M0_5:
			str_format(Text, 256, "Event : Gravity modified to -0.5 ! | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case BOUNCE_10:
			str_format(Text, 256, "Event : Laser Bounce Num modified to 10 ! | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case HOOK_VERY_LONG:
			str_format(Text, 256, "Event : Hook Length is now very long ! | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case SPEED_X10:
			str_format(Text, 256, "Event : All Get Speed X10 ! | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case WEAPON_SLOW:
			str_format(Text, 256, "Event : All bullets are slow ! | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
		case ALL:
			str_format(Text, 256, "Event : All events are active ! | Remaining : %02d:%02d", (150 - Elapsed[0]/Server()->TickSpeed()) / 60, (150 - Elapsed[0]/Server()->TickSpeed()) % 60);
			break;
	}

	if ( IsTwoEvent() )
	{
		char Temp[256] = "";
		switch(m_ActualEvent[1])
		{
			case NOTHING:
				str_format(Temp, 256, "\nEvent 2 : Nothing. | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case HAMMER:
				str_format(Temp, 256, "\nEvent 2 : All Get Hammer ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case GUN:
				str_format(Temp, 256, "\nEvent 2 : All Get Gun ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case SHOTGUN:
				str_format(Temp, 256, "\nEvent 2 : All Get Shotgun ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case GRENADE:
				str_format(Temp, 256, "\nEvent 2 : All Get Grenade ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case RIFLE:
				str_format(Temp, 256, "\nEvent 2 : All Get Rifle ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case KATANA:
				str_format(Temp, 256, "\nEvent 2 : All Get Katana ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case UNLIMITED_AMMO:
				str_format(Temp, 256, "\nEvent 2 : All Get Unlimited Ammo ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case LIFE_ARMOR_CRAZY:
				str_format(Temp, 256, "\nEvent 2 : All Get Life and Armor Crazy ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case SURVIVOR:
				str_format(Temp, 256, "\nEvent 2 : Mode Survivor ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case PROTECT_X2:
				str_format(Temp, 256, "\nEvent 2 : All Get Protect X2 ! | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case INSTAGIB:
				str_format(Temp, 256, "\nEvent 2 : Mode Instagib ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case WALLSHOT:
				str_format(Temp, 256, "\nEvent 2 : Mode WallShot ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case BULLET_PIERCING:
				str_format(Temp, 256, "\nEvent 2 : All Bullets can pierce Tee and Tiles ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case BULLET_BOUNCE:
				str_format(Temp, 256, "\nEvent 2 : All Bullets can bounce ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case HAVE_ALL_WEAPON:
				str_format(Temp, 256, "\nEvent 2 : All get all weapons when respawn ! | Remaininig %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case GRAVITY_0:
				str_format(Temp, 256, "\nEvent 2 : Gravity modified to 0 ! | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case GRAVITY_M0_5:
				str_format(Temp, 256, "\nEvent 2 : Gravity modified to -0.5 ! | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case BOUNCE_10:
				str_format(Temp, 256, "\nEvent 2 : Laser Bounce Num modified to 10 ! | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case HOOK_VERY_LONG:
				str_format(Temp, 256, "\nEvent 2 : Hook Length is now very long ! | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case SPEED_X10:
				str_format(Temp, 256, "\nEvent 2 : All Get Speed X10 ! | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case WEAPON_SLOW:
				str_format(Temp, 256, "\nEvent 2 : All bullets are slow ! | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
			case ALL:
				str_format(Temp, 256, "\nEvent 2 : All events are active ! | Remaining : %02d:%02d", (150 - Elapsed[1]/Server()->TickSpeed()) / 60, (150 - Elapsed[1]/Server()->TickSpeed()) % 60);
				break;
		}
		str_append(Text, Temp, 256);
	}

	if ( Controller()->IsTeamplay() )
	{
		int ElapsedTeam = Server()->Tick() - m_StartEventTeam;
		if ( ElapsedTeam >= 300 * Server()->TickSpeed())
		{
			int NewEvent = (rand() % ((T_END - 1) - T_NOTHING + 1)) + T_NOTHING;
			while ( (NewEvent = (rand() % ((T_END - 1) - T_NOTHING + 1)) + T_NOTHING) == m_ActualEventTeam || (NewEvent >= STEAL_TEE && str_comp(g_Config.m_SvGametype, "ctf") == 0) );

			if ( m_ActualEventTeam >= STEAL_TEE )
				Controller()->EndRound();

			m_ActualEventTeam = NewEvent;
			m_StartEventTeam = Server()->Tick();
			if ( NewEvent == TEE_VS_ZOMBIE )
				Controller()->EndRound();
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
			case RIFLE_HEAL:
				str_format(Temp, 256, "\nEvent Team : Rifle Heal Teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
				break;
			case CAN_KILL:
				str_format(Temp, 256, "\nEvent Team : Can kill teammate ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
				break;
			case STEAL_TEE:
				str_format(Temp, 256, "\nEvent Team : Steal Tee ! | Remaining : %02d:%02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60);
				break;
			case TEE_VS_ZOMBIE:
				str_format(Temp, 256, "\nEvent Team : Tee vs Zombies ! | Remaining : %02d:%02d\nTime Left : %02d", (300 - ElapsedTeam/Server()->TickSpeed()) / 60, (300 - ElapsedTeam/Server()->TickSpeed()) % 60, ((m_StartEventRound+Server()->TickSpeed()*35) - Server()->Tick()) / Server()->TickSpeed());
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

void CEvent::SetTune()
{
	ResetTune();
	if ( IsActualEvent(GRAVITY_0) )
	{
			GameServer()->Tuning()->Set("gravity", static_cast<float>(0.0));
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
		Controller()->EndRound();

	if ( m_ActualEvent[0] + 1 < END && (!Controller()->IsTeamplay() || m_ActualEvent[0] + 1 != SURVIVOR))
		m_ActualEvent[0]++;
	else if (Controller()->IsTeamplay() && m_ActualEvent[0] + 1 == SURVIVOR)
		m_ActualEvent[0]++;
	else
		m_ActualEvent[0] = 0;

	m_StartEvent[0] = Server()->Tick();
	SetTune();
}

void CEvent::NextRandomEvent()
{
	if ( m_ActualEvent[0] == SURVIVOR )
		Controller()->EndRound();

	int NewEvent = (rand() % ((END - 1) - NOTHING + 1)) + NOTHING;
	while ( (NewEvent = (rand() % ((END - 1) - NOTHING + 1)) + NOTHING) == m_ActualEvent[0] || (NewEvent == SURVIVOR && Controller()->IsTeamplay()));

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

		if ( event == SURVIVOR && Controller()->IsTeamplay() )
		{
			GameServer()->SendChatTarget(-1, "The event Survivor can't be used in wTDM and wCTF");
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
	if ( m_ActualEventTeam >= STEAL_TEE )
		Controller()->EndRound();

	if ( (m_ActualEventTeam + 1 >= STEAL_TEE && str_comp(g_Config.m_SvGametype, "ctf") == 0) || m_ActualEventTeam + 1 >= T_END )
		m_ActualEventTeam = 0;
	else if ( m_ActualEventTeam + 1 < T_END )
		m_ActualEventTeam++;

	if ( m_ActualEventTeam == TEE_VS_ZOMBIE )
		Controller()->EndRound();

	m_StartEventTeam = Server()->Tick();
}

void CEvent::NextRandomEventTeam()
{
	if ( m_ActualEventTeam >= STEAL_TEE )
		Controller()->EndRound();

	int NewEvent = (rand() % ((T_END - 1) - T_NOTHING + 1)) + T_NOTHING;
	while ( (NewEvent = (rand() % ((T_END - 1) - T_NOTHING + 1)) + T_NOTHING) == m_ActualEventTeam || (NewEvent >= STEAL_TEE && str_comp(g_Config.m_SvGametype, "ctf") == 0) );

	m_ActualEventTeam = NewEvent;
	m_StartEventTeam = Server()->Tick();
	if ( m_ActualEventTeam == TEE_VS_ZOMBIE )
		Controller()->EndRound();
}

bool CEvent::SetEventTeam(int event)
{
	if ( event >= 0 && event < T_END )
	{
		if ( m_ActualEventTeam >= STEAL_TEE )
			Controller()->EndRound();

		if (event >= STEAL_TEE && str_comp(g_Config.m_SvGametype, "ctf") == 0)
		{
			GameServer()->SendChatTarget(-1, "The event Steal tee and Tee vs Zombies can't be used in wCTF");
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

