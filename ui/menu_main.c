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

// menu_main.c -- the main menu & support functions
 
#include "../client/client.h"
#include "ui_local.h"

#define USE_MODELVIEW_CURSOR

/*
=======================================================================

MAIN MENU

=======================================================================
*/

#define MAIN_MENU_ITEM_SPACE 40

menuFramework_s			s_main_menu;
static menuImage_s		s_main_plaque;
static menuImage_s		s_main_logo;
static menuButton_s		s_main_game_button;
static menuButton_s		s_main_multiplayer_button;
static menuButton_s		s_main_options_button;
static menuButton_s		s_main_video_button;
static menuButton_s		s_main_mods_button;
static menuButton_s		s_main_quit_button;
#ifdef USE_MODELVIEW_CURSOR
static menuModelView_s	s_main_cursor;
#else
static menuImage_s		s_main_cursor;
#endif	// USE_MODELVIEW_CURSOR

//=======================================================================

static void M_MainGameFunc (void *unused)
{
	Menu_Game_f ();
}

static void M_MainMultiplayerFunc (void *unused)
{
	Menu_Multiplayer_f ();
}

static void M_MainOptionsFunc (void *unused)
{
	Menu_Options_f ();
}

static void M_MainVideoFunc (void *unused)
{
	Menu_Video_f ();
}

static void M_MainModsFunc (void *unused)
{
	Menu_Mods_f ();
}

static void M_MainQuitFunc (void *unused)
{
	Menu_Quit_f ();
}

//=======================================================================

#if 0
void M_DrawMainCursor (menuFramework_s *menu)
{
	menuCommon_s *item;
	char	cursorname[80];
	int		x, y, w, h;

	if (!menu)	return;
	item = UI_ItemAtMenuCursor(menu);
	if (!item)	return;

	x = menu->x + item->x - 25;
	y = menu->y + item->y + 1;
	Com_sprintf (cursorname, sizeof(cursorname), "/pics/m_cursor%d.pcx", (int)(cls.realtime/100)%NUM_MAINMENU_CURSOR_FRAMES);
	R_DrawGetPicSize (&w, &h, cursorname);
	UI_DrawPic (x, y, w, h, ALIGN_CENTER, false, cursorname, 1.0);
}
#endif

//=======================================================================

