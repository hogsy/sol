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

// menu_options_interface.c -- the interface options menu

#include "../client/client.h"
#include "ui_local.h"

/*
=======================================================================

INTERFACE MENU

=======================================================================
*/

static menuFramework_s	s_options_interface_menu;
static menuImage_s		s_options_interface_banner;
static menuLabel_s		s_options_interface_header;
static menuSlider_s		s_options_interface_conalpha_slider;
//static menuSlider_s	s_options_interface_conheight_slider;
static menuSlider_s		s_options_interface_menumouse_slider;
static menuSlider_s		s_options_interface_menualpha_slider;
static menuPicker_s		s_options_interface_confont_box;
static menuSlider_s		s_options_interface_fontsize_slider;
static menuPicker_s		s_options_interface_uifont_box;
static menuPicker_s		s_options_interface_scrfont_box;
static menuPicker_s		s_options_interface_alt_text_color_box;
static menuPicker_s		s_options_interface_simple_loadscreen_box;
static menuPicker_s		s_options_interface_newconback_box;
static menuPicker_s		s_options_interface_noalttab_box;
static menuAction_s		s_options_interface_defaults_action;
static menuAction_s		s_options_interface_back_action;

//=======================================================================

static void M_InterfaceResetDefaults (void)
{
	Cvar_SetToDefault ("ui_sensitivity");	
	Cvar_SetToDefault ("ui_background_alpha");	
	Cvar_SetToDefault ("con_font");	
	Cvar_SetToDefault ("con_font_size");	
	Cvar_SetToDefault ("ui_font");	
	Cvar_SetToDefault ("scr_font");	
	Cvar_SetToDefault ("alt_text_color");	
	Cvar_SetToDefault ("scr_alpha");	
//	Cvar_SetToDefault ("scr_conheight");	
	Cvar_SetToDefault ("scr_simple_loadscreen");
	Cvar_SetToDefault ("scr_newconback");
	Cvar_SetToDefault ("win_noalttab");	
}

//=======================================================================

