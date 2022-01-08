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

// menu_mp_joinserver.c -- the join server menu 

#include "../client/client.h"
#include "ui_local.h"

static menuFramework_s	s_joinserver_menu;
static menuImage_s		s_joinserver_banner;
static menuLabel_s		s_joinserver_server_title;

// Knightmare- client compatibility option
static menuPicker_s		s_joinserver_compatibility_box;
static menuLabel_s		s_joinserver_compat_title;

static menuAction_s		s_joinserver_search_action;
static menuAction_s		s_joinserver_address_book_action;
static menuAction_s		s_joinserver_server_actions[UI_MAX_LOCAL_SERVERS];
static menuAction_s		s_joinserver_back_action;

/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/

// Knightmare- client compatibility option
static void ClientCompatibilityFunc (void *unused)
{
	UI_MenuPicker_SaveValue (&s_joinserver_compatibility_box, "cl_servertrick");
}

void JoinServerFunc (void *self)
{
	int	index = (menuAction_s *)self - s_joinserver_server_actions;

	UI_JoinServer (index);
}

void AddressBookFunc (void *self)
{
	Menu_AddressBook_f ();
}

void SearchLocalGamesFunc (void *self)
{
	UI_SearchLocalGames ();
}

//=======================================================================

// Knightmare- init client compatibility menu option
static void M_Joinserver_SetMenuItemValues (void)
{
	UI_MenuPicker_SetValue (&s_joinserver_compatibility_box, "cl_servertrick", 0, 1, true);
}

//=========================================================

