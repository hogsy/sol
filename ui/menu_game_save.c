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

// menu_game_save.c -- the single player save menu

#include "../client/client.h"
#include "ui_local.h"

/*
=============================================================================

SAVEGAME MENU

=============================================================================
*/

static menuFramework_s	s_savegame_menu;
static menuImage_s		s_savegame_banner;
static menuAction_s		s_savegame_actions[UI_MAX_SAVEGAMES];
static menuImage_s		s_savegame_saveshot;
static menuAction_s		s_savegame_back_action;

//=========================================================

void M_SaveGameUpdateSaveshot (void)
{
	int		slotNum, shotNum;

	slotNum = s_savegame_menu.cursor - 1;	// index is shifted by 1 to skip banner
	if ( (slotNum < 0) || (slotNum >= UI_MAX_SAVEGAMES-1) )	// catch back action
		shotNum = UI_MAX_SAVEGAMES;
	else
		shotNum = s_savegame_actions[slotNum].generic.localdata[0];

	s_savegame_saveshot.imageName = UI_UpdateSaveshot (shotNum);
}

void M_SaveGameCallback (void *self)
{
	menuAction_s *a = (menuAction_s *) self;

	Cbuf_AddText (va("save kmq2save%03i\n", a->generic.localdata[0] ));
	UI_ForceMenuOff ();
}

//=======================================================================

void Menu_SaveGame_Init (void)
{
	int		i, x, y;

	UI_UpdateSavegameData ();

	// menu.x = 80, menu.y = 172
	x = SCREEN_WIDTH*0.5 - 240;
	y = SCREEN_HEIGHT*0.5 - 68;

	s_savegame_menu.x				= 0;	// SCREEN_WIDTH*0.5 - 240;
	s_savegame_menu.y				= 0;	// SCREEN_HEIGHT*0.5 - 68;
	s_savegame_menu.nitems			= 0;
//	s_savegame_menu.isPopup			= false;
//	s_savegame_menu.keyFunc			= UI_DefaultMenuKey;
//	s_savegame_menu.canOpenFunc		= UI_CanOpenSaveMenu;
//	s_savegame_menu.cantOpenMessage	= "You must be in a game to save";

	s_savegame_banner.generic.type		= MTYPE_IMAGE;
	s_savegame_banner.generic.x			= 0;
	s_savegame_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_savegame_banner.width				= 275;
	s_savegame_banner.height			= 32;
	s_savegame_banner.imageName			= "/pics/m_banner_save_game.pcx";
	s_savegame_banner.alpha				= 255;
	s_savegame_banner.border			= 0;
	s_savegame_banner.hCentered			= true;
	s_savegame_banner.vCentered			= false;
	s_savegame_banner.generic.isHidden	= false;

	// don't include the autosave slot
	for ( i = 0; i < UI_MAX_SAVEGAMES-1; i++ )
	{
		s_savegame_actions[i].generic.name = ui_savestrings[i+1];
		s_savegame_actions[i].generic.localdata[0] = i+1;
		s_savegame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_savegame_actions[i].generic.callback = M_SaveGameCallback;

		s_savegame_actions[i].generic.x = x;
		s_savegame_actions[i].generic.y = y + ( i ) * MENU_LINE_SIZE;

		s_savegame_actions[i].generic.type		= MTYPE_ACTION;
		s_savegame_actions[i].generic.textSize	= MENU_FONT_SIZE;
	}

//	x = SCREEN_WIDTH/2+46, y = SCREEN_HEIGHT/2-68, w = 240, h = 180
	s_savegame_saveshot.generic.type		= MTYPE_IMAGE;
	s_savegame_saveshot.generic.x			= x + 286;
	s_savegame_saveshot.generic.y			= y;
//	s_savegame_saveshot.generic.scrAlign	= ALIGN_STRETCH;
	s_savegame_saveshot.width				= 240;
	s_savegame_saveshot.height				= 180;
//	s_savegame_saveshot.imageName			= UI_UpdateSaveshot (s_savegame_gamelist.curValue+1);
	s_savegame_saveshot.alpha				= 255;
	s_savegame_saveshot.border				= 2;
	s_savegame_saveshot.borderColor[0]		= 60;
	s_savegame_saveshot.borderColor[1]		= 60;
	s_savegame_saveshot.borderColor[2]		= 60;
	s_savegame_saveshot.borderColor[3]		= 255;
	s_savegame_saveshot.hCentered			= false;
	s_savegame_saveshot.vCentered			= false;
	s_savegame_saveshot.generic.isHidden	= false;

	s_savegame_back_action.generic.type	= MTYPE_ACTION;
	s_savegame_back_action.generic.textSize	= MENU_FONT_SIZE;
	s_savegame_back_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_savegame_back_action.generic.x		= x;
	s_savegame_back_action.generic.y		= y + (UI_MAX_SAVEGAMES+1)*MENU_LINE_SIZE;
	s_savegame_back_action.generic.name		= "Back";
	s_savegame_back_action.generic.callback = UI_BackMenu;

	UI_AddMenuItem (&s_savegame_menu, &s_savegame_banner);
	for ( i = 0; i < UI_MAX_SAVEGAMES-1; i++ )
		UI_AddMenuItem (&s_savegame_menu, &s_savegame_actions[i]);
	UI_AddMenuItem (&s_savegame_menu, &s_savegame_saveshot);
	UI_AddMenuItem (&s_savegame_menu, &s_savegame_back_action);

	M_SaveGameUpdateSaveshot ();
}

void Menu_SaveGame_Draw (void)
{
	UI_AdjustMenuCursor (&s_savegame_menu, 1);
	M_SaveGameUpdateSaveshot ();
	UI_DrawMenu (&s_savegame_menu);
}

const char *Menu_SaveGame_Key (int key)
{
	return UI_DefaultMenuKey (&s_savegame_menu, key);
}

void Menu_SaveGame_f (void)
{
	if ( !Com_ServerState() )
		return;		// not playing a game

	Menu_SaveGame_Init ();
	UI_PushMenu (&s_savegame_menu, Menu_SaveGame_Draw, Menu_SaveGame_Key);
}
