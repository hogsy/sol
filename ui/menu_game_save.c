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
static menuListBox_s	s_savegame_gamelist;
static menuAction_s		s_savegame_save_action;
static menuImage_s		s_savegame_saveshot;
static menuAction_s		s_savegame_back_action;

//=========================================================

void M_SaveGameListCallback (void *self)
{
	s_savegame_saveshot.imageName = UI_UpdateSaveshot (s_savegame_gamelist.curValue + 1);
}

void M_SaveGameCallback (void *self)
{
	// check bounds
	if (s_savegame_gamelist.curValue < 0
		|| s_savegame_gamelist.curValue >= (UI_MAX_SAVEGAMES-1))
		return;

	Cbuf_AddText (va("save kmq2save%03i\n", s_savegame_gamelist.curValue+1 ));
	UI_ForceMenuOff ();
}

//=======================================================================

void Menu_SaveGame_Init (void)
{
	int		x, y;

	UI_UpdateSavegameData ();	// update savestrings and levelshots

	// menu.x = 80, menu.y = 162
	x = SCREEN_WIDTH*0.5 - 270;		// was - 240
	y = SCREEN_HEIGHT*0.5 - 78;		// was - 68

	s_savegame_menu.x				= 0;
	s_savegame_menu.y				= 0;
	s_savegame_menu.nitems			= 0;
	s_savegame_menu.isPopup			= false;
	s_savegame_menu.background		= NULL;
	s_savegame_menu.drawFunc		= UI_DefaultMenuDraw;
	s_savegame_menu.keyFunc			= UI_DefaultMenuKey;
	s_savegame_menu.canOpenFunc		= UI_CanOpenSaveMenu;
	s_savegame_menu.cantOpenMessage	= "You must be in a game to save";

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

	s_savegame_gamelist.generic.type		= MTYPE_LISTBOX;
	s_savegame_gamelist.generic.name		= "";
	s_savegame_gamelist.generic.header		= "";
	s_savegame_gamelist.generic.flags		= QMF_NOLOOP;
	s_savegame_gamelist.generic.x			= x - 4*MENU_FONT_SIZE;
	s_savegame_gamelist.generic.y			= y;
	s_savegame_gamelist.itemNames			= ui_savegame_names;
	s_savegame_gamelist.itemWidth			= 33;	// was 34
	s_savegame_gamelist.itemHeight			= 2;	// was 1
	s_savegame_gamelist.items_y				= 11;	// was 20
	s_savegame_gamelist.itemSpacing			= 0;
	s_savegame_gamelist.itemTextSize		= 8;
	s_savegame_gamelist.border				= 2;
	s_savegame_gamelist.borderColor[0]		= 60;
	s_savegame_gamelist.borderColor[1]		= 60;
	s_savegame_gamelist.borderColor[2]		= 60;
	s_savegame_gamelist.borderColor[3]		= 255;
	s_savegame_gamelist.backColor[0]		= 0;
	s_savegame_gamelist.backColor[1]		= 0;
	s_savegame_gamelist.backColor[2]		= 0;
	s_savegame_gamelist.backColor[3]		= 192;
	s_savegame_gamelist.altBackColor[0]		= 10;
	s_savegame_gamelist.altBackColor[1]		= 10;
	s_savegame_gamelist.altBackColor[2]		= 10;
	s_savegame_gamelist.altBackColor[3]		= 192;
	s_savegame_gamelist.generic.callback	= M_SaveGameListCallback;
	s_savegame_gamelist.generic.dblClkCallback = M_SaveGameCallback;
	s_savegame_gamelist.generic.statusbar	= "select a slot and click Save";
	s_savegame_gamelist.curValue			= 0;

//	x = SCREEN_WIDTH/2+46, y = SCREEN_HEIGHT/2-68, w = 240, h = 180
	s_savegame_saveshot.generic.type		= MTYPE_IMAGE;
	s_savegame_saveshot.generic.x			= x + 278; // was x + 286
	s_savegame_saveshot.generic.y			= y;
//	s_savegame_saveshot.generic.scrAlign	= ALIGN_STRETCH;
	s_savegame_saveshot.width				= 240;
	s_savegame_saveshot.height				= 180;
	s_savegame_saveshot.imageName			= UI_UpdateSaveshot (s_savegame_gamelist.curValue+1);
	s_savegame_saveshot.alpha				= 255;
	s_savegame_saveshot.border				= 2;
	s_savegame_saveshot.borderColor[0]		= 60;
	s_savegame_saveshot.borderColor[1]		= 60;
	s_savegame_saveshot.borderColor[2]		= 60;
	s_savegame_saveshot.borderColor[3]		= 255;
	s_savegame_saveshot.hCentered			= false;
	s_savegame_saveshot.vCentered			= false;
	s_savegame_saveshot.useScreenAspect		= true;
	s_savegame_saveshot.generic.isHidden	= false;

	s_savegame_save_action.generic.type				= MTYPE_ACTION;
	s_savegame_save_action.generic.name				= "Save";
	s_savegame_save_action.generic.flags			= QMF_LEFT_JUSTIFY;
	s_savegame_save_action.generic.x				= x;
	s_savegame_save_action.generic.y				= y += 23*MENU_LINE_SIZE;
	s_savegame_save_action.generic.callback			= M_SaveGameCallback;
	s_savegame_save_action.generic.statusbar		= "click to save in selected slot";

	s_savegame_back_action.generic.type				= MTYPE_ACTION;
	s_savegame_back_action.generic.textSize			= MENU_FONT_SIZE;
	s_savegame_back_action.generic.flags			= QMF_LEFT_JUSTIFY;
	s_savegame_back_action.generic.x				= x;
	s_savegame_back_action.generic.y				= y += 3*MENU_LINE_SIZE;
	s_savegame_back_action.generic.name				= "Back";
	s_savegame_back_action.generic.callback			= UI_BackMenu;

	UI_AddMenuItem (&s_savegame_menu, &s_savegame_banner);
	UI_AddMenuItem (&s_savegame_menu, &s_savegame_gamelist);
	UI_AddMenuItem (&s_savegame_menu, &s_savegame_saveshot);
	UI_AddMenuItem (&s_savegame_menu, &s_savegame_save_action);
	UI_AddMenuItem (&s_savegame_menu, &s_savegame_back_action);
}


void Menu_SaveGame_f (void)
{
	Menu_SaveGame_Init ();
	UI_PushMenu (&s_savegame_menu);
}
