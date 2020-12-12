//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h> // SRC only!!
#endif

#include "ModOptionsSubCrosshair.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/CheckButton.h>
#include "tier1/KeyValues.h"
#include <vgui_controls/Label.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/Cursor.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/MessageBox.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui_controls/MessageBox.h>

#include "CvarTextEntry.h"
#include "CvarToggleCheckButton.h"
#include "cvarslider.h"
#include "LabeledCommandComboBox.h"
#include "filesystem.h"
#include "EngineInterface.h"
#include "BitmapImagePanel.h"
#include "tier1/utlbuffer.h"
#include "ModInfo.h"
#include "tier1/convar.h"
#include "tier0/icommandline.h"

#ifdef WIN32
#include <io.h>
#endif

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;


#define DEFAULT_SUIT_HUE 30
#define DEFAULT_PLATE_HUE 6

void UpdateLogoWAD( void *hdib, int r, int g, int b );

struct ColorItem_t
{
	const char	*name;
	int			r, g, b;
};

static ColorItem_t s_crosshairColors[] = 
{
	{ "#Valve_Green",	50,		250,	50 },
	{ "#Valve_Red",		250,	50,		50 },
	{ "#Valve_Blue",	50,		50,		250 },
	{ "#Valve_Yellow",	250,	250,	50 },
	{ "#Valve_Ltblue",	50,		250,	250 },
};
static const int NumCrosshairColors = ARRAYSIZE(s_crosshairColors);


//-----------------------------------------------------------------------------
class CrosshairImagePanelCS : public CrosshairImagePanelBase
{
	DECLARE_CLASS_SIMPLE( CrosshairImagePanelCS, CrosshairImagePanelBase );

public:
	CrosshairImagePanelCS( Panel *parent, const char *name, CModOptionsSubCrosshair* pOptionsPanel );
	virtual void ResetData();
	virtual void ApplyChanges();

protected:
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data );
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC( OnCheckButtonChecked, "CheckButtonChecked" );

	virtual void Paint();
	void DrawCrosshairRect( int r, int g, int b, int a, int x0, int y0, int x1, int y1, bool bAdditive );
	void InitCrosshairColorEntries();
	void UpdateCrosshair();

private:
	CModOptionsSubCrosshair	*m_pOptionsPanel;
	vgui::ComboBox			*m_pColorComboBox;
	CLabeledCommandComboBox	*m_pStyleComboBox;
	CCvarToggleCheckButton	*m_pOutlineCheckbox;
	CCvarSlider				*m_pOutlineSlider;
	CCvarToggleCheckButton	*m_pDotCheckbox;
	CCvarToggleCheckButton	*m_pAlphaCheckbox;
	int m_R, m_G, m_B;
	float m_barSize;
	float m_barThickness;
	int m_iCrosshairTextureID;
};

//-----------------------------------------------------------------------------
CrosshairImagePanelCS::CrosshairImagePanelCS( Panel *parent, const char *name, CModOptionsSubCrosshair* pOptionsPanel ) : CrosshairImagePanelBase( parent, name )
{
	m_pOptionsPanel = pOptionsPanel;
	m_pColorComboBox = new ComboBox(m_pOptionsPanel, "CrosshairColorComboBox", NumCrosshairColors, false);
	m_pStyleComboBox = new CLabeledCommandComboBox( m_pOptionsPanel, "CrosshairStyleComboBox" );
	m_pOutlineCheckbox = new CCvarToggleCheckButton( m_pOptionsPanel, "CrosshairOutlineCheckBox", "#GameUI_Crosshair_Outline", "cl_crosshair_drawoutline" );
	m_pOutlineSlider = new CCvarSlider( m_pOptionsPanel, "CrosshairOutlineSlider", "#GameUI_Crosshair_Outline_Thickness", 0.1f, 3.0f, "cl_crosshair_outlinethickness" );
	m_pAlphaCheckbox = new CCvarToggleCheckButton( m_pOptionsPanel, "CrosshairAlphaCheckbox", "#GameUI_Crosshair_AlphaCheckBox", "cl_crosshairusealpha" );

	InitCrosshairColorEntries();

	// TODO: rewrite this shit
	m_pStyleComboBox->AddItem( "#GameUI_Crosshair_Style_0", "cl_crosshairstyle 0" );
	m_pStyleComboBox->AddItem( "#GameUI_Crosshair_Style_1", "cl_crosshairstyle 1" );
	m_pStyleComboBox->AddItem( "#GameUI_Crosshair_Style_2", "cl_crosshairstyle 2" );
	m_pStyleComboBox->AddItem( "#GameUI_Crosshair_Style_3", "cl_crosshairstyle 3" );
	m_pStyleComboBox->AddItem( "#GameUI_Crosshair_Style_4", "cl_crosshairstyle 4" );
	m_pStyleComboBox->AddItem( "#GameUI_Crosshair_Style_5", "cl_crosshairstyle 5" );

	m_pColorComboBox->AddActionSignalTarget( this );
	m_pStyleComboBox->AddActionSignalTarget( this );
	m_pOutlineCheckbox->AddActionSignalTarget( this );
	m_pOutlineSlider->AddActionSignalTarget( this );

	m_iCrosshairTextureID = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile( m_iCrosshairTextureID, "vgui/white_additive" , true, false);

	ResetData();
}

