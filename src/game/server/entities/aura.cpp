/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "aura.h"

CAura::CAura(CGameWorld *pGameWorld, CCharacter *Character, float StartDegres, int Type)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_pCharacter = Character;
	m_Type = Type;
	m_degres = StartDegres;

	m_ProximityRadius = PickupPhysSize;
	GameWorld()->InsertEntity(this);
}

void CAura::Reset()
{

}

void CAura::Tick()
{
	m_Pos = m_pCharacter->m_Pos + (GetDir(m_degres*M_PIl/180) * 62);
	if ( m_degres + 5 < 360 )
		m_degres += 5;
	else
		m_degres = 0;
}

void CAura::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;

	pP->m_Type = m_Type;
	pP->m_Subtype = 0;
}
