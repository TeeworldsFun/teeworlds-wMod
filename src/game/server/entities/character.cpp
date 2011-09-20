/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include <game/server/statistiques.h>
#include <game/server/event.h>

#include "character.h"
#include "laser.h"
#include "projectile.h"
#include "aura.h"

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;
	str_copy(m_aWeapons[WEAPON_HAMMER].m_Name, "Hammer", 50);
	str_copy(m_aWeapons[WEAPON_GUN].m_Name, "Gun", 50);
	str_copy(m_aWeapons[WEAPON_SHOTGUN].m_Name, "Shotgun", 50);
	str_copy(m_aWeapons[WEAPON_GRENADE].m_Name, "Grenade", 50);
	str_copy(m_aWeapons[WEAPON_RIFLE].m_Name, "Laser", 50);
	str_copy(m_aWeapons[WEAPON_NINJA].m_Name, "Katana", 50);
}

CCharacter::~CCharacter()
{
	if ( m_AuraProtect[0] != 0 )
	{
		for ( int i = 0; i < 12; i++ )
			delete m_AuraProtect[i];
		m_AuraProtect[0] = 0;
	}
	if ( m_AuraCaptain[0] != 0 )
	{
		for ( int i = 0; i < 3; i++ )
			delete m_AuraCaptain[i];
		m_AuraCaptain[0] = 0;
	}
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_ActiveWeapon = WEAPON_GUN;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;

	m_HealthRegenStart = 0;
	m_HealthIncrase = true;

	m_pPlayer = pPlayer;
	m_Pos = Pos;

	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
	m_Core.m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));
	m_Protect = Server()->Tick();
	m_AuraProtect[0] = 0;
	m_AuraCaptain[0] = 0;

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;

	GameServer()->m_pController->OnCharacterSpawn(this);
	if ( GameServer()->m_pEventsGame->IsActualEvent(HAVE_ALL_WEAPON) )
	{
		GiveWeapon(WEAPON_RIFLE, 20);
		GiveWeapon(WEAPON_GRENADE, 20);
		GiveWeapon(WEAPON_SHOTGUN, 20);
	}
	return true;
}

void CCharacter::Destroy()
{
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);

	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS)
		m_ActiveWeapon = 0;

	char a[256] = "";
	str_format(a, 256, " Ammo of the %s is : %d/%d.", m_aWeapons[W].m_Name, m_aWeapons[W].m_Ammo, g_pData->m_Weapons.m_aId[W].m_Maxammo);
	GameServer()->SendChatTarget(m_pPlayer->GetCID(), a);

	GameServer()->m_pStatistiques->AddChangeWeapon(m_pPlayer->GetSID());
}

bool CCharacter::IsGrounded()
{
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}


