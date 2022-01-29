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

/*
=======================================================================

EFFECTS MENU

=======================================================================
*/

static menuFramework_s	s_options_effects_menu;
static menuImage_s		s_options_effects_banner;
static menuLabel_s		s_options_effects_header;
static menuPicker_s	s_options_effects_blood_box;
static menuPicker_s	s_options_effects_oldexplosions_box;
static menuPicker_s	s_options_effects_plasmaexplosound_box;
static menuPicker_s	s_options_effects_itembob_box;
static menuSlider_s		s_options_effects_decal_slider;
static menuSlider_s		s_options_effects_particle_comp_slider;
static menuPicker_s	s_options_effects_railtrail_box;
static menuSlider_s		s_options_effects_railcolor_slider[3];
static menuPicker_s	s_options_effects_footstep_box;
static menuAction_s		s_options_effects_defaults_action;
static menuAction_s		s_options_effects_back_action;

//=======================================================================

static void BloodFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_effects_blood_box, "cl_blood"); 
}

static void OldExplosionFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_effects_oldexplosions_box, "cl_old_explosions");
}

static void PlasmaExploSoundFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_effects_plasmaexplosound_box, "cl_plasma_explo_sound");
}

static void ItemBobFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_effects_itembob_box, "cl_item_bobbing");
}

static void ParticleCompFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_effects_particle_comp_slider, "cl_particle_scale");
}

static void DecalCallback (void *unused)
{
	MenuSlider_SaveValue (&s_options_effects_decal_slider, "r_decals");
}

// Psychospaz's changeable rail trail
static void RailTrailFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_effects_railtrail_box, "cl_railtype");
}

static void RailColorRedFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_effects_railcolor_slider[0], "cl_railred");
}

static void RailColorGreenFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_effects_railcolor_slider[1], "cl_railgreen");
}

static void RailColorBlueFunc (void *unused)
{
	MenuSlider_SaveValue (&s_options_effects_railcolor_slider[2], "cl_railblue");
}

// foostep override option
static void FootStepFunc (void *unused)
{
	MenuPicker_SaveValue (&s_options_effects_footstep_box, "cl_footstep_override");
}


//=======================================================================

static void M_EffectsSetMenuItemValues (void)
{
	MenuPicker_SetValue (&s_options_effects_blood_box, "cl_blood", 0, 4, true);

	MenuPicker_SetValue (&s_options_effects_oldexplosions_box, "cl_old_explosions", 0, 1, true);
	MenuPicker_SetValue (&s_options_effects_plasmaexplosound_box, "cl_plasma_explo_sound", 0, 1, true);
	MenuPicker_SetValue (&s_options_effects_itembob_box, "cl_item_bobbing", 0, 1, true);

	MenuSlider_SetValue (&s_options_effects_decal_slider, "r_decals", 0, 1000, true);
	MenuSlider_SetValue (&s_options_effects_particle_comp_slider, "cl_particle_scale", 1, 5, true);

	MenuPicker_SetValue (&s_options_effects_railtrail_box, "cl_railtype", 0, 2, true);
	MenuSlider_SetValue (&s_options_effects_railcolor_slider[0], "cl_railred", 0, 256, true);
	MenuSlider_SetValue (&s_options_effects_railcolor_slider[1], "cl_railgreen", 0, 256, true);
	MenuSlider_SetValue (&s_options_effects_railcolor_slider[2], "cl_railblue", 0, 256, true);

	MenuPicker_SetValue (&s_options_effects_footstep_box, "cl_footstep_override", 0, 1, true);
}

