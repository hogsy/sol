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

// menu_options_keys.c -- the key binding menu

#include "../client/client.h"
#include "ui_local.h"

#define USE_BINDLIST	// enable to use new keyBindList control

/*
=======================================================================

KEYS MENU

=======================================================================
*/

#define UI_KEYBIND_LIST_HEIGHT 26
#define UI_KEYBIND_LIST_WIDTH 48

#ifdef USE_BINDLIST
static keyBindSubitem_t ui_binds_vanilla[] = 
{
#include "menu_options_keys_baseq2.h"
};

static keyBindSubitem_t ui_binds_xatrix[] = 
{
#include "menu_options_keys_xatrix.h"
};

static keyBindSubitem_t ui_binds_rogue[] = 
{
#include "menu_options_keys_rogue.h"
};

static keyBindSubitem_t ui_binds_zaero[] = 
{
#include "menu_options_keys_zaero.h"
};
#else	// USE_BINDLIST
char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"+attack2", 		"alternate attack"},
{"+use", 			"activate"},
{"weapprev", 		"prev weapon"},
{"weapnext", 		"next weapon"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
{"+moveup",			"up / jump"},
{"+movedown",		"down / crouch"},
{"inven",			"inventory"},
{"invuse",			"use item"},
{"invdrop",			"drop item"},
{"invprev",			"prev item"},
{"invnext",			"next item"},
{"cmd help", 		"help computer" }, 
{ 0, 0 }
};
#endif	// USE_BINDLIST

static menuFramework_s	s_keys_menu;
static menuImage_s		s_keys_banner;
#ifdef USE_BINDLIST
static menuKeyBindList_s	s_keys_bindList;
#else
static menuKeyBind_s	s_keys_binds[64];
#endif	// USE_BINDLIST
static menuAction_s		s_keys_back_action;

//=======================================================================

static void Menu_Keys_Init (void)
{
	int		BINDS_MAX;
	int		i = 0, x, y;

	// menu.x = 320, menu.y = 162
#ifdef USE_BINDLIST
	x = SCREEN_WIDTH*0.5 - (UI_KEYBIND_LIST_WIDTH*MENU_FONT_SIZE + LIST_SCROLLBAR_CONTROL_SIZE) / 2 + 2*MENU_FONT_SIZE;
#else	// USE_BINDLIST
	x = SCREEN_WIDTH*0.5;
#endif	// USE_BINDLIST
	y = SCREEN_HEIGHT*0.5 - 78;

	s_keys_menu.x					= 0;
	s_keys_menu.y					= 0;
	s_keys_menu.nitems				= 0;
	s_keys_menu.isPopup				= false;
	s_keys_menu.background			= NULL;
	s_keys_menu.drawFunc			= UI_DefaultMenuDraw;
	s_keys_menu.keyFunc				= UI_DefaultMenuKey;
	s_keys_menu.canOpenFunc			= NULL;

	s_keys_banner.generic.type		= MTYPE_IMAGE;
	s_keys_banner.generic.x			= 0;
	s_keys_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_keys_banner.width				= 275;
	s_keys_banner.height			= 32;
	s_keys_banner.imageName			= "/pics/m_banner_customize.pcx";
	s_keys_banner.alpha				= 255;
	s_keys_banner.border			= 0;
	s_keys_banner.hCentered			= true;
	s_keys_banner.vCentered			= false;
	s_keys_banner.useAspectRatio	= false;
	s_keys_banner.generic.isHidden	= false;

#ifdef USE_BINDLIST
	BINDS_MAX = 26;
	s_keys_bindList.generic.type		= MTYPE_KEYBINDLIST;
	s_keys_bindList.generic.name		= "";
	s_keys_bindList.generic.header		= "";
	s_keys_bindList.generic.x			= x - 4*MENU_FONT_SIZE;
	s_keys_bindList.generic.y			= y;
	s_keys_bindList.useCustomBindList	= true;
	if ( FS_XatrixPath() )				// Xatrix
		s_keys_bindList.bindList		= ui_binds_xatrix;
	else if ( FS_RoguePath() )			// Rogue
		s_keys_bindList.bindList		= ui_binds_rogue;
	else if ( FS_ModType("zaero") )		// Zaero
		s_keys_bindList.bindList		= ui_binds_zaero;
	else								// Vanilla
		s_keys_bindList.bindList		= ui_binds_vanilla;
	s_keys_bindList.lineWidth			= UI_KEYBIND_LIST_WIDTH;
	s_keys_bindList.itemNameWidth		= UI_KEYBIND_LIST_WIDTH/2 - 1;
	s_keys_bindList.items_y				= UI_KEYBIND_LIST_HEIGHT;
	s_keys_bindList.itemSpacing			= 0;
	s_keys_bindList.itemTextSize		= 8;
	s_keys_bindList.border				= 2;
	s_keys_bindList.borderColor[0]		= 60;
	s_keys_bindList.borderColor[1]		= 60;
	s_keys_bindList.borderColor[2]		= 60;
	s_keys_bindList.borderColor[3]		= 255;
	s_keys_bindList.backColor[0]		= 0;
	s_keys_bindList.backColor[1]		= 0;
	s_keys_bindList.backColor[2]		= 0;
	s_keys_bindList.backColor[3]		= 192;
	s_keys_bindList.altBackColor[0]		= 10;
	s_keys_bindList.altBackColor[1]		= 10;
	s_keys_bindList.altBackColor[2]		= 10;
	s_keys_bindList.altBackColor[3]		= 192;
	s_keys_bindList.generic.statusbar	= "enter or mouse1 doubleclick to change, backspace or del to clear";
#else	// USE_BINDLIST
	BINDS_MAX = listSize(bindnames);
	for (i=0; i<BINDS_MAX; i++)
	{
		s_keys_binds[i].generic.type			= MTYPE_KEYBIND;
		s_keys_binds[i].generic.flags			= QMF_ALTCOLOR;
		s_keys_binds[i].generic.x				= x;
		s_keys_binds[i].generic.y				= y + i*MENU_LINE_SIZE;
		s_keys_binds[i].generic.name			= bindnames[i][1];
		s_keys_binds[i].generic.statusbar		= "enter or mouse1 to change, backspace or del to clear";
		s_keys_binds[i].commandName				= bindnames[i][0];
		s_keys_binds[i].enter_statusbar			= "press a key or button for this action";
	}
#endif	// USE_BINDLIST

	s_keys_back_action.generic.type		= MTYPE_ACTION;
	s_keys_back_action.generic.textSize	= MENU_FONT_SIZE;
	s_keys_back_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_keys_back_action.generic.x		= x;
	s_keys_back_action.generic.y		= y + (BINDS_MAX+2)*MENU_LINE_SIZE;
	s_keys_back_action.generic.name		= "Back";
	s_keys_back_action.generic.callback	= UI_BackMenu;

	UI_AddMenuItem (&s_keys_menu, (void *) &s_keys_banner);
#ifdef USE_BINDLIST
	UI_AddMenuItem (&s_keys_menu, (void *) &s_keys_bindList);
#else
	for (i=0; i<BINDS_MAX; i++)
		UI_AddMenuItem (&s_keys_menu, (void *) &s_keys_binds[i]);
#endif	// USE_BINDLIST
	UI_AddMenuItem (&s_keys_menu, (void *) &s_keys_back_action);
}


void Menu_Keys_f (void)
{
	Menu_Keys_Init ();
	UI_PushMenu (&s_keys_menu);
}
