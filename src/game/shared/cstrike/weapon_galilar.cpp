//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponGalilAR C_WeaponGalilAR
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponGalilAR : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponGalilAR, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponGalilAR();

	virtual void Spawn();
	virtual void PrimaryAttack();

	virtual CSWeaponID GetCSWeaponID( void ) const		{ return WEAPON_GALILAR; }

private:

	CWeaponGalilAR( const CWeaponGalilAR & );

	void GalilFire( float flSpread );

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGalilAR, DT_WeaponGalilAR )

BEGIN_NETWORK_TABLE( CWeaponGalilAR, DT_WeaponGalilAR )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponGalilAR )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_galilar, CWeaponGalilAR );
#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS( weapon_galil, CWeaponGalilAR ); // for backwards compatibility
#endif
PRECACHE_WEAPON_REGISTER( weapon_galilar );



CWeaponGalilAR::CWeaponGalilAR()
{
}

void CWeaponGalilAR::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	// don't fire underwater
	if (pPlayer->GetWaterLevel() == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		return;
	}
	
	if ( !CSBaseGunFire( GetCSWpnData().m_flCycleTime, Primary_Mode ) )
		return;

	// CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
	pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	if (pPlayer->GetAbsVelocity().Length2D() > 5)
		pPlayer->KickBack (1.0, 0.45, 0.28, 0.045, 3.75, 3, 7);
	else if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		pPlayer->KickBack (1.2, 0.5, 0.23, 0.15, 5.5, 3.5, 6);
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		pPlayer->KickBack (0.6, 0.3, 0.2, 0.0125, 3.25, 2, 7);
	else
		pPlayer->KickBack (0.65, 0.35, 0.25, 0.015, 3.5, 2.25, 7);
}

void CWeaponGalilAR::Spawn()
{
	SetClassname( "weapon_galilar" ); // for backwards compatibility
	BaseClass::Spawn();
}