/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.

This file is part of Quake 2 source code.

Quake 2 source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake 2 source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake 2 source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// menu_mp_playersetup.c -- the player setup menu 

#include "../client/client.h"
#include "ui_local.h"

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/
extern menuFramework_s	s_multiplayer_menu;

static menuFramework_s	s_player_config_menu;
static menuImage_s		s_playerconfig_banner;
static menuField_s		s_playerconfig_name_field;
static menuPicker_s		s_playerconfig_model_box;
static menuPicker_s		s_playerconfig_skin_box;
static menuPicker_s		s_playerconfig_handedness_box;
static menuPicker_s		s_playerconfig_rate_box;
static menuRectangle_s	s_playerconfig_railcolor_background;
static menuImage_s		s_playerconfig_railcolor_display[2];
static menuSlider_s		s_playerconfig_railcolor_slider[3];
static menuLabel_s		s_playerconfig_skin_title;
static menuLabel_s		s_playerconfig_model_title;
static menuLabel_s		s_playerconfig_hand_title;
static menuLabel_s		s_playerconfig_rate_title;
static menuLabel_s		s_playerconfig_railcolor_title;
static menuAction_s		s_playerconfig_back_action;
static menuModelView_s	s_playerconfig_model_display;

//=======================================================================

#define	NUM_SKINBOX_ITEMS 7

static void Menu_PlayerRateCallback (void *unused)
{
	MenuPicker_SaveValue (&s_playerconfig_rate_box, "rate");
}


static void Menu_LoadPlayerRailColor (void)
{
	color_t	railColor;

	if ( Com_ParseColorString(Cvar_VariableString("color1"), railColor) ) {
		Cvar_SetInteger ("ui_player_railred", railColor[0]);
		Cvar_SetInteger ("ui_player_railgreen", railColor[1]);
		Cvar_SetInteger ("ui_player_railblue", railColor[2]);
		s_playerconfig_railcolor_display[1].imageColor[0] = min(max(railColor[0], 0), 255);
		s_playerconfig_railcolor_display[1].imageColor[1] = min(max(railColor[1], 0), 255);
		s_playerconfig_railcolor_display[1].imageColor[2] = min(max(railColor[2], 0), 255);
	}
	MenuSlider_SetValue (&s_playerconfig_railcolor_slider[0], "ui_player_railred", 0, 256, true);
	MenuSlider_SetValue (&s_playerconfig_railcolor_slider[1], "ui_player_railgreen", 0, 256, true);
	MenuSlider_SetValue (&s_playerconfig_railcolor_slider[2], "ui_player_railblue", 0, 256, true);
}


static void Menu_SavePlayerRailColor (void)
{
	Cvar_Set ( "color1", va("%02X%02X%02X",
			min(max(Cvar_VariableInteger("ui_player_railred"), 0), 255),
			min(max(Cvar_VariableInteger("ui_player_railgreen"), 0), 255),
			min(max(Cvar_VariableInteger("ui_player_railblue"), 0), 255)) );
	s_playerconfig_railcolor_display[1].imageColor[0] = min(max(Cvar_VariableInteger("ui_player_railred"), 0), 255);
	s_playerconfig_railcolor_display[1].imageColor[1] = min(max(Cvar_VariableInteger("ui_player_railgreen"), 0), 255);
	s_playerconfig_railcolor_display[1].imageColor[2] = min(max(Cvar_VariableInteger("ui_player_railblue"), 0), 255);
}


static void Menu_PlayerRailColorRedFunc (void *unused)
{
	MenuSlider_SaveValue (&s_playerconfig_railcolor_slider[0], "ui_player_railred");
	Menu_SavePlayerRailColor ();
}


static void Menu_PlayerRailColorGreenFunc (void *unused)
{
	MenuSlider_SaveValue (&s_playerconfig_railcolor_slider[1], "ui_player_railgreen");
	Menu_SavePlayerRailColor ();
}


static void Menu_PlayerRailColorBlueFunc (void *unused)
{
	MenuSlider_SaveValue (&s_playerconfig_railcolor_slider[2], "ui_player_railblue");
	Menu_SavePlayerRailColor ();
}