static void M_EffectsResetDefaultsFunc (void *unused)
{
	Cvar_SetToDefault ("cl_blood");
	Cvar_SetToDefault ("cl_old_explosions");
	Cvar_SetToDefault ("cl_plasma_explo_sound");
	Cvar_SetToDefault ("cl_item_bobbing");
	Cvar_SetToDefault ("r_decals");
	Cvar_SetToDefault ("cl_particle_scale");
	Cvar_SetToDefault ("cl_railtype");
	Cvar_SetToDefault ("cl_railred");
	Cvar_SetToDefault ("cl_railgreen");
	Cvar_SetToDefault ("cl_railblue");	
	Cvar_SetToDefault ("cl_footstep_override");

	M_EffectsSetMenuItemValues ();
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
	s_options_effects_menu.keyFunc			= UI_DefaultMenuKey;
	s_options_effects_menu.canOpenFunc		= NULL;
//	s_options_effects_menu.defaultsFunc		= M_EffectsResetDefaults;
//	s_options_effects_menu.defaultsMessage	= "Reset all Effects settings to defaults?";

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
	
	s_options_effects_blood_box.generic.type		= MTYPE_PICKER;
	s_options_effects_blood_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_blood_box.generic.x			= x;
	s_options_effects_blood_box.generic.y			= y += 4*MENU_LINE_SIZE;
	s_options_effects_blood_box.generic.name		= "blood type";
	s_options_effects_blood_box.generic.callback	= BloodFunc;
	s_options_effects_blood_box.itemNames			= blood_names;
	s_options_effects_blood_box.generic.statusbar	= "changes blood effect type";

	s_options_effects_oldexplosions_box.generic.type		= MTYPE_PICKER;
	s_options_effects_oldexplosions_box.generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_oldexplosions_box.generic.x			= x;
	s_options_effects_oldexplosions_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_options_effects_oldexplosions_box.generic.name		= "old style explosions";
	s_options_effects_oldexplosions_box.generic.callback	= OldExplosionFunc;
	s_options_effects_oldexplosions_box.itemNames			= yesno_names;
	s_options_effects_oldexplosions_box.generic.statusbar	= "brings back those cheesy model explosions";

	s_options_effects_plasmaexplosound_box.generic.type			= MTYPE_PICKER;
	s_options_effects_plasmaexplosound_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_effects_plasmaexplosound_box.generic.x			= x;
	s_options_effects_plasmaexplosound_box.generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_plasmaexplosound_box.generic.name			= "unique plasma explode sound";
	s_options_effects_plasmaexplosound_box.generic.callback		= PlasmaExploSoundFunc;
	s_options_effects_plasmaexplosound_box.itemNames			= yesno_names;
	s_options_effects_plasmaexplosound_box.generic.statusbar	= "gives Phalanx Cannon plasma explosions a unique sound";

	s_options_effects_itembob_box.generic.type					= MTYPE_PICKER;
	s_options_effects_itembob_box.generic.textSize				= MENU_FONT_SIZE;
	s_options_effects_itembob_box.generic.x						= x;
	s_options_effects_itembob_box.generic.y						= y += MENU_LINE_SIZE;
	s_options_effects_itembob_box.generic.name					= "item bobbing";
	s_options_effects_itembob_box.generic.callback				= ItemBobFunc;
	s_options_effects_itembob_box.itemNames						= yesno_names;
	s_options_effects_itembob_box.generic.statusbar				= "adds bobbing effect to rotating items";

	s_options_effects_decal_slider.generic.type					= MTYPE_SLIDER;
	s_options_effects_decal_slider.generic.textSize				= MENU_FONT_SIZE;
	s_options_effects_decal_slider.generic.x					= x;
	s_options_effects_decal_slider.generic.y					= y += 2*MENU_LINE_SIZE;
	s_options_effects_decal_slider.generic.name					= "decal quantity";
	s_options_effects_decal_slider.generic.callback				= DecalCallback;
	s_options_effects_decal_slider.maxPos						= 20;
	s_options_effects_decal_slider.baseValue					= 0.0f;
	s_options_effects_decal_slider.increment					= 50.0f;
	s_options_effects_decal_slider.displayAsPercent				= false;
	s_options_effects_decal_slider.generic.statusbar			= "how many decals to display at once (max = 1000)";

	s_options_effects_particle_comp_slider.generic.type			= MTYPE_SLIDER;
	s_options_effects_particle_comp_slider.generic.textSize		= MENU_FONT_SIZE;
	s_options_effects_particle_comp_slider.generic.x			= x;
	s_options_effects_particle_comp_slider.generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_particle_comp_slider.generic.name			= "particle effect complexity";
	s_options_effects_particle_comp_slider.generic.callback		= ParticleCompFunc;
	s_options_effects_particle_comp_slider.maxPos				= 4;
	s_options_effects_particle_comp_slider.baseValue			= 5.0f;
	s_options_effects_particle_comp_slider.increment			= -1.0f;
	s_options_effects_particle_comp_slider.displayAsPercent		= false;
	s_options_effects_particle_comp_slider.generic.statusbar	= "lower = faster performance";

	// Psychospaz's changeable rail trail
	s_options_effects_railtrail_box.generic.type			= MTYPE_PICKER;
	s_options_effects_railtrail_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_effects_railtrail_box.generic.x				= x;
	s_options_effects_railtrail_box.generic.y				= y += 2*MENU_LINE_SIZE;
	s_options_effects_railtrail_box.generic.name			= "rail trail type";
	s_options_effects_railtrail_box.generic.callback		= RailTrailFunc;
	s_options_effects_railtrail_box.itemNames				= railtrail_names;
	s_options_effects_railtrail_box.generic.statusbar		= "changes railgun particle effect";

	s_options_effects_railcolor_slider[0].generic.type		= MTYPE_SLIDER;
	s_options_effects_railcolor_slider[0].generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_railcolor_slider[0].generic.x			= x;
	s_options_effects_railcolor_slider[0].generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_railcolor_slider[0].generic.name		= "rail trail: red";
	s_options_effects_railcolor_slider[0].generic.callback	= RailColorRedFunc;
	s_options_effects_railcolor_slider[0].maxPos			= 64;
	s_options_effects_railcolor_slider[0].baseValue			= 0.0f;
	s_options_effects_railcolor_slider[0].increment			= 4.0f;
	s_options_effects_railcolor_slider[0].displayAsPercent	= false;
	s_options_effects_railcolor_slider[0].generic.statusbar	= "changes railgun particle effect red component";

	s_options_effects_railcolor_slider[1].generic.type		= MTYPE_SLIDER;
	s_options_effects_railcolor_slider[1].generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_railcolor_slider[1].generic.x			= x;
	s_options_effects_railcolor_slider[1].generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_railcolor_slider[1].generic.name		= "rail trail: green";
	s_options_effects_railcolor_slider[1].generic.callback	= RailColorGreenFunc;
	s_options_effects_railcolor_slider[1].maxPos			= 64;
	s_options_effects_railcolor_slider[1].baseValue			= 0.0f;
	s_options_effects_railcolor_slider[1].increment			= 4.0f;
	s_options_effects_railcolor_slider[1].displayAsPercent	= false;
	s_options_effects_railcolor_slider[1].generic.statusbar	= "changes railgun particle effect green component";

	s_options_effects_railcolor_slider[2].generic.type		= MTYPE_SLIDER;
	s_options_effects_railcolor_slider[2].generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_railcolor_slider[2].generic.x			= x;
	s_options_effects_railcolor_slider[2].generic.y			= y += MENU_LINE_SIZE;
	s_options_effects_railcolor_slider[2].generic.name		= "rail trail: blue";
	s_options_effects_railcolor_slider[2].generic.callback	= RailColorBlueFunc;
	s_options_effects_railcolor_slider[2].maxPos			= 64;
	s_options_effects_railcolor_slider[2].baseValue			= 0.0f;
	s_options_effects_railcolor_slider[2].increment			= 4.0f;
	s_options_effects_railcolor_slider[2].displayAsPercent	= false;
	s_options_effects_railcolor_slider[2].generic.statusbar	= "changes railgun particle effect blue component";

	// foostep override option
	s_options_effects_footstep_box.generic.type			= MTYPE_PICKER;
	s_options_effects_footstep_box.generic.textSize		= MENU_FONT_SIZE;
	s_options_effects_footstep_box.generic.x			= x;
	s_options_effects_footstep_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_options_effects_footstep_box.generic.name			= "override footstep sounds";
	s_options_effects_footstep_box.generic.callback		= FootStepFunc;
	s_options_effects_footstep_box.itemNames			= yesno_names;
	s_options_effects_footstep_box.generic.statusbar	= "sets footstep sounds with definitions in texsurfs.txt";

	s_options_effects_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_effects_defaults_action.generic.textSize	= MENU_FONT_SIZE;
	s_options_effects_defaults_action.generic.x			= x + MENU_FONT_SIZE;
	s_options_effects_defaults_action.generic.y			= 39*MENU_LINE_SIZE;
	s_options_effects_defaults_action.generic.name		= "Reset to Defaults";
	s_options_effects_defaults_action.generic.callback	= M_EffectsResetDefaultsFunc;
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
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_decal_slider);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_particle_comp_slider);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_railtrail_box);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_railcolor_slider[0]);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_railcolor_slider[1]);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_railcolor_slider[2]);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_footstep_box);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_defaults_action);
	UI_AddMenuItem (&s_options_effects_menu, (void *) &s_options_effects_back_action);

	M_EffectsSetMenuItemValues ();
}

void Menu_Options_Effects_Draw (void)
{
	UI_AdjustMenuCursor (&s_options_effects_menu, 1);
	UI_DrawMenu (&s_options_effects_menu);
}

const char *Menu_Options_Effects_Key (int key)
{
	return UI_DefaultMenuKey (&s_options_effects_menu, key);
}

void Menu_Options_Effects_f (void)
{
	Options_Effects_MenuInit ();
	UI_PushMenu (&s_options_effects_menu, Menu_Options_Effects_Draw, Menu_Options_Effects_Key);
}
