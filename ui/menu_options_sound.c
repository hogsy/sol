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

// menu_options_sound.c -- the sound options menu

#include "../client/client.h"
#include "ui_local.h"

#define USE_COMBOBOX

/*
=======================================================================

SOUND MENU

=======================================================================
*/

static menuFramework_s	s_options_sound_menu;
static menuImage_s		s_options_sound_banner;
static menuLabel_s		s_options_sound_header;
static menuSlider_s		s_options_sound_sfxvolume_slider;
static menuSlider_s		s_options_sound_musicvolume_slider;
static menuPicker_s		s_options_sound_oggmusic_box;
static menuPicker_s		s_options_sound_cdmusic_box;

#ifdef USE_COMBOBOX
static menuComboBox_s	s_options_sound_quality_box;
static menuComboBox_s	s_options_sound_compatibility_box;
#else
static menuPicker_s		s_options_sound_quality_box;
static menuPicker_s		s_options_sound_compatibility_box;
#endif	// USE_COMBOBOX

static menuAction_s		s_options_sound_defaults_action;
static menuAction_s		s_options_sound_back_action;

//=======================================================================

void M_Sound_DrawUpdateMessage (void)
{
	UI_DrawPopupMessage ("Restarting the sound system.\nThis could take up to a minute,\nso please be patient.");
}

static void M_UpdateSoundQualityFunc (void *unused)
{
	Cvar_SetInteger ("s_loadas8bit", (s_options_sound_quality_box.curValue == 0)); // 11 KHz is 8-bit

	// show update wait message
	M_Sound_DrawUpdateMessage ();	
	CL_Snd_Restart_f ();
}

//=======================================================================

static void M_SoundResetDefaults (void)
{
	Cvar_SetToDefault ("s_volume");
	Cvar_SetToDefault ("s_musicvolume");
	Cvar_SetToDefault ("cl_ogg_music");
	Cvar_SetToDefault ("cd_nocd");
	Cvar_SetToDefault ("cd_loopcount");
	Cvar_SetToDefault ("s_khz");
	Cvar_SetToDefault ("s_loadas8bit");
	Cvar_SetToDefault ("s_primary");

	// show update wait message
	M_Sound_DrawUpdateMessage ();
	CL_Snd_Restart_f ();
}

//=======================================================================