void CCharacter::HandleNinja()
{
	if(m_ActiveWeapon != WEAPON_NINJA)
		return;

	int active_player = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( GameServer()->IsClientPlayer(i) && ( (GameServer()->m_pController->IsTeamplay() && GameServer()->m_apPlayers[i]->GetTeam() != m_pPlayer->GetTeam() && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS) || !GameServer()->m_pController->IsTeamplay() ) )
			active_player++;
	}

	if ( m_Ninja.m_Killed >= 3 || (active_player != 1 && m_Ninja.m_Killed >= active_player - 1) )
	{
		m_aWeapons[WEAPON_NINJA].m_Got = false;
		m_ActiveWeapon = m_LastWeapon;
		if(m_ActiveWeapon == WEAPON_NINJA)
			m_ActiveWeapon = WEAPON_GUN;

		SetWeapon(m_ActiveWeapon);
		return;
	}

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir*m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				if(aEnts[i]->TakeDamage(vec2(0, 10.0f), 20, m_pPlayer->GetCID(), WEAPON_NINJA))
					m_Ninja.m_Killed++;
			}
		}

		return;
	}

	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_aWeapons[WEAPON_NINJA].m_Got || (GameServer()->m_pEventsGame->GetActualEvent() >= HAMMER && GameServer()->m_pEventsGame->GetActualEvent() <= KATANA) || GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) )
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon+1)%NUM_WEAPONS;
			if(m_aWeapons[WantedWeapon].m_Got)
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_aWeapons[WantedWeapon].m_Got)
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_aWeapons[WantedWeapon].m_Got)
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0)
		return;

	const int Race = m_pPlayer->m_Race;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_ActiveWeapon != WEAPON_HAMMER || Race != ENGINEER)
		FullAuto = true;

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && m_aWeapons[m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_aWeapons[m_ActiveWeapon].m_Ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
		return;
	}

	bool sound = false;

	if ( Server()->Tick() >= m_SoundReloadStart+(Server()->TickSpeed()*150)/1000 )
	{
		m_SoundReloadStart = Server()->Tick();
		sound = true;
	}

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;

	switch(m_ActiveWeapon)
	{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if ((pTarget == this) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
					m_pPlayer->GetCID(), m_ActiveWeapon);
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;
			
			if ( Race == WARRIOR )
			{
				GameServer()->CreateExplosion(m_Pos, m_pPlayer->GetCID(), m_ActiveWeapon, true, false);
				if (sound)
					GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
			}
			else if ( Race == ENGINEER )
			{
				/* LASER WALL */
			}
			else if ( Race == ORC )
			{
				GameServer()->CreateSound(m_Pos, SOUND_HIT);
			}
			else if ( Race == MINER )
			{
				GameServer()->CreateExplosion(m_Pos + (Direction * -1), m_pPlayer->GetCID(), m_ActiveWeapon, false, false);
			}

		} break;

		case WEAPON_GUN:
		{
			if ( Race == WARRIOR )
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
					1, true, 0, sound ? SOUND_GRENADE_EXPLODE : -1, WEAPON_GUN, false);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(1);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);

				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}
			if ( Race == ENGINEER )
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread*2+1);

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GUN,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
						1, false, 0, -1, WEAPON_GUN);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}
			else if ( Race == ORC )
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread*2+1);

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-2.5133f, -1.2566f, 0, 1.2566f, 2.5133f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GUN,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
						1, false, 0, -1, WEAPON_GUN);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}
			else if ( Race == MINER )
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_RIFLE,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					1, 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GUN);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(1);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);

				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}

			if (sound)
				GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
		} break;

		case WEAPON_SHOTGUN:
		{
			if ( Race == WARRIOR )
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread*2+1);

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
						1, true, 0, sound ? SOUND_GRENADE_EXPLODE : -1, WEAPON_SHOTGUN, false, true);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, 0,m_pPlayer->GetCID());
			}
			else if ( Race == ENGINEER )
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread*2+1);

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime * 2),
						1, true, 0, sound ? SOUND_GRENADE_EXPLODE : -1, WEAPON_SHOTGUN, false, false, true);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, 0,m_pPlayer->GetCID());
			}
			else if ( Race == ORC )
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread*2+1);

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
						1, true, 0, sound ? SOUND_GRENADE_EXPLODE : -1, WEAPON_SHOTGUN, true);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, 0,m_pPlayer->GetCID());
			}
			else if ( Race == MINER )
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_RIFLE,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					1, 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_SHOTGUN);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(1);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);

				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}

			if (sound)
				GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);

		} break;

		case WEAPON_GRENADE:
		{
			if ( Race == WARRIOR )
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread*2+1);

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
						1, true, 0, sound ? SOUND_GRENADE_EXPLODE : -1, WEAPON_GRENADE, true);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}
			else if ( Race == ENGINEER )
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread*2+1);

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
						1, true, 0, sound ? SOUND_GRENADE_EXPLODE : -1, WEAPON_GRENADE, false, false, true);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}
			else if ( Race == ORC )
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime * 3),
					1, true, 0, sound ? SOUND_GRENADE_EXPLODE : -1, WEAPON_GRENADE, true, false, true);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(1);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}
			else if ( Race == MINER )
			{
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_RIFLE,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					1, 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(1);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);

				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
			}

			if (sound)
				GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);

		} break;

		case WEAPON_RIFLE:
		{
			if ( Race == ORC )
			{
				new CLaser(GameWorld(), m_Pos, Direction, 300, m_pPlayer->GetCID());
				m_ReloadTimer = 1;
			}
			else
			{
				int ShotSpread = 2;

				for(int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = GetAngle(Direction);
					a += Spreading[i+2];
					float v = 1-(absolute(i)/(float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

					new CLaser(GameWorld(), m_Pos, vec2(cosf(a), sinf(a))*Speed, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID());
				}
			}

			if (sound)
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
		} break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			m_NumObjectsHit = 0;

			m_Ninja.m_ActivationDir = Direction;
			m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

			if (sound)
				GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);
		} break;

	}

	m_AttackTick = Server()->Tick();

	if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0 && !GameServer()->m_pEventsGame->IsActualEvent(UNLIMITED_AMMO) && ( Race != ORC || m_ActiveWeapon != WEAPON_RIFLE ) ) // -1 == unlimited
		m_aWeapons[m_ActiveWeapon].m_Ammo--;

	if(!m_ReloadTimer)
		m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 7500;

	GameServer()->m_pStatistiques->AddFire(m_pPlayer->GetSID());
}

