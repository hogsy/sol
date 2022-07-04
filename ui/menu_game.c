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

// menu_game.c -- the single player menu and credits

#include "../client/client.h"
#include "ui_local.h"

/*
=============================================================================

GAME MENU

=============================================================================
*/

static menuFramework_s	s_game_menu;
static menuImage_s		s_game_banner;
static menuAction_s		s_easy_game_action;
static menuAction_s		s_medium_game_action;
static menuAction_s		s_hard_game_action;
static menuAction_s		s_nitemare_game_action;
static menuAction_s		s_load_game_action;
static menuAction_s		s_save_game_action;
static menuAction_s		s_credits_action;
static menuAction_s		s_game_back_action;

//=======================================================================

static void M_EasyGameFunc (void *data)
{
	Cvar_ForceSet ("skill", "0");
	UI_StartSPGame ();
}

static void M_MediumGameFunc (void *data)
{
	Cvar_ForceSet ("skill", "1");
	UI_StartSPGame ();
}

static void M_HardGameFunc (void *data)
{
	Cvar_ForceSet ("skill", "2");
	UI_StartSPGame ();
}

static void M_NitemareGameFunc (void *data)
{
	Cvar_ForceSet ("skill", "3");
	UI_StartSPGame ();
}

static void M_LoadGameFunc (void *unused)
{
	Menu_LoadGame_f ();
}

static void M_SaveGameFunc (void *unused)
{
	Menu_SaveGame_f ();
}

static void M_CreditsFunc (void *unused)
{
	Menu_Credits_f ();
}

//=======================================================================

void Menu_Game_Init (void)
{
	int		x, y;

	// menu.x = 296, menu.y = 190
	x = SCREEN_WIDTH*0.5 - 3*MENU_LINE_SIZE;
	y = SCREEN_HEIGHT*0.5 - 5*MENU_LINE_SIZE;

	s_game_menu.x			= 0;	// SCREEN_WIDTH*0.5 - 3*MENU_LINE_SIZE;
	s_game_menu.y			= 0;	// SCREEN_HEIGHT*0.5 - 5*MENU_LINE_SIZE;
	s_game_menu.nitems		= 0;
	s_game_menu.isPopup		= false;
	s_game_menu.drawFunc	= UI_DefaultMenuDraw;
	s_game_menu.keyFunc		= UI_DefaultMenuKey;
	s_game_menu.canOpenFunc	= NULL;

	s_game_banner.generic.type		= MTYPE_IMAGE;
	s_game_banner.generic.x			= 0;
	s_game_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_game_banner.width				= 275;
	s_game_banner.height			= 32;
	s_game_banner.imageName			= "/pics/m_banner_game.pcx";
	s_game_banner.alpha				= 255;
	s_game_banner.border			= 0;
	s_game_banner.hCentered			= true;
	s_game_banner.vCentered			= false;
	s_game_banner.generic.isHidden	= false;

	s_easy_game_action.generic.type			= MTYPE_ACTION;
	s_easy_game_action.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_easy_game_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_easy_game_action.generic.x			= x;
	s_easy_game_action.generic.y			= y; // 0
	s_easy_game_action.generic.name			= "Easy";
	s_easy_game_action.generic.callback		= M_EasyGameFunc;

	s_medium_game_action.generic.type		= MTYPE_ACTION;
	s_medium_game_action.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_medium_game_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_medium_game_action.generic.x			= x;
	s_medium_game_action.generic.y			= y += 1.5*MENU_LINE_SIZE;
	s_medium_game_action.generic.name		= "Medium";
	s_medium_game_action.generic.callback	= M_MediumGameFunc;

	s_hard_game_action.generic.type			= MTYPE_ACTION;
	s_hard_game_action.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_hard_game_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_hard_game_action.generic.x			= x;
	s_hard_game_action.generic.y			= y += 1.5*MENU_LINE_SIZE;
	s_hard_game_action.generic.name			= "Hard";
	s_hard_game_action.generic.callback		= M_HardGameFunc;

	s_nitemare_game_action.generic.type			= MTYPE_ACTION;
	s_nitemare_game_action.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_nitemare_game_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_nitemare_game_action.generic.x			= x;
	s_nitemare_game_action.generic.y			= y += 1.5*MENU_LINE_SIZE;
	s_nitemare_game_action.generic.name			= "Nightmare";
	s_nitemare_game_action.generic.callback		= M_NitemareGameFunc;

	s_load_game_action.generic.type			= MTYPE_ACTION;
	s_load_game_action.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_load_game_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_load_game_action.generic.x			= x;
	s_load_game_action.generic.y			= y += 2*MENU_HEADER_LINE_SIZE;
	s_load_game_action.generic.name			= "Load Game";
	s_load_game_action.generic.callback		= M_LoadGameFunc;

	s_save_game_action.generic.type			= MTYPE_ACTION;
	s_save_game_action.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_save_game_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_save_game_action.generic.x			= x;
	s_save_game_action.generic.y			= y += 1.5*MENU_LINE_SIZE;
	s_save_game_action.generic.name			= "Save Game";
	s_save_game_action.generic.callback		= M_SaveGameFunc;

	s_credits_action.generic.type			= MTYPE_ACTION;
	s_credits_action.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_credits_action.generic.flags			= QMF_LEFT_JUSTIFY;
	s_credits_action.generic.x				= x;
	s_credits_action.generic.y				= y += 2*MENU_HEADER_LINE_SIZE;
	s_credits_action.generic.name			= "Credits";
	s_credits_action.generic.callback		= M_CreditsFunc;

	s_game_back_action.generic.type			= MTYPE_ACTION;
	s_game_back_action.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_game_back_action.generic.flags		= QMF_LEFT_JUSTIFY;
	s_game_back_action.generic.x			= x;
	s_game_back_action.generic.y			= y += 3*MENU_HEADER_LINE_SIZE;
	s_game_back_action.generic.name			= "Back to Main";
	s_game_back_action.generic.callback		= UI_BackMenu;

	UI_AddMenuItem (&s_game_menu, (void *) &s_game_banner);
	UI_AddMenuItem (&s_game_menu, (void *) &s_easy_game_action);
	UI_AddMenuItem (&s_game_menu, (void *) &s_medium_game_action);
	UI_AddMenuItem (&s_game_menu, (void *) &s_hard_game_action);
	UI_AddMenuItem (&s_game_menu, (void *) &s_nitemare_game_action);

	UI_AddMenuItem (&s_game_menu, (void *) &s_load_game_action);
	UI_AddMenuItem (&s_game_menu, (void *) &s_save_game_action);

	UI_AddMenuItem (&s_game_menu, (void *) &s_credits_action);

	UI_AddMenuItem (&s_game_menu, (void *) &s_game_back_action);
}


void Menu_Game_f (void)
{
	Menu_Game_Init ();
	UI_PushMenu (&s_game_menu);
}