static void Menu_PlayerModelCallback (void *unused)
{
	int		mNum, sNum;

	mNum = s_playerconfig_model_box.curValue;
	s_playerconfig_skin_box.itemNames = ui_pmi[mNum].skinDisplayNames;
	s_playerconfig_skin_box.curValue = 0;
	sNum = s_playerconfig_skin_box.curValue;

	UI_UpdatePlayerModelInfo (mNum, sNum);
}


static void Menu_PlayerSkinCallback (void *unused)
{
	int		mNum, sNum;

	mNum = s_playerconfig_model_box.curValue;
	sNum = s_playerconfig_skin_box.curValue;

	UI_UpdatePlayerSkinInfo (mNum, sNum);
}


static void Menu_PlayerHandednessCallback (void *unused)
{
	int			i;
	qboolean	lefthand;

	MenuPicker_SaveValue (&s_playerconfig_handedness_box, "hand");

	// update player model display
	lefthand = (Cvar_VariableValue("hand") == 1);
	s_playerconfig_model_display.isMirrored = lefthand;
	for (i=0; i<2; i++)
		VectorSet (s_playerconfig_model_display.modelRotation[i], 0, (lefthand ? -0.1 : 0.1), 0);
}


void Menu_PConfigSaveChanges (void *unused)
{
	int		mNum, sNum;
	char	scratch[1024];

	Cvar_Set ("name", s_playerconfig_name_field.buffer);

	mNum = s_playerconfig_model_box.curValue;
	sNum = s_playerconfig_skin_box.curValue;
	Com_sprintf (scratch, sizeof( scratch ), "%s/%s", 
		ui_pmi[mNum].directory, ui_pmi[mNum].skinDisplayNames[sNum]);
	Cvar_Set ("skin", scratch);
}

//=======================================================================

