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

// menu_mp_download.c -- the autodownload options menu 

#include "../client/client.h"
#include "ui_local.h"

/*
=============================================================================

DOWNLOAD OPTIONS BOOK MENU

=============================================================================
*/
static menuFramework_s	s_downloadoptions_menu;
static menuImage_s		s_downloadoptions_banner;
static menuLabel_s		s_download_title;
static menuPicker_s		s_allow_download_box;

#ifdef USE_CURL	// HTTP downloading from R1Q2
static menuPicker_s		s_http_download_box;
static menuPicker_s		s_http_fallback_box;
#endif	// USE_CURL

static menuPicker_s		s_allow_download_maps_box;
static menuPicker_s		s_allow_download_textures_24bit_box;	// option to allow downloading 24-bit textures
static menuPicker_s		s_allow_download_models_box;
static menuPicker_s		s_allow_download_players_box;
static menuPicker_s		s_allow_download_sounds_box;

static menuAction_s		s_download_back_action;

//=======================================================================

static void AllowDownloadCallback (void *unused)
{
	MenuPicker_SaveValue (&s_allow_download_box, "allow_download");
}

#ifdef USE_CURL	// HTTP downloading from R1Q2
static void HTTPDownloadCallback (void *unused)
{
	MenuPicker_SaveValue (&s_http_download_box, "cl_http_downloads");
}

static void HTTPFallbackCallback (void *unused)
{
	MenuPicker_SaveValue (&s_http_fallback_box, "cl_http_fallback");
}
#endif	// USE_CURL

static void DownloadMapsCallback (void *unused)
{
	MenuPicker_SaveValue (&s_allow_download_maps_box, "allow_download_maps");
}

static void DownloadTextures24BitCallback (void *unused)
{
	MenuPicker_SaveValue (&s_allow_download_textures_24bit_box, "allow_download_textures_24bit");
}

static void DownloadPlayersCallback (void *unused)
{
	MenuPicker_SaveValue (&s_allow_download_players_box, "allow_download_players");
}

static void DownloadModelsCallback (void *unused)
{
	MenuPicker_SaveValue (&s_allow_download_models_box, "allow_download_models");
}

static void DownloadSoundsCallback (void *unused)
{
	MenuPicker_SaveValue (&s_allow_download_sounds_box, "allow_download_sounds");
}

//=======================================================================

static void M_Download_SetMenuItemValues (void)
{
	MenuPicker_SetValue (&s_allow_download_box, "allow_download", 0, 1, true);
#ifdef USE_CURL	// HTTP downloading from R1Q2
	MenuPicker_SetValue (&s_http_download_box, "cl_http_downloads", 0, 1, true);
	MenuPicker_SetValue (&s_http_fallback_box, "cl_http_fallback", 0, 1, true);
#endif	// USE_CURL
	MenuPicker_SetValue (&s_allow_download_maps_box, "allow_download_maps", 0, 1, true);
	MenuPicker_SetValue (&s_allow_download_textures_24bit_box, "allow_download_textures_24bit", 0, 1, true);
	MenuPicker_SetValue (&s_allow_download_players_box, "allow_download_players", 0, 1, true);
	MenuPicker_SetValue (&s_allow_download_models_box, "allow_download_models", 0, 1, true);
	MenuPicker_SetValue (&s_allow_download_sounds_box, "allow_download_sounds", 0, 1, true);
}

//=======================================================================