void CrosshairImagePanelCS::InitCrosshairColorEntries()
{
	if (m_pColorComboBox != NULL)
	{
		KeyValues *data = new KeyValues("data");

		// add in the "Default" selection
		data->Clear();

		// add in the colors for the color list
		for ( int i = 0; i < NumCrosshairColors; i++ )
		{
			data->SetInt("color", i);
			m_pColorComboBox->AddItem( s_crosshairColors[ i ].name, data);
		}

		data->SetInt("color", NumCrosshairColors);
		m_pColorComboBox->AddItem( "Custom", data);

		data->deleteThis();
	}
}

//-----------------------------------------------------------------------------
void CrosshairImagePanelCS::DrawCrosshairRect( int r, int g, int b, int a, int x0, int y0, int x1, int y1, bool bAdditive )
{
	if ( m_pOutlineCheckbox->IsSelected() )
	{
		float flThick = m_pOutlineSlider->GetSliderValue();
		vgui::surface()->DrawSetColor( 0, 0, 0, a );
		vgui::surface()->DrawFilledRect( x0-flThick, y0-flThick, x1+flThick, y1+flThick );
	}

	vgui::surface()->DrawSetColor( r, g, b, a );

	if ( bAdditive )
	{
		vgui::surface()->DrawTexturedRect( x0, y0, x1, y1 );
	}
	else
	{
		// Alpha-blended crosshair
		vgui::surface()->DrawFilledRect( x0, y0, x1, y1 );
	}
}

//-----------------------------------------------------------------------------
void CrosshairImagePanelCS::Paint()
{
	int screenWide, screenTall;
	surface()->GetScreenSize( screenWide, screenTall );;

	BaseClass::Paint();

	int wide, tall;
	GetSize( wide, tall );

	bool bAdditive = !m_pAlphaCheckbox->IsSelected();
	bool bDynamic = m_pDynamicCheckbox->IsSelected();

	int a = 255;
	if ( !bAdditive )
		a = m_pColorAlphaSlider->GetSliderValue();

	vgui::surface()->DrawSetColor( m_R, m_G, m_B, a );

	if ( bAdditive )
	{
		vgui::surface()->DrawSetTexture( m_iCrosshairTextureID );
	}

	int centerX = wide / 2;
	int centerY = tall / 2;

	int iBarSize = RoundFloatToInt(m_barSize * screenTall / 480.0f);
	int iBarThickness = max(1, RoundFloatToInt(m_barThickness * (float)screenTall / 480.0f));

	float fBarGap = 4.0f;
	if ( bDynamic )
	{
		float curtime = system()->GetFrameTime();
		fBarGap *= (1.0f + cosf(curtime * 1.5f) * 0.5f);
	}

	int iBarGap = RoundFloatToInt(fBarGap * screenTall / 480.0f);

	// draw horizontal crosshair lines
	int iInnerLeft	= centerX - iBarGap - iBarThickness / 2;
	int iInnerRight	= iInnerLeft + 2 * iBarGap + iBarThickness;
	int iOuterLeft	= iInnerLeft - iBarSize;
	int iOuterRight	= iInnerRight + iBarSize;
	int y0 = centerY - iBarThickness / 2;
	int y1 = y0 + iBarThickness;
	DrawCrosshairRect( iOuterLeft, y0, iInnerLeft, y1, bAdditive );
	DrawCrosshairRect( iInnerRight, y0, iOuterRight, y1, bAdditive );

	// draw vertical crosshair lines
	int iInnerTop		= centerY - iBarGap - iBarThickness / 2;
	int iInnerBottom	= iInnerTop + 2 * iBarGap + iBarThickness;
	int iOuterTop		= iInnerTop - iBarSize;
	int iOuterBottom	= iInnerBottom + iBarSize;
	int x0 = centerX - iBarThickness / 2;
	int x1 = x0 + iBarThickness;
	DrawCrosshairRect( x0, iOuterTop, x1, iInnerTop, bAdditive );
	DrawCrosshairRect( x0, iInnerBottom, x1, iOuterBottom, bAdditive );

	// draw dot
	if ( m_pDotCheckbox->IsSelected() )
	{
		x0 = centerX - iBarThickness / 2;
		x1 = x0 + iBarThickness;
		y0 = centerY - iBarThickness / 2;
		y1 = y0 + iBarThickness;
		DrawCrosshairRect( x0, y0, x1, y1, bAdditive );
	}
}


