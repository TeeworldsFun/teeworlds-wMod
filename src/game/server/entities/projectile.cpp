/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "game/server/event.h"
#include "projectile.h"

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
    float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
    float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
    vec2 PrevPos = GetPos(Pt);
    vec2 CurPos = GetPos(Ct);
    int Collide = distance(CurPos, PrevPos) ? GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0) : GameServer()->Collision()->CheckPoint(CurPos);
    CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
    CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);
    if ( !distance(CurPos, PrevPos) && !TargetChr )
    {
        CCharacter *apEnts[MAX_CLIENTS] = {0};
        int Num = GameServer()->m_World.FindEntities(CurPos, 6.0f, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
        for(int i = 0; i < Num; i++)
        {
            if ( OwnerChar != apEnts[i] && (!GameServer()->m_pController->IsTeamplay() || OwnerChar->GetPlayer()->GetTeam() != apEnts[i]->GetPlayer()->GetTeam()))
            {
                TargetChr = apEnts[i];
                break;
            }
        }
    }

    if (m_Smoke && (Server()->Tick()-m_StartTick) % 2 == 0 && (!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) || GameServer()->Collision()->CheckPoint(PrevPos) == false) && !Collide)
        GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false, true);

    m_LifeSpan--;

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
                                                 1, m_Explosive, 0, m_SoundImpact, m_Weapon, m_Smoke);

            // pack the Projectile and send it to the client Directly
            CNetObj_Projectile p;
            pProj->FillInfo(&p);

            for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
                Msg.AddInt(((int *)&p)[i]);
        }

        Server()->SendMsg(&Msg, 0, m_Owner);
    }

    if(TargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
    {
        if(Collide && (!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) || GameServer()->Collision()->CheckPoint(PrevPos) == false))
            GameServer()->CreateSound(CurPos, m_SoundImpact);

        if(m_Explosive && (!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) || GameServer()->Collision()->CheckPoint(PrevPos) == false))
            GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false, false);

        if(TargetChr && TargetChr->IsAlive())
            TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon, false);

        if ((!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) && !GameServer()->m_pEventsGame->IsActualEvent(BULLET_BOUNCE) && m_Bounce <= 0 && !GameServer()->m_pEventsGame->IsActualEvent(BULLET_GLUE)) || m_LifeSpan < 0)
        {
            if (GameServer()->m_apPlayers[m_Owner] && m_Limit)
                GameServer()->m_apPlayers[m_Owner]->m_Mine -= GameServer()->m_apPlayers[m_Owner]->m_Mine > 0 ? 1 : 0;
            GameServer()->m_World.DestroyEntity(this);
        }

        if ( Collide && (GameServer()->m_pEventsGame->IsActualEvent(BULLET_BOUNCE) || (m_Bounce && !GameServer()->m_pEventsGame->IsActualEvent(BULLET_GLUE))) )
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
        
        if ( Collide && GameServer()->m_pEventsGame->IsActualEvent(BULLET_GLUE) )
        {
            GameServer()->Collision()->IntersectLine(PrevPos, GetPos(Ct), 0, &m_Pos);
            m_StartTick = Server()->Tick();
            m_Direction = normalize(GetPos(Ct) - PrevPos);
        }
    }
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