void Menu_PlayerConfig_Init (void)
{
	int			i, x, y, mNum = 0, sNum = 0;
	qboolean	lefthand = (Cvar_VariableValue("hand") == 1);

	static const char *handedness_names[] = { "right", "left", "center", 0 };

	static const char *rate_names[] =
	{
		"28.8 Modem",
		"33.6 Modem",
		"56K/Single ISDN",
		"Dual ISDN",
		"Cable/DSL",
		"T1/LAN",
		"User defined",
		0
	};
	static const char *rate_values[] =
	{
		"2500",
		"3200",
		"5000",
		"10000",
		"15000",
		"25000",
		UI_ITEMVALUE_WILDCARD,
		0
	};

	// get model and skin index and precache them
	UI_InitPlayerModelInfo (&mNum, &sNum);

	// menu.x = 46, menu.y = 170
	x = 110;					// SCREEN_WIDTH*0.5 - 210
	y = 17*MENU_LINE_SIZE;		// SCREEN_HEIGHT*0.5 - 70

	s_player_config_menu.x					= 0;	// SCREEN_WIDTH*0.5 - 210;
	s_player_config_menu.y					= 0;	// SCREEN_HEIGHT*0.5 - 70;
	s_player_config_menu.nitems				= 0;
	s_player_config_menu.isPopup			= false;
	s_player_config_menu.drawFunc			= UI_MenuPlayerConfig_Draw;	// UI_DefaultMenuDraw
	s_player_config_menu.keyFunc			= UI_DefaultMenuKey;
	s_player_config_menu.canOpenFunc		= UI_HaveValidPlayerModels;
	s_player_config_menu.cantOpenMessage	= "No valid player models found";
	s_player_config_menu.onExitFunc			= Menu_PConfigSaveChanges;
	
	s_playerconfig_banner.generic.type		= MTYPE_IMAGE;
	s_playerconfig_banner.generic.x			= 0;
	s_playerconfig_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_playerconfig_banner.width				= 275;
	s_playerconfig_banner.height			= 32;
	s_playerconfig_banner.imageName			= "/pics/m_banner_plauer_setup.pcx";
	s_playerconfig_banner.alpha				= 255;
	s_playerconfig_banner.border			= 0;
	s_playerconfig_banner.hCentered			= true;
	s_playerconfig_banner.vCentered			= false;
	s_playerconfig_banner.generic.isHidden	= false;

	s_playerconfig_name_field.generic.type		= MTYPE_FIELD;
	s_playerconfig_name_field.generic.textSize	= MENU_FONT_SIZE;
	s_playerconfig_name_field.generic.flags		= QMF_LEFT_JUSTIFY;
	s_playerconfig_name_field.generic.name		= "name";
	s_playerconfig_name_field.generic.callback	= 0;
	s_playerconfig_name_field.generic.x			= x + -MENU_FONT_SIZE;
	s_playerconfig_name_field.generic.y			= y;
	s_playerconfig_name_field.length			= 20;
	s_playerconfig_name_field.visible_length	= 20;
	Q_strncpyz (s_playerconfig_name_field.buffer, sizeof(s_playerconfig_name_field.buffer), Cvar_VariableString("name"));
	s_playerconfig_name_field.cursor = (int)strlen(s_playerconfig_name_field.buffer);

	s_playerconfig_model_title.generic.type		= MTYPE_LABEL;
	s_playerconfig_model_title.generic.textSize	= MENU_FONT_SIZE;
	s_playerconfig_model_title.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerconfig_model_title.generic.name		= "model";
	s_playerconfig_model_title.generic.x		= x + -2*MENU_FONT_SIZE;
	s_playerconfig_model_title.generic.y		= y += 3*MENU_LINE_SIZE;
	
	s_playerconfig_model_box.generic.type			= MTYPE_PICKER;
	s_playerconfig_model_box.generic.textSize		= MENU_FONT_SIZE;
	s_playerconfig_model_box.generic.x				= x + -8*MENU_FONT_SIZE;
	s_playerconfig_model_box.generic.y				= y += MENU_LINE_SIZE;
	s_playerconfig_model_box.generic.callback		= Menu_PlayerModelCallback;
	s_playerconfig_model_box.generic.cursor_offset	= -1*MENU_FONT_SIZE;
	s_playerconfig_model_box.curValue				= mNum;
	s_playerconfig_model_box.itemNames				= ui_pmnames;
	
	s_playerconfig_skin_title.generic.type		= MTYPE_LABEL;
	s_playerconfig_skin_title.generic.textSize	= MENU_FONT_SIZE;
	s_playerconfig_skin_title.generic.flags		= QMF_LEFT_JUSTIFY;
	s_playerconfig_skin_title.generic.name		= "skin";
	s_playerconfig_skin_title.generic.x			= x + -3*MENU_FONT_SIZE;
	s_playerconfig_skin_title.generic.y			= y += 2*MENU_LINE_SIZE;
	
	s_playerconfig_skin_box.generic.type			= MTYPE_PICKER;
	s_playerconfig_skin_box.generic.textSize		= MENU_FONT_SIZE;
	s_playerconfig_skin_box.generic.x				= x + -8*MENU_FONT_SIZE;
	s_playerconfig_skin_box.generic.y				= y += MENU_LINE_SIZE;
	s_playerconfig_skin_box.generic.name			= 0;
	s_playerconfig_skin_box.generic.callback		= Menu_PlayerSkinCallback; // Knightmare added, was 0
	s_playerconfig_skin_box.generic.cursor_offset	= -1*MENU_FONT_SIZE;
	s_playerconfig_skin_box.curValue				= sNum;
	s_playerconfig_skin_box.itemNames				= ui_pmi[mNum].skinDisplayNames;
	s_playerconfig_skin_box.generic.flags			|= QMF_SKINLIST;
	
	s_playerconfig_hand_title.generic.type		= MTYPE_LABEL;
	s_playerconfig_hand_title.generic.textSize	= MENU_FONT_SIZE;
	s_playerconfig_hand_title.generic.flags		= QMF_LEFT_JUSTIFY;
	s_playerconfig_hand_title.generic.name		= "handedness";
	s_playerconfig_hand_title.generic.x			= x + 3*MENU_FONT_SIZE;
	s_playerconfig_hand_title.generic.y			= y += 2*MENU_LINE_SIZE;
	
	s_playerconfig_handedness_box.generic.type			= MTYPE_PICKER;
	s_playerconfig_handedness_box.generic.textSize		= MENU_FONT_SIZE;
	s_playerconfig_handedness_box.generic.x				= x + -8*MENU_FONT_SIZE;
	s_playerconfig_handedness_box.generic.y				= y += MENU_LINE_SIZE;
	s_playerconfig_handedness_box.generic.name			= 0;
	s_playerconfig_handedness_box.generic.cursor_offset	= -1*MENU_FONT_SIZE;
	s_playerconfig_handedness_box.generic.callback		= Menu_PlayerHandednessCallback;
	s_playerconfig_handedness_box.itemNames				= handedness_names;
	MenuPicker_SetValue (&s_playerconfig_handedness_box, "hand", 0, 2, true);
			
	s_playerconfig_rate_title.generic.type		= MTYPE_LABEL;
	s_playerconfig_rate_title.generic.textSize	= MENU_FONT_SIZE;
	s_playerconfig_rate_title.generic.flags		= QMF_LEFT_JUSTIFY;
	s_playerconfig_rate_title.generic.name		= "connect speed";
	s_playerconfig_rate_title.generic.x			= x + 6*MENU_FONT_SIZE;
	s_playerconfig_rate_title.generic.y			= y += 2*MENU_LINE_SIZE;
		
	s_playerconfig_rate_box.generic.type			= MTYPE_PICKER;
	s_playerconfig_rate_box.generic.textSize		= MENU_FONT_SIZE;
	s_playerconfig_rate_box.generic.x				= x + -8*MENU_FONT_SIZE;
	s_playerconfig_rate_box.generic.y				= y += MENU_LINE_SIZE;
	s_playerconfig_rate_box.generic.name			= 0;
	s_playerconfig_rate_box.generic.cursor_offset	= -1*MENU_FONT_SIZE;
	s_playerconfig_rate_box.generic.callback		= Menu_PlayerRateCallback;
	s_playerconfig_rate_box.itemNames				= rate_names;
	s_playerconfig_rate_box.itemValues				= rate_values;
	MenuPicker_SetValue (&s_playerconfig_rate_box, "rate", 0, 0, false);

	s_playerconfig_railcolor_title.generic.type		= MTYPE_LABEL;
	s_playerconfig_railcolor_title.generic.textSize	= MENU_FONT_SIZE;
	s_playerconfig_railcolor_title.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerconfig_railcolor_title.generic.name		= "railgun effect color";
	s_playerconfig_railcolor_title.generic.x		= x + 13*MENU_FONT_SIZE;
	s_playerconfig_railcolor_title.generic.y		= y += 2*MENU_LINE_SIZE;

	s_playerconfig_railcolor_background.generic.type		= MTYPE_RECTANGLE;
	s_playerconfig_railcolor_background.generic.x			= x + -6*MENU_FONT_SIZE;
	s_playerconfig_railcolor_background.generic.y			= y += 1.5*MENU_LINE_SIZE;
	s_playerconfig_railcolor_background.width				= 160;
	s_playerconfig_railcolor_background.height				= 40;
	s_playerconfig_railcolor_background.color[0]			= 0;
	s_playerconfig_railcolor_background.color[1]			= 0;
	s_playerconfig_railcolor_background.color[2]			= 0;
	s_playerconfig_railcolor_background.color[3]			= 255;
	s_playerconfig_railcolor_background.border				= 2;
	s_playerconfig_railcolor_background.borderColor[0]		= 60;
	s_playerconfig_railcolor_background.borderColor[1]		= 60;
	s_playerconfig_railcolor_background.borderColor[2]		= 60;
	s_playerconfig_railcolor_background.borderColor[3]		= 255;
	s_playerconfig_railcolor_background.hCentered			= false;
	s_playerconfig_railcolor_background.vCentered			= false;
	s_playerconfig_railcolor_background.generic.isHidden	= false;

	s_playerconfig_railcolor_display[0].generic.type		= MTYPE_IMAGE;
	s_playerconfig_railcolor_display[0].generic.x			= x + -6*MENU_FONT_SIZE;
	s_playerconfig_railcolor_display[0].generic.y			= y;
	s_playerconfig_railcolor_display[0].width				= 160;
	s_playerconfig_railcolor_display[0].height				= 40;
	s_playerconfig_railcolor_display[0].imageName			= UI_RAILCORE_PIC;
	s_playerconfig_railcolor_display[0].alpha				= 254;
	s_playerconfig_railcolor_display[0].border				= 0;
	s_playerconfig_railcolor_display[0].hCentered			= false;
	s_playerconfig_railcolor_display[0].vCentered			= false;
	s_playerconfig_railcolor_display[0].generic.isHidden	= false;

	s_playerconfig_railcolor_display[1].generic.type		= MTYPE_IMAGE;
	s_playerconfig_railcolor_display[1].generic.x			= x + -6*MENU_FONT_SIZE;
	s_playerconfig_railcolor_display[1].generic.y			= y;
	s_playerconfig_railcolor_display[1].width				= 160;
	s_playerconfig_railcolor_display[1].height				= 40;
	s_playerconfig_railcolor_display[1].imageName			= UI_RAILSPIRAL_PIC;
	s_playerconfig_railcolor_display[1].alpha				= 254;
	s_playerconfig_railcolor_display[1].overrideColor		= true;
	s_playerconfig_railcolor_display[1].imageColor[0]		= 255;
	s_playerconfig_railcolor_display[1].imageColor[1]		= 255;
	s_playerconfig_railcolor_display[1].imageColor[2]		= 255;
	s_playerconfig_railcolor_display[1].imageColor[3]		= 254;
	s_playerconfig_railcolor_display[1].border				= 0;
	s_playerconfig_railcolor_display[1].hCentered			= false;
	s_playerconfig_railcolor_display[1].vCentered			= false;
	s_playerconfig_railcolor_display[1].generic.isHidden	= false;

	s_playerconfig_railcolor_slider[0].generic.type			= MTYPE_SLIDER;
	s_playerconfig_railcolor_slider[0].generic.textSize		= MENU_FONT_SIZE;
	s_playerconfig_railcolor_slider[0].generic.x			= x + 0*MENU_FONT_SIZE;
	s_playerconfig_railcolor_slider[0].generic.y			= y += 4.5*MENU_LINE_SIZE;
	s_playerconfig_railcolor_slider[0].generic.name			= "red";
	s_playerconfig_railcolor_slider[0].generic.callback		= Menu_PlayerRailColorRedFunc;
	s_playerconfig_railcolor_slider[0].maxPos				= 64;
	s_playerconfig_railcolor_slider[0].baseValue			= 0.0f;
	s_playerconfig_railcolor_slider[0].increment			= 4.0f;
	s_playerconfig_railcolor_slider[0].displayAsPercent		= false;
	s_playerconfig_railcolor_slider[0].generic.statusbar	= "changes player's railgun particle effect red component";

	s_playerconfig_railcolor_slider[1].generic.type			= MTYPE_SLIDER;
	s_playerconfig_railcolor_slider[1].generic.textSize		= MENU_FONT_SIZE;
	s_playerconfig_railcolor_slider[1].generic.x			= x + 0*MENU_FONT_SIZE;
	s_playerconfig_railcolor_slider[1].generic.y			= y += MENU_LINE_SIZE;
	s_playerconfig_railcolor_slider[1].generic.name			= "green";
	s_playerconfig_railcolor_slider[1].generic.callback		= Menu_PlayerRailColorGreenFunc;
	s_playerconfig_railcolor_slider[1].maxPos				= 64;
	s_playerconfig_railcolor_slider[1].baseValue			= 0.0f;
	s_playerconfig_railcolor_slider[1].increment			= 4.0f;
	s_playerconfig_railcolor_slider[1].displayAsPercent		= false;
	s_playerconfig_railcolor_slider[1].generic.statusbar	= "changes player's railgun particle effect green component";

	s_playerconfig_railcolor_slider[2].generic.type			= MTYPE_SLIDER;
	s_playerconfig_railcolor_slider[2].generic.textSize		= MENU_FONT_SIZE;
	s_playerconfig_railcolor_slider[2].generic.x			= x + 0*MENU_FONT_SIZE;
	s_playerconfig_railcolor_slider[2].generic.y			= y += MENU_LINE_SIZE;
	s_playerconfig_railcolor_slider[2].generic.name			= "blue";
	s_playerconfig_railcolor_slider[2].generic.callback		= Menu_PlayerRailColorBlueFunc;
	s_playerconfig_railcolor_slider[2].maxPos				= 64;
	s_playerconfig_railcolor_slider[2].baseValue			= 0.0f;
	s_playerconfig_railcolor_slider[2].increment			= 4.0f;
	s_playerconfig_railcolor_slider[2].displayAsPercent		= false;
	s_playerconfig_railcolor_slider[2].generic.statusbar	= "changes player's railgun particle effect blue component";

	s_playerconfig_back_action.generic.type			= MTYPE_ACTION;
	s_playerconfig_back_action.generic.textSize		= MENU_FONT_SIZE;
	s_playerconfig_back_action.generic.name			= "Back to Multiplayer";
	s_playerconfig_back_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_playerconfig_back_action.generic.x			= x + -5*MENU_FONT_SIZE;
	s_playerconfig_back_action.generic.y			= y += 3*MENU_LINE_SIZE;
	s_playerconfig_back_action.generic.statusbar	= NULL;
	s_playerconfig_back_action.generic.callback		= UI_BackMenu;

	s_playerconfig_model_display.generic.type	= MTYPE_MODELVIEW;
	s_playerconfig_model_display.generic.x		= 0;
	s_playerconfig_model_display.generic.y		= 0;
	s_playerconfig_model_display.generic.name	= 0;
	s_playerconfig_model_display.width			= SCREEN_WIDTH;
	s_playerconfig_model_display.height			= SCREEN_HEIGHT;
	s_playerconfig_model_display.fov			= 50;
	s_playerconfig_model_display.isMirrored		= lefthand;
	s_playerconfig_model_display.modelName[0]	= ui_playerconfig_playermodelname;
	s_playerconfig_model_display.skinName[0]	= ui_playerconfig_playerskinname;
	s_playerconfig_model_display.modelName[1]	= ui_playerconfig_weaponmodelname;
	for (i=0; i<2; i++)
	{
		VectorSet (s_playerconfig_model_display.modelOrigin[i], 150, -25, 0);	// -8
		VectorSet (s_playerconfig_model_display.modelBaseAngles[i], 0, 0, 0);
		VectorSet (s_playerconfig_model_display.modelRotation[i], 0, (lefthand ? -0.1 : 0.1), 0);
		s_playerconfig_model_display.modelFrame[i]			= 0;
		s_playerconfig_model_display.modelFrameNumbers[i]	= 0;
	//	s_playerconfig_model_display.modelFrameTime[i]		= 0.01;
		s_playerconfig_model_display.entFlags[i]			= RF_FULLBRIGHT|RF_NOSHADOW|RF_DEPTHHACK;
	}
	s_playerconfig_model_display.generic.isHidden	= false;

	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_banner);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_name_field);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_model_title);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_model_box);
	if ( s_playerconfig_skin_box.itemNames )
	{
		UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_skin_title);
		UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_skin_box);
	}
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_hand_title);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_handedness_box);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_rate_title);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_rate_box);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_railcolor_title);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_railcolor_background);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_railcolor_display[0]);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_railcolor_display[1]);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_railcolor_slider[0]);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_railcolor_slider[1]);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_railcolor_slider[2]);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_back_action);
	UI_AddMenuItem (&s_player_config_menu, &s_playerconfig_model_display);

	// get color components from color1 cvar
	Menu_LoadPlayerRailColor ();
}