//-----------------------------------------------------------------------------
// Purpose: takes the settings from the crosshair settings combo boxes and sliders
//          and apply it to the crosshair illustrations.
//-----------------------------------------------------------------------------
void CrosshairImagePanelCS::UpdateCrosshair()
{
	// get the color selected in the combo box.
	KeyValues *data = m_pColorComboBox->GetActiveItemUserData();
	int colorIndex = data->GetInt("color");
	colorIndex = clamp( colorIndex, 0, NumCrosshairColors + 1 );

	int actualVal = 0;
	int selectedColor = m_pColorComboBox->GetActiveItem();

	ConVarRef cl_crosshaircolor( "cl_crosshaircolor", true );
	if ( cl_crosshaircolor.IsValid() )
	{
		actualVal = clamp( cl_crosshaircolor.GetInt(), 0, NumCrosshairColors + 1 );
	}

	if ( selectedColor != NumCrosshairColors )	// not custom
	{
		m_R = s_crosshairColors[selectedColor].r;
		m_G = s_crosshairColors[selectedColor].g;
		m_B = s_crosshairColors[selectedColor].b;
		m_pColorRSlider->SetSliderValue(m_R);
		m_pColorGSlider->SetSliderValue(m_G);
		m_pColorBSlider->SetSliderValue(m_B);
	}
	else
	{
		m_R = clamp( m_pColorRSlider->GetSliderValue(), 0, 255 );
		m_G = clamp( m_pColorGSlider->GetSliderValue(), 0, 255 );
		m_B = clamp( m_pColorBSlider->GetSliderValue(), 0, 255 );
	}

	m_barSize = m_pSizeSlider->GetSliderValue();
	m_barThickness = m_pThicknessSlider->GetSliderValue();
}


void CrosshairImagePanelCS::OnSliderMoved(KeyValues *data)
{
	vgui::Panel* pPanel = static_cast<vgui::Panel*>(data->GetPtr("panel"));

	if ( pPanel == m_pColorRSlider || pPanel == m_pColorGSlider || pPanel == m_pColorBSlider )
	{
		m_pColorComboBox->ActivateItem(NumCrosshairColors);
	}
	m_pOptionsPanel->OnControlModified();

	UpdateCrosshair();
}


//-----------------------------------------------------------------------------
// Purpose: Called whenever color combo changes
//-----------------------------------------------------------------------------
void CrosshairImagePanelCS::OnTextChanged(vgui::Panel *panel)
{
	m_pOptionsPanel->OnControlModified();
	UpdateCrosshair();
}

void CrosshairImagePanelCS::OnCheckButtonChecked()
{
	m_pColorAlphaSlider->SetEnabled(m_pAlphaCheckbox->IsSelected());
	m_pOptionsPanel->OnControlModified();
	UpdateCrosshair();
}

void CrosshairImagePanelCS::ResetData()
{
	// parse the string for the custom color settings and get the initial settings.
	ConVarRef cl_crosshaircolor( "cl_crosshaircolor", true );
	int index = 0;
	if ( cl_crosshaircolor.IsValid() )
	{
		index = clamp( cl_crosshaircolor.GetInt(), 0, NumCrosshairColors + 1);
	}
	m_pColorComboBox->ActivateItemByRow(index);

	m_pAlphaCheckbox->Reset();
	m_pDynamicCheckbox->Reset();
	m_pDotCheckbox->Reset();
	m_pColorRSlider->Reset();
	m_pColorGSlider->Reset();
	m_pColorBSlider->Reset();
	m_pColorAlphaSlider->Reset();
	m_pSizeSlider->Reset();
	m_pThicknessSlider->Reset();

	UpdateCrosshair();
}

