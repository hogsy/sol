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

#define USE_LISTBOX	// enable to use new listBox control

/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/

static menuFramework_s	s_joinserver_menu;
static menuImage_s		s_joinserver_banner;

#ifdef USE_LISTBOX
static menuComboBox_s	s_joinserver_compatibility_box;
#else	// USE_LISTBOX
static menuLabel_s		s_joinserver_compat_title;
static menuPicker_s		s_joinserver_compatibility_box;
#endif	// USE_LISTBOX

static menuAction_s		s_joinserver_search_action;
static menuAction_s		s_joinserver_address_book_action;

#ifdef USE_LISTBOX
static menuListBox_s	s_joinserver_serverlist;
static menuAction_s		s_joinserver_join_action;
#else	// USE_LISTBOX
static menuLabel_s		s_joinserver_server_title;
static menuAction_s		s_joinserver_server_actions[UI_MAX_LOCAL_SERVERS];
#endif	// USE_LISTBOX

static menuAction_s		s_joinserver_back_action;

//=========================================================

void M_JoinServerFunc (void *self)
{
#ifdef USE_LISTBOX
	UI_JoinServer (s_joinserver_serverlist.curValue);
#else	// USE_LISTBOX
	int	index = (menuAction_s *)self - s_joinserver_server_actions;

	UI_JoinServer (index);
#endif	// USE_LISTBOX
}


void AddressBookFunc (void *self)
{
	Menu_AddressBook_f ();
}


void M_SearchLocalGamesFunc (void *self)
{
	UI_SearchLocalGames ();
}

//=========================================================