void Menu_DownloadOptions_Init (void)
{
	static const char *yes_no_names[] =
	{
		"no",
		"yes",
		0
	};
	int		x, y;
//	int y = 3*MENU_LINE_SIZE;	// 0

	// menu.x = 320, menu.y = 162
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 78;

	s_downloadoptions_menu.x			= 0;	// SCREEN_WIDTH*0.5;
	s_downloadoptions_menu.y			= 0;	// SCREEN_HEIGHT*0.5 - 58;
	s_downloadoptions_menu.nitems		= 0;
	s_downloadoptions_menu.isPopup		= false;
	s_downloadoptions_menu.keyFunc		= UI_DefaultMenuKey;
	s_downloadoptions_menu.canOpenFunc	= NULL;

	s_downloadoptions_banner.generic.type		= MTYPE_IMAGE;
	s_downloadoptions_banner.generic.x			= 0;
	s_downloadoptions_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_downloadoptions_banner.width				= 275;
	s_downloadoptions_banner.height				= 32;
	s_downloadoptions_banner.imageName			= "/pics/m_banner_multiplayer.pcx";
	s_downloadoptions_banner.alpha				= 255;
	s_downloadoptions_banner.border				= 0;
	s_downloadoptions_banner.hCentered			= true;
	s_downloadoptions_banner.vCentered			= false;
	s_downloadoptions_banner.generic.isHidden	= false;

	s_download_title.generic.type		= MTYPE_LABEL;
	s_download_title.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_download_title.generic.name		= "Download Options";
	s_download_title.generic.x			= x + MENU_HEADER_FONT_SIZE/2 * (int)strlen(s_download_title.generic.name); // was 48
	s_download_title.generic.y			= y;	// - 2*MENU_LINE_SIZE;

	s_allow_download_box.generic.type		= MTYPE_PICKER;
	s_allow_download_box.generic.textSize	= MENU_FONT_SIZE;
	s_allow_download_box.generic.x			= x;
	s_allow_download_box.generic.y			= y += 4*MENU_LINE_SIZE;
	s_allow_download_box.generic.name		= "allow downloading";
	s_allow_download_box.generic.callback	= AllowDownloadCallback;	// DownloadCallback
	s_allow_download_box.itemNames			= yes_no_names;
	s_allow_download_box.generic.statusbar	= "enable or disable all downloading";

#ifdef USE_CURL	// HTTP downloading from R1Q2
	s_http_download_box.generic.type		= MTYPE_PICKER;
	s_http_download_box.generic.textSize	= MENU_FONT_SIZE;
	s_http_download_box.generic.x			= x;
	s_http_download_box.generic.y			= y += MENU_LINE_SIZE;
	s_http_download_box.generic.name		= "HTTP downloading";
	s_http_download_box.generic.callback	= HTTPDownloadCallback;	// DownloadCallback
	s_http_download_box.itemNames			= yes_no_names;
	s_http_download_box.generic.statusbar	= "use HTTP downloading on supported servers";

	s_http_fallback_box.generic.type		= MTYPE_PICKER;
	s_http_fallback_box.generic.textSize	= MENU_FONT_SIZE;
	s_http_fallback_box.generic.x			= x;
	s_http_fallback_box.generic.y			= y += MENU_LINE_SIZE;
	s_http_fallback_box.generic.name		= "HTTP fallback";
	s_http_fallback_box.generic.callback	= HTTPFallbackCallback;	// DownloadCallback
	s_http_fallback_box.itemNames			= yes_no_names;
	s_http_fallback_box.generic.statusbar	= "enable to allow HTTP downloads to fall back to Q2Pro path and UDP";
#endif	// USE_CURL

	s_allow_download_maps_box.generic.type		= MTYPE_PICKER;
	s_allow_download_maps_box.generic.textSize	= MENU_FONT_SIZE;
	s_allow_download_maps_box.generic.x			= x;
	s_allow_download_maps_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_allow_download_maps_box.generic.name		= "maps/textures";
	s_allow_download_maps_box.generic.callback	= DownloadMapsCallback;	// DownloadCallback
	s_allow_download_maps_box.itemNames			= yes_no_names;
	s_allow_download_maps_box.generic.statusbar	= "enable to allow downloading of maps and textures";

	// Knightmare- option to allow downloading 24-bit textures
	s_allow_download_textures_24bit_box.generic.type		= MTYPE_PICKER;
	s_allow_download_textures_24bit_box.generic.textSize	= MENU_FONT_SIZE;
	s_allow_download_textures_24bit_box.generic.x			= x;
	s_allow_download_textures_24bit_box.generic.y			= y += MENU_LINE_SIZE;
	s_allow_download_textures_24bit_box.generic.name		= "24-bit textures";
	s_allow_download_textures_24bit_box.generic.callback	= DownloadTextures24BitCallback;	// DownloadCallback
	s_allow_download_textures_24bit_box.generic.statusbar	= "enable to allow downloading of JPG and TGA textures";
	s_allow_download_textures_24bit_box.itemNames			= yes_no_names;
	s_allow_download_textures_24bit_box.generic.statusbar	= "enable to allow downloading of JPG and TGA textures";

	s_allow_download_players_box.generic.type		= MTYPE_PICKER;
	s_allow_download_players_box.generic.textSize	= MENU_FONT_SIZE;
	s_allow_download_players_box.generic.x			= x;
	s_allow_download_players_box.generic.y			= y += MENU_LINE_SIZE;
	s_allow_download_players_box.generic.name		= "player models/skins";
	s_allow_download_players_box.generic.callback	= DownloadPlayersCallback;	// DownloadCallback
	s_allow_download_players_box.itemNames			= yes_no_names;
	s_allow_download_players_box.generic.statusbar	= "enable to allow downloading of player models";

	s_allow_download_models_box.generic.type		= MTYPE_PICKER;
	s_allow_download_models_box.generic.textSize	= MENU_FONT_SIZE;
	s_allow_download_models_box.generic.x			= x;
	s_allow_download_models_box.generic.y			= y += MENU_LINE_SIZE;
	s_allow_download_models_box.generic.name		= "models";
	s_allow_download_models_box.generic.callback	= DownloadModelsCallback;	// DownloadCallback
	s_allow_download_models_box.itemNames			= yes_no_names;
	s_allow_download_models_box.generic.statusbar	= "enable to allow downloading of models";

	s_allow_download_sounds_box.generic.type		= MTYPE_PICKER;
	s_allow_download_sounds_box.generic.textSize	= MENU_FONT_SIZE;
	s_allow_download_sounds_box.generic.x			= x;
	s_allow_download_sounds_box.generic.y			= y += MENU_LINE_SIZE;
	s_allow_download_sounds_box.generic.name		= "sounds";
	s_allow_download_sounds_box.generic.callback	= DownloadSoundsCallback;	// DownloadCallback
	s_allow_download_sounds_box.itemNames			= yes_no_names;
	s_allow_download_sounds_box.generic.statusbar	= "enable to allow downloading of sounds";

	s_download_back_action.generic.type			= MTYPE_ACTION;
	s_download_back_action.generic.textSize		= MENU_FONT_SIZE;
	s_download_back_action.generic.flags		= x;				// QMF_LEFT_JUSTIFY
	s_download_back_action.generic.x			= x + MENU_FONT_SIZE;	// 0
	s_download_back_action.generic.y			= 41*MENU_LINE_SIZE;	// y += 3*MENU_LINE_SIZE
	s_download_back_action.generic.name			= "Back to Multiplayer";
	s_download_back_action.generic.callback		= UI_BackMenu;

	M_Download_SetMenuItemValues ();

	UI_AddMenuItem (&s_downloadoptions_menu, &s_downloadoptions_banner);
	UI_AddMenuItem (&s_downloadoptions_menu, &s_download_title);
	UI_AddMenuItem (&s_downloadoptions_menu, &s_allow_download_box);

#ifdef USE_CURL	// HTTP downloading from R1Q2
	UI_AddMenuItem (&s_downloadoptions_menu, &s_http_download_box);
	UI_AddMenuItem (&s_downloadoptions_menu, &s_http_fallback_box);
#endif	// USE_CURL

	UI_AddMenuItem (&s_downloadoptions_menu, &s_allow_download_maps_box);
	UI_AddMenuItem (&s_downloadoptions_menu, &s_allow_download_textures_24bit_box);	// option to allow downloading 24-bit textures
	UI_AddMenuItem (&s_downloadoptions_menu, &s_allow_download_players_box);
	UI_AddMenuItem (&s_downloadoptions_menu, &s_allow_download_models_box);
	UI_AddMenuItem (&s_downloadoptions_menu, &s_allow_download_sounds_box);

	UI_AddMenuItem (&s_downloadoptions_menu, &s_download_back_action );
}

void Menu_DownloadOptions_Draw(void)
{
	UI_AdjustMenuCursor (&s_downloadoptions_menu, 1);
	UI_DrawMenu (&s_downloadoptions_menu);
}

const char *Menu_DownloadOptions_Key (int key)
{
	return UI_DefaultMenuKey (&s_downloadoptions_menu, key);
}

void Menu_DownloadOptions_f (void)
{
	Menu_DownloadOptions_Init ();
	UI_PushMenu (&s_downloadoptions_menu, Menu_DownloadOptions_Draw, Menu_DownloadOptions_Key);
}