void Options_Interface_MenuInit (void)
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *textcolor_names[] =
	{
		"gray",
		"red",
		"green",
		"yellow",
		"blue",
		"cyan",
		"magenta",
		"white",
		"black",
		"orange",
		0
	};
	int		x, y;

	// menu.x = 320, menu.y = 162
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 78;

	s_options_interface_menu.x					= 0;	// SCREEN_WIDTH*0.5;
	s_options_interface_menu.y					= 0;	// SCREEN_HEIGHT*0.5 - 58;
	s_options_interface_menu.nitems				= 0;
	s_options_interface_menu.isPopup			= false;
	s_options_interface_menu.drawFunc			= UI_DefaultMenuDraw;
	s_options_interface_menu.keyFunc			= UI_DefaultMenuKey;
	s_options_interface_menu.canOpenFunc		= NULL;
	s_options_interface_menu.defaultsFunc		= M_InterfaceResetDefaults;
	s_options_interface_menu.defaultsMessage	= "Reset all Interface settings to defaults?";

	s_options_interface_banner.generic.type		= MTYPE_IMAGE;
	s_options_interface_banner.generic.x		= 0;
	s_options_interface_banner.generic.y		= 9*MENU_LINE_SIZE;
	s_options_interface_banner.width			= 275;
	s_options_interface_banner.height			= 32;
	s_options_interface_banner.imageName		= "/pics/m_banner_options.pcx";
	s_options_interface_banner.alpha			= 255;
	s_options_interface_banner.border			= 0;
	s_options_interface_banner.hCentered		= true;
	s_options_interface_banner.vCentered		= false;
	s_options_interface_banner.generic.isHidden	= false;

	s_options_interface_header.generic.type		= MTYPE_LABEL;
	s_options_interface_header.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_interface_header.generic.name		= "Interface";
	s_options_interface_header.generic.x		= x + MENU_HEADER_FONT_SIZE/2 * (int)strlen(s_options_interface_header.generic.name);
	s_options_interface_header.generic.y		= y;	//  + -2*MENU_LINE_SIZE

	// Psychospaz's menu mouse support
	s_options_interface_menumouse_slider.generic.type		= MTYPE_SLIDER;
	s_options_interface_menumouse_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_interface_menumouse_slider.generic.x			= x;
	s_options_interface_menumouse_slider.generic.y			= y += 4*MENU_LINE_SIZE;
	s_options_interface_menumouse_slider.generic.name		= "mouse speed";
	s_options_interface_menumouse_slider.maxPos				= 7;
	s_options_interface_menumouse_slider.baseValue			= 0.25f;
	s_options_interface_menumouse_slider.increment			= 0.25f;
	s_options_interface_menumouse_slider.displayAsPercent	= false;
	s_options_interface_menumouse_slider.generic.cvar		= "ui_sensitivity";
	s_options_interface_menumouse_slider.generic.cvarClamp	= true;
	s_options_interface_menumouse_slider.generic.cvarMin	= 0.25f;
	s_options_interface_menumouse_slider.generic.cvarMax	= 2.0f;
	s_options_interface_menumouse_slider.generic.statusbar	= "changes sensitivity of mouse in menus";

	s_options_interface_menualpha_slider.generic.type		= MTYPE_SLIDER;
	s_options_interface_menualpha_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_interface_menualpha_slider.generic.x			= x;
	s_options_interface_menualpha_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_interface_menualpha_slider.generic.name		= "ingame menu transparency";
	s_options_interface_menualpha_slider.maxPos				= 20;
	s_options_interface_menualpha_slider.baseValue			= 0.0f;
	s_options_interface_menualpha_slider.increment			= 0.05f;
	s_options_interface_menualpha_slider.displayAsPercent	= true;
	s_options_interface_menualpha_slider.generic.cvar		= "ui_background_alpha";
	s_options_interface_menualpha_slider.generic.cvarClamp	= true;
	s_options_interface_menualpha_slider.generic.cvarMin	= 0;
	s_options_interface_menualpha_slider.generic.cvarMax	= 1.0f;
	s_options_interface_menualpha_slider.generic.statusbar	= "changes opacity of menu background";

	s_options_interface_confont_box.generic.type			= MTYPE_PICKER;
	s_options_interface_confont_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_interface_confont_box.generic.x				= x;
	s_options_interface_confont_box.generic.y				= y+=2*MENU_LINE_SIZE;
	s_options_interface_confont_box.generic.name			= "console font";
	s_options_interface_confont_box.itemNames				= ui_font_names;
	s_options_interface_confont_box.itemValues				= ui_font_names;
	s_options_interface_confont_box.generic.cvar			= "con_font";
	s_options_interface_confont_box.generic.cvarClamp		= false;
	s_options_interface_confont_box.generic.statusbar		= "changes font of console text";

	s_options_interface_fontsize_slider.generic.type		= MTYPE_SLIDER;
	s_options_interface_fontsize_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_interface_fontsize_slider.generic.x			= x;
	s_options_interface_fontsize_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_interface_fontsize_slider.generic.name		= "console font size";
	s_options_interface_fontsize_slider.maxPos				= 5;
	s_options_interface_fontsize_slider.baseValue			= 6.0f;
	s_options_interface_fontsize_slider.increment			= 2.0f;
	s_options_interface_fontsize_slider.displayAsPercent	= false;
	s_options_interface_fontsize_slider.generic.cvar		= "con_font_size";
	s_options_interface_fontsize_slider.generic.cvarClamp	= true;
	s_options_interface_fontsize_slider.generic.cvarMin		= 6;
	s_options_interface_fontsize_slider.generic.cvarMax		= 16;
	s_options_interface_fontsize_slider.generic.statusbar	= "changes size of console text";

	s_options_interface_uifont_box.generic.type				= MTYPE_PICKER;
	s_options_interface_uifont_box.generic.textSize			= MENU_FONT_SIZE;
	s_options_interface_uifont_box.generic.x				= x;
	s_options_interface_uifont_box.generic.y				= y+=MENU_LINE_SIZE;
	s_options_interface_uifont_box.generic.name				= "menu font";
	s_options_interface_uifont_box.itemNames				= ui_font_names;
	s_options_interface_uifont_box.itemValues				= ui_font_names;
	s_options_interface_uifont_box.generic.cvar				= "ui_font";
	s_options_interface_uifont_box.generic.cvarClamp		= false;
	s_options_interface_uifont_box.generic.statusbar		= "changes font of menu text";

	s_options_interface_scrfont_box.generic.type			= MTYPE_PICKER;
	s_options_interface_scrfont_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_interface_scrfont_box.generic.x				= x;
	s_options_interface_scrfont_box.generic.y				= y+=MENU_LINE_SIZE;
	s_options_interface_scrfont_box.generic.name			= "HUD font";
	s_options_interface_scrfont_box.itemNames				= ui_font_names;
	s_options_interface_scrfont_box.itemValues				= ui_font_names;
	s_options_interface_scrfont_box.generic.cvar			= "scr_font";
	s_options_interface_scrfont_box.generic.cvarClamp		= false;
	s_options_interface_scrfont_box.generic.statusbar		= "changes font of HUD text";

	s_options_interface_alt_text_color_box.generic.type			= MTYPE_PICKER;
	s_options_interface_alt_text_color_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_interface_alt_text_color_box.generic.x			= x;
	s_options_interface_alt_text_color_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_interface_alt_text_color_box.generic.name			= "alt text color";
	s_options_interface_alt_text_color_box.itemNames			= textcolor_names;
	s_options_interface_alt_text_color_box.generic.cvar			= "alt_text_color";
	s_options_interface_alt_text_color_box.generic.cvarClamp	= true;
	s_options_interface_alt_text_color_box.generic.cvarMin		= 0;
	s_options_interface_alt_text_color_box.generic.cvarMax		= 9;
	s_options_interface_alt_text_color_box.generic.statusbar	= "changes color of highlighted text";

	s_options_interface_conalpha_slider.generic.type		= MTYPE_SLIDER;
	s_options_interface_conalpha_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_interface_conalpha_slider.generic.x			= x;
	s_options_interface_conalpha_slider.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_interface_conalpha_slider.generic.name		= "console transparency";
	s_options_interface_conalpha_slider.maxPos				= 20;
	s_options_interface_conalpha_slider.baseValue			= 0.0f;
	s_options_interface_conalpha_slider.increment			= 0.05f;
	s_options_interface_conalpha_slider.displayAsPercent	= true;
	s_options_interface_conalpha_slider.generic.cvar		= "scr_conalpha";
	s_options_interface_conalpha_slider.generic.cvarClamp	= true;
	s_options_interface_conalpha_slider.generic.cvarMin		= 0.0f;
	s_options_interface_conalpha_slider.generic.cvarMax		= 1.0f;
	s_options_interface_conalpha_slider.generic.statusbar	= "changes opacity of console";
