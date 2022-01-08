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

// menu_quit.c -- the quit menu

#include "../client/client.h"
#include "ui_local.h"

//#define QUITMENU_NOKEY

/*
=======================================================================

QUIT MENU

=======================================================================
*/

static menuFramework_s	s_quit_menu;

#ifdef QUITMENU_NOKEY
static menuLabel_s		s_quit_header;
static menuAction_s		s_quit_yes_action;
static menuAction_s		s_quit_no_action;
#else
static menuImage_s		s_quit_pic;
#endif // QUITMENU_NOKEY

//=======================================================================

static void QuitYesFunc (void *unused)
{
	cls.key_dest = key_console;
	CL_Quit_f ();
}

//=======================================================================

void Menu_Quit_Init (void)
{
	s_quit_menu.x			= SCREEN_WIDTH*0.5 - 24;
	s_quit_menu.y			= SCREEN_HEIGHT*0.5 - 58;
	s_quit_menu.nitems		= 0;
	s_quit_menu.nitems		= 0;
//	s_quit_menu.isPopup		= false;
//	s_quit_menu.canOpenFunc	= NULL;

#ifdef QUITMENU_NOKEY
	s_quit_menu.keyFunc		= UI_DefaultMenuKey;

	s_quit_header.generic.type		= MTYPE_LABEL;
	s_quit_header.generic.textSize	= MENU_FONT_SIZE;
	s_quit_header.generic.name		= "Quit game?";
	s_quit_header.generic.x			= MENU_FONT_SIZE*0.7 * (int)strlen(s_quit_header.generic.name);
	s_quit_header.generic.y			= 20;

	s_quit_yes_action.generic.type			= MTYPE_ACTION;
	s_quit_yes_action.generic.textSize		= MENU_FONT_SIZE;
	s_quit_yes_action.generic.flags			= QMF_LEFT_JUSTIFY;
	s_quit_yes_action.generic.x				= MENU_FONT_SIZE*3;
	s_quit_yes_action.generic.y				= 60;
	s_quit_yes_action.generic.name			= "yes";
	s_quit_yes_action.generic.callback		= QuitYesFunc;
	s_quit_yes_action.generic.cursor_offset	= -MENU_FONT_SIZE;

	s_quit_no_action.generic.type			= MTYPE_ACTION;
	s_quit_no_action.generic.textSize		= MENU_FONT_SIZE;
	s_quit_no_action.generic.flags			= QMF_LEFT_JUSTIFY;
	s_quit_no_action.generic.x				= MENU_FONT_SIZE*3;
	s_quit_no_action.generic.y				= 80;
	s_quit_no_action.generic.name			= "no";
	s_quit_no_action.generic.callback		= UI_BackMenu;
	s_quit_no_action.generic.cursor_offset	= -MENU_FONT_SIZE;

	UI_AddMenuItem (&s_quit_menu, (void *) &s_quit_header);
	UI_AddMenuItem (&s_quit_menu, (void *) &s_quit_yes_action);
	UI_AddMenuItem (&s_quit_menu, (void *) &s_quit_no_action);
#else // QUITMENU_NOKEY
//	s_quit_menu.keyFunc		= UI_QuitMenuKey;

	s_quit_pic.generic.type		= MTYPE_IMAGE;
	s_quit_pic.generic.x		= 0;
	s_quit_pic.generic.y		= 12*MENU_LINE_SIZE;
	s_quit_pic.width			= 320;
	s_quit_pic.height			= 240;
	s_quit_pic.imageName		= "/pics/quit.pcx";
	s_quit_pic.alpha			= 255;
	s_quit_pic.border			= 0;
	s_quit_pic.hCentered		= true;
	s_quit_pic.vCentered		= true;
	s_quit_pic.generic.isHidden	= false;

	UI_AddMenuItem (&s_quit_menu, (void *) &s_quit_pic);
#endif // QUITMENU_NOKEY

}


const char *Menu_Quit_Key (int key)
{
#ifdef QUITMENU_NOKEY
	return UI_DefaultMenuKey (&s_quit_menu, key);
#else // QUITMENU_NOKEY
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		UI_PopMenu ();
		break;

	case 'Y':
	case 'y':
		cls.key_dest = key_console;
		CL_Quit_f ();
		break;

	default:
		break;
	}
	return NULL;
#endif // QUITMENU_NOKEY
}


void Menu_Quit_Draw (void)
{
	UI_AdjustMenuCursor (&s_quit_menu, 1);
	UI_DrawMenu (&s_quit_menu);
}


void Menu_Quit_f (void)
{
	Menu_Quit_Init ();
	UI_PushMenu (&s_quit_menu, Menu_Quit_Draw, Menu_Quit_Key);
}
