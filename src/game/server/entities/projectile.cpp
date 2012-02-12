/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "game/server/event.h"
#include "projectile.h"
#include "turret.h"
#include "teleporter.h"
#include "explodewall.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
                         int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, bool Limit, bool Smoke, bool Deploy, int Bounce)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
    m_Type = Type;
    m_Pos = Pos;
    m_Direction = Dir;
    m_LifeSpan = Span;
    m_Owner = Owner;
    m_Force = Force;
    m_Damage = Damage;
    m_SoundImpact = SoundImpact;
    m_Weapon = Weapon;
    m_StartTick = Server()->Tick();
    m_Explosive = Explosive;
    m_Limit = Limit;
    m_Smoke = Smoke;
    m_Deploy = Deploy;
    m_Bounce = Bounce;
    GameWorld()->InsertEntity(this);
    if (GameServer()->m_apPlayers[m_Owner] && m_Limit)
        GameServer()->m_apPlayers[Owner]->m_Mine++;
}

void CProjectile::Reset()
{
    if (GameServer()->m_apPlayers[m_Owner] && m_Limit)
        GameServer()->m_apPlayers[m_Owner]->m_Mine -= GameServer()->m_apPlayers[m_Owner]->m_Mine > 0 ? 1 : 0;
    GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
    float Curvature = 0;
    float Speed = 0;

    switch(m_Type)
    {
    case WEAPON_GRENADE:
        Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
        Speed = GameServer()->Tuning()->m_GrenadeSpeed;
        break;

    case WEAPON_SHOTGUN:
        Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
        Speed = GameServer()->Tuning()->m_ShotgunSpeed;
        break;

    case WEAPON_GUN:
        Curvature = GameServer()->Tuning()->m_GunCurvature;
        Speed = GameServer()->Tuning()->m_GunSpeed;
        break;
    }

    return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
    float Pt = 0;
    if (!GameServer()->m_pEventsGame->IsActualEvent(WEAPON_SLOW))
        Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
    else
        Pt = (Server()->Tick()-m_StartTick-2)/(float)Server()->TickSpeed();

    float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
    vec2 PrevPos = GetPos(Pt);
    vec2 CurPos = GetPos(Ct);
    m_LifeSpan--;

    //REDUCE LAG SERVER
    if ((GameServer()->m_pEventsGame->IsActualEvent(WEAPON_SLOW) || !distance(CurPos, PrevPos)) && (Server()->Tick()-m_StartTick) % 2 != 0)
        return;

    CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
    CCharacter *TargetChr = 0;
    CTurret *TargetTurret = 0;
    CExplodeWall *TargetExplodeWall = 0;
    int Collide = false;

    if (distance(CurPos, PrevPos))
    {
        vec2 At;
        CTeleporter *CollideTeleporter = (CTeleporter *)GameServer()->m_World.IntersectEntity(PrevPos, CurPos, 24.0f, At, CGameWorld::ENTTYPE_TELEPORTER);
        if (CollideTeleporter && CollideTeleporter->GetNext())
        {
            m_Direction = normalize(GetPos(Ct) - PrevPos);
            m_Pos = CollideTeleporter->GetNext()->m_Pos + (m_Direction * (50.0f));
            m_StartTick = Server()->Tick();
            Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
            Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
            PrevPos = GetPos(Pt);
            CurPos = GetPos(Ct);
        }

        Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
        TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);
        TargetTurret = (CTurret *)GameServer()->m_World.IntersectEntity(PrevPos, CurPos, 6.0f, CurPos, CGameWorld::ENTTYPE_TURRET);

        {
            float ClosestLen = -1;
            CExplodeWall *p = (CExplodeWall *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_EXPLODEWALL);
            for(; p; p = (CExplodeWall *)p->TypeNext())
            {
                // Store the values for fast access and easy
                // equations-to-code conversion
                float x1 = PrevPos.x, x2 = CurPos.x, x3 = p->m_From.x, x4 = p->m_Pos.x;
                float y1 = PrevPos.y, y2 = CurPos.y, y3 = p->m_From.y, y4 = p->m_Pos.y;

                float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
                // If d is zero, there is no intersection
                if (d == 0)
                    continue;

                // Get the x and y
                float pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
                float x = ( pre * (x3 - x4) - (x1 - x2) * post ) / d;
                float y = ( pre * (y3 - y4) - (y1 - y2) * post ) / d;

                // Check if the x and y coordinates are within both lines
                if ( x < min(x1, x2) || x > max(x1, x2) || x < min(x3, x4) || x > max(x3, x4) )
                    continue;
                if ( y < min(y1, y2) || y > max(y1, y2) || y < min(y3, y4) || y > max(y3, y4) )
                    continue;

                if ( ClosestLen > distance(CurPos, vec2(x, y)) || ClosestLen == -1 )
                {
                    ClosestLen = distance(CurPos, vec2(x, y));
                    CurPos.x = x;
                    CurPos.y = y;
                    TargetExplodeWall = p;
                }
            }
        }
        
        if ( m_Deploy && m_Type == WEAPON_SHOTGUN && (!Collide || GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING)) && m_LifeSpan < 0 )
        {
            int ShotSpread = 2;

            CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
            Msg.AddInt(ShotSpread*2+1);

            for(int i = -ShotSpread; i <= ShotSpread; ++i)
            {
                float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
                float a = GetAngle(m_Direction);
                a += Spreading[i+2];
                float v = 1-(absolute(i)/(float)ShotSpread);
                float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

                CProjectile *pProj = new CProjectile(GameWorld(), m_Weapon,
                                                     m_Owner,
                                                     CurPos,
                                                     vec2(cosf(a), sinf(a))*Speed,
                                                     (int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
                                                     1, m_Explosive, 0, m_SoundImpact, m_Weapon, m_Limit, m_Smoke);

                // pack the Projectile and send it to the client Directly
                CNetObj_Projectile p;
                pProj->FillInfo(&p);

                for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
                    Msg.AddInt(((int *)&p)[i]);
            }

            Server()->SendMsg(&Msg, 0, m_Owner);
        }
        
        if (m_Smoke && !Collide && (!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) || GameServer()->Collision()->CheckPoint(PrevPos) == false) && (Server()->Tick()-m_StartTick) % 2 == 0 )
            GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false, true);
    }
    else
    {
        Collide = GameServer()->Collision()->CheckPoint(CurPos);
        CCharacter *apEnts[MAX_CLIENTS] = {0};
        int Num = GameServer()->m_World.FindEntities(CurPos, 6.0f, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
        for(int i = 0; i < Num; i++)
        {
            if ( OwnerChar != apEnts[i] )
            {
                if (m_Type != WEAPON_RIFLE || !GameServer()->m_pController->IsTeamplay() ||
                !GameServer()->m_apPlayers[m_Owner] || GameServer()->m_apPlayers[m_Owner]->GetTeam() != apEnts[i]->GetPlayer()->GetTeam()
                || GameServer()->m_pEventsGame->GetActualEventTeam() == SHOTGUN_HEAL)
                TargetChr = apEnts[i];
                break;
            }
        }
        
        if (!Collide && m_Smoke)
            GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false, true);
    }

    if(TargetChr || TargetTurret || TargetExplodeWall || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
    {
        if(GameLayerClipped(CurPos))
            m_LifeSpan = -1;

        if(Collide && (!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) || GameServer()->Collision()->CheckPoint(PrevPos) == false))
            GameServer()->CreateSound(CurPos, m_SoundImpact);

        if(m_Explosive && (!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) || GameServer()->Collision()->CheckPoint(PrevPos) == false))
            GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false, false);

        if(TargetChr && TargetChr->IsAlive())
            TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon, false);

        if(TargetTurret)
            TargetTurret->TakeDamage(m_Damage, m_Owner, m_Weapon, false);

        if(TargetExplodeWall)
            TargetExplodeWall->TakeDamage(m_Damage, m_Owner, m_Weapon, false);

        if (m_LifeSpan < 0 || TargetExplodeWall || GameLayerClipped(CurPos))
        {
            if (GameServer()->m_apPlayers[m_Owner] && m_Limit)
                GameServer()->m_apPlayers[m_Owner]->m_Mine -= GameServer()->m_apPlayers[m_Owner]->m_Mine > 0 ? 1 : 0;
            GameServer()->m_World.DestroyEntity(this);
        }
        else if (GameServer()->m_pEventsGame->IsActualEvent(BULLET_BOUNCE) || m_Bounce)
        {
                vec2 TempPos(0.0f , 0.0f);
                GameServer()->Collision()->IntersectLine(PrevPos, GetPos(Ct), 0, &TempPos);
                vec2 TempDir = normalize(GetPos(Ct) - PrevPos);
                GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
                m_Direction = normalize(TempDir);
                m_Pos = TempPos;
                if (GameServer()->Collision()->CheckPoint(m_Pos))
                    m_Pos += m_Direction * 21.0f;
                m_StartTick = Server()->Tick();
                if (m_Bounce)
                    m_Bounce--;
        }
        else if ( GameServer()->m_pEventsGame->IsActualEvent(BULLET_GLUE) )
        {
            GameServer()->Collision()->IntersectLine(PrevPos, GetPos(Ct), 0, &m_Pos);
            m_StartTick = Server()->Tick();
            m_Direction = normalize(GetPos(Ct) - PrevPos);
        }
        else if (!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING))
        {
            if (GameServer()->m_apPlayers[m_Owner] && m_Limit)
                GameServer()->m_apPlayers[m_Owner]->m_Mine -= GameServer()->m_apPlayers[m_Owner]->m_Mine > 0 ? 1 : 0;
            GameServer()->m_World.DestroyEntity(this);
        }
    }
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
    pProj->m_X = (int)m_Pos.x;
    pProj->m_Y = (int)m_Pos.y;
    pProj->m_VelX = (int)(m_Direction.x*100.0f);
    pProj->m_VelY = (int)(m_Direction.y*100.0f);
    pProj->m_StartTick = m_StartTick;
    pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
    float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

    if(NetworkClipped(SnappingClient, GetPos(Ct)))
        return;

    CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
    if(pProj)
        FillInfo(pProj);
}