//=======================================================================

qboolean Menu_PlayerConfig_CheckIncrement (int dir, float x, float y, float w, float h)
{
	float min[2], max[2], x1, y1, w1, h1;
	char *sound = NULL;

	x1 = x;	y1 = y;	w1 = w;	h1 = h;
	SCR_ScaleCoords (&x1, &y1, &w1, &h1, ALIGN_CENTER);
	min[0] = x1;	max[0] = x1 + w1;
	min[1] = y1;	max[1] = y1 + h1;

	if ( (ui_mousecursor.x >= min[0]) && (ui_mousecursor.x <= max[0]) &&
		(ui_mousecursor.y >= min[1]) && (ui_mousecursor.y <= max[1]) &&
		!ui_mousecursor.buttonused[MOUSEBUTTON1] &&
		ui_mousecursor.buttonclicks[MOUSEBUTTON1]==1)
	{
		if (dir) // dir == 1 is left
		{
			if (s_playerconfig_skin_box.curValue > 0)
				s_playerconfig_skin_box.curValue--;
		}
		else
		{
			if (s_playerconfig_skin_box.curValue < ui_pmi[s_playerconfig_model_box.curValue].nskins)
				s_playerconfig_skin_box.curValue++;
		}

		sound = ui_menu_move_sound;
		ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
		ui_mousecursor.buttonclicks[MOUSEBUTTON1] = 0;

		if ( sound )
			S_StartLocalSound( sound );
		Menu_PlayerSkinCallback (NULL);

		return true;
	}
	return false;
}


