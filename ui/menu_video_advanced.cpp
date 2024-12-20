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

// menu_video_advanced.c -- the advanced video menu
 
#include "../client/client.h"
#include "ui_local.h"

/*
=======================================================================

ADVANCED VIDEO MENU

=======================================================================
*/
static menuFramework_s	s_video_advanced_menu;
static menuImage_s		s_video_advanced_banner;
static menuLabel_s		s_options_advanced_header;	

static menuSlider_s		s_lightmapscale_slider;
//static menuSlider_s		s_textureintensity_slider;
static menuPicker_s  	s_rgbscale_box;
static menuPicker_s  	s_trans_lighting_box;
static menuPicker_s  	s_warp_lighting_box;
static menuSlider_s		s_lightcutoff_slider;
static menuPicker_s  	s_solidalpha_box;
static menuPicker_s  	s_texshader_warp_box;
static menuSlider_s  	s_waterwave_slider;
static menuPicker_s  	s_caustics_box;
static menuPicker_s		s_particle_overdraw_box;
static menuPicker_s		s_flare_occlusionquery_box;
static menuPicker_s		s_lightbloom_box;
static menuPicker_s		s_modelshading_box;
static menuPicker_s		s_shadows_box;
static menuPicker_s		s_two_side_stencil_box;
static menuPicker_s  	s_ent_shell_box;
static menuPicker_s  	s_celshading_box;
static menuSlider_s  	s_celshading_width_slider;
static menuPicker_s  	s_glass_envmap_box;
static menuPicker_s  	s_screenshotformat_box;
static menuSlider_s  	s_screenshotjpegquality_slider;
static menuPicker_s		s_npot_mipmap_box;
static menuPicker_s		s_sgis_mipmap_box;
static menuPicker_s		s_upscale_font_box;

static menuAction_s		s_apply_action;
static menuAction_s		s_back_action;

//=======================================================================

static void M_ShowApplyChanges (void *unused)
{
	s_apply_action.generic.isHidden = ( !s_npot_mipmap_box.generic.valueChanged &&
										!s_sgis_mipmap_box.generic.valueChanged && 
									//	!s_textureintensity_slider.generic.valueChanged && 
										!s_upscale_font_box.generic.valueChanged );
}

//=======================================================================

static void M_ApplyAdvVideoChanges (void)
{
	// hide this again
	s_apply_action.generic.isHidden = true;

	// save values to cvars that don't instant-adjust
//	UI_SaveMenuItemValue (&s_textureintensity_slider);
	UI_SaveMenuItemValue (&s_npot_mipmap_box);
	UI_SaveMenuItemValue (&s_sgis_mipmap_box);
	UI_SaveMenuItemValue (&s_upscale_font_box);

	// tell them they're modified so they refresh
	Cvar_SetModified ("vid_ref", true);
}

//=======================================================================