void CCharacter::HandleWeapons()
{
	if ( GameServer()->m_pEventsGame->GetActualEvent() >= HAMMER && GameServer()->m_pEventsGame->GetActualEvent() <= KATANA && m_ActiveWeapon != WEAPON_NINJA)
	{
		switch(GameServer()->m_pEventsGame->GetActualEvent())
		{
			case HAMMER:
				m_ActiveWeapon = WEAPON_HAMMER;
				break;
			case GUN:
				m_ActiveWeapon = WEAPON_GUN;
				break;
			case SHOTGUN:
				if ( m_aWeapons[WEAPON_SHOTGUN].m_Got == false )
					GiveWeapon(WEAPON_SHOTGUN, 20);
				m_ActiveWeapon = WEAPON_SHOTGUN;
				break;
			case GRENADE:
				if ( m_aWeapons[WEAPON_GRENADE].m_Got == false )
					GiveWeapon(WEAPON_GRENADE, 20);
				m_ActiveWeapon = WEAPON_GRENADE;
				break;
			case RIFLE:
				if ( m_aWeapons[WEAPON_RIFLE].m_Got == false )
					GiveWeapon(WEAPON_RIFLE, 20);
				m_ActiveWeapon = WEAPON_RIFLE;
				break;
			case KATANA:
				if ( m_aWeapons[WEAPON_NINJA].m_Got == false )
					GiveNinja();
				m_ActiveWeapon = WEAPON_NINJA;
				break;
		}
	}
	else if ( GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) )
	{
		if ( m_aWeapons[WEAPON_RIFLE].m_Got == false )
			GiveWeapon(WEAPON_RIFLE, 20);
		m_ActiveWeapon = WEAPON_RIFLE;
	}

	//ninja
	HandleNinja();

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	if( m_pPlayer->m_Race == WARRIOR && m_ActiveWeapon == WEAPON_HAMMER && !GameServer()->m_pEventsGame->IsActualEvent(HAMMER) && m_LatestInput.m_Fire&1 )
	{
		if ((Server()->Tick() - m_HealthRegenStart) >= 350 * Server()->TickSpeed() / 1000)
		{
			if (m_Armor > 0)
				m_Armor--;
		        else
				m_Health--;
			if (m_Health <= 0)
				Die(m_pPlayer->GetCID(), WEAPON_WORLD);

			m_HealthRegenStart = Server()->Tick();
		}
	}
	else if ( GameServer()->m_pEventsGame->IsActualEvent(LIFE_ARMOR_CRAZY) )
	{
		if ( (Server()->Tick() - m_HealthRegenStart) >= 150 * Server()->TickSpeed() / 1000 )
		{
			if ( m_HealthIncrase == true )
			{
	        		if (m_Health < 10)
		        	    m_Health++;
		        	else if (m_Armor < 10)
		        	    m_Armor++;
					
				if (m_Armor == 10)
					m_HealthIncrase = false;
			}
			else
			{
				if (m_Armor > 0)
		           		m_Armor--;
		        	else if (m_Health > 1)
		            		m_Health--;

				if (m_Health == 1)
					m_HealthIncrase = true;
			}

			m_HealthRegenStart = Server()->Tick();
		}
	}
	else if ( m_ActiveWeapon != WEAPON_NINJA && !GameServer()->m_pEventsGame->IsActualEvent(HAMMER) )
	{
		if ( (Server()->Tick() - m_HealthRegenStart) >= 150 * Server()->TickSpeed() / 1000 )
		{
	        	if (m_Health < 10)
		            m_Health++;
		        else if (m_Armor < 10)
		            m_Armor++;

			m_HealthRegenStart = Server()->Tick();
		}
	}

	if ( m_ActiveWeapon != WEAPON_NINJA && m_ActiveWeapon != WEAPON_HAMMER && !(m_LatestInput.m_Fire&1))
	{
		if (m_ActiveWeapon != WEAPON_GUN && (Server()->Tick() - m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart) >= 250 * Server()->TickSpeed() / 1000 && m_aWeapons[m_ActiveWeapon].m_Ammo < 20)
		{
			m_aWeapons[m_ActiveWeapon].m_Ammo++;
			m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();
		}
		else if ( m_ActiveWeapon == WEAPON_GUN && (Server()->Tick() - m_aWeapons[WEAPON_GUN].m_AmmoRegenStart) >= 50 * Server()->TickSpeed() / 1000 && m_aWeapons[m_ActiveWeapon].m_Ammo < 100)
		{
			m_aWeapons[WEAPON_GUN].m_Ammo++;
			m_aWeapons[WEAPON_GUN].m_AmmoRegenStart = Server()->Tick();
		}
	}
	return;
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	if(m_aWeapons[Weapon].m_Ammo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo || !m_aWeapons[Weapon].m_Got)
	{
		if (!m_aWeapons[Weapon].m_Got)
			m_aWeapons[Weapon].m_Ammo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
		else
			m_aWeapons[Weapon].m_Ammo += Ammo;

		m_aWeapons[Weapon].m_Got = true;
		
		if ( m_aWeapons[Weapon].m_Ammo > 100 )
			m_aWeapons[Weapon].m_Ammo = 100;

		if ( Weapon == m_ActiveWeapon && m_ActiveWeapon != WEAPON_NINJA && m_ActiveWeapon != WEAPON_HAMMER && m_ActiveWeapon != WEAPON_GUN && m_aWeapons[Weapon].m_Ammo >= 20)
		{
			char a[256] = "";
			str_format(a, 256, "Ammo of the %s is : %d/%d.", m_aWeapons[Weapon].m_Name, m_aWeapons[Weapon].m_Ammo, g_pData->m_Weapons.m_aId[Weapon].m_Maxammo);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), a);
		}
		
		GameServer()->m_pStatistiques->AddPickUpWeapon(m_pPlayer->GetSID());
		return true;
	}
	return false;
}