void Menu_Options_Sound_Init (void)
{
	static const char *enabled_items[] =
	{
		"disabled",
		"enabled",
		0
	};
	static const char *cd_music_values[] =
	{
		"1",
		"0",
		0
	};
	static const char *quality_items[] =
	{
		"low (11KHz/8-bit)",			//** DMP - changed text
		"normal (22KHz/16-bit)",		//** DMP - changed text
		"high (44KHz/16-bit)",			//** DMP - added 44 Khz menu item
		"highest (48KHz/16-bit)",		//** DMP - added 48 Khz menu item
		0
	};
	static const char *quality_values[] =
	{
		"11",
		"22",	
		"44",
		"48",
		0
	};
	static const char *compatibility_items[] =
	{
		"max compatibility",
		"max performance",
		0
	};
	int		x, y;

	// menu.x = 320, menu.y = 162
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 78;

	s_options_sound_menu.x						= 0;	// SCREEN_WIDTH*0.5;
	s_options_sound_menu.y						= 0;	// SCREEN_HEIGHT*0.5 - 58;
	s_options_sound_menu.nitems					= 0;
	s_options_sound_menu.isPopup				= false;
	s_options_sound_menu.background				= NULL;
	s_options_sound_menu.drawFunc				= UI_DefaultMenuDraw;
	s_options_sound_menu.keyFunc				= UI_DefaultMenuKey;
	s_options_sound_menu.canOpenFunc			= NULL;
	s_options_sound_menu.defaultsFunc			= M_SoundResetDefaults;
	s_options_sound_menu.defaultsMessage		= "Reset all Sound settings to defaults?";

	s_options_sound_banner.generic.type			= MTYPE_IMAGE;
	s_options_sound_banner.generic.x			= 0;
	s_options_sound_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_options_sound_banner.width				= 275;
	s_options_sound_banner.height				= 32;
	s_options_sound_banner.imageName			= "/pics/m_banner_options.pcx";
	s_options_sound_banner.alpha				= 255;
	s_options_sound_banner.border				= 0;
	s_options_sound_banner.hCentered			= true;
	s_options_sound_banner.vCentered			= false;
	s_options_sound_banner.generic.isHidden		= false;

	s_options_sound_header.generic.type		= MTYPE_LABEL;
	s_options_sound_header.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_sound_header.generic.name		= "Sound";
	s_options_sound_header.generic.x		= x + MENU_HEADER_FONT_SIZE/2 * (int)strlen(s_options_sound_header.generic.name);
	s_options_sound_header.generic.y		= y;	//  + -2*MENU_LINE_SIZE

	s_options_sound_sfxvolume_slider.generic.type		= MTYPE_SLIDER;
	s_options_sound_sfxvolume_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_sound_sfxvolume_slider.generic.x			= x;
	s_options_sound_sfxvolume_slider.generic.y			= y += 4*MENU_LINE_SIZE;
	s_options_sound_sfxvolume_slider.generic.name		= "effects volume";
	s_options_sound_sfxvolume_slider.maxPos				= 20;
	s_options_sound_sfxvolume_slider.baseValue			= 0.0f;
	s_options_sound_sfxvolume_slider.increment			= 0.05f;
	s_options_sound_sfxvolume_slider.displayAsPercent	= true;
	s_options_sound_sfxvolume_slider.generic.cvar		= "s_volume";
	s_options_sound_sfxvolume_slider.generic.cvarClamp	= true;
	s_options_sound_sfxvolume_slider.generic.cvarMin	= 0.0f;
	s_options_sound_sfxvolume_slider.generic.cvarMax	= 1.0f;
	s_options_sound_sfxvolume_slider.generic.statusbar	= "volume of sound effects";

	s_options_sound_musicvolume_slider.generic.type			= MTYPE_SLIDER;
	s_options_sound_musicvolume_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_sound_musicvolume_slider.generic.x			= x;
	s_options_sound_musicvolume_slider.generic.y			= y += MENU_LINE_SIZE;
	s_options_sound_musicvolume_slider.generic.name			= "music volume";
	s_options_sound_musicvolume_slider.maxPos				= 20;
	s_options_sound_musicvolume_slider.baseValue			= 0.0f;
	s_options_sound_musicvolume_slider.increment			= 0.05f;
	s_options_sound_musicvolume_slider.displayAsPercent		= true;
	s_options_sound_musicvolume_slider.generic.cvar			= "s_musicvolume";
	s_options_sound_musicvolume_slider.generic.cvarClamp	= true;
	s_options_sound_musicvolume_slider.generic.cvarMin		= 0.0f;
	s_options_sound_musicvolume_slider.generic.cvarMax		= 1.0f;
	s_options_sound_musicvolume_slider.generic.statusbar	= "volume of ogg vorbis music";

	s_options_sound_oggmusic_box.generic.type		= MTYPE_PICKER;
	s_options_sound_oggmusic_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_sound_oggmusic_box.generic.x			= x;
	s_options_sound_oggmusic_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_options_sound_oggmusic_box.generic.name		= "ogg vorbis music";
	s_options_sound_oggmusic_box.itemNames			= enabled_items;
	s_options_sound_oggmusic_box.generic.cvar		= "cl_ogg_music";
	s_options_sound_oggmusic_box.generic.cvarClamp	= true;
	s_options_sound_oggmusic_box.generic.cvarMin	= 0;
	s_options_sound_oggmusic_box.generic.cvarMax	= 1;
	s_options_sound_oggmusic_box.generic.statusbar	= "override of CD music with ogg vorbis tracks";

	s_options_sound_cdmusic_box.generic.type		= MTYPE_PICKER;
	s_options_sound_cdmusic_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_sound_cdmusic_box.generic.x			= x;
	s_options_sound_cdmusic_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_sound_cdmusic_box.generic.name		= "CD music";
	s_options_sound_cdmusic_box.itemNames			= enabled_items;
	s_options_sound_cdmusic_box.itemValues			= cd_music_values;
	s_options_sound_cdmusic_box.generic.cvar		= "cd_nocd";
	s_options_sound_cdmusic_box.generic.cvarClamp	= true;
	s_options_sound_cdmusic_box.generic.cvarMin		= 0;
	s_options_sound_cdmusic_box.generic.cvarMax		= 1;
	s_options_sound_cdmusic_box.generic.statusbar	= "enables or disables CD music";

#ifdef USE_COMBOBOX
	s_options_sound_quality_box.generic.type		= MTYPE_COMBOBOX;
	s_options_sound_quality_box.generic.x			= x;
	s_options_sound_quality_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_options_sound_quality_box.generic.name		= "sound quality";
	s_options_sound_quality_box.itemNames			= quality_items;
	s_options_sound_quality_box.itemValues			= quality_values;
	s_options_sound_quality_box.items_y				= 4;
	s_options_sound_quality_box.itemWidth			= 23;
	s_options_sound_quality_box.itemSpacing			= 1;
	s_options_sound_quality_box.itemTextSize		= 8;
	s_options_sound_quality_box.border				= 1;
	s_options_sound_quality_box.borderColor[0]		= 60;
	s_options_sound_quality_box.borderColor[1]		= 60;
	s_options_sound_quality_box.borderColor[2]		= 60;
	s_options_sound_quality_box.borderColor[3]		= 255;
	s_options_sound_quality_box.backColor[0]		= 0;
	s_options_sound_quality_box.backColor[1]		= 0;
	s_options_sound_quality_box.backColor[2]		= 0;
	s_options_sound_quality_box.backColor[3]		= 192;
	s_options_sound_quality_box.generic.callback	= M_UpdateSoundQualityFunc;
	s_options_sound_quality_box.generic.cvar		= "s_khz";
	s_options_sound_quality_box.generic.cvarClamp	= false;
	s_options_sound_quality_box.generic.statusbar	= "changes quality of sound";

	s_options_sound_compatibility_box.generic.type		= MTYPE_COMBOBOX;
	s_options_sound_compatibility_box.generic.x			= x;
	s_options_sound_compatibility_box.generic.y			= y += 1.5*MENU_LINE_SIZE;
	s_options_sound_compatibility_box.generic.name		= "sound compatibility";
	s_options_sound_compatibility_box.itemNames			= compatibility_items;
	s_options_sound_compatibility_box.items_y			= 2;
	s_options_sound_compatibility_box.itemWidth			= 18;
	s_options_sound_compatibility_box.itemSpacing		= 1;
	s_options_sound_compatibility_box.itemTextSize		= 8;
	s_options_sound_compatibility_box.border			= 1;
	s_options_sound_compatibility_box.borderColor[0]	= 60;
	s_options_sound_compatibility_box.borderColor[1]	= 60;
	s_options_sound_compatibility_box.borderColor[2]	= 60;
	s_options_sound_compatibility_box.borderColor[3]	= 255;
	s_options_sound_compatibility_box.backColor[0]		= 0;
	s_options_sound_compatibility_box.backColor[1]		= 0;
	s_options_sound_compatibility_box.backColor[2]		= 0;
	s_options_sound_compatibility_box.backColor[3]		= 192;
	s_options_sound_compatibility_box.generic.callback	= M_UpdateSoundQualityFunc;
	s_options_sound_compatibility_box.generic.cvar		= "s_primary";
	s_options_sound_compatibility_box.generic.cvarClamp	= true;
	s_options_sound_compatibility_box.generic.cvarMin	= 0;
	s_options_sound_compatibility_box.generic.cvarMax	= 1;
	s_options_sound_compatibility_box.generic.statusbar	= "changes buffering mode of sound system";
#else	// USE_COMBOBOX
	s_options_sound_quality_box.generic.type		= MTYPE_PICKER;
	s_options_sound_quality_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_sound_quality_box.generic.x			= x;
	s_options_sound_quality_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_options_sound_quality_box.generic.name		= "sound quality";
	s_options_sound_quality_box.generic.callback	= M_UpdateSoundQualityFunc;
	s_options_sound_quality_box.itemNames			= quality_items;
	s_options_sound_quality_box.itemValues			= quality_values;
	s_options_sound_quality_box.generic.cvar		= "s_khz";
	s_options_sound_quality_box.generic.cvarClamp	= false;
	s_options_sound_quality_box.generic.statusbar	= "changes quality of sound";

	s_options_sound_compatibility_box.generic.type		= MTYPE_PICKER;
	s_options_sound_compatibility_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_sound_compatibility_box.generic.x			= x;
	s_options_sound_compatibility_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_sound_compatibility_box.generic.name		= "sound compatibility";
	s_options_sound_compatibility_box.generic.callback	= M_UpdateSoundQualityFunc;
	s_options_sound_compatibility_box.itemNames			= compatibility_items;
	s_options_sound_compatibility_box.generic.cvar		= "s_primary";
	s_options_sound_compatibility_box.generic.cvarClamp	= true;
	s_options_sound_compatibility_box.generic.cvarMin	= 0;
	s_options_sound_compatibility_box.generic.cvarMax	= 1;
	s_options_sound_compatibility_box.generic.statusbar	= "changes buffering mode of sound system";
#endif	// USE_COMBOBOX

	s_options_sound_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_sound_defaults_action.generic.textSize	= MENU_FONT_SIZE;
	s_options_sound_defaults_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_sound_defaults_action.generic.y			= 39*MENU_LINE_SIZE;
	s_options_sound_defaults_action.generic.name		= "Reset to Defaults";
	s_options_sound_defaults_action.generic.callback	= UI_Defaults_Popup;	// M_SoundResetDefaults
	s_options_sound_defaults_action.generic.statusbar	= "resets all sound settings to internal defaults";

	s_options_sound_back_action.generic.type			= MTYPE_ACTION;
	s_options_sound_back_action.generic.textSize		= MENU_FONT_SIZE;
	s_options_sound_back_action.generic.x				= x + MENU_FONT_SIZE;
	s_options_sound_back_action.generic.y				= 41*MENU_LINE_SIZE;
	s_options_sound_back_action.generic.name			= "Back to Options";
	s_options_sound_back_action.generic.callback		= UI_BackMenu;

	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_banner);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_header);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_sfxvolume_slider);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_musicvolume_slider);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_oggmusic_box);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_cdmusic_box);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_quality_box);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_compatibility_box);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_defaults_action);
	UI_AddMenuItem (&s_options_sound_menu, (void *) &s_options_sound_back_action);
}


void Menu_Options_Sound_f (void)
{
	Menu_Options_Sound_Init ();
	UI_PushMenu (&s_options_sound_menu);
}