void CrosshairImagePanelCS::ApplyChanges()
{
	m_pAlphaCheckbox->ApplyChanges();
	m_pDynamicCheckbox->ApplyChanges();
	m_pDotCheckbox->ApplyChanges();
	m_pColorRSlider->ApplyChanges();
	m_pColorGSlider->ApplyChanges();
 	m_pColorBSlider->ApplyChanges();
	m_pColorAlphaSlider->ApplyChanges();
	m_pSizeSlider->ApplyChanges();
	m_pThicknessSlider->ApplyChanges();

	char cmd[256];
	cmd[0] = 0;

	if (m_pColorComboBox != NULL)
	{
		int val = m_pColorComboBox->GetActiveItem();
		Q_snprintf( cmd, sizeof(cmd), "cl_crosshaircolor %d\n", val );
		engine->ClientCmd_Unrestricted( cmd );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CModOptionsSubCrosshair::CModOptionsSubCrosshair(vgui::Panel *parent) : vgui::PropertyPage(parent, "ModOptionsSubCrosshair") 
{
	Button *cancel = new Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	Button *ok = new Button( this, "OK", "#GameUI_OK" );
	ok->SetCommand( "Ok" );

	Button *apply = new Button( this, "Apply", "#GameUI_Apply" );
	apply->SetCommand( "Apply" );

	m_pCrosshairImage = new CrosshairImagePanelCS( this, "CrosshairImage", this );

	//=========

	LoadControlSettings("Resource/ModOptionsSubCrosshair.res");

	// this is necessary because some of the game .res files don't have visiblity flags set up correctly for their controls
	if ( m_pCrosshairImage )
		m_pCrosshairImage->UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CModOptionsSubCrosshair::~CModOptionsSubCrosshair()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubCrosshair::OnControlModified()
{
	PostMessage(GetParent(), new KeyValues("ApplyButtonEnable"));
	InvalidateLayout();
}

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')
#define SUIT_HUE_START 192
#define SUIT_HUE_END 223
#define PLATE_HUE_START 160
#define PLATE_HUE_END 191

#ifdef POSIX 
typedef struct tagRGBQUAD { 
  uint8 rgbBlue;
  uint8 rgbGreen;
  uint8 rgbRed;
  uint8 rgbReserved;
} RGBQUAD;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void PaletteHueReplace( RGBQUAD *palSrc, int newHue, int Start, int end )
{
	int i;
	float r, b, g;
	float maxcol, mincol;
	float hue, val, sat;

	hue = (float)(newHue * (360.0 / 255));

	for (i = Start; i <= end; i++)
	{
		b = palSrc[ i ].rgbBlue;
		g = palSrc[ i ].rgbGreen;
		r = palSrc[ i ].rgbRed;
		
		maxcol = max( max( r, g ), b ) / 255.0f;
		mincol = min( min( r, g ), b ) / 255.0f;
		
		val = maxcol;
		sat = (maxcol - mincol) / maxcol;

		mincol = val * (1.0f - sat);

		if (hue <= 120)
		{
			b = mincol;
			if (hue < 60)
			{
				r = val;
				g = mincol + hue * (val - mincol)/(120 - hue);
			}
			else
			{
				g = val;
				r = mincol + (120 - hue)*(val-mincol)/hue;
			}
		}
		else if (hue <= 240)
		{
			r = mincol;
			if (hue < 180)
			{
				g = val;
				b = mincol + (hue - 120)*(val-mincol)/(240 - hue);
			}
			else
			{
				b = val;
				g = mincol + (240 - hue)*(val-mincol)/(hue - 120);
			}
		}
		else
		{
			g = mincol;
			if (hue < 300)
			{
				b = val;
				r = mincol + (hue - 240)*(val-mincol)/(360 - hue);
			}
			else
			{
				r = val;
				b = mincol + (360 - hue)*(val-mincol)/(hue - 240);
			}
		}

		palSrc[ i ].rgbBlue = (unsigned char)(b * 255);
		palSrc[ i ].rgbGreen = (unsigned char)(g * 255);
		palSrc[ i ].rgbRed = (unsigned char)(r * 255);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubCrosshair::OnResetData()
{
	if ( m_pCrosshairImage )
		m_pCrosshairImage->ResetData();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubCrosshair::OnApplyChanges()
{
	if ( m_pCrosshairImage != NULL )
		m_pCrosshairImage->ApplyChanges();
}

