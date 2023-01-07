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

// menu_options_controls.c -- the controls options menu

#include "../client/client.h"
#include "ui_local.h"

/*
=======================================================================

CONTROLS MENU

=======================================================================
*/

static menuFramework_s	s_options_controls_menu;
static menuImage_s		s_options_controls_banner;
static menuLabel_s		s_options_controls_header;
static menuSlider_s		s_options_controls_sensitivity_slider;
static menuPicker_s		s_options_controls_mouseaccel_box;
static menuPicker_s		s_options_controls_mousefilter_box;
static menuPicker_s		s_options_controls_invertmouse_box;
static menuPicker_s		s_options_controls_autosensitivity_box;
static menuPicker_s		s_options_controls_thirdperson_box;
static menuSlider_s		s_options_controls_thirdperson_distance_slider;
static menuSlider_s		s_options_controls_thirdperson_offset_slider;
static menuSlider_s		s_options_controls_thirdperson_angle_slider;
static menuPicker_s		s_options_controls_alwaysrun_box;
static menuPicker_s		s_options_controls_lookspring_box;
static menuPicker_s		s_options_controls_lookstrafe_box;
static menuPicker_s		s_options_controls_freelook_box;
static menuPicker_s		s_options_controls_joystick_box;
static menuAction_s		s_options_controls_customize_keys_action;
static menuAction_s		s_options_controls_defaults_action;
static menuAction_s		s_options_controls_back_action;

//=======================================================================

static void CustomizeControlsFunc(void *unused)
{
	Menu_Keys_f ();
}

//=======================================================================

static void M_ControlsResetDefaults (void)
{
	Cvar_SetToDefault ("sensitivity");
	Cvar_SetToDefault ("m_noaccel");
	Cvar_SetToDefault ("m_filter");
//	Cvar_SetToDefault ("m_pitch");
	Cvar_SetToDefault ("in_autosensitivity");
	Cvar_SetToDefault ("cg_thirdperson");
	Cvar_SetToDefault ("cg_thirdperson_dist");
	Cvar_SetToDefault ("cg_thirdperson_offset");
	Cvar_SetToDefault ("cg_thirdperson_angle");
//	Cvar_SetToDefault ("cl_run");
//	Cvar_SetToDefault ("lookspring");
//	Cvar_SetToDefault ("lookstrafe");
//	Cvar_SetToDefault ("freelook");
	Cvar_SetToDefault ("in_joystick");

	Cbuf_AddText ("exec defaultbinds.cfg\n"); // reset default binds
	Cbuf_Execute ();
}

//=======================================================================

