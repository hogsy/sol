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

// menu_options_effects.c -- the effects options menu

#include "../client/client.h"
#include "ui_local.h"

#define USE_COMBOBOX

/*
=======================================================================

EFFECTS MENU

=======================================================================
*/

static menuFramework_s	s_options_effects_menu;
static menuImage_s		s_options_effects_banner;
static menuLabel_s		s_options_effects_header;

#ifdef USE_COMBOBOX
static menuComboBox_s	s_options_effects_blood_box;
#else
static menuPicker_s		s_options_effects_blood_box;
#endif	// USE_COMBOBOX

static menuPicker_s		s_options_effects_oldexplosions_box;
static menuPicker_s		s_options_effects_plasmaexplosound_box;
static menuPicker_s		s_options_effects_itembob_box;
static menuPicker_s		s_options_effects_footstep_box;
static menuSlider_s		s_options_effects_decal_slider;
static menuSlider_s		s_options_effects_particle_comp_slider;

#ifdef USE_COMBOBOX
static menuComboBox_s	s_options_effects_railtrail_box;
#else
static menuPicker_s		s_options_effects_railtrail_box;
#endif	// USE_COMBOBOX

static menuSlider_s		s_options_effects_railcolor_slider[3];
static menuAction_s		s_options_effects_defaults_action;
static menuAction_s		s_options_effects_back_action;

//=======================================================================

static void M_EffectsResetDefaults (void)
{
	Cvar_SetToDefault ("cl_blood");
	Cvar_SetToDefault ("cl_old_explosions");
	Cvar_SetToDefault ("cl_plasma_explo_sound");
	Cvar_SetToDefault ("cl_item_bobbing");
	Cvar_SetToDefault ("cl_footstep_override");
	Cvar_SetToDefault ("r_decals");
	Cvar_SetToDefault ("cl_particle_scale");
	Cvar_SetToDefault ("cl_railtype");
	Cvar_SetToDefault ("cl_railred");
	Cvar_SetToDefault ("cl_railgreen");
	Cvar_SetToDefault ("cl_railblue");	
}

//=======================================================================

void Options_Effects_MenuInit (void)
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *blood_names[] =
	{
		"none",
		"puff",
		"splat",
		"bleed",
		"gore",
		0
	};
	static const char *railtrail_names[] =
	{
		"colored spiral",
		"colored beam", // laser
		"colored devrail",
		0
	};
	int		x, y;

	// menu.x = 320, menu.y = 162
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 78;

	s_options_effects_menu.x				= 0;	// SCREEN_WIDTH*0.5;
	s_options_effects_menu.y				= 0;	// SCREEN_HEIGHT*0.5 - 58;
	s_options_effects_menu.nitems			= 0;
	s_options_effects_menu.isPopup			= false;
	s_options_effects_menu.background		= NULL;
	s_options_effects_menu.drawFunc			= UI_DefaultMenuDraw;
	s_options_effects_menu.keyFunc			= UI_DefaultMenuKey;
	s_options_effects_menu.canOpenFunc		= NULL;
	s_options_effects_menu.defaultsFunc		= M_EffectsResetDefaults;
	s_options_effects_menu.defaultsMessage	= "Reset all Effects settings to defaults?";

	s_options_effects_banner.generic.type		= MTYPE_IMAGE;
	s_options_effects_banner.generic.x			= 0;
	s_options_effects_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_options_effects_banner.width				= 275;
	s_options_effects_banner.height				= 32;
	s_options_effects_banner.imageName			= "/pics/m_banner_options.pcx";
	s_options_effects_banner.alpha				= 255;
	s_options_effects_banner.border				= 0;
	s_options_effects_banner.hCentered			= true;
	s_options_effects_banner.vCentered			= false;
	s_options_effects_banner.generic.isHidden	= false;

	s_options_effects_header.generic.type			= MTYPE_LABEL;
	s_options_effects_header.generic.textSize		= MENU_HEADER_FONT_SIZE;
	s_options_effects_header.generic.name			= "Effects";
	s_options_effects_header.generic.x				= x + MENU_HEADER_FONT_SIZE/2 * (int)strlen(s_options_effects_header.generic.name);
	s_options_effects_header.generic.y				= y;	//  + -2*MENU_LINE_SIZE
	
