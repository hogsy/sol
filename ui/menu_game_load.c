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

// menu_game_load.c -- the single load menu

#include "../client/client.h"
#include "ui_local.h"

/*
=============================================================================

LOADGAME MENU

=============================================================================
*/

static menuFramework_s	s_loadgame_menu;
static menuImage_s		s_loadgame_banner;
static menuAction_s		s_loadgame_actions[UI_MAX_SAVEGAMES];
static menuImage_s		s_loadgame_saveshot;
static menuAction_s		s_loadgame_back_action;

extern	char *scr_load_saveshot;
char loadshotname[MAX_QPATH];

//=========================================================

void M_LoadGameUpdateSaveshot (void)
{
	int		slotNum, shotNum;

	slotNum = s_loadgame_menu.cursor - 1;	// index is shifted by 1 to skip banner
	if ( (slotNum < 0) || (slotNum >= UI_MAX_SAVEGAMES) )	// catch back action
		shotNum = UI_MAX_SAVEGAMES;
	else
		shotNum = s_loadgame_actions[slotNum].generic.localdata[0];

	s_loadgame_saveshot.imageName = UI_UpdateSaveshot (shotNum);
}

void M_LoadGameCallback (void *self)
{
	menuAction_s *a = (menuAction_s *) self;

	// set saveshot name here
	if ( ui_saveshotvalid[ a->generic.localdata[0] ] && (a->generic.localdata[0] != 0) )	// autosave has no saveshot, but uses levelshot instead
	{
	//	Com_sprintf(loadshotname, sizeof(loadshotname), "/save/kmq2save%03i/shot.jpg", a->generic.localdata[0]);
		Com_sprintf(loadshotname, sizeof(loadshotname), "/"SAVEDIRNAME"/kmq2save%03i/shot.jpg", a->generic.localdata[0]);
		scr_load_saveshot = loadshotname; }
	else {
		scr_load_saveshot = NULL;
	}

	if ( UI_SaveIsValid(a->generic.localdata[0]) ) {
		Cbuf_AddText (va("load kmq2save%03i\n",  a->generic.localdata[0] ) );
		UI_ForceMenuOff ();
	}
}

//=======================================================================

void Menu_LoadGame_Init (void)
{
	int		i, x, y;

	UI_UpdateSavegameData ();

	// menu.x = 80, menu.y = 172
	x = SCREEN_WIDTH*0.5 - 240;
	y = SCREEN_HEIGHT*0.5 - 68;

	s_loadgame_menu.x			= 0;	// SCREEN_WIDTH*0.5 - 240;
	s_loadgame_menu.y			= 0;	// SCREEN_HEIGHT*0.5 - 68;
	s_loadgame_menu.nitems		= 0;
	s_loadgame_menu.isPopup		= false;
	s_loadgame_menu.drawFunc	= UI_DefaultMenuDraw;
	s_loadgame_menu.keyFunc		= UI_DefaultMenuKey;
	s_loadgame_menu.canOpenFunc	= NULL;

	s_loadgame_banner.generic.type		= MTYPE_IMAGE;
	s_loadgame_banner.generic.x			= 0;
	s_loadgame_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_loadgame_banner.width				= 275;
	s_loadgame_banner.height			= 32;
	s_loadgame_banner.imageName			= "/pics/m_banner_load_game.pcx";
	s_loadgame_banner.alpha				= 255;
	s_loadgame_banner.border			= 0;
	s_loadgame_banner.hCentered			= true;
	s_loadgame_banner.vCentered			= false;
	s_loadgame_banner.generic.isHidden	= false;

	for (i = 0; i < UI_MAX_SAVEGAMES; i++)
	{
		s_loadgame_actions[i].generic.name			= ui_savestrings[i];
		s_loadgame_actions[i].generic.flags			= QMF_LEFT_JUSTIFY;
		s_loadgame_actions[i].generic.localdata[0]	= i;
		s_loadgame_actions[i].generic.callback		= M_LoadGameCallback;

		s_loadgame_actions[i].generic.x = x;
		s_loadgame_actions[i].generic.y = y + (i) * MENU_LINE_SIZE;
		if (i>0)	// separate from autosave
			s_loadgame_actions[i].generic.y += 10;

		s_loadgame_actions[i].generic.type			= MTYPE_ACTION;
		s_loadgame_actions[i].generic.textSize		= MENU_FONT_SIZE;
	}

//	x = SCREEN_WIDTH/2+46, y = SCREEN_HEIGHT/2-68, w = 240, h = 180
	s_loadgame_saveshot.generic.type		= MTYPE_IMAGE;
	s_loadgame_saveshot.generic.x			= x + 286;
	s_loadgame_saveshot.generic.y			= y;
//	s_loadgame_saveshot.generic.scrAlign	= ALIGN_STRETCH;
	s_loadgame_saveshot.width				= 240;
	s_loadgame_saveshot.height				= 180;
//	s_loadgame_saveshot.imageName			= UI_UpdateSaveshot (s_loadgame_gamelist.curValue);
	s_loadgame_saveshot.alpha				= 255;
	s_loadgame_saveshot.border				= 2;
	s_loadgame_saveshot.borderColor[0]		= 60;
	s_loadgame_saveshot.borderColor[1]		= 60;
	s_loadgame_saveshot.borderColor[2]		= 60;
	s_loadgame_saveshot.borderColor[3]		= 255;
	s_loadgame_saveshot.hCentered			= false;
	s_loadgame_saveshot.vCentered			= false;
	s_loadgame_saveshot.generic.isHidden	= false;

	s_loadgame_back_action.generic.type		= MTYPE_ACTION;
	s_loadgame_back_action.generic.textSize	= MENU_FONT_SIZE;
	s_loadgame_back_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_loadgame_back_action.generic.x		= x;
	s_loadgame_back_action.generic.y		= y + (UI_MAX_SAVEGAMES+3)*MENU_LINE_SIZE;
	s_loadgame_back_action.generic.name		= "Back";
	s_loadgame_back_action.generic.callback = UI_BackMenu;

	UI_AddMenuItem (&s_loadgame_menu, &s_loadgame_banner);
	for (i = 0; i < UI_MAX_SAVEGAMES; i++)
		UI_AddMenuItem (&s_loadgame_menu, &s_loadgame_actions[i]);
	UI_AddMenuItem (&s_loadgame_menu, &s_loadgame_saveshot);
	UI_AddMenuItem (&s_loadgame_menu, &s_loadgame_back_action);

	M_LoadGameUpdateSaveshot ();
}


void Menu_LoadGame_f (void)
{
	Menu_LoadGame_Init ();
	UI_PushMenu (&s_loadgame_menu);
}
