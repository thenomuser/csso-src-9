//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VGUI panel which can edit player's loadout
//
//=============================================================================

// ffs make that thing work

#include "cbase.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include "ienginevgui.h"
#include "cs_gamerules.h"

using namespace vgui;

extern ConVar loadout_slot_m4_weapon;
extern ConVar loadout_slot_hkp2000_weapon;
extern ConVar loadout_slot_knife_weapon_ct;
extern ConVar loadout_slot_knife_weapon_t;
extern ConVar loadout_slot_fiveseven_weapon;
extern ConVar loadout_slot_tec9_weapon;
extern ConVar loadout_slot_mp7_weapon_ct;
extern ConVar loadout_slot_mp7_weapon_t;

class CLoadoutPanel : public Frame
{
	DECLARE_CLASS_SIMPLE( CLoadoutPanel, Frame );

	CLoadoutPanel();

	void OnLoadoutChanged();

private:
	ComboBox *m_pM4Slot;
	ComboBox *m_pHKP2000Slot;
	ComboBox *m_pKnifeCTSlot;
	ComboBox *m_pKnifeTSlot;
	ComboBox *m_pFiveSevenlot;
	ComboBox *m_pTec9lot;
	ComboBox *m_pMP7CTlot;
	ComboBox *m_pMP7Tlot;
};

CLoadoutPanel *loadoutPanel;

class LoadoutComboBox : public ComboBox
{
public:
	LoadoutComboBox( CLoadoutPanel *parent, const char *name, int numEntries, bool editable )
		: ComboBox( parent, name, numEntries, editable )
	{
		m_pBox = parent;
	}

	virtual void OnSetText( const wchar_t *newText )
	{
		ComboBox::OnSetText( newText );
		m_pBox->OnLoadoutChanged();
	}

private:
	CLoadoutPanel *m_pBox;
};


CLoadoutPanel::CLoadoutPanel()
	: BaseClass( NULL, "LoadoutPanel" )
{
	SetParent( enginevgui->GetPanel( PANEL_CLIENTDLL ) );
	SetSize( 640, 480 );
	SetVisible( true );

	/*SetSizeable( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );*/

	m_pM4Slot			= new LoadoutComboBox( this, "m4_slot",			2,			false );
	m_pHKP2000Slot		= new LoadoutComboBox( this, "hkp2000_slot",	2,			false );
	m_pKnifeCTSlot		= new LoadoutComboBox( this, "knife_ct_slot",	MAX_KNIVES,	false );
	m_pKnifeTSlot		= new LoadoutComboBox( this, "knife_t_slot",	MAX_KNIVES,	false );
	m_pFiveSevenlot		= new LoadoutComboBox( this, "fiveseven_slot",	2,			false );
	m_pTec9lot			= new LoadoutComboBox( this, "tec9_slot",		2,			false );
	m_pMP7CTlot			= new LoadoutComboBox( this, "mp7_ct_slot",		2,			false );
	m_pMP7Tlot			= new LoadoutComboBox( this, "mp7_t_slot",		2,			false );

	//SetScheme( scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" ) );
	LoadControlSettings( "resource/ui/loadout_panel.res" );

	m_pM4Slot->AddItem( "M4A4", NULL );
	m_pM4Slot->AddItem( "M4A1-S", NULL );

	m_pM4Slot->ActivateItem(loadout_slot_m4_weapon.GetInt());
}

void CLoadoutPanel::OnLoadoutChanged()
{

}

CON_COMMAND( showloadoutpanel, "Show loadout panel" )
{
	if ( !loadoutPanel )
		loadoutPanel = new CLoadoutPanel();

	loadoutPanel->Activate();
	loadoutPanel->SetVisible(true);
}