#ifdef USE_COMBOBOX
	s_options_effects_blood_box.generic.type		= MTYPE_COMBOBOX;
	s_options_effects_blood_box.generic.x			= x;
	s_options_effects_blood_box.generic.y			= y += 4*MENU_LINE_SIZE;
	s_options_effects_blood_box.generic.name		= "blood type";
	s_options_effects_blood_box.itemNames			= blood_names;
	s_options_effects_blood_box.items_y				= 5;
	s_options_effects_blood_box.itemWidth			= 6;
	s_options_effects_blood_box.itemSpacing			= 1;
	s_options_effects_blood_box.itemTextSize		= 8;
	s_options_effects_blood_box.border				= 1;
	s_options_effects_blood_box.borderColor[0]		= 60;
	s_options_effects_blood_box.borderColor[1]		= 60;
	s_options_effects_blood_box.borderColor[2]		= 60;
	s_options_effects_blood_box.borderColor[3]		= 255;
	s_options_effects_blood_box.backColor[0]		= 0;
	s_options_effects_blood_box.backColor[1]		= 0;
	s_options_effects_blood_box.backColor[2]		= 0;
	s_options_effects_blood_box.backColor[3]		= 192;
	s_options_effects_blood_box.generic.cvar		= "cl_blood";
	s_options_effects_blood_box.generic.cvarClamp	= true;
	s_options_effects_blood_box.generic.cvarMin		= 0;
	s_options_effects_blood_box.generic.cvarMax		= 4;
	s_options_effects_blood_box.generic.statusbar	= "changes blood effect type";
#else	// USE_COMBOBOX
	s_options_effects_blood_box.generic.type		= MTYPE_PICKER;
	s_options_effects_blood_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_blood_box.generic.x			= x;
	s_options_effects_blood_box.generic.y			= y += 4*MENU_LINE_SIZE;
	s_options_effects_blood_box.generic.name		= "blood type";
	s_options_effects_blood_box.itemNames			= blood_names;
	s_options_effects_blood_box.generic.cvar		= "cl_blood";
	s_options_effects_blood_box.generic.cvarClamp	= true;
	s_options_effects_blood_box.generic.cvarMin		= 0;
	s_options_effects_blood_box.generic.cvarMax		= 4;
	s_options_effects_blood_box.generic.statusbar	= "changes blood effect type";
#endif	// USE_COMBOBOX

	s_options_effects_oldexplosions_box.generic.type		= MTYPE_PICKER;
	s_options_effects_oldexplosions_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_oldexplosions_box.generic.x			= x;
	s_options_effects_oldexplosions_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_options_effects_oldexplosions_box.generic.name		= "old style explosions";
	s_options_effects_oldexplosions_box.itemNames			= yesno_names;
	s_options_effects_oldexplosions_box.generic.cvar		= "cl_old_explosions";
	s_options_effects_oldexplosions_box.generic.statusbar	= "brings back those cheesy model explosions";

	s_options_effects_plasmaexplosound_box.generic.type			= MTYPE_PICKER;
	s_options_effects_plasmaexplosound_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_effects_plasmaexplosound_box.generic.x			= x;
	s_options_effects_plasmaexplosound_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_plasmaexplosound_box.generic.name			= "unique plasma explode sound";
	s_options_effects_plasmaexplosound_box.itemNames			= yesno_names;
	s_options_effects_plasmaexplosound_box.generic.cvar			= "cl_plasma_explo_sound";
	s_options_effects_plasmaexplosound_box.generic.statusbar	= "gives Phalanx Cannon plasma explosions a unique sound";

	s_options_effects_itembob_box.generic.type					= MTYPE_PICKER;
	s_options_effects_itembob_box.generic.textSize				= MENU_FONT_SIZE;
	s_options_effects_itembob_box.generic.x						= x;
	s_options_effects_itembob_box.generic.y						= y += MENU_LINE_SIZE;
	s_options_effects_itembob_box.generic.name					= "item bobbing";
	s_options_effects_itembob_box.itemNames						= yesno_names;
	s_options_effects_itembob_box.generic.cvar					= "cl_item_bobbing";
	s_options_effects_itembob_box.generic.statusbar				= "adds bobbing effect to rotating items";

	// foostep override option
	s_options_effects_footstep_box.generic.type					= MTYPE_PICKER;
	s_options_effects_footstep_box.generic.textSize				= MENU_FONT_SIZE;
	s_options_effects_footstep_box.generic.x					= x;
	s_options_effects_footstep_box.generic.y					= y += 2*MENU_LINE_SIZE;
	s_options_effects_footstep_box.generic.name					= "override footstep sounds";
	s_options_effects_footstep_box.itemNames					= yesno_names;
	s_options_effects_footstep_box.generic.cvar					= "cl_footstep_override";
	s_options_effects_footstep_box.generic.statusbar			= "sets footstep sounds with definitions in texsurfs.txt";

	s_options_effects_decal_slider.generic.type					= MTYPE_SLIDER;
	s_options_effects_decal_slider.generic.textSize				= MENU_FONT_SIZE;
	s_options_effects_decal_slider.generic.x					= x;
	s_options_effects_decal_slider.generic.y					= y += 2*MENU_LINE_SIZE;
	s_options_effects_decal_slider.generic.name					= "decal quantity";
	s_options_effects_decal_slider.maxPos						= 20;
	s_options_effects_decal_slider.baseValue					= 0.0f;
	s_options_effects_decal_slider.increment					= 50.0f;
	s_options_effects_decal_slider.displayAsPercent				= false;
	s_options_effects_decal_slider.generic.cvar					= "r_decals";
	s_options_effects_decal_slider.generic.cvarClamp			= true;
	s_options_effects_decal_slider.generic.cvarMin				= 0;
	s_options_effects_decal_slider.generic.cvarMax				= 1000;
	s_options_effects_decal_slider.generic.statusbar			= "how many decals to display at once (max = 1000)";

	s_options_effects_particle_comp_slider.generic.type			= MTYPE_SLIDER;
	s_options_effects_particle_comp_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_effects_particle_comp_slider.generic.x			= x;
	s_options_effects_particle_comp_slider.generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_particle_comp_slider.generic.name			= "particle effect complexity";
	s_options_effects_particle_comp_slider.maxPos				= 4;
	s_options_effects_particle_comp_slider.baseValue			= 5.0f;
	s_options_effects_particle_comp_slider.increment			= -1.0f;
	s_options_effects_particle_comp_slider.displayAsPercent		= false;
	s_options_effects_particle_comp_slider.generic.cvar			= "cl_particle_scale";
	s_options_effects_particle_comp_slider.generic.cvarClamp	= true;
	s_options_effects_particle_comp_slider.generic.cvarMin		= 1;
	s_options_effects_particle_comp_slider.generic.cvarMax		= 5;
	s_options_effects_particle_comp_slider.generic.statusbar	= "lower = faster performance";

	// Psychospaz's changeable rail trail
