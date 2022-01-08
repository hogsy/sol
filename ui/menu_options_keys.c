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

#define USE_KEYBIND_CONTROL

/*
=======================================================================

KEYS MENU

=======================================================================
*/

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

static menuFramework_s	s_keys_menu;
static menuImage_s		s_keys_banner;
static menuKeyBind_s	s_keys_binds[64];
static menuAction_s		s_keys_back_action;

//=======================================================================

static void Menu_Keys_Init (void)
{
	int		BINDS_MAX;
	int		i = 0, x, y;

	// menu.x = 320, menu.y = 168
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 72;

	s_keys_menu.x			= 0;	// SCREEN_WIDTH*0.5;
	s_keys_menu.y			= 0;	// SCREEN_HEIGHT*0.5 - 72;
	s_keys_menu.nitems		= 0;
//	s_keys_menu.isPopup		= false;
//	s_keys_menu.keyFunc		= UI_DefaultMenuKey;
//	s_keys_menu.canOpenFunc	= NULL;

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
	s_keys_banner.generic.isHidden	= false;

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

	s_keys_back_action.generic.type		= MTYPE_ACTION;
	s_keys_back_action.generic.textSize	= MENU_FONT_SIZE;
	s_keys_back_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_keys_back_action.generic.x		= x;
	s_keys_back_action.generic.y		= y + (BINDS_MAX+2)*MENU_LINE_SIZE;
	s_keys_back_action.generic.name		= "Back";
	s_keys_back_action.generic.callback	= UI_BackMenu;

	UI_AddMenuItem (&s_keys_menu, (void *) &s_keys_banner);
	for (i=0;i<BINDS_MAX;i++)
		UI_AddMenuItem (&s_keys_menu, (void *) &s_keys_binds[i]);
	UI_AddMenuItem (&s_keys_menu, (void *) &s_keys_back_action);
}

static void Menu_Keys_Draw (void)
{
	UI_AdjustMenuCursor (&s_keys_menu, 1);
	UI_DrawMenu (&s_keys_menu);
}

static const char *Menu_Keys_Key (int key)
{
	return UI_DefaultMenuKey (&s_keys_menu, key);
}

void Menu_Keys_f (void)
{
	Menu_Keys_Init ();
	UI_PushMenu (&s_keys_menu, Menu_Keys_Draw, Menu_Keys_Key);
}