void Menu_Video_Advanced_Init (void)
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *rgbscale_names[] =
	{
		"1x",
		"2x",
		"4x",
		0
	};
	static const char *rgbscale_values[] =
	{
		"1",
		"2",
		"4",
		0
	};
	static const char *lighting_names[] =
	{
		"no",
		"vertex",
		"lightmap",
		0
	};
	static const char *warp_lighting_names[] =
	{
		"no",
		"vertex",
		"lightmap (if available)",
		0
	};
	static const char *shading_names[] =
	{
		"off",
		"low",
		"medium",
		"high",
		0
	};
	static const char *shadow_names[] =
	{
		"no",
		"static planar",
		"dynamic planar",
		"projection",
		0
	};
	static const char *ifsupported_names[] =
	{
		"no",
		"if supported",
		0
	};
	static const char *caustics_names[] =
	{
		"no",
		"standard",
		"hardware warp (if supported)",
		0
	};
	static const char *shell_names[] =
	{
		"solid",
		"flowing",
		"envmap",
		0
	};
	static const char *screenshotformat_names[] =
	{
		"JPEG",
		"PNG",
		"TGA",
		0
	};
	static const char *screenshotformat_values[] =
	{
		"jpg",
		"png",
		"tga",
		0
	};
	static const char *font_upscale_names[] =
	{
		"no",
		"pixel copy",
		"blended",
		0
	};
	int		x, y;

	// menu.x = 320, menu.y = 140
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 10*MENU_LINE_SIZE;

	s_video_advanced_menu.x						= 0;	// SCREEN_WIDTH*0.5;
	s_video_advanced_menu.y						= 0;	// SCREEN_HEIGHT*0.5 - 100;
	s_video_advanced_menu.nitems				= 0;
	s_video_advanced_menu.isPopup				= false;
	s_video_advanced_menu.background			= NULL;
	s_video_advanced_menu.drawFunc				= UI_DefaultMenuDraw;
	s_video_advanced_menu.keyFunc				= UI_DefaultMenuKey;
	s_video_advanced_menu.canOpenFunc			= NULL;
	s_video_advanced_menu.applyChangesFunc		= M_ApplyAdvVideoChanges;
	s_video_advanced_menu.applyChangesMessage[0]	= "This will restart the video system to";
	s_video_advanced_menu.applyChangesMessage[1]	= "apply settings and return to the menu.";
	s_video_advanced_menu.applyChangesMessage[2]	= "Continue?";

	s_video_advanced_banner.generic.type		= MTYPE_IMAGE;
	s_video_advanced_banner.generic.x			= 0;
	s_video_advanced_banner.generic.y			= 9*MENU_LINE_SIZE;
	s_video_advanced_banner.width				= 275;
	s_video_advanced_banner.height				= 32;
	s_video_advanced_banner.imageName			= "/pics/m_banner_video.pcx";
	s_video_advanced_banner.alpha				= 255;
	s_video_advanced_banner.border				= 0;
	s_video_advanced_banner.hCentered			= true;
	s_video_advanced_banner.vCentered			= false;
	s_video_advanced_banner.useAspectRatio		= false;
	s_video_advanced_banner.generic.isHidden	= false;

	s_options_advanced_header.generic.type		= MTYPE_LABEL;
	s_options_advanced_header.generic.textSize	= MENU_HEADER_FONT_SIZE;
	s_options_advanced_header.generic.name		= "Advanced Options";
	s_options_advanced_header.generic.x			= x + MENU_HEADER_FONT_SIZE/2 * (int)strlen(s_options_advanced_header.generic.name);
	s_options_advanced_header.generic.y			= y;

	s_lightmapscale_slider.generic.type			= MTYPE_SLIDER;
	s_lightmapscale_slider.generic.textSize		= MENU_FONT_SIZE;
	s_lightmapscale_slider.generic.x			= x;
	s_lightmapscale_slider.generic.y			= y += 2*MENU_LINE_SIZE;
	s_lightmapscale_slider.generic.name			= "lightmap scale";
	s_lightmapscale_slider.maxPos				= 10;
	s_lightmapscale_slider.baseValue			= 1.0f;
	s_lightmapscale_slider.increment			= 0.1f;
	s_lightmapscale_slider.displayAsPercent		= false;
	s_lightmapscale_slider.generic.cvar			= "r_modulate";
	s_lightmapscale_slider.generic.cvarClamp	= true;
	s_lightmapscale_slider.generic.cvarMin		= 1;
	s_lightmapscale_slider.generic.cvarMax		= 2;
	s_lightmapscale_slider.generic.statusbar	= "leave at minimum, washes out textures";