void Menu_JoinServer_Init (void)
{
	static const char *compatibility_names[] =
	{
		"version 56 (KMQuake2)",
		"version 34 (stock Quake2)",
		0
	};
	int i, x, y;

//	int y = 0;
	// menu.x = 200, menu.y = 160
	x = SCREEN_WIDTH*0.5 - 160;
	y = SCREEN_HEIGHT*0.5 - 80;

	s_joinserver_menu.x				= 0;	// SCREEN_WIDTH*0.5 - 160;
	s_joinserver_menu.y				= 0;	// SCREEN_HEIGHT*0.5 - 80;
	s_joinserver_menu.nitems		= 0;
//	s_joinserver_menu.isPopup		= false;
//	s_joinserver_menu.keyFunc		= UI_DefaultMenuKey;
//	s_joinserver_menu.canOpenFunc	= NULL;
//	s_joinserver_menu.onOpenFunc	= M_SearchLocalGamesFunc;

	s_joinserver_banner.generic.type		= MTYPE_IMAGE;
	s_joinserver_banner.generic.x			= 0;
	s_joinserver_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_joinserver_banner.width				= 275;
	s_joinserver_banner.height				= 32;
	s_joinserver_banner.imageName			= "/pics/m_banner_join_server.pcx";
	s_joinserver_banner.alpha				= 255;
	s_joinserver_banner.border				= 0;
	s_joinserver_banner.hCentered			= true;
	s_joinserver_banner.vCentered			= false;
	s_joinserver_banner.generic.isHidden	= false;

	// init client compatibility menu option
	s_joinserver_compat_title.generic.type			= MTYPE_LABEL;
	s_joinserver_compat_title.generic.textSize		= MENU_FONT_SIZE;
	s_joinserver_compat_title.generic.name			= "client protocol compatibility";
	s_joinserver_compat_title.generic.x				= x + 200;
	s_joinserver_compat_title.generic.y				= y;

	s_joinserver_compatibility_box.generic.type				= MTYPE_PICKER;
	s_joinserver_compatibility_box.generic.textSize			= MENU_FONT_SIZE;
	s_joinserver_compatibility_box.generic.name				= "";
	s_joinserver_compatibility_box.generic.x				= x - 32;
	s_joinserver_compatibility_box.generic.y				= y += MENU_LINE_SIZE;
	s_joinserver_compatibility_box.generic.callback			= ClientCompatibilityFunc;
	s_joinserver_compatibility_box.itemNames				= compatibility_names;
	s_joinserver_compatibility_box.generic.statusbar		= "set to version 34 to join non-KMQuake2 servers";
	s_joinserver_compatibility_box.generic.cursor_offset	= 0;

	s_joinserver_address_book_action.generic.type		= MTYPE_ACTION;
	s_joinserver_address_book_action.generic.textSize	= MENU_FONT_SIZE;
	s_joinserver_address_book_action.generic.name		= "address book";
	s_joinserver_address_book_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_joinserver_address_book_action.generic.x			= x;
	s_joinserver_address_book_action.generic.y			= y += 2*MENU_LINE_SIZE;
	s_joinserver_address_book_action.generic.callback	= AddressBookFunc;

	s_joinserver_search_action.generic.type			= MTYPE_ACTION;
	s_joinserver_search_action.generic.textSize		= MENU_FONT_SIZE;
	s_joinserver_search_action.generic.name			= "refresh server list";
	s_joinserver_search_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_joinserver_search_action.generic.x			= x;
	s_joinserver_search_action.generic.y			= y += MENU_LINE_SIZE;
	s_joinserver_search_action.generic.callback		= SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar	= "search for servers";

	s_joinserver_server_title.generic.type		= MTYPE_LABEL;
	s_joinserver_server_title.generic.textSize	= MENU_FONT_SIZE;
	s_joinserver_server_title.generic.name		= "connect to...";
	s_joinserver_server_title.generic.x			= x + 80;
	s_joinserver_server_title.generic.y			= y += 2*MENU_LINE_SIZE;

	y += MENU_LINE_SIZE;
	for ( i = 0; i < UI_MAX_LOCAL_SERVERS; i++ )
	{
		s_joinserver_server_actions[i].generic.type	= MTYPE_ACTION;
		s_joinserver_server_actions[i].generic.textSize	= MENU_FONT_SIZE;
		Q_strncpyz (ui_local_server_names[i], sizeof(ui_local_server_names[i]), NO_SERVER_STRING);
		s_joinserver_server_actions[i].generic.name	= ui_local_server_names[i];
		s_joinserver_server_actions[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_joinserver_server_actions[i].generic.x		= x;
		s_joinserver_server_actions[i].generic.y		= y + i*MENU_LINE_SIZE;
		s_joinserver_server_actions[i].generic.callback = JoinServerFunc;
		s_joinserver_server_actions[i].generic.statusbar = "press ENTER to connect";
	}

	s_joinserver_back_action.generic.type = MTYPE_ACTION;
	s_joinserver_back_action.generic.textSize = MENU_FONT_SIZE;
	s_joinserver_back_action.generic.name	= "Back to Multiplayer";
	s_joinserver_back_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_back_action.generic.x	= x;
	s_joinserver_back_action.generic.y	= y += (UI_MAX_LOCAL_SERVERS+2)*MENU_LINE_SIZE;
	s_joinserver_back_action.generic.callback = UI_BackMenu;

	M_Joinserver_SetMenuItemValues (); // init item values

	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_banner);
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_compat_title);
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_compatibility_box);
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_address_book_action);
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_server_title);
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_search_action);
	for ( i = 0; i < UI_MAX_LOCAL_SERVERS; i++ )
		UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_server_actions[i] );
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_back_action );

	UI_SearchLocalGames ();
}

void Menu_JoinServer_Draw (void)
{
	UI_AdjustMenuCursor (&s_joinserver_menu, 1);
	UI_DrawMenu (&s_joinserver_menu);
}


const char *Menu_JoinServer_Key (int key)
{
	return UI_DefaultMenuKey (&s_joinserver_menu, key);
}

void Menu_JoinServer_f (void)
{
	Menu_JoinServer_Init ();
	UI_PushMenu (&s_joinserver_menu, Menu_JoinServer_Draw, Menu_JoinServer_Key);
}