/*
	s_options_interface_conheight_slider.generic.type		= MTYPE_SLIDER;
	s_options_interface_conheight_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_interface_conheight_slider.generic.x			= x;
	s_options_interface_conheight_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_interface_conheight_slider.generic.name		= "console height";
	s_options_interface_conheight_slider.maxPos				= 10;
	s_options_interface_conheight_slider.baseValue			= 0.25f;
	s_options_interface_conheight_slider.increment			= 0.05f;
	s_options_interface_conheight_slider.displayAsPercent	= false;
	s_options_interface_conheight_slider.generic.cvar		= "scr_conheight";
	s_options_interface_conheight_slider.generic.cvarClamp	= true;
	s_options_interface_conheight_slider.generic.cvarMin	= 0.25f;
	s_options_interface_conheight_slider.generic.cvarMax	= 0.75f;
	s_options_interface_conheight_slider.generic.statusbar	= "changes drop height of console";
*/
	s_options_interface_simple_loadscreen_box.generic.type		= MTYPE_PICKER;
	s_options_interface_simple_loadscreen_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_interface_simple_loadscreen_box.generic.x			= x;
	s_options_interface_simple_loadscreen_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_interface_simple_loadscreen_box.generic.name		= "simple load screens";
	s_options_interface_simple_loadscreen_box.itemNames			= yesno_names;
	s_options_interface_simple_loadscreen_box.generic.cvar		= "scr_simple_loadscreen";
	s_options_interface_simple_loadscreen_box.generic.statusbar	= "toggles simple map load screen";

	s_options_interface_newconback_box.generic.type			= MTYPE_PICKER;
	s_options_interface_newconback_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_interface_newconback_box.generic.x			= x;
	s_options_interface_newconback_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_interface_newconback_box.generic.name			= "new console background";
	s_options_interface_newconback_box.itemNames			= yesno_names;
	s_options_interface_newconback_box.generic.cvar			= "scr_newconback";
	s_options_interface_newconback_box.generic.statusbar	= "enables Q3-style console background";

	s_options_interface_noalttab_box.generic.type			= MTYPE_PICKER;
	s_options_interface_noalttab_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_interface_noalttab_box.generic.x				= x;
	s_options_interface_noalttab_box.generic.y				= y += 2*MENU_LINE_SIZE;
	s_options_interface_noalttab_box.generic.name			= "disable alt-tab";
	s_options_interface_noalttab_box.itemNames				= yesno_names;
	s_options_interface_noalttab_box.generic.cvar			= "win_noalttab";
	s_options_interface_noalttab_box.generic.statusbar		= "disables alt-tabbing to desktop";

	s_options_interface_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_interface_defaults_action.generic.textSize	= MENU_FONT_SIZE;
	s_options_interface_defaults_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_interface_defaults_action.generic.y			= 39*MENU_LINE_SIZE;
	s_options_interface_defaults_action.generic.name		= "Reset to Defaults";
	s_options_interface_defaults_action.generic.callback	= UI_Defaults_Popup;	// M_InterfaceResetDefaults
	s_options_interface_defaults_action.generic.statusbar	= "resets all interface settings to internal defaults";

	s_options_interface_back_action.generic.type		= MTYPE_ACTION;
	s_options_interface_back_action.generic.textSize	= MENU_FONT_SIZE;
	s_options_interface_back_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_interface_back_action.generic.y			= 41*MENU_LINE_SIZE;
	s_options_interface_back_action.generic.name		= "Back to Options";
	s_options_interface_back_action.generic.callback	= UI_BackMenu;

	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_banner);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_header);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_menumouse_slider);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_menualpha_slider);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_confont_box);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_fontsize_slider);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_uifont_box);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_scrfont_box);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_alt_text_color_box);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_conalpha_slider);
//	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_conheight_slider);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_simple_loadscreen_box);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_newconback_box);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_noalttab_box);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_defaults_action);
	UI_AddMenuItem (&s_options_interface_menu, (void *) &s_options_interface_back_action);
}


void Menu_Options_Interface_f (void)
{
	Options_Interface_MenuInit ();
	UI_PushMenu (&s_options_interface_menu);
}