void Menu_Options_Controls_Init (void)
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *mouseaccel_values[] =
	{
		"1",
		"0",
		0
	};
	int		x, y;

	// menu.x = 320, menu.y = 162
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 78;

	s_options_controls_menu.x				= 0;	// SCREEN_WIDTH*0.5;
	s_options_controls_menu.y				= 0;	// SCREEN_HEIGHT*0.5 - 58;
	s_options_controls_menu.nitems			= 0;
	s_options_controls_menu.isPopup			= false;
	s_options_controls_menu.background		= NULL;
	s_options_controls_menu.drawFunc		= UI_DefaultMenuDraw;
	s_options_controls_menu.keyFunc			= UI_DefaultMenuKey;
	s_options_controls_menu.canOpenFunc		= NULL;
	s_options_controls_menu.defaultsFunc	= M_ControlsResetDefaults;
	s_options_controls_menu.defaultsMessage	= "Reset all Control settings to defaults?";

	s_options_controls_banner.generic.type		= MTYPE_IMAGE;
	s_options_controls_banner.generic.x			= 0;
	s_options_controls_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_options_controls_banner.width				= 275;
	s_options_controls_banner.height			= 32;
	s_options_controls_banner.imageName			= "/pics/m_banner_options.pcx";
	s_options_controls_banner.alpha				= 255;
	s_options_controls_banner.border			= 0;
	s_options_controls_banner.hCentered			= true;
	s_options_controls_banner.vCentered			= false;
	s_options_controls_banner.generic.isHidden	= false;

	s_options_controls_header.generic.type		= MTYPE_LABEL;
	s_options_controls_header.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_controls_header.generic.name		= "Controls";
	s_options_controls_header.generic.x			= x + MENU_HEADER_FONT_SIZE/2 * (int)strlen(s_options_controls_header.generic.name);
	s_options_controls_header.generic.y			= y;	// + -2*MENU_LINE_SIZE

	s_options_controls_sensitivity_slider.generic.type		= MTYPE_SLIDER;
	s_options_controls_sensitivity_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_sensitivity_slider.generic.x			= x;
	s_options_controls_sensitivity_slider.generic.y			= y += 4*MENU_LINE_SIZE;
	s_options_controls_sensitivity_slider.generic.name		= "mouse speed";
	s_options_controls_sensitivity_slider.maxPos			= 20;
	s_options_controls_sensitivity_slider.baseValue			= 1.0f;
	s_options_controls_sensitivity_slider.increment			= 0.5f;
	s_options_controls_sensitivity_slider.displayAsPercent	= false;
	s_options_controls_sensitivity_slider.generic.cvar		= "sensitivity";
	s_options_controls_sensitivity_slider.generic.cvarClamp	= true;
	s_options_controls_sensitivity_slider.generic.cvarMin	= 1.0f;
	s_options_controls_sensitivity_slider.generic.cvarMax	= 11.0f;
	s_options_controls_sensitivity_slider.generic.statusbar	= "changes sensitivity of mouse for head movement";

	s_options_controls_mouseaccel_box.generic.type			= MTYPE_PICKER;
	s_options_controls_mouseaccel_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_controls_mouseaccel_box.generic.x				= x;
	s_options_controls_mouseaccel_box.generic.y				= y+=MENU_LINE_SIZE;
	s_options_controls_mouseaccel_box.generic.name			= "mouse acceleration";
	s_options_controls_mouseaccel_box.itemNames				= yesno_names;
	s_options_controls_mouseaccel_box.itemValues			= mouseaccel_values;
	s_options_controls_mouseaccel_box.generic.cvar			= "m_noaccel";
	s_options_controls_mouseaccel_box.generic.cvarClamp		= false;
	s_options_controls_mouseaccel_box.generic.statusbar		= "enables mouse acceleration";

	s_options_controls_mousefilter_box.generic.type			= MTYPE_PICKER;
	s_options_controls_mousefilter_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_controls_mousefilter_box.generic.x			= x;
	s_options_controls_mousefilter_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_mousefilter_box.generic.name			= "mouse filtering";
	s_options_controls_mousefilter_box.itemNames			= yesno_names;
	s_options_controls_mousefilter_box.generic.cvar			= "m_filter";
	s_options_controls_mousefilter_box.generic.statusbar	= "enables filtering of mouse input";

	s_options_controls_invertmouse_box.generic.type			= MTYPE_PICKER;
	s_options_controls_invertmouse_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_controls_invertmouse_box.generic.x			= x;
	s_options_controls_invertmouse_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_invertmouse_box.generic.name			= "invert mouse";
	s_options_controls_invertmouse_box.itemNames			= yesno_names;
	s_options_controls_invertmouse_box.invertValue			= true;
	s_options_controls_invertmouse_box.generic.cvar			= "m_pitch";
	s_options_controls_invertmouse_box.generic.statusbar	= "inverts mouse y-axis movement";

	s_options_controls_autosensitivity_box.generic.type			= MTYPE_PICKER;
	s_options_controls_autosensitivity_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_controls_autosensitivity_box.generic.x			= x;
	s_options_controls_autosensitivity_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_autosensitivity_box.generic.name			= "scale mouse to FOV";
	s_options_controls_autosensitivity_box.itemNames			= yesno_names;
	s_options_controls_autosensitivity_box.generic.cvar			= "in_autosensitivity";
	s_options_controls_autosensitivity_box.generic.statusbar	= "adjusts mouse sensitivity to zoomed FOV";
	
	s_options_controls_thirdperson_box.generic.type			= MTYPE_PICKER;
	s_options_controls_thirdperson_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_controls_thirdperson_box.generic.x			= x;
	s_options_controls_thirdperson_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_controls_thirdperson_box.generic.name			= "third person";
	s_options_controls_thirdperson_box.itemNames			= yesno_names;
	s_options_controls_thirdperson_box.generic.cvar			= "cg_thirdperson";
	s_options_controls_thirdperson_box.generic.statusbar	= "enables third-person mode";

	s_options_controls_thirdperson_distance_slider.generic.type			= MTYPE_SLIDER;
	s_options_controls_thirdperson_distance_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_controls_thirdperson_distance_slider.generic.x			= x;
	s_options_controls_thirdperson_distance_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_thirdperson_distance_slider.generic.name			= "camera distance";
	s_options_controls_thirdperson_distance_slider.maxPos				= 10;
	s_options_controls_thirdperson_distance_slider.baseValue			= 25.0f;
	s_options_controls_thirdperson_distance_slider.increment			= 5.0f;
	s_options_controls_thirdperson_distance_slider.displayAsPercent		= false;
	s_options_controls_thirdperson_distance_slider.generic.cvar			= "cg_thirdperson_dist";
	s_options_controls_thirdperson_distance_slider.generic.cvarClamp	= true;
	s_options_controls_thirdperson_distance_slider.generic.cvarMin		= 25;
	s_options_controls_thirdperson_distance_slider.generic.cvarMax		= 75;
	s_options_controls_thirdperson_distance_slider.generic.statusbar	= "changes camera distance in third-person mode";

	s_options_controls_thirdperson_offset_slider.generic.type		= MTYPE_SLIDER;
	s_options_controls_thirdperson_offset_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_thirdperson_offset_slider.generic.x			= x;
	s_options_controls_thirdperson_offset_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_thirdperson_offset_slider.generic.name		= "camera offset";
	s_options_controls_thirdperson_offset_slider.maxPos				= 8;
	s_options_controls_thirdperson_offset_slider.baseValue			= 0.0f;
	s_options_controls_thirdperson_offset_slider.increment			= 4.0f;
	s_options_controls_thirdperson_offset_slider.displayAsPercent	= false;
	s_options_controls_thirdperson_offset_slider.generic.cvar		= "cg_thirdperson_offset";
	s_options_controls_thirdperson_offset_slider.generic.cvarClamp	= true;
	s_options_controls_thirdperson_offset_slider.generic.cvarMin	= 0;
	s_options_controls_thirdperson_offset_slider.generic.cvarMax	= 32;
	s_options_controls_thirdperson_offset_slider.generic.statusbar	= "changes horizontal camera offset in third-person mode";

	s_options_controls_thirdperson_angle_slider.generic.type		= MTYPE_SLIDER;
	s_options_controls_thirdperson_angle_slider.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_thirdperson_angle_slider.generic.x			= x;
	s_options_controls_thirdperson_angle_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_thirdperson_angle_slider.generic.name		= "camera angle";
	s_options_controls_thirdperson_angle_slider.maxPos				= 6;
	s_options_controls_thirdperson_angle_slider.baseValue			= 0.0f;
	s_options_controls_thirdperson_angle_slider.increment			= 5.0f;
	s_options_controls_thirdperson_angle_slider.displayAsPercent	= false;
	s_options_controls_thirdperson_angle_slider.generic.cvar		= "cg_thirdperson_angle";
	s_options_controls_thirdperson_angle_slider.generic.cvarClamp	= true;
	s_options_controls_thirdperson_angle_slider.generic.cvarMin		= 0;
	s_options_controls_thirdperson_angle_slider.generic.cvarMax		= 30;
	s_options_controls_thirdperson_angle_slider.generic.statusbar	= "changes camera angle in third-person mode";

	s_options_controls_alwaysrun_box.generic.type		= MTYPE_PICKER;
	s_options_controls_alwaysrun_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_alwaysrun_box.generic.x			= x;
	s_options_controls_alwaysrun_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_options_controls_alwaysrun_box.generic.name		= "always run";
	s_options_controls_alwaysrun_box.itemNames			= yesno_names;
	s_options_controls_alwaysrun_box.generic.cvar		= "cl_run";
	s_options_controls_alwaysrun_box.generic.statusbar	= "enables running as default movement";

	s_options_controls_lookspring_box.generic.type		= MTYPE_PICKER;
	s_options_controls_lookspring_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_lookspring_box.generic.x			= x;
	s_options_controls_lookspring_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_controls_lookspring_box.generic.name		= "lookspring";
	s_options_controls_lookspring_box.itemNames			= yesno_names;
	s_options_controls_lookspring_box.generic.cvar		= "lookspring";

	s_options_controls_lookstrafe_box.generic.type		= MTYPE_PICKER;
	s_options_controls_lookstrafe_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_lookstrafe_box.generic.x			= x;
	s_options_controls_lookstrafe_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_controls_lookstrafe_box.generic.name		= "lookstrafe";
	s_options_controls_lookstrafe_box.itemNames			= yesno_names;
	s_options_controls_lookstrafe_box.generic.cvar		= "lookstrafe";

	s_options_controls_freelook_box.generic.type		= MTYPE_PICKER;
	s_options_controls_freelook_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_freelook_box.generic.x			= x;
	s_options_controls_freelook_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_controls_freelook_box.generic.name		= "free look";
	s_options_controls_freelook_box.itemNames			= yesno_names;
	s_options_controls_freelook_box.generic.cvar		= "freelook";
	s_options_controls_freelook_box.generic.statusbar	= "enables free head movement with mouse";

	s_options_controls_joystick_box.generic.type		= MTYPE_PICKER;
	s_options_controls_joystick_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_joystick_box.generic.x			= x;
	s_options_controls_joystick_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_controls_joystick_box.generic.name		= "use joystick";
	s_options_controls_joystick_box.itemNames			= yesno_names;
	s_options_controls_joystick_box.generic.cvar		= "in_joystick";
	s_options_controls_joystick_box.generic.statusbar	= "enables use of joystick";

	s_options_controls_customize_keys_action.generic.type		= MTYPE_ACTION;
	s_options_controls_customize_keys_action.generic.textSize	= MENU_FONT_SIZE;
	s_options_controls_customize_keys_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_controls_customize_keys_action.generic.y			= 37*MENU_LINE_SIZE;
	s_options_controls_customize_keys_action.generic.name		= "Customize Controls";
	s_options_controls_customize_keys_action.generic.callback	= CustomizeControlsFunc;

	s_options_controls_defaults_action.generic.type			= MTYPE_ACTION;
	s_options_controls_defaults_action.generic.textSize		= MENU_FONT_SIZE;
	s_options_controls_defaults_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_controls_defaults_action.generic.y			= 39*MENU_LINE_SIZE;
	s_options_controls_defaults_action.generic.name			= "Reset to Defaults";
	s_options_controls_defaults_action.generic.callback		= UI_Defaults_Popup;	// M_ControlsResetDefaults
	s_options_controls_defaults_action.generic.statusbar	= "resets all control settings to internal defaults";

	s_options_controls_back_action.generic.type			= MTYPE_ACTION;
	s_options_controls_back_action.generic.textSize		= MENU_FONT_SIZE;
	s_options_controls_back_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_controls_back_action.generic.y			= 41*MENU_LINE_SIZE;
	s_options_controls_back_action.generic.name			= "Back to Options";
	s_options_controls_back_action.generic.callback		= UI_BackMenu;

	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_banner);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_header);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_sensitivity_slider);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_mouseaccel_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_mousefilter_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_invertmouse_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_autosensitivity_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_thirdperson_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_thirdperson_distance_slider);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_thirdperson_offset_slider);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_thirdperson_angle_slider);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_alwaysrun_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_lookspring_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_lookstrafe_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_freelook_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_joystick_box);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_customize_keys_action);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_defaults_action);
	UI_AddMenuItem (&s_options_controls_menu, (void *) &s_options_controls_back_action);
}


void Menu_Options_Controls_f (void)
{
	Menu_Options_Controls_Init ();
	UI_PushMenu (&s_options_controls_menu);
}