void UI_MenuPlayerConfig_MouseClick (void)
{
	float	icon_x = SCREEN_WIDTH*0.5 - 5, // width - 325
			icon_y = SCREEN_HEIGHT - 108,
			icon_offset = 0;
	int		i, count;
	char	*sound = NULL;
	buttonmenuobject_t buttons[NUM_SKINBOX_ITEMS];

	for (i=0; i<NUM_SKINBOX_ITEMS; i++)
		buttons[i].index =- 1;

	if ( (ui_pmi[s_playerconfig_model_box.curValue].nskins < NUM_SKINBOX_ITEMS) || (s_playerconfig_skin_box.curValue < 4) )
		i = 0;
	else if (s_playerconfig_skin_box.curValue > ui_pmi[s_playerconfig_model_box.curValue].nskins-4)
		i = ui_pmi[s_playerconfig_model_box.curValue].nskins-NUM_SKINBOX_ITEMS;
	else
		i = s_playerconfig_skin_box.curValue-3;

	if (i > 0)
		if (Menu_PlayerConfig_CheckIncrement (1, icon_x-39, icon_y, 32, 32))
			return;

	for (count=0; count<NUM_SKINBOX_ITEMS; i++,count++)
	{
		if ( (i < 0) || (i >= ui_pmi[s_playerconfig_model_box.curValue].nskins) )
			continue;

		UI_AddButton (&buttons[count], i, icon_x+icon_offset, icon_y, 32, 32);
		icon_offset += 34;
	}

	icon_offset = NUM_SKINBOX_ITEMS*34;
	if (ui_pmi[s_playerconfig_model_box.curValue].nskins-i > 0)
		if (Menu_PlayerConfig_CheckIncrement (0, icon_x+icon_offset+5, icon_y, 32, 32))
			return;

	for (i=0; i<NUM_SKINBOX_ITEMS; i++)
	{
		if (buttons[i].index == -1)
			continue;

		if ( (ui_mousecursor.x >= buttons[i].min[0]) && (ui_mousecursor.x <= buttons[i].max[0]) &&
			(ui_mousecursor.y >= buttons[i].min[1]) && (ui_mousecursor.y <= buttons[i].max[1]) )
		{
			if (!ui_mousecursor.buttonused[MOUSEBUTTON1] && ui_mousecursor.buttonclicks[MOUSEBUTTON1]==1)
			{
				s_playerconfig_skin_box.curValue = buttons[i].index;

				sound = ui_menu_move_sound;
				ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
				ui_mousecursor.buttonclicks[MOUSEBUTTON1] = 0;

				if (sound)
					S_StartLocalSound (sound);
				Menu_PlayerSkinCallback (NULL);

				return;
			}
			break;
		}
	}
}


