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

// menu_options.c -- the options menu

#include "../client/client.h"
#include "ui_local.h"

/*
=======================================================================

OPTIONS MENU

=======================================================================
*/

static menuFramework_s	s_options_menu;
static menuImage_s		s_options_banner;
static menuAction_s		s_options_sound_section;
static menuAction_s		s_options_controls_section;
static menuAction_s		s_options_screen_section;
static menuAction_s		s_options_effects_section;
static menuAction_s		s_options_interface_section;
static menuAction_s		s_options_back_action;

//=======================================================================

static void M_MenuSoundFunc (void *unused)
{
	Menu_Options_Sound_f ();
}

static void M_MenuControlsFunc (void *unused)
{
	Menu_Options_Controls_f ();
}

static void M_MenuScreenFunc (void *unused)
{
	Menu_Options_Screen_f ();
}

static void M_MenuEffectsFunc (void *unused)
{
	Menu_Options_Effects_f ();
}

static void M_MenuInterfaceFunc (void *unused)
{
	Menu_Options_Interface_f ();
}

//=======================================================================

void Menu_Options_Init (void)
{
	int		x, y;

	// menu.x = 304, menu.y = 198
	x = SCREEN_WIDTH*0.5 - 3*MENU_FONT_SIZE;
	y = SCREEN_HEIGHT*0.5 - 5*MENU_LINE_SIZE;

	s_options_menu.x			= 0;	// SCREEN_WIDTH*0.5 - 3*MENU_FONT_SIZE;
	s_options_menu.y			= 0;	// SCREEN_HEIGHT*0.5 - 5*MENU_LINE_SIZE;
	s_options_menu.nitems		= 0;
//	s_options_menu.isPopup		= false;
//	s_options_menu.keyFunc		= UI_DefaultMenuKey;
//	s_options_menu.canOpenFunc	= NULL;

	s_options_banner.generic.type		= MTYPE_IMAGE;
	s_options_banner.generic.x			= 0;
	s_options_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_options_banner.width				= 275;
	s_options_banner.height				= 32;
	s_options_banner.imageName			= "/pics/m_banner_options.pcx";
	s_options_banner.alpha				= 255;
	s_options_banner.border				= 0;
	s_options_banner.hCentered			= true;
	s_options_banner.vCentered			= false;
	s_options_banner.generic.isHidden	= false;

	s_options_sound_section.generic.type		= MTYPE_ACTION;
	s_options_sound_section.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_sound_section.generic.flags		= QMF_LEFT_JUSTIFY;
	s_options_sound_section.generic.name		= "Sound";
	s_options_sound_section.generic.x			= x;
	s_options_sound_section.generic.y			= y;	// MENU_FONT_SIZE * 2
	s_options_sound_section.generic.callback	= M_MenuSoundFunc;
	s_options_sound_section.generic.statusbar	= "change sound settings";
	
	s_options_controls_section.generic.type			= MTYPE_ACTION;
	s_options_controls_section.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_options_controls_section.generic.flags		= QMF_LEFT_JUSTIFY;
	s_options_controls_section.generic.name			= "Controls";
	s_options_controls_section.generic.x			= x;
	s_options_controls_section.generic.y			= y += 2*MENU_LINE_SIZE;	// MENU_FONT_SIZE * 4
	s_options_controls_section.generic.callback		= M_MenuControlsFunc;
	s_options_controls_section.generic.statusbar	= "change control settings and bind keys";
	
	s_options_screen_section.generic.type		= MTYPE_ACTION;
	s_options_screen_section.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_screen_section.generic.flags		= QMF_LEFT_JUSTIFY;
	s_options_screen_section.generic.name		= "Screen";
	s_options_screen_section.generic.x			= x;
	s_options_screen_section.generic.y			= y += 2*MENU_LINE_SIZE;	// MENU_FONT_SIZE * 6s
	s_options_screen_section.generic.callback	= M_MenuScreenFunc;
	s_options_screen_section.generic.statusbar	= "change HUD/crosshair settings";

	s_options_effects_section.generic.type		= MTYPE_ACTION;
	s_options_effects_section.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_effects_section.generic.flags		= QMF_LEFT_JUSTIFY;
	s_options_effects_section.generic.name		= "Effects";
	s_options_effects_section.generic.x			= x;
	s_options_effects_section.generic.y			= y += 2*MENU_LINE_SIZE;	// MENU_FONT_SIZE * 8
	s_options_effects_section.generic.callback	= M_MenuEffectsFunc;
	s_options_effects_section.generic.statusbar	= "change ingame effects settings";

	s_options_interface_section.generic.type		= MTYPE_ACTION;
	s_options_interface_section.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_interface_section.generic.flags		= QMF_LEFT_JUSTIFY;
	s_options_interface_section.generic.name		= "Interface";
	s_options_interface_section.generic.x			= x;
	s_options_interface_section.generic.y			= y += 2*MENU_LINE_SIZE;	// MENU_FONT_SIZE * 10
	s_options_interface_section.generic.callback	= M_MenuInterfaceFunc;
	s_options_interface_section.generic.statusbar	= "change menu/console settings";

	s_options_back_action.generic.type		= MTYPE_ACTION;
	s_options_back_action.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_back_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_options_back_action.generic.name		= "Back to Main";
	s_options_back_action.generic.x			= x;
	s_options_back_action.generic.y			= y += 3*MENU_HEADER_LINE_SIZE;	// MENU_FONT_SIZE * 13
	s_options_back_action.generic.callback	= UI_BackMenu;

	UI_AddMenuItem (&s_options_menu, (void *) &s_options_banner);
	UI_AddMenuItem (&s_options_menu,	(void *) &s_options_sound_section);
	UI_AddMenuItem (&s_options_menu,	(void *) &s_options_controls_section);
	UI_AddMenuItem (&s_options_menu,	(void *) &s_options_screen_section);
	UI_AddMenuItem (&s_options_menu,	(void *) &s_options_effects_section);
	UI_AddMenuItem (&s_options_menu,	(void *) &s_options_interface_section);
	UI_AddMenuItem (&s_options_menu,	(void *) &s_options_back_action);
}

void Menu_Options_Draw (void)
{
	UI_AdjustMenuCursor (&s_options_menu, 1);
	UI_DrawMenu (&s_options_menu);
}

const char *Menu_Options_Key (int key)
{
	return UI_DefaultMenuKey (&s_options_menu, key);
}

void Menu_Options_f (void)
{
	Menu_Options_Init ();
	UI_PushMenu (&s_options_menu, Menu_Options_Draw, Menu_Options_Key);
}