#ifdef USE_COMBOBOX
	s_options_effects_railtrail_box.generic.type			= MTYPE_COMBOBOX;
	s_options_effects_railtrail_box.generic.x				= x;
	s_options_effects_railtrail_box.generic.y				= y += 2*MENU_LINE_SIZE;
	s_options_effects_railtrail_box.generic.name			= "rail trail type";
	s_options_effects_railtrail_box.itemNames				= railtrail_names;
	s_options_effects_railtrail_box.items_y					= 3;
	s_options_effects_railtrail_box.itemWidth				= 16;
	s_options_effects_railtrail_box.itemSpacing				= 1;
	s_options_effects_railtrail_box.itemTextSize			= 8;
	s_options_effects_railtrail_box.border					= 1;
	s_options_effects_railtrail_box.borderColor[0]			= 60;
	s_options_effects_railtrail_box.borderColor[1]			= 60;
	s_options_effects_railtrail_box.borderColor[2]			= 60;
	s_options_effects_railtrail_box.borderColor[3]			= 255;
	s_options_effects_railtrail_box.backColor[0]			= 0;
	s_options_effects_railtrail_box.backColor[1]			= 0;
	s_options_effects_railtrail_box.backColor[2]			= 0;
	s_options_effects_railtrail_box.backColor[3]			= 192;
	s_options_effects_railtrail_box.generic.cvar			= "cl_railtype";
	s_options_effects_railtrail_box.generic.cvarClamp		= true;
	s_options_effects_railtrail_box.generic.cvarMin			= 0;
	s_options_effects_railtrail_box.generic.cvarMax			= 2;
	s_options_effects_railtrail_box.generic.statusbar		= "changes railgun particle effect";
#else	// USE_COMBOBOX
	s_options_effects_railtrail_box.generic.type			= MTYPE_PICKER;
	s_options_effects_railtrail_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_effects_railtrail_box.generic.x				= x;
	s_options_effects_railtrail_box.generic.y				= y += 2*MENU_LINE_SIZE;
	s_options_effects_railtrail_box.generic.name			= "rail trail type";
	s_options_effects_railtrail_box.itemNames				= railtrail_names;
	s_options_effects_railtrail_box.generic.cvar			= "cl_railtype";
	s_options_effects_railtrail_box.generic.cvarClamp		= true;
	s_options_effects_railtrail_box.generic.cvarMin			= 0;
	s_options_effects_railtrail_box.generic.cvarMax			= 2;
	s_options_effects_railtrail_box.generic.statusbar		= "changes railgun particle effect";
