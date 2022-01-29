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

// menu_options_screen.c -- the screen options menu

#include "../client/client.h"
#include "ui_local.h"

/*
=======================================================================

SCREEN MENU

=======================================================================
*/

static menuFramework_s	s_options_screen_menu;
static menuImage_s		s_options_screen_banner;
static menuLabel_s		s_options_screen_header;
static menuPicker_s		s_options_screen_crosshair_box;
static menuRectangle_s	s_options_screen_crosshair_background;
static menuButton_s		s_options_screen_crosshair_display;
static menuSlider_s		s_options_screen_crosshairscale_slider;
static menuSlider_s		s_options_screen_crosshairalpha_slider;
static menuSlider_s		s_options_screen_crosshairpulse_slider;
static menuSlider_s		s_options_screen_hudscale_slider;
static menuSlider_s		s_options_screen_hudalpha_slider;
static menuPicker_s		s_options_screen_hudsqueezedigits_box;
static menuPicker_s		s_options_screen_fps_box;
static menuAction_s		s_options_screen_defaults_action;
static menuAction_s		s_options_screen_back_action;

//=======================================================================

static void CrosshairFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_screen_crosshair_box, "crosshair");

	s_options_screen_crosshair_display.imageName = ui_crosshair_display_names[s_options_screen_crosshair_box.curValue];
}

static void CrosshairButtonFunc (void *unused)
{
	s_options_screen_crosshair_box.curValue++;
	if (s_options_screen_crosshair_box.curValue > ui_numcrosshairs-1)
		s_options_screen_crosshair_box.curValue = 0; // wrap around
	CrosshairFunc (NULL);
//	UI_SlideMenuItem ((void *) &s_options_screen_crosshair_box, 1);
}

static void CrosshairButtonMouse2Func (void *unused)
{
	s_options_screen_crosshair_box.curValue--;
	if (s_options_screen_crosshair_box.curValue < 0)
		s_options_screen_crosshair_box.curValue = ui_numcrosshairs-1; // wrap around
	CrosshairFunc (NULL);
//	UI_SlideMenuItem ((void *) &s_options_screen_crosshair_box, -1);
}

void CrosshairButtonCursor (void *unused)
{
	// Do nothing
}

// Psychospaz's changeable size crosshair
static void CrosshairSizeFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_screen_crosshairscale_slider, "crosshair_scale");
}

static void CrosshairAlphaFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_screen_crosshairalpha_slider, "crosshair_alpha");
}

static void CrosshairPulseFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_screen_crosshairpulse_slider, "crosshair_pulse");
}

// hud scaling option
static void HudScaleFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_screen_hudscale_slider, "scr_hudsize");
}

// hud trans option
static void HudAlphaFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_screen_hudalpha_slider, "scr_hudalpha");
}

// hud squeeze digits option
static void HudSqueezeDigitsFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_screen_hudsqueezedigits_box, "scr_hudsqueezedigits");
}

// FPS counter option
static void FPSFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_screen_fps_box, "cl_drawfps");
}

//=======================================================================

static void M_ScreenSetMenuItemValues (void)
{
	MenuPicker_SetValue (&s_options_screen_crosshair_box, "crosshair", 0, 100, true);
	s_options_screen_crosshair_display.imageName = ui_crosshair_display_names[s_options_screen_crosshair_box.curValue];
	MenuSlider_SetValue (&s_options_screen_crosshairscale_slider, "crosshair_scale", 0.25f, 5.0f, true);
	MenuSlider_SetValue (&s_options_screen_crosshairalpha_slider, "crosshair_alpha", 0.05f, 1.0f, true);
	MenuSlider_SetValue (&s_options_screen_crosshairpulse_slider, "crosshair_pulse", 0.0f, 0.5f, true);
	MenuSlider_SetValue (&s_options_screen_hudscale_slider, "scr_hudsize", 0, 8, true);
	MenuSlider_SetValue (&s_options_screen_hudalpha_slider, "scr_hudalpha", 0.0f, 1.0f, true);
	MenuPicker_SetValue (&s_options_screen_hudsqueezedigits_box, "scr_hudsqueezedigits", 0, 1, true);
	MenuPicker_SetValue (&s_options_screen_fps_box, "cl_drawfps", 0, 1, true);
}