void Menu_JoinServer_Init (void)
{
	static const char *compatibility_names[] =
	{
		"version 57 (KMQuake2)",
		"version 34 (stock Quake2)",
		0
	};
	int		x, y;
#ifndef USE_LISTBOX
	int		i;
#endif	// USE_LISTBOX

	// menu.x = 200, menu.y = 160
#ifdef USE_LISTBOX
	x = SCREEN_WIDTH*0.5 - 120;
#else	// USE_LISTBOX
	x = SCREEN_WIDTH*0.5 - 160;
#endif	// USE_LISTBOX
	y = SCREEN_HEIGHT*0.5 - 80;

	s_joinserver_menu.x				= 0;	// SCREEN_WIDTH*0.5 - 160;
	s_joinserver_menu.y				= 0;	// SCREEN_HEIGHT*0.5 - 80;
	s_joinserver_menu.nitems		= 0;
	s_joinserver_menu.isPopup		= false;
	s_joinserver_menu.drawFunc		= UI_DefaultMenuDraw;
	s_joinserver_menu.keyFunc		= UI_DefaultMenuKey;
	s_joinserver_menu.canOpenFunc	= NULL;
	s_joinserver_menu.onOpenFunc	= M_SearchLocalGamesFunc;

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

#ifdef USE_LISTBOX
	s_joinserver_compatibility_box.generic.type				= MTYPE_COMBOBOX;
	s_joinserver_compatibility_box.generic.header			= "client protocol compatibility";
	s_joinserver_compatibility_box.generic.x				= x - 16*MENU_FONT_SIZE;
	s_joinserver_compatibility_box.generic.y				= y += MENU_LINE_SIZE;
	s_joinserver_compatibility_box.items_y					= 2;
	s_joinserver_compatibility_box.itemWidth				= 26;
	s_joinserver_compatibility_box.itemSpacing				= 1;
	s_joinserver_compatibility_box.itemTextSize				= 8;
	s_joinserver_compatibility_box.border					= 1;
	s_joinserver_compatibility_box.borderColor[0]			= 60;
	s_joinserver_compatibility_box.borderColor[1]			= 60;
	s_joinserver_compatibility_box.borderColor[2]			= 60;
	s_joinserver_compatibility_box.borderColor[3]			= 255;
	s_joinserver_compatibility_box.backColor[0]				= 0;
	s_joinserver_compatibility_box.backColor[1]				= 0;
	s_joinserver_compatibility_box.backColor[2]				= 0;
	s_joinserver_compatibility_box.backColor[3]				= 192;
	s_joinserver_compatibility_box.itemNames				= compatibility_names;
	s_joinserver_compatibility_box.generic.cvar				= "cl_servertrick";
	s_joinserver_compatibility_box.generic.cvarClamp		= true;
	s_joinserver_compatibility_box.generic.cvarMin			= 0;
	s_joinserver_compatibility_box.generic.cvarMax			= 1;
	s_joinserver_compatibility_box.generic.statusbar		= "set to version 34 to join non-KMQuake2 servers";
#else	// USE_LISTBOX
	s_joinserver_compat_title.generic.type					= MTYPE_LABEL;
	s_joinserver_compat_title.generic.textSize				= MENU_FONT_SIZE;
	s_joinserver_compat_title.generic.name					= "client protocol compatibility";
	s_joinserver_compat_title.generic.x						= x + 200;
	s_joinserver_compat_title.generic.y						= y;

	s_joinserver_compatibility_box.generic.type				= MTYPE_PICKER;
	s_joinserver_compatibility_box.generic.textSize			= MENU_FONT_SIZE;
	s_joinserver_compatibility_box.generic.name				= "";
	s_joinserver_compatibility_box.generic.x				= x - 32;
	s_joinserver_compatibility_box.generic.y				= y += MENU_LINE_SIZE;
	s_joinserver_compatibility_box.itemNames				= compatibility_names;
	s_joinserver_compatibility_box.generic.cvar				= "cl_servertrick";
	s_joinserver_compatibility_box.generic.cvarClamp		= true;
	s_joinserver_compatibility_box.generic.cvarMin			= 0;
	s_joinserver_compatibility_box.generic.cvarMax			= 1;
	s_joinserver_compatibility_box.generic.statusbar		= "set to version 34 to join non-KMQuake2 servers";
	s_joinserver_compatibility_box.generic.cursor_offset	= 0;
#endif	// USE_LISTBOX

	s_joinserver_address_book_action.generic.type		= MTYPE_ACTION;
	s_joinserver_address_book_action.generic.textSize	= MENU_FONT_SIZE;
	s_joinserver_address_book_action.generic.name		= "Address Book";
	s_joinserver_address_book_action.generic.flags		= QMF_LEFT_JUSTIFY;
#ifdef USE_LISTBOX
	s_joinserver_address_book_action.generic.x			= x - 12*MENU_FONT_SIZE;
#else	// USE_LISTBOX
	s_joinserver_address_book_action.generic.x			= x;
#endif	// USE_LISTBOX
	s_joinserver_address_book_action.generic.y			= y += 2.5*MENU_LINE_SIZE;	// was 2*MENU_LINE_SIZE
	s_joinserver_address_book_action.generic.callback	= AddressBookFunc;

	s_joinserver_search_action.generic.type				= MTYPE_ACTION;
	s_joinserver_search_action.generic.textSize			= MENU_FONT_SIZE;
	s_joinserver_search_action.generic.name				= "Refresh Server List";
	s_joinserver_search_action.generic.flags			= QMF_LEFT_JUSTIFY;
#ifdef USE_LISTBOX
	s_joinserver_search_action.generic.x				= x - 12*MENU_FONT_SIZE;
#else	// USE_LISTBOX
	s_joinserver_search_action.generic.x				= x;
#endif	// USE_LISTBOX
	s_joinserver_search_action.generic.y				= y += 2*MENU_LINE_SIZE;
	s_joinserver_search_action.generic.callback			= M_SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar		= "search for servers";

#ifdef USE_LISTBOX
	s_joinserver_serverlist.generic.type			= MTYPE_LISTBOX;
	s_joinserver_serverlist.generic.header			= "connect to...";
	s_joinserver_serverlist.generic.x				= x - 16*MENU_FONT_SIZE;
	s_joinserver_serverlist.generic.y				= y += 3*MENU_LINE_SIZE;
	s_joinserver_serverlist.itemNames				= ui_serverlist_names;
	s_joinserver_serverlist.curValue				= -1;
	s_joinserver_serverlist.itemWidth				= 64;
	s_joinserver_serverlist.itemHeight				= 1;
	s_joinserver_serverlist.items_y					= 12;
	s_joinserver_serverlist.itemSpacing				= 0;
	s_joinserver_serverlist.itemTextSize			= 8;
	s_joinserver_serverlist.border					= 2;
	s_joinserver_serverlist.borderColor[0]			= 60;
	s_joinserver_serverlist.borderColor[1]			= 60;
	s_joinserver_serverlist.borderColor[2]			= 60;
	s_joinserver_serverlist.borderColor[3]			= 255;
	s_joinserver_serverlist.backColor[0]			= 0;
	s_joinserver_serverlist.backColor[1]			= 0;
	s_joinserver_serverlist.backColor[2]			= 0;
	s_joinserver_serverlist.backColor[3]			= 192;
	s_joinserver_serverlist.altBackColor[0]			= 10;
	s_joinserver_serverlist.altBackColor[1]			= 10;
	s_joinserver_serverlist.altBackColor[2]			= 10;
	s_joinserver_serverlist.altBackColor[3]			= 192;
	s_joinserver_serverlist.generic.dblClkCallback	= M_JoinServerFunc;
	s_joinserver_serverlist.generic.statusbar		= "select a server and click Join";

	s_joinserver_join_action.generic.type			= MTYPE_ACTION;
	s_joinserver_join_action.generic.name			= "Join";
	s_joinserver_join_action.generic.flags			= QMF_LEFT_JUSTIFY;
	s_joinserver_join_action.generic.x				= x - 12*MENU_FONT_SIZE;
	s_joinserver_join_action.generic.y				= y += (UI_MAX_LOCAL_SERVERS+1)*MENU_LINE_SIZE;
	s_joinserver_join_action.generic.callback		= M_JoinServerFunc;
	s_joinserver_join_action.generic.statusbar		= "click to connect to selected server";
#else	// USE_LISTBOX
	s_joinserver_server_title.generic.type			= MTYPE_LABEL;
	s_joinserver_server_title.generic.textSize		= MENU_FONT_SIZE;
	s_joinserver_server_title.generic.name			= "connect to...";
	s_joinserver_server_title.generic.x				= x + 80;
	s_joinserver_server_title.generic.y				= y += 2*MENU_LINE_SIZE;

	y += MENU_LINE_SIZE;
	for ( i = 0; i < UI_MAX_LOCAL_SERVERS; i++ )
	{
		s_joinserver_server_actions[i].generic.type		= MTYPE_ACTION;
		s_joinserver_server_actions[i].generic.textSize	= MENU_FONT_SIZE;
		Q_strncpyz (ui_local_server_names[i], sizeof(ui_local_server_names[i]), NO_SERVER_STRING);
		s_joinserver_server_actions[i].generic.name	= ui_local_server_names[i];
		s_joinserver_server_actions[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_joinserver_server_actions[i].generic.x		= x;
		s_joinserver_server_actions[i].generic.y		= y + i*MENU_LINE_SIZE;
		s_joinserver_server_actions[i].generic.callback = M_JoinServerFunc;
		s_joinserver_server_actions[i].generic.statusbar = "press ENTER to connect";
	}
#endif	// USE_LISTBOX

	s_joinserver_back_action.generic.type			= MTYPE_ACTION;
	s_joinserver_back_action.generic.textSize		= MENU_FONT_SIZE;
	s_joinserver_back_action.generic.name			= "Back to Multiplayer";
	s_joinserver_back_action.generic.flags			= QMF_LEFT_JUSTIFY;
#ifdef USE_LISTBOX
	s_joinserver_back_action.generic.x				= x - 12*MENU_FONT_SIZE;
	s_joinserver_back_action.generic.y				= y += 3*MENU_LINE_SIZE;
#else	// USE_LISTBOX
	s_joinserver_back_action.generic.x				= x;
	s_joinserver_back_action.generic.y				= y += (UI_MAX_LOCAL_SERVERS+2)*MENU_LINE_SIZE;
#endif	// USE_LISTBOX
	s_joinserver_back_action.generic.callback		= UI_BackMenu;

	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_banner);
#ifndef USE_LISTBOX
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_compat_title);
#endif	// USE_LISTBOX
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_compatibility_box);
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_address_book_action);
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_search_action);
#ifdef USE_LISTBOX
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_serverlist);
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_join_action);
#else	// USE_LISTBOX
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_server_title);
	for ( i = 0; i < UI_MAX_LOCAL_SERVERS; i++ )
		UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_server_actions[i] );
#endif	// USE_LISTBOX
	UI_AddMenuItem (&s_joinserver_menu, &s_joinserver_back_action );
}


void Menu_JoinServer_f (void)
{
	Menu_JoinServer_Init ();
	UI_PushMenu (&s_joinserver_menu);
}