bool CCharacter::GiveNinja()
{
	if ( !m_aWeapons[WEAPON_NINJA].m_Got || m_Ninja.m_Killed > 0 )
	{ 
		m_aWeapons[WEAPON_NINJA].m_Got = true;
		m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
		m_LastWeapon = m_ActiveWeapon;
		m_ActiveWeapon = WEAPON_NINJA;
		m_Ninja.m_Damage = 0;
		m_Ninja.m_Killed = 0;

		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);
		if ( !GameServer()->m_pEventsGame->IsActualEvent(KATANA) )
			GameServer()->m_pStatistiques->AddPickUpNinja(m_pPlayer->GetSID());
		return true;
	}
	return false;
}

bool CCharacter::RemoveWeapon(int Weapon)
{
	if ( m_aWeapons[Weapon].m_Got )
	{
		m_aWeapons[Weapon].m_Got = false;
		if ( m_ActiveWeapon == Weapon )
		{
			m_ActiveWeapon = m_LastWeapon;
			if(m_ActiveWeapon == WEAPON_NINJA || m_ActiveWeapon == Weapon)
				m_ActiveWeapon = WEAPON_GUN;
			SetWeapon(m_ActiveWeapon);
		}
		return true;
	}
	return false;
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// or are not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::Tick()
{
	if(m_pPlayer->m_ForceBalanced)
	{
		char Buf[128];
		str_format(Buf, sizeof(Buf), "You were moved to %s due to team balancing", GameServer()->m_pController->GetTeamName(m_pPlayer->GetTeam()));
		GameServer()->SendBroadcast(Buf, m_pPlayer->GetCID());
		m_pPlayer->m_BroadcastTick = Server()->Tick();
		m_pPlayer->m_ForceBalanced = false;
	}

	if(m_Core.m_Jumped & 2)
		m_Core.m_Jumped &= ~2;

	m_Core.m_Input = m_Input;
	m_Core.Tick(true);

	if( m_Core.m_Jumped & 3 && GameServer()->m_pEventsGame->IsActualEvent(GRAVITY_M0_5) )
		m_Core.m_Vel.y = GameServer()->Tuning()->m_AirJumpImpulse;

	// handle death-tiles and leaving gamelayer
	if(GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameLayerClipped(m_Pos))
	{
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
	}

	if (!m_AuraProtect[0] && (m_Protect == -1 || (m_Protect != 0 && (Server()->Tick() - m_Protect) < Server()->TickSpeed())))
	{
		for ( int i = 0; i < 12; i++ )
			m_AuraProtect[i] = new CAura(&(GameServer()->m_World), this, i * 30, 60, i % 2 ? POWERUP_HEALTH : POWERUP_ARMOR);
	}

	else if ( m_AuraProtect[0] && !(m_Protect == -1 || (m_Protect != 0 && (Server()->Tick() - m_Protect) < Server()->TickSpeed())) )
	{
		for ( int i = 0; i < 12; i++ )
			delete m_AuraProtect[i];
		m_AuraProtect[0] = 0;
	}

	if ( !m_AuraCaptain[0] )
	{
		if ((GetPlayer() == GameServer()->m_pController->m_pCaptain[0] || GetPlayer() == GameServer()->m_pController->m_pCaptain[1]))
		{
				for ( int i = 0; i < 3; i++ )
					m_AuraCaptain[i] = new CAura(&(GameServer()->m_World), this, i * 120, 45, i % 2 ? POWERUP_HEALTH : POWERUP_ARMOR);
		}
	}
	else if (!(GetPlayer() == GameServer()->m_pController->m_pCaptain[0] || GetPlayer() == GameServer()->m_pController->m_pCaptain[1]))
	{
		for ( int i = 0; i < 3; i++ )
			delete m_AuraCaptain[i];
		m_AuraCaptain[0] = 0;
	}

	// handle Weapons
	HandleWeapons();

	// Previnput
	m_PrevInput = m_Input;
	return;
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision());
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	m_Core.Move();
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Mask);

	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Mask);


	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= 10 || m_ActiveWeapon == WEAPON_NINJA)
		return false;
	m_Health = clamp(m_Health+Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if(m_Armor >= 10 || m_ActiveWeapon == WEAPON_NINJA)
		return false;
	m_Armor = clamp(m_Armor+Amount, 0, 10);
	return true;
}