/*
	s_textureintensity_slider.generic.type		= MTYPE_SLIDER;
	s_textureintensity_slider.generic.textSize	= MENU_FONT_SIZE;
	s_textureintensity_slider.generic.x			= x;
	s_textureintensity_slider.generic.y			= y += MENU_LINE_SIZE;
	s_textureintensity_slider.generic.name		= "texture intensity";
	s_textureintensity_slider.maxPos			= 10;
	s_textureintensity_slider.baseValue			= 1.0f;
	s_textureintensity_slider.increment			= 0.1f;
	s_textureintensity_slider.displayAsPercent	= false;
	s_textureintensity_slider.generic.cvar		= "r_intensity";
	s_textureintensity_slider.generic.cvarClamp	= true;
	s_textureintensity_slider.generic.cvarMin	= 1;
	s_textureintensity_slider.generic.cvarMax	= 2;
	s_textureintensity_slider.generic.statusbar	= "leave at minimum, washes out textures";
	s_textureintensity_slider.generic.callback	= M_ShowApplyChanges;
*/
	s_rgbscale_box.generic.type				= MTYPE_PICKER;
	s_rgbscale_box.generic.textSize			= MENU_FONT_SIZE;
	s_rgbscale_box.generic.x				= x;
	s_rgbscale_box.generic.y				= y += MENU_LINE_SIZE;
	s_rgbscale_box.generic.name				= "RGB enhance factor";
	s_rgbscale_box.itemNames				= rgbscale_names;
	s_rgbscale_box.itemValues				= rgbscale_values;
	s_rgbscale_box.generic.cvar				= "r_rgbscale";
	s_rgbscale_box.generic.cvarClamp		= true;
	s_rgbscale_box.generic.cvarMin			= 1;
	s_rgbscale_box.generic.cvarMax			= 4;
	s_rgbscale_box.generic.statusbar		= "brightens textures without washing them out";

	s_trans_lighting_box.generic.type		= MTYPE_PICKER;
	s_trans_lighting_box.generic.textSize	= MENU_FONT_SIZE;
	s_trans_lighting_box.generic.x			= x;
	s_trans_lighting_box.generic.y			= y += MENU_LINE_SIZE;
	s_trans_lighting_box.generic.name		= "translucent lighting";
	s_trans_lighting_box.itemNames			= lighting_names;
	s_trans_lighting_box.generic.cvar		= "r_trans_lighting";
	s_trans_lighting_box.generic.cvarClamp	= true;
	s_trans_lighting_box.generic.cvarMin	= 0;
	s_trans_lighting_box.generic.cvarMax	= 2;
	s_trans_lighting_box.generic.statusbar	= "lighting on translucent surfaces";

	s_warp_lighting_box.generic.type		= MTYPE_PICKER;
	s_warp_lighting_box.generic.textSize	= MENU_FONT_SIZE;
	s_warp_lighting_box.generic.x			= x;
	s_warp_lighting_box.generic.y			= y += MENU_LINE_SIZE;
	s_warp_lighting_box.generic.name		= "warp surface lighting";
	s_warp_lighting_box.itemNames			= warp_lighting_names;
	s_warp_lighting_box.generic.cvar		= "r_warp_lighting";
	s_warp_lighting_box.generic.cvarClamp	= true;
	s_warp_lighting_box.generic.cvarMin		= 0;
	s_warp_lighting_box.generic.cvarMax		= 2;
	s_warp_lighting_box.generic.statusbar	= "lighting on water and other warping surfaces";

	s_lightcutoff_slider.generic.type		= MTYPE_SLIDER;
	s_lightcutoff_slider.generic.textSize	= MENU_FONT_SIZE;
	s_lightcutoff_slider.generic.x			= x;
	s_lightcutoff_slider.generic.y			= y += MENU_LINE_SIZE;
	s_lightcutoff_slider.generic.name		= "dynamic light cutoff";
	s_lightcutoff_slider.maxPos				= 8;
	s_lightcutoff_slider.baseValue			= 0.0f;
	s_lightcutoff_slider.increment			= 8.0f;
	s_lightcutoff_slider.displayAsPercent	= false;
	s_lightcutoff_slider.generic.cvar		= "r_lightcutoff";
	s_lightcutoff_slider.generic.cvarClamp	= true;
	s_lightcutoff_slider.generic.cvarMin	= 0;
	s_lightcutoff_slider.generic.cvarMax	= 64;
	s_lightcutoff_slider.generic.statusbar	= "lower = smoother blend, higher = faster";

	s_glass_envmap_box.generic.type			= MTYPE_PICKER;
	s_glass_envmap_box.generic.textSize		= MENU_FONT_SIZE;
	s_glass_envmap_box.generic.x			= x;
	s_glass_envmap_box.generic.y			= y += MENU_LINE_SIZE;
	s_glass_envmap_box.generic.name			= "glass envmaps";
	s_glass_envmap_box.itemNames			= yesno_names;
	s_glass_envmap_box.generic.cvar			= "r_glass_envmaps";
	s_glass_envmap_box.generic.statusbar	= "enable environment mapping on transparent surfaces";

	s_solidalpha_box.generic.type			= MTYPE_PICKER;
	s_solidalpha_box.generic.textSize		= MENU_FONT_SIZE;
	s_solidalpha_box.generic.x				= x;
	s_solidalpha_box.generic.y				= y += MENU_LINE_SIZE;
	s_solidalpha_box.generic.name			= "solid alphas";
	s_solidalpha_box.itemNames				= yesno_names;
	s_solidalpha_box.generic.cvar			= "r_solidalpha";
	s_solidalpha_box.generic.statusbar		= "enable solid drawing of trans33 + trans66 surfaces";

	s_texshader_warp_box.generic.type		= MTYPE_PICKER;
	s_texshader_warp_box.generic.textSize	= MENU_FONT_SIZE;
	s_texshader_warp_box.generic.x			= x;
	s_texshader_warp_box.generic.y			= y += MENU_LINE_SIZE;
	s_texshader_warp_box.generic.name		= "texture shader warp";
	s_texshader_warp_box.itemNames			= ifsupported_names;
	s_texshader_warp_box.generic.cvar		= "r_pixel_shader_warp";
	s_texshader_warp_box.generic.statusbar	= "enables hardware water warping effect";

	s_waterwave_slider.generic.type			= MTYPE_SLIDER;
	s_waterwave_slider.generic.textSize		= MENU_FONT_SIZE;
	s_waterwave_slider.generic.x			= x;
	s_waterwave_slider.generic.y			= y += MENU_LINE_SIZE;
	s_waterwave_slider.generic.name			= "water wave size";
	s_waterwave_slider.maxPos				= 24;
	s_waterwave_slider.baseValue			= 0.0f;
	s_waterwave_slider.increment			= 1.0f;
	s_waterwave_slider.displayAsPercent		= false;
	s_waterwave_slider.generic.cvar			= "r_waterwave";
	s_waterwave_slider.generic.cvarClamp	= true;
	s_waterwave_slider.generic.cvarMin		= 0;
	s_waterwave_slider.generic.cvarMax		= 24;
	s_waterwave_slider.generic.statusbar	= "size of waves on flat water surfaces";

	s_caustics_box.generic.type				= MTYPE_PICKER;
	s_caustics_box.generic.textSize			= MENU_FONT_SIZE;
	s_caustics_box.generic.x				= x;
	s_caustics_box.generic.y				= y += MENU_LINE_SIZE;
	s_caustics_box.generic.name				= "underwater caustics";
	s_caustics_box.itemNames				= caustics_names;
	s_caustics_box.generic.cvar				= "r_caustics";
	s_caustics_box.generic.cvarClamp		= true;
	s_caustics_box.generic.cvarMin			= 0;
	s_caustics_box.generic.cvarMax			= 2;
	s_caustics_box.generic.statusbar		= "caustic effect on underwater surfaces";

	s_particle_overdraw_box.generic.type		= MTYPE_PICKER;
	s_particle_overdraw_box.generic.textSize	= MENU_FONT_SIZE;
	s_particle_overdraw_box.generic.x			= x;
	s_particle_overdraw_box.generic.y			= y += MENU_LINE_SIZE;
	s_particle_overdraw_box.generic.name		= "particle overdraw";
	s_particle_overdraw_box.itemNames			= yesno_names;
	s_particle_overdraw_box.generic.cvar		= "r_particle_overdraw";
	s_particle_overdraw_box.generic.statusbar	= "redraw particles over trans surfaces";

	s_flare_occlusionquery_box.generic.type			= MTYPE_PICKER;
	s_flare_occlusionquery_box.generic.textSize		= MENU_FONT_SIZE;
	s_flare_occlusionquery_box.generic.x			= x;
	s_flare_occlusionquery_box.generic.y			= y += MENU_LINE_SIZE;
	s_flare_occlusionquery_box.generic.name			= "flare occlusion queries";
	s_flare_occlusionquery_box.itemNames			= yesno_names;
	s_flare_occlusionquery_box.generic.cvar			= "r_occlusion_test";
	s_flare_occlusionquery_box.generic.statusbar	= "use occlusion queries for flare rendering";

	s_lightbloom_box.generic.type			= MTYPE_PICKER;
	s_lightbloom_box.generic.textSize		= MENU_FONT_SIZE;
	s_lightbloom_box.generic.x				= x;
	s_lightbloom_box.generic.y				= y += MENU_LINE_SIZE;
	s_lightbloom_box.generic.name			= "light blooms";
	s_lightbloom_box.itemNames				= yesno_names;
	s_lightbloom_box.generic.cvar			= "r_bloom";
	s_lightbloom_box.generic.statusbar		= "enables blooming of bright lights";

	s_modelshading_box.generic.type			= MTYPE_PICKER;
	s_modelshading_box.generic.textSize		= MENU_FONT_SIZE;
	s_modelshading_box.generic.x			= x;
	s_modelshading_box.generic.y			= y += MENU_LINE_SIZE;
	s_modelshading_box.generic.name			= "model shading";
	s_modelshading_box.itemNames			= shading_names;
	s_modelshading_box.generic.cvar			= "r_model_shading";
	s_modelshading_box.generic.cvarClamp	= true;
	s_modelshading_box.generic.cvarMin		= 0;
	s_modelshading_box.generic.cvarMax		= 3;
	s_modelshading_box.generic.statusbar	= "level of shading to use on models";

	s_shadows_box.generic.type				= MTYPE_PICKER;
	s_shadows_box.generic.textSize			= MENU_FONT_SIZE;
	s_shadows_box.generic.x					= x;
	s_shadows_box.generic.y					= y += MENU_LINE_SIZE;
	s_shadows_box.generic.name				= "entity shadows";
	s_shadows_box.itemNames					= shadow_names;
	s_shadows_box.generic.cvar				= "r_shadows";
	s_shadows_box.generic.cvarClamp			= true;
	s_shadows_box.generic.cvarMin			= 0;
	s_shadows_box.generic.cvarMax			= 3;
	s_shadows_box.generic.statusbar			= "type of model shadows to draw";

	s_two_side_stencil_box.generic.type			= MTYPE_PICKER;
	s_two_side_stencil_box.generic.textSize		= MENU_FONT_SIZE;
	s_two_side_stencil_box.generic.x			= x;
	s_two_side_stencil_box.generic.y			= y += MENU_LINE_SIZE;
	s_two_side_stencil_box.generic.name			= "two-sided stenciling";
	s_two_side_stencil_box.itemNames			= ifsupported_names;
	s_two_side_stencil_box.generic.cvar			= "r_stencilTwoSide";
	s_two_side_stencil_box.generic.statusbar	= "use single-pass shadow stenciling";

	s_ent_shell_box.generic.type				= MTYPE_PICKER;
	s_ent_shell_box.generic.textSize			= MENU_FONT_SIZE;
	s_ent_shell_box.generic.x					= x;
	s_ent_shell_box.generic.y					= y += MENU_LINE_SIZE;
	s_ent_shell_box.generic.name				= "entity shell type";
	s_ent_shell_box.itemNames					= shell_names;
	s_ent_shell_box.generic.cvar				= "r_shelltype";
	s_ent_shell_box.generic.cvarClamp			= true;
	s_ent_shell_box.generic.cvarMin				= 0;
	s_ent_shell_box.generic.cvarMax				= 2;
	s_ent_shell_box.generic.statusbar			= "envmap effect may cause instability on ATI cards";

	s_celshading_box.generic.type				= MTYPE_PICKER;
	s_celshading_box.generic.textSize			= MENU_FONT_SIZE;
	s_celshading_box.generic.x					= x;
	s_celshading_box.generic.y					= y += MENU_LINE_SIZE;
	s_celshading_box.generic.name				= "cel shading";
	s_celshading_box.itemNames					= yesno_names;
	s_celshading_box.generic.cvar				= "r_celshading";
	s_celshading_box.generic.statusbar			= "cartoon-style rendering of models";

	s_celshading_width_slider.generic.type		= MTYPE_SLIDER;
	s_celshading_width_slider.generic.textSize	= MENU_FONT_SIZE;
	s_celshading_width_slider.generic.x			= x;
	s_celshading_width_slider.generic.y			= y += MENU_LINE_SIZE;
	s_celshading_width_slider.generic.name		= "cel shading width";
	s_celshading_width_slider.maxPos			= 11;
	s_celshading_width_slider.baseValue			= 1.0f;
	s_celshading_width_slider.increment			= 1.0f;
	s_celshading_width_slider.displayAsPercent	= false;
	s_celshading_width_slider.generic.cvar		= "r_celshading_width";
	s_celshading_width_slider.generic.cvarClamp	= true;
	s_celshading_width_slider.generic.cvarMin	= 0;
	s_celshading_width_slider.generic.cvarMax	= 10;
	s_celshading_width_slider.generic.statusbar	= "width of cel shading outlines";

	s_screenshotformat_box.generic.type			= MTYPE_PICKER;
	s_screenshotformat_box.generic.textSize		= MENU_FONT_SIZE;
	s_screenshotformat_box.generic.x			= x;
	s_screenshotformat_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_screenshotformat_box.generic.name			= "screenshot format";
	s_screenshotformat_box.itemNames			= screenshotformat_names;
	s_screenshotformat_box.itemValues			= screenshotformat_values;
	s_screenshotformat_box.generic.cvar			= "r_screenshot_format";
	s_screenshotformat_box.generic.statusbar	= "image format for screenshots";

	s_screenshotjpegquality_slider.generic.type			= MTYPE_SLIDER;
	s_screenshotjpegquality_slider.generic.textSize		= MENU_FONT_SIZE;
	s_screenshotjpegquality_slider.generic.x			= x;
	s_screenshotjpegquality_slider.generic.y			= y += MENU_LINE_SIZE;
	s_screenshotjpegquality_slider.generic.name			= "JPEG screenshot quality";
	s_screenshotjpegquality_slider.maxPos				= 10;
	s_screenshotjpegquality_slider.baseValue			= 50.0f;
	s_screenshotjpegquality_slider.increment			= 5.0f;
	s_screenshotjpegquality_slider.displayAsPercent		= false;
	s_screenshotjpegquality_slider.generic.cvar			= "r_screenshot_jpeg_quality";
	s_screenshotjpegquality_slider.generic.cvarClamp	= true;
	s_screenshotjpegquality_slider.generic.cvarMin		= 50;
	s_screenshotjpegquality_slider.generic.cvarMax		= 100;
	s_screenshotjpegquality_slider.generic.statusbar	= "quality of JPG screenshots, 50-100%";

	s_npot_mipmap_box.generic.type				= MTYPE_PICKER;
	s_npot_mipmap_box.generic.textSize			= MENU_FONT_SIZE;
	s_npot_mipmap_box.generic.x					= x;
	s_npot_mipmap_box.generic.y					= y += 2*MENU_LINE_SIZE;
	s_npot_mipmap_box.generic.name				= "non-power-of-2 mipmaps";
	s_npot_mipmap_box.itemNames					= yesno_names;
	s_npot_mipmap_box.generic.cvar				= "r_nonpoweroftwo_mipmaps";
	s_npot_mipmap_box.generic.cvarNoSave		= true;
	s_npot_mipmap_box.generic.statusbar			= "enables non-power-of-2 mipmapped textures (requires driver support)";
	s_npot_mipmap_box.generic.callback			= M_ShowApplyChanges;

	s_sgis_mipmap_box.generic.type				= MTYPE_PICKER;
	s_sgis_mipmap_box.generic.textSize			= MENU_FONT_SIZE;
	s_sgis_mipmap_box.generic.x					= x;
	s_sgis_mipmap_box.generic.y					= y += MENU_LINE_SIZE;
	s_sgis_mipmap_box.generic.name				= "SGIS mipmaps";
	s_sgis_mipmap_box.itemNames					= yesno_names;
	s_sgis_mipmap_box.generic.cvar				= "r_sgis_generatemipmap";
	s_sgis_mipmap_box.generic.cvarNoSave		= true;
	s_sgis_mipmap_box.generic.statusbar			= "enables driver-based mipmap generation";
	s_sgis_mipmap_box.generic.callback			= M_ShowApplyChanges;

	s_upscale_font_box.generic.type				= MTYPE_PICKER;
	s_upscale_font_box.generic.textSize			= MENU_FONT_SIZE;
	s_upscale_font_box.generic.x				= x;
	s_upscale_font_box.generic.y				= y += MENU_LINE_SIZE;
	s_upscale_font_box.generic.name				= "upscale old fonts";
	s_upscale_font_box.itemNames				= font_upscale_names;
	s_upscale_font_box.generic.cvar				= "r_font_upscale";
	s_upscale_font_box.generic.cvarNoSave		= true;
	s_upscale_font_box.generic.cvarClamp		= true;
	s_upscale_font_box.generic.cvarMin			= 0;
	s_upscale_font_box.generic.cvarMax			= 2;
	s_upscale_font_box.generic.statusbar		= "upscales 128x128 fonts to higher res based on screen resolution";
	s_upscale_font_box.generic.callback			= M_ShowApplyChanges;

	s_apply_action.generic.type					= MTYPE_ACTION;
	s_apply_action.generic.textSize				= MENU_FONT_SIZE;
	s_apply_action.generic.name					= "Apply Changes";
	s_apply_action.generic.x					= x;
	s_apply_action.generic.y					= y += 2*MENU_LINE_SIZE;
	s_apply_action.generic.callback				= UI_ApplyChanges_Popup;	// M_ApplyAdvVideoChanges
	s_apply_action.generic.statusbar			= "applies changes that require a video restart";
	s_apply_action.generic.isHidden				= true;

	s_back_action.generic.type					= MTYPE_ACTION;
	s_back_action.generic.textSize				= MENU_FONT_SIZE;
	s_back_action.generic.name					= "Back";
	s_back_action.generic.x						= x;
	s_back_action.generic.y						= y += 2*MENU_LINE_SIZE;
	s_back_action.generic.callback				= UI_BackMenu;

	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_video_advanced_banner);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_options_advanced_header);

	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_lightmapscale_slider);
//	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_textureintensity_slider);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_rgbscale_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_trans_lighting_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_warp_lighting_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_lightcutoff_slider);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_glass_envmap_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_solidalpha_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_texshader_warp_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_waterwave_slider);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_caustics_box);

	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_particle_overdraw_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_flare_occlusionquery_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_lightbloom_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_modelshading_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_shadows_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_two_side_stencil_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_ent_shell_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_celshading_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_celshading_width_slider);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_screenshotformat_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_screenshotjpegquality_slider);

	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_npot_mipmap_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_sgis_mipmap_box);
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_upscale_font_box);

	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_apply_action );
	UI_AddMenuItem (&s_video_advanced_menu, (void *) &s_back_action );
}


void Menu_Video_Advanced_f (void)
{
	Menu_Video_Advanced_Init ();
	UI_PushMenu (&s_video_advanced_menu);
}