void Menu_Main_Init (void)
{
	int		x, y;

	// menu.x = 247, menu.y = 140
	x = 247;
	y = 143;

	s_main_menu.x							= 0;	// was 247
	s_main_menu.y							= 0;	// was 140
	s_main_menu.nitems						= 0;
	s_main_menu.hide_statusbar				= true;
	s_main_menu.isPopup						= false;
	s_main_menu.background					= NULL;
	s_main_menu.drawFunc					= UI_DefaultMenuDraw;
	s_main_menu.keyFunc						= UI_DefaultMenuKey;
	s_main_menu.canOpenFunc					= NULL;
//	s_main_menu.cursordraw					= M_DrawMainCursor;
	s_main_menu.cursorItem					= &s_main_cursor;

	s_main_game_button.generic.type			= MTYPE_BUTTON;
	s_main_game_button.generic.x			= x;
	s_main_game_button.generic.y			= y;
	s_main_game_button.width				= 99;
	s_main_game_button.height				= 32;
	s_main_game_button.imageName			= "/pics/m_main_game.pcx";
	s_main_game_button.hoverImageName		= "/pics/m_main_game_sel.pcx";
	s_main_game_button.alpha				= 255;
	s_main_game_button.border				= 0;
	s_main_game_button.generic.isHidden		= false;
	s_main_game_button.usesMouse2			= false;
	s_main_game_button.generic.callback		= M_MainGameFunc;
//	s_main_game_button.generic.statusbar	= "start a new game, or load/save a game";

	s_main_multiplayer_button.generic.type		= MTYPE_BUTTON;
	s_main_multiplayer_button.generic.x			= x;
	s_main_multiplayer_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_multiplayer_button.width				= 215;
	s_main_multiplayer_button.height			= 32;
	s_main_multiplayer_button.imageName			= "/pics/m_main_multiplayer.pcx";
	s_main_multiplayer_button.hoverImageName	= "/pics/m_main_multiplayer_sel.pcx";
	s_main_multiplayer_button.alpha				= 255;
	s_main_multiplayer_button.border			= 0;
	s_main_multiplayer_button.generic.isHidden	= false;
	s_main_multiplayer_button.usesMouse2		= false;
	s_main_multiplayer_button.generic.callback	= M_MainMultiplayerFunc;
//	s_main_multiplayer_button.generic.statusbar	= "join a server, start a server, or player setup";

	s_main_options_button.generic.type		= MTYPE_BUTTON;
	s_main_options_button.generic.x			= x;
	s_main_options_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_options_button.width				= 150;
	s_main_options_button.height			= 32;
	s_main_options_button.imageName			= "/pics/m_main_options.pcx";
	s_main_options_button.hoverImageName	= "/pics/m_main_options_sel.pcx";
	s_main_options_button.alpha				= 255;
	s_main_options_button.border			= 0;
	s_main_options_button.generic.isHidden	= false;
	s_main_options_button.usesMouse2		= false;
	s_main_options_button.generic.callback	= M_MainOptionsFunc;
//	s_main_options_button.generic.statusbar	= "change sound, controls, HUD, effects, and menu settings";

	s_main_video_button.generic.type		= MTYPE_BUTTON;
	s_main_video_button.generic.x			= x;
	s_main_video_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_video_button.width				= 118;
	s_main_video_button.height				= 32;
	s_main_video_button.imageName			= "/pics/m_main_video.pcx";
	s_main_video_button.hoverImageName		= "/pics/m_main_video_sel.pcx";
	s_main_video_button.alpha				= 255;
	s_main_video_button.border				= 0;
	s_main_video_button.generic.isHidden	= false;
	s_main_video_button.usesMouse2			= false;
	s_main_video_button.generic.callback	= M_MainVideoFunc;
//	s_main_video_button.generic.statusbar	= "change display/graphics settings";

	s_main_mods_button.generic.type			= MTYPE_BUTTON;
	s_main_mods_button.generic.x			= x;
	s_main_mods_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_mods_button.width				= 96;
	s_main_mods_button.height				= 32;
	s_main_mods_button.imageName			= "/pics/m_main_mods.pcx";
	s_main_mods_button.hoverImageName		= "/pics/m_main_mods_sel.pcx";
	s_main_mods_button.alpha				= 255;
	s_main_mods_button.border				= 0;
	s_main_mods_button.generic.isHidden		= false;
	s_main_mods_button.usesMouse2			= false;
	s_main_mods_button.generic.callback		= M_MainModsFunc;
//	s_main_mods_button.generic.statusbar	= "load a modification";

	s_main_quit_button.generic.type			= MTYPE_BUTTON;
	s_main_quit_button.generic.x			= x;
	s_main_quit_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_quit_button.width				= 100;
	s_main_quit_button.height				= 32;
	s_main_quit_button.imageName			= "/pics/m_main_quit.pcx";
	s_main_quit_button.hoverImageName		= "/pics/m_main_quit_sel.pcx";
	s_main_quit_button.alpha				= 255;
	s_main_quit_button.border				= 0;
	s_main_quit_button.generic.isHidden		= false;
	s_main_quit_button.usesMouse2			= false;
	s_main_quit_button.generic.callback		= M_MainQuitFunc;
//	s_main_quit_button.generic.statusbar	= "exit Quake II";

#ifdef USE_MODELVIEW_CURSOR
	s_main_cursor.generic.type				= MTYPE_MODELVIEW;
	s_main_cursor.generic.x					= 220; // -27
	s_main_cursor.generic.y					= 142; // -1
	s_main_cursor.generic.name				= 0;
	s_main_cursor.generic.isCursorItem		= true;
	Vector2Set (s_main_cursor.generic.cursorItemOffset, -27, -1);
	s_main_cursor.width						= 24;
	s_main_cursor.height					= 34;
	s_main_cursor.fov						= 40;
	s_main_cursor.isMirrored				= false;
	s_main_cursor.modelName[0]				= "models/ui/quad_cursor.md2";
	VectorSet (s_main_cursor.modelOrigin[0], 40, 0, -18);
	VectorSet (s_main_cursor.modelBaseAngles[0], 0, 0, 0);
	VectorSet (s_main_cursor.modelRotation[0], 0, 0.15, 0);
	s_main_cursor.modelFrame[0]				= 0;
	s_main_cursor.modelFrameNumbers[0]		= 0;
	s_main_cursor.entFlags[0]				= RF_FULLBRIGHT|RF_NOSHADOW|RF_DEPTHHACK;
	s_main_cursor.generic.isHidden			= false;
#else	// USE_MODELVIEW_CURSOR
	s_main_cursor.generic.type				= MTYPE_IMAGE;
	s_main_cursor.generic.x					= 220; // -27
	s_main_cursor.generic.y					= 142; // -1
	s_main_cursor.generic.name				= 0;
	s_main_cursor.generic.isCursorItem		= true;
	Vector2Set (s_main_cursor.generic.cursorItemOffset, -26, 1);
	s_main_cursor.width						= -1;	// 22
	s_main_cursor.height					= -1;	// 29
	s_main_cursor.animTemplate				= "/pics/m_cursor%d.pcx";
	s_main_cursor.numAnimFrames				= NUM_MAINMENU_CURSOR_FRAMES;
	s_main_cursor.animTimeScale				= 0.01f;
	s_main_cursor.alpha						= 255;
	s_main_cursor.border					= 0;
	s_main_cursor.hCentered					= false;
	s_main_cursor.vCentered					= false;
	s_main_cursor.generic.isHidden			= false;
#endif	// USE_MODELVIEW_CURSOR

	s_main_plaque.generic.type				= MTYPE_IMAGE;
	s_main_plaque.generic.x					= x - 69;
	s_main_plaque.generic.y					= 140;	// y-163, 0
	s_main_plaque.width						= 38;
	s_main_plaque.height					= 166;
	s_main_plaque.imageName					= "/pics/m_main_plaque.pcx";
	s_main_plaque.alpha						= 255;
	s_main_plaque.border					= 0;
	s_main_plaque.hCentered					= false;
	s_main_plaque.vCentered					= false;
	s_main_plaque.generic.isHidden			= false;

	s_main_logo.generic.type				= MTYPE_IMAGE;
	s_main_logo.generic.x					= x - 68;
	s_main_logo.generic.y					= 326; // y+23, 186
	s_main_logo.width						= 36;
	s_main_logo.height						= 42;
	s_main_logo.imageName					= "/pics/m_main_logo.pcx";
	s_main_logo.alpha						= 255;
	s_main_logo.border						= 0;
	s_main_logo.hCentered					= false;
	s_main_logo.vCentered					= false;
	s_main_logo.generic.isHidden			= false;

	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_game_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_multiplayer_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_options_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_video_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_mods_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_quit_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_cursor);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_plaque);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_logo);
}


void Menu_Main_f (void)
{
	Menu_Main_Init ();
	UI_PushMenu (&s_main_menu);
}