#endif	// USE_COMBOBOX

	s_options_effects_railcolor_slider[0].generic.type		= MTYPE_SLIDER;
	s_options_effects_railcolor_slider[0].generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_railcolor_slider[0].generic.x			= x;
	s_options_effects_railcolor_slider[0].generic.y			= y += 1.5*MENU_LINE_SIZE;
	s_options_effects_railcolor_slider[0].generic.name		= "rail trail: red";
	s_options_effects_railcolor_slider[0].maxPos			= 64;
	s_options_effects_railcolor_slider[0].baseValue			= 0.0f;
	s_options_effects_railcolor_slider[0].increment			= 4.0f;
	s_options_effects_railcolor_slider[0].displayAsPercent	= false;
	s_options_effects_railcolor_slider[0].generic.cvar		= "cl_railred";
	s_options_effects_railcolor_slider[0].generic.cvarClamp	= true;
	s_options_effects_railcolor_slider[0].generic.cvarMin	= 0;
	s_options_effects_railcolor_slider[0].generic.cvarMax	= 256;
	s_options_effects_railcolor_slider[0].generic.statusbar	= "changes railgun particle effect red component";

	s_options_effects_railcolor_slider[1].generic.type		= MTYPE_SLIDER;
	s_options_effects_railcolor_slider[1].generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_railcolor_slider[1].generic.x			= x;
	s_options_effects_railcolor_slider[1].generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_railcolor_slider[1].generic.name		= "rail trail: green";
	s_options_effects_railcolor_slider[1].maxPos			= 64;
	s_options_effects_railcolor_slider[1].baseValue			= 0.0f;
	s_options_effects_railcolor_slider[1].increment			= 4.0f;
	s_options_effects_railcolor_slider[1].displayAsPercent	= false;
	s_options_effects_railcolor_slider[1].generic.cvar		= "cl_railgreen";
	s_options_effects_railcolor_slider[1].generic.cvarClamp	= true;
	s_options_effects_railcolor_slider[1].generic.cvarMin	= 0;
	s_options_effects_railcolor_slider[1].generic.cvarMax	= 256;
	s_options_effects_railcolor_slider[1].generic.statusbar	= "changes railgun particle effect green component";

	s_options_effects_railcolor_slider[2].generic.type		= MTYPE_SLIDER;
	s_options_effects_railcolor_slider[2].generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_railcolor_slider[2].generic.x			= x;
	s_options_effects_railcolor_slider[2].generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_railcolor_slider[2].generic.name		= "rail trail: blue";
	s_options_effects_railcolor_slider[2].maxPos			= 64;
	s_options_effects_railcolor_slider[2].baseValue			= 0.0f;
	s_options_effects_railcolor_slider[2].increment			= 4.0f;
	s_options_effects_railcolor_slider[2].displayAsPercent	= false;
	s_options_effects_railcolor_slider[2].generic.cvar		= "cl_railblue";
	s_options_effects_railcolor_slider[2].generic.cvarClamp	= true;
	s_options_effects_railcolor_slider[2].generic.cvarMin	= 0;
	s_options_effects_railcolor_slider[2].generic.cvarMax	= 256;
	s_options_effects_railcolor_slider[2].generic.statusbar	= "changes railgun particle effect blue component";

	s_options_effects_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_effects_defaults_action.generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_defaults_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_effects_defaults_action.generic.y			= 39*MENU_LINE_SIZE;
	s_options_effects_defaults_action.generic.name		= "Reset to Defaults";
	s_options_effects_defaults_action.generic.callback	= UI_Defaults_Popup;	// M_EffectsResetDefaults
	s_options_effects_defaults_action.generic.statusbar	= "resets all effects settings to internal defaults";

	s_options_effects_back_action.generic.type			= MTYPE_ACTION;
	s_options_effects_back_action.generic.textSize		= MENU_FONT_SIZE;
	s_options_effects_back_action.generic.x				= x + MENU_FONT_SIZE;
	s_options_effects_back_action.generic.y				= 41*MENU_LINE_SIZE;
	s_options_effects_back_action.generic.name			= "Back to Options";
	s_options_effects_back_action.generic.callback		= UI_BackMenu;

	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_banner);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_header);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_blood_box);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_oldexplosions_box);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_plasmaexplosound_box);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_itembob_box);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_footstep_box);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_decal_slider);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_particle_comp_slider);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_railtrail_box);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_railcolor_slider[0]);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_railcolor_slider[1]);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_railcolor_slider[2]);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_defaults_action);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_back_action);
}


void Menu_Options_Effects_f (void)
{
	Options_Effects_MenuInit ();
	UI_PushMenu (&s_options_effects_menu);
}
