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

// menu_game_credits.c -- the credits scroll

#include "../client/client.h"
#include "ui_local.h"

/*
=============================================================================

CREDITS MENU

=============================================================================
*/

static menuFramework_s	s_credits_menu;
static menuTextScroll_s	s_credits_textscroll;
static menuAction_s		s_credits_back_action;

//=======================================================================

static const char *ui_idcredits[] =
{
#include "menu_credits_id.h"
};

static const char *ui_xatrixcredits[] =
{
#include "menu_credits_xatrix.h"
};

static const char *ui_roguecredits[] =
{
#include "menu_credits_rogue.h"
};

//=======================================================================

void Menu_Credits_Init (void)
{
	s_credits_menu.x				= 0;
	s_credits_menu.y				= 0;
	s_credits_menu.nitems			= 0;
	s_credits_menu.hide_statusbar	= true;
//	s_credits_menu.isPopup			= false;
//	s_credits_menu.keyFunc			= UI_DefaultMenuKey;
//	s_credits_menu.canOpenFunc		= NULL;

	s_credits_textscroll.generic.type			= MTYPE_TEXTSCROLL;
	s_credits_textscroll.generic.x				= 0;
	s_credits_textscroll.generic.y				= 0;
	s_credits_textscroll.generic.textSize		= 10;
	s_credits_textscroll.width					= SCREEN_WIDTH;
	s_credits_textscroll.height					= SCREEN_HEIGHT;
	s_credits_textscroll.lineSize				= 12;
	s_credits_textscroll.time_scale				= 0.025f;
	s_credits_textscroll.fileName				= "credits";
	if ( FS_ModType("xatrix") )			// Xatrix
		s_credits_textscroll.scrollText			= ui_xatrixcredits;
	else if ( FS_ModType("rogue") )		// Rogue
		s_credits_textscroll.scrollText			= ui_roguecredits;
	else
		s_credits_textscroll.scrollText			= ui_idcredits;	
	s_credits_textscroll.generic.isHidden		= false;

	s_credits_back_action.generic.type			= MTYPE_ACTION;
	s_credits_back_action.generic.flags			= QMF_LEFT_JUSTIFY;
	s_credits_back_action.generic.x				= MENU_FONT_SIZE*6;
	s_credits_back_action.generic.y				= 460;
	s_credits_back_action.generic.name			= "Back";
	s_credits_back_action.generic.callback		= UI_BackMenu;

	UI_AddMenuItem (&s_credits_menu, (void *) &s_credits_textscroll);
	UI_AddMenuItem (&s_credits_menu, (void *) &s_credits_back_action);
}

void Menu_Credits_Draw (void)
{
	UI_AdjustMenuCursor (&s_credits_menu, 1);
	UI_DrawMenu (&s_credits_menu);
}

const char *Menu_Credits_Key (int key)
{
	return UI_DefaultMenuKey (&s_credits_menu, key);
}

void Menu_Credits_f (void)
{
	Menu_Credits_Init ();
	UI_PushMenu (&s_credits_menu, Menu_Credits_Draw, Menu_Credits_Key);
}