void CCharacter::Die(int Killer, int Weapon)
{
	// we got to wait 0.5 secs before respawning
	m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
		Killer, Server()->ClientName(Killer),
		m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// send the kill message
	CNetMsg_Sv_KillMsg Msg;
	Msg.m_Killer = Killer;
	Msg.m_Victim = m_pPlayer->GetCID();
	Msg.m_Weapon = Weapon;
	Msg.m_ModeSpecial = ModeSpecial;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();

	m_Alive = false;
	if ( m_AuraProtect[0] != 0 )
	{
		for ( int i = 0; i < 12; i++ )
			delete m_AuraProtect[i];
		m_AuraProtect[0] = 0;
	}
	if ( m_AuraCaptain[0] != 0 )
	{
		for ( int i = 0; i < 3; i++ )
			delete m_AuraCaptain[i];
		m_AuraCaptain[0] = 0;
	}
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
	if ( GameServer()->m_pEventsGame->GetActualEventTeam() == STEAL_TEE && GameServer()->m_apPlayers[Killer] )
		GetPlayer()->SetCaptureTeam(GameServer()->m_apPlayers[Killer]->GetTeam());
	else if ( GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE )
		GetPlayer()->SetCaptureTeam(TEAM_RED);
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if ( !(m_Protect == -1 || (m_Protect != 0 && (Server()->Tick() - m_Protect) < Server()->TickSpeed())) )
		m_Core.m_Vel += Force;

	if( ((GameServer()->m_pEventsGame->GetActualEventTeam() == HAMMER_HEAL && Weapon == WEAPON_HAMMER) || (GameServer()->m_pEventsGame->GetActualEventTeam() == RIFLE_HEAL && Weapon == WEAPON_RIFLE)) && GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From))
	{
		int heal = 5;
		if(m_Health < 10)
		{
			m_Health += heal;
			if ( m_Health - 10 > 0 )
				heal = m_Health - 10;
			else
				heal = 0;
		}

		if(heal && m_Armor < 10 )
		{
			m_Armor += heal;
			if ( m_Armor > 10 )
				m_Armor = 0;
		}
		return false;
	}
	else if (GameServer()->m_pEventsGame->GetActualEventTeam() == TEE_VS_ZOMBIE && m_pPlayer->GetTeam() == TEAM_RED)
		return false;

	if(GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From) && !g_Config.m_SvTeamdamage && GameServer()->m_pEventsGame->GetActualEventTeam() != CAN_KILL)
		return false;

	if ( m_Protect == -1 || (m_Protect != 0 && (Server()->Tick() - m_Protect) < Server()->TickSpeed()))
		return false;
	else
		m_Protect = 0;

	if(From == m_pPlayer->GetCID())
		return false;

	if ( m_ActiveWeapon == WEAPON_NINJA && Weapon != WEAPON_NINJA )
	{
		m_Ninja.m_Damage += Dmg;
		if ( m_Ninja.m_Damage >= 40 )
		{
			Dmg = m_Ninja.m_Damage / 40;
			m_Ninja.m_Damage = 0;
		}
		else
		{
			return false;
		}
	}
	else if ( Weapon != WEAPON_NINJA && m_ActiveWeapon == WEAPON_HAMMER && (m_LatestInput.m_Fire&1) && !GameServer()->m_pEventsGame->IsActualEvent(HAMMER) && m_pPlayer->m_Race == WARRIOR)
		return false;
	else if ( GameServer()->m_pEventsGame->IsActualEvent(PROTECT_X2) )
		Dmg = min(1, Dmg/2);

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

	if(Dmg && Weapon != WEAPON_NINJA && !GameServer()->m_pEventsGame->IsActualEvent(INSTAGIB) && (Weapon != WEAPON_HAMMER || GameServer()->m_apPlayers[From]->m_Race != ORC))
	{
		if(m_Armor)
		{
			if(Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if(Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
	}
	else if ( Weapon == WEAPON_NINJA || GameServer()->m_pEventsGame->IsActualEvent(INSTAGIB) || (Weapon == WEAPON_HAMMER && GameServer()->m_apPlayers[From]->m_Race == ORC) )
	{
		m_Health = 0;
		m_Armor = 0;
	}

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
	}

	// check for death
	if(m_Health <= 0)
	{
		Die(From, Weapon);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChr->m_EmoteType = EMOTE_HAPPY;
				pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
			}
		}

		return true;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// write down the m_Core
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = m_ActiveWeapon;
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;
		if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0 && m_ActiveWeapon != WEAPON_GUN && m_ActiveWeapon != WEAPON_NINJA && m_ActiveWeapon != WEAPON_HAMMER)
			pCharacter->m_AmmoCount = m_aWeapons[m_ActiveWeapon].m_Ammo / 2;
		else if(m_ActiveWeapon == WEAPON_GUN && m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
			pCharacter->m_AmmoCount = m_aWeapons[m_ActiveWeapon].m_Ammo / 10;
		else
			pCharacter->m_AmmoCount = m_aWeapons[m_ActiveWeapon].m_Ammo;
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;
}