void Menu_PlayerConfig_DrawSkinSelection (void)
{
	char		scratch[MAX_QPATH];
	float		icon_x = SCREEN_WIDTH*0.5 - 5; // width - 325
	float		icon_y = SCREEN_HEIGHT - 108;
	float		icon_offset = 0;
	float		x, y, w, h;
	int			i, count, color[3];
	color_t		arrowColor;
	vec4_t		arrowTemp[2];

	CL_TextColor ((int)Cvar_VariableValue("alt_text_color"), &color[0], &color[1], &color[2]);
	Vector4Copy (stCoord_arrow_left, arrowTemp[0]);
	Vector4Copy (stCoord_arrow_right, arrowTemp[1]);

	if ( (ui_pmi[s_playerconfig_model_box.curValue].nskins < NUM_SKINBOX_ITEMS) || (s_playerconfig_skin_box.curValue < 4) )
		i = 0;
	else if ( s_playerconfig_skin_box.curValue > (ui_pmi[s_playerconfig_model_box.curValue].nskins - 4) )
		i = ui_pmi[s_playerconfig_model_box.curValue].nskins-NUM_SKINBOX_ITEMS;
	else
		i = s_playerconfig_skin_box.curValue - 3;

	// left arrow
	if (i > 0) {
		Vector4Set (arrowColor, color[0], color[1], color[2], 255);
	//	Com_sprintf (scratch, sizeof(scratch), "/gfx/ui/arrows/arrow_left.pcx");
	}
	else {
		Vector4Set (arrowColor, 150, 150, 150, 255);
		arrowTemp[0][1] += 0.25;
		arrowTemp[0][3] += 0.25;
	//	Com_sprintf (scratch, sizeof(scratch), "/gfx/ui/arrows/arrow_left_d.pcx");
	}
	UI_DrawPicST (icon_x-39, icon_y+2, 32, 32, arrowTemp[0], ALIGN_CENTER, false, arrowColor, UI_ARROWS_PIC);
//	UI_DrawPic (icon_x-39, icon_y+2, 32, 32,  ALIGN_CENTER, false, scratch, 1.0);

	// background
	UI_DrawFill (icon_x-3, icon_y-3, NUM_SKINBOX_ITEMS*34+4, 38, ALIGN_CENTER, false, 0, 0, 0, 255);
	if (R_DrawFindPic("/gfx/ui/widgets/listbox_background.pcx")) {
		x = icon_x-2;	y = icon_y-2;	w = NUM_SKINBOX_ITEMS * 34 + 2;	h = 36;
		UI_DrawTiledPic (x, y, w, h, ALIGN_CENTER, true, "/gfx/ui/widgets/listbox_background.pcx", 255);
	}
	else
		UI_DrawFill (icon_x-2, icon_y-2, NUM_SKINBOX_ITEMS*34+2, 36, ALIGN_CENTER, false, 60, 60, 60, 255);
		
	for (count=0; count<NUM_SKINBOX_ITEMS; i++,count++)
	{
		if ( (i < 0) || (i >= ui_pmi[s_playerconfig_model_box.curValue].nskins) )
			continue;

		Com_sprintf (scratch, sizeof(scratch), "/players/%s/%s_i.pcx", 
			ui_pmi[s_playerconfig_model_box.curValue].directory,
			ui_pmi[s_playerconfig_model_box.curValue].skinDisplayNames[i] );

		if (i == s_playerconfig_skin_box.curValue)
			UI_DrawFill (icon_x + icon_offset-1, icon_y-1, 34, 34, ALIGN_CENTER, false, color[0], color[1] ,color[2], 255);
		UI_DrawPic (icon_x + icon_offset, icon_y, 32, 32,  ALIGN_CENTER, false, scratch, 1.0);
		icon_offset += 34;
	}

	// right arrow
	icon_offset = NUM_SKINBOX_ITEMS*34;
	if ( ui_pmi[s_playerconfig_model_box.curValue].nskins-i > 0 ) {
		Vector4Set (arrowColor, color[0], color[1], color[2], 255);
	//	Com_sprintf (scratch, sizeof(scratch), "/gfx/ui/arrows/arrow_right.pcx");
	}
	else {
		Vector4Set (arrowColor, 150, 150, 150, 255);
		arrowTemp[1][1] += 0.25;
		arrowTemp[1][3] += 0.25;
	//	Com_sprintf (scratch, sizeof(scratch), "/gfx/ui/arrows/arrow_right_d.pcx");
	}
	UI_DrawPicST (icon_x+icon_offset+5, icon_y+2, 32, 32, arrowTemp[1], ALIGN_CENTER, false, arrowColor, UI_ARROWS_PIC);
//	UI_DrawPic (icon_x+icon_offset+5, icon_y+2, 32, 32,  ALIGN_CENTER, false, scratch, 1.0);
}

//=======================================================================

void UI_MenuPlayerConfig_Draw (menuFramework_s *menu)
{
	UI_AdjustMenuCursor (&s_player_config_menu, 1);
	UI_DrawMenu (&s_player_config_menu);

	// skin selection preview
	Menu_PlayerConfig_DrawSkinSelection ();
}


void Menu_PlayerConfig_f (void)
{
	UI_RefreshPlayerModels ();	// Reload player models if we recently downloaded anything
	Menu_PlayerConfig_Init ();
	UI_PushMenu (&s_player_config_menu);
}