static void M_ScreenResetDefaultsFunc (void *unused)
{
	Cvar_SetToDefault ("crosshair");
	Cvar_SetToDefault ("crosshair_scale");
	Cvar_SetToDefault ("crosshair_alpha");
	Cvar_SetToDefault ("crosshair_pulse");
	Cvar_SetToDefault ("scr_hudsize");
	Cvar_SetToDefault ("scr_hudalpha");
	Cvar_SetToDefault ("scr_hudsqueezedigits");
	Cvar_SetToDefault ("cl_drawfps");

	M_ScreenSetMenuItemValues ();
}

//=======================================================================

void Menu_Options_Screen_Init (void)
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	int		x, y;

	// menu.x = 320, menu.y = 162
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 78;

	s_options_screen_menu.x					= 0;	// SCREEN_WIDTH*0.5;
	s_options_screen_menu.y					= 0;	// SCREEN_HEIGHT*0.5 - 58;
	s_options_screen_menu.nitems			= 0;
	s_options_screen_menu.isPopup			= false;
	s_options_screen_menu.keyFunc			= UI_DefaultMenuKey;
	s_options_screen_menu.canOpenFunc		= NULL;
//	s_options_screen_menu.defaultsFunc		= M_ScreenResetDefaults;
//	s_options_screen_menu.defaultsMessage	= "Reset all Screen settings to defaults?";

	s_options_screen_banner.generic.type		= MTYPE_IMAGE;
	s_options_screen_banner.generic.x			= 0;
	s_options_screen_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_options_screen_banner.width				= 275;
	s_options_screen_banner.height				= 32;
	s_options_screen_banner.imageName			= "/pics/m_banner_options.pcx";
	s_options_screen_banner.alpha				= 255;
	s_options_screen_banner.border				= 0;
	s_options_screen_banner.hCentered			= true;
	s_options_screen_banner.vCentered			= false;
	s_options_screen_banner.generic.isHidden	= false;

	s_options_screen_header.generic.type		= MTYPE_LABEL;
	s_options_screen_header.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_screen_header.generic.name		= "Screen";
	s_options_screen_header.generic.x			= x + MENU_HEADER_FONT_SIZE/2 * (int)strlen(s_options_screen_header.generic.name);
	s_options_screen_header.generic.y			= y;	//  + -2*MENU_LINE_SIZE;

	s_options_screen_crosshair_box.generic.type				= MTYPE_PICKER;
	s_options_screen_crosshair_box.generic.textSize			= MENU_FONT_SIZE;
	s_options_screen_crosshair_box.generic.x				= x;
	s_options_screen_crosshair_box.generic.y				= y += 4*MENU_LINE_SIZE;
	s_options_screen_crosshair_box.generic.name				= "crosshair";
	s_options_screen_crosshair_box.generic.callback			= CrosshairFunc;
	s_options_screen_crosshair_box.itemNames				= ui_crosshair_names;
	s_options_screen_crosshair_box.itemValues				= ui_crosshair_values;
	s_options_screen_crosshair_box.generic.statusbar		= "changes crosshair";

	s_options_screen_crosshair_background.generic.type		= MTYPE_RECTANGLE;
	s_options_screen_crosshair_background.generic.x			= SCREEN_WIDTH*0.5 - 18;
	s_options_screen_crosshair_background.generic.y			= SCREEN_HEIGHT*0.5 - 26;
	s_options_screen_crosshair_background.width				= 36;
	s_options_screen_crosshair_background.height			= 36;
	s_options_screen_crosshair_background.color[0]			= 0;
	s_options_screen_crosshair_background.color[1]			= 0;
	s_options_screen_crosshair_background.color[2]			= 0;
	s_options_screen_crosshair_background.color[3]			= 255;
	s_options_screen_crosshair_background.border			= 1;
	s_options_screen_crosshair_background.borderColor[0]	= 60;
	s_options_screen_crosshair_background.borderColor[1]	= 60;
	s_options_screen_crosshair_background.borderColor[2]	= 60;
	s_options_screen_crosshair_background.borderColor[3]	= 255;
	s_options_screen_crosshair_background.hCentered			= false;
	s_options_screen_crosshair_background.vCentered			= false;
	s_options_screen_crosshair_background.generic.isHidden	= false;

	s_options_screen_crosshair_display.generic.type				= MTYPE_BUTTON;
	s_options_screen_crosshair_display.generic.x				= SCREEN_WIDTH*0.5 - 16;
	s_options_screen_crosshair_display.generic.y				= SCREEN_HEIGHT*0.5 - 24;
	s_options_screen_crosshair_display.width					= 32;
	s_options_screen_crosshair_display.height					= 32;
	s_options_screen_crosshair_display.imageName				= NULL;		// This is set in M_ScreenSetMenuItemValues()
	s_options_screen_crosshair_display.alpha					= 255;
	s_options_screen_crosshair_display.border					= 0;
	s_options_screen_crosshair_display.hCentered				= false;
	s_options_screen_crosshair_display.vCentered				= false;
	s_options_screen_crosshair_display.usesMouse2				= true;
	s_options_screen_crosshair_display.generic.isHidden			= false;
	s_options_screen_crosshair_display.generic.callback			= CrosshairButtonFunc;
	s_options_screen_crosshair_display.generic.mouse2Callback	= CrosshairButtonMouse2Func;
	s_options_screen_crosshair_display.generic.cursordraw		= CrosshairButtonCursor;

	// Psychospaz's changeable size crosshair
	s_options_screen_crosshairscale_slider.generic.type			= MTYPE_SLIDER;
	s_options_screen_crosshairscale_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_screen_crosshairscale_slider.generic.x			= x;
	s_options_screen_crosshairscale_slider.generic.y			= y += 5*MENU_LINE_SIZE;
	s_options_screen_crosshairscale_slider.generic.name			= "crosshair scale";
	s_options_screen_crosshairscale_slider.generic.callback		= CrosshairSizeFunc;
	s_options_screen_crosshairscale_slider.maxPos				= 19;
	s_options_screen_crosshairscale_slider.baseValue			= 0.25f;
	s_options_screen_crosshairscale_slider.increment			= 0.25f;
	s_options_screen_crosshairscale_slider.displayAsPercent		= false;
	s_options_screen_crosshairscale_slider.generic.statusbar	= "changes size of crosshair";

	s_options_screen_crosshairalpha_slider.generic.type			= MTYPE_SLIDER;
	s_options_screen_crosshairalpha_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_screen_crosshairalpha_slider.generic.x			= x;
	s_options_screen_crosshairalpha_slider.generic.y			= y += MENU_LINE_SIZE;
	s_options_screen_crosshairalpha_slider.generic.name			= "crosshair alpha";
	s_options_screen_crosshairalpha_slider.generic.callback		= CrosshairAlphaFunc;
	s_options_screen_crosshairalpha_slider.maxPos				= 19;
	s_options_screen_crosshairalpha_slider.baseValue			= 0.05f;
	s_options_screen_crosshairalpha_slider.increment			= 0.05f;
	s_options_screen_crosshairalpha_slider.displayAsPercent		= true;
	s_options_screen_crosshairalpha_slider.generic.statusbar	= "changes opacity of crosshair";

	s_options_screen_crosshairpulse_slider.generic.type			= MTYPE_SLIDER;
	s_options_screen_crosshairpulse_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_screen_crosshairpulse_slider.generic.x			= x;
	s_options_screen_crosshairpulse_slider.generic.y			= y += MENU_LINE_SIZE;
	s_options_screen_crosshairpulse_slider.generic.name			= "crosshair pulse";
	s_options_screen_crosshairpulse_slider.generic.callback		= CrosshairPulseFunc;
	s_options_screen_crosshairpulse_slider.maxPos				= 10;
	s_options_screen_crosshairpulse_slider.baseValue			= 0.0f;
	s_options_screen_crosshairpulse_slider.increment			= 0.05f;
	s_options_screen_crosshairpulse_slider.displayAsPercent		= true;
	s_options_screen_crosshairpulse_slider.generic.statusbar	= "changes pulse amplitude of crosshair";

	// hud scaling option
	s_options_screen_hudscale_slider.generic.type			= MTYPE_SLIDER;
	s_options_screen_hudscale_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_screen_hudscale_slider.generic.x				= x;
	s_options_screen_hudscale_slider.generic.y				= y += 2*MENU_LINE_SIZE;
	s_options_screen_hudscale_slider.generic.name			= "status bar scale";
	s_options_screen_hudscale_slider.generic.callback		= HudScaleFunc;
	s_options_screen_hudscale_slider.maxPos					= 8;
	s_options_screen_hudscale_slider.baseValue				= 0.0f;
	s_options_screen_hudscale_slider.increment				= 1.0f;
	s_options_screen_hudscale_slider.displayAsPercent		= false;
	s_options_screen_hudscale_slider.generic.statusbar		= "changes size of HUD elements";

	// hud trans option
	s_options_screen_hudalpha_slider.generic.type			= MTYPE_SLIDER;
	s_options_screen_hudalpha_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_screen_hudalpha_slider.generic.x				= x;
	s_options_screen_hudalpha_slider.generic.y				= y += MENU_LINE_SIZE;
	s_options_screen_hudalpha_slider.generic.name			= "status bar transparency";
	s_options_screen_hudalpha_slider.generic.callback		= HudAlphaFunc;
	s_options_screen_hudalpha_slider.maxPos					= 20;
	s_options_screen_hudalpha_slider.baseValue				= 0.0f;
	s_options_screen_hudalpha_slider.increment				= 0.05f;
	s_options_screen_hudalpha_slider.displayAsPercent		= true;
	s_options_screen_hudalpha_slider.generic.statusbar		= "changes opacity of HUD elements";

	// hud squeeze digits option
	s_options_screen_hudsqueezedigits_box.generic.type		= MTYPE_PICKER;
	s_options_screen_hudsqueezedigits_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_screen_hudsqueezedigits_box.generic.x			= x;
	s_options_screen_hudsqueezedigits_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_screen_hudsqueezedigits_box.generic.name		= "status bar digit squeezing";
	s_options_screen_hudsqueezedigits_box.generic.callback	= HudSqueezeDigitsFunc;
	s_options_screen_hudsqueezedigits_box.itemNames			= yesno_names;
	s_options_screen_hudsqueezedigits_box.generic.statusbar	= "enables showing of longer numbers on HUD";

	s_options_screen_fps_box.generic.type				= MTYPE_PICKER;
	s_options_screen_fps_box.generic.textSize			= MENU_FONT_SIZE;
	s_options_screen_fps_box.generic.x					= x;
	s_options_screen_fps_box.generic.y					= y += 2*MENU_LINE_SIZE;
	s_options_screen_fps_box.generic.name				= "FPS counter";
	s_options_screen_fps_box.generic.callback			= FPSFunc;
	s_options_screen_fps_box.itemNames					= yesno_names;
	s_options_screen_fps_box.generic.statusbar			= "enables FPS counter";

	s_options_screen_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_screen_defaults_action.generic.textSize	= MENU_FONT_SIZE;
	s_options_screen_defaults_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_screen_defaults_action.generic.y			= 39*MENU_LINE_SIZE;
	s_options_screen_defaults_action.generic.name		= "Reset to Defaults";
	s_options_screen_defaults_action.generic.callback	= M_ScreenResetDefaultsFunc;
	s_options_screen_defaults_action.generic.statusbar	= "resets all screen settings to internal defaults";

	s_options_screen_back_action.generic.type			= MTYPE_ACTION;
	s_options_screen_back_action.generic.textSize		= MENU_FONT_SIZE;
	s_options_screen_back_action.generic.x				= x + MENU_FONT_SIZE;
	s_options_screen_back_action.generic.y				= 41*MENU_LINE_SIZE;
	s_options_screen_back_action.generic.name			= "Back to Options";
	s_options_screen_back_action.generic.callback		= UI_BackMenu;

	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_banner);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_header);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_crosshair_box);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_crosshair_background);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_crosshair_display);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_crosshairscale_slider);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_crosshairalpha_slider);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_crosshairpulse_slider);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_hudscale_slider);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_hudalpha_slider);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_hudsqueezedigits_box);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_fps_box);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_defaults_action);
	UI_AddMenuItem (&s_options_screen_menu, (void *) &s_options_screen_back_action);

	M_ScreenSetMenuItemValues ();
}

void Menu_Options_Screen_Draw (void)
{
	UI_AdjustMenuCursor (&s_options_screen_menu, 1);
	UI_DrawMenu (&s_options_screen_menu);
}

const char *Menu_Options_Screen_Key (int key)
{
	return UI_DefaultMenuKey (&s_options_screen_menu, key);
}

void Menu_Options_Screen_f (void)
{
	Menu_Options_Screen_Init ();
	UI_PushMenu (&s_options_screen_menu, Menu_Options_Screen_Draw, Menu_Options_Screen_Key);
}
