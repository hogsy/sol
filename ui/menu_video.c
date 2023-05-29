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

// menu_video.c -- the video options menu

#include "../client/client.h"
#include "ui_local.h"

//void Menu_Video_Init (void);

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

static menuFramework_s	s_video_menu;
static menuImage_s		s_video_banner;
static menuComboBox_s	s_mode_list;
static menuField_s		s_customwidth_field;
static menuField_s		s_customheight_field;
static menuComboBox_s  	s_fs_box;
static menuSlider_s		s_brightness_slider;
static menuComboBox_s	s_refresh_box;
static menuComboBox_s	s_texqual_box;
static menuComboBox_s	s_texfilter_box;
static menuComboBox_s	s_aniso_box;
//static menuPicker_s  	s_texcompress_box;
static menuPicker_s  	s_vsync_box;
static menuPicker_s  	s_adjust_fov_box;
static menuPicker_s  	s_async_box;

static menuAction_s		s_advanced_action;
static menuAction_s		s_defaults_action;
static menuAction_s		s_apply_action;
static menuAction_s		s_backmain_action;

//=======================================================================

static void M_AdvancedVideoOptionsFunc (void *s)
{
	Menu_Video_Advanced_f ();
}

static void M_ShowApplyChanges (void *unused)
{
//	s_apply_action.generic.isHidden = false;
	s_apply_action.generic.isHidden = ( !s_mode_list.generic.valueChanged &&
										!(s_customwidth_field.generic.valueChanged && !s_customwidth_field.generic.isHidden) &&
										!(s_customheight_field.generic.valueChanged && !s_customheight_field.generic.isHidden) &&
										!s_fs_box.generic.valueChanged &&
										!s_refresh_box.generic.valueChanged &&
										!s_texqual_box.generic.valueChanged );
									//	!s_texcompress_box.generic.valueChanged &&
									
}

static void M_ShowCustomFields (void)
{
	s_customwidth_field.generic.isHidden = 
	s_customheight_field.generic.isHidden = (strcmp(ui_video_modes[s_mode_list.curValue], "-1") != 0);
}

static void M_ModeListCallback (void *unused)
{
	M_ShowCustomFields ();
	M_ShowApplyChanges (unused);
}

//=======================================================================

static void M_PrepareVideoRefresh (void)
{
	// set the right mode for refresh
	Cvar_Set( "vid_ref", "gl" );
	Cvar_Set( "gl_driver", "opengl32" );

	// tell them they're modified so they refresh
	Cvar_SetModified ("vid_ref", true);
}

static void M_ResetVideoDefaults (void)
{
	Cvar_SetToDefault ("r_mode");
	Cvar_SetToDefault ("vid_fullscreen");
	Cvar_SetToDefault ("vid_gamma");
	Cvar_SetToDefault ("r_displayrefresh");
	Cvar_SetToDefault ("r_texturemode");
	Cvar_SetToDefault ("r_anisotropic");
	Cvar_SetToDefault ("r_picmip");
//	Cvar_SetToDefault ("r_ext_texture_compression");
	Cvar_SetToDefault ("r_swapinterval");
	Cvar_SetToDefault ("cl_widescreen_fov");
	Cvar_SetToDefault ("cl_async");

	// Advanced section
	Cvar_SetToDefault ("r_modulate");
//	Cvar_SetToDefault ("r_intensity");
	Cvar_SetToDefault ("r_rgbscale");
	Cvar_SetToDefault ("r_trans_lighting");
	Cvar_SetToDefault ("r_warp_lighting");
	Cvar_SetToDefault ("r_lightcutoff");
	Cvar_SetToDefault ("r_glass_envmaps");
	Cvar_SetToDefault ("r_solidalpha");
	Cvar_SetToDefault ("r_pixel_shader_warp");
	Cvar_SetToDefault ("r_waterwave");
	Cvar_SetToDefault ("r_caustics");
	Cvar_SetToDefault ("r_particle_overdraw");
	Cvar_SetToDefault ("r_bloom");
	Cvar_SetToDefault ("r_model_shading");
	Cvar_SetToDefault ("r_shadows");
	Cvar_SetToDefault ("r_stencilTwoSide");
	Cvar_SetToDefault ("r_shelltype");
	Cvar_SetToDefault ("r_celshading");
	Cvar_SetToDefault ("r_celshading_width");
	Cvar_SetToDefault ("r_screenshot_format");
	Cvar_SetToDefault ("r_screenshot_jpeg_quality");
	Cvar_SetToDefault ("r_saveshotsize");
	Cvar_SetToDefault ("r_nonpoweroftwo_mipmaps");
	Cvar_SetToDefault ("r_sgis_generatemipmap");
	Cvar_SetToDefault ("r_font_upscale");

	M_PrepareVideoRefresh ();
}

static void M_ApplyVideoChanges (void)
{
	// hide this again
	s_apply_action.generic.isHidden = true;

	// save values to cvars that don't instant-adjust
	UI_SaveMenuItemValue (&s_mode_list);
	UI_SaveMenuItemValue (&s_customwidth_field);
	UI_SaveMenuItemValue (&s_customheight_field);
	UI_SaveMenuItemValue (&s_fs_box);
	UI_SaveMenuItemValue (&s_refresh_box);
	UI_SaveMenuItemValue (&s_texqual_box);
//	UI_SaveMenuItemValue (&s_texcompress_box);

	M_PrepareVideoRefresh ();
}

//=======================================================================

void Menu_Video_Init (void)
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *fullscreen_names[] =
	{
		"windowed",
		"fullscreen",
		"borderless",
		0
	};
	static const char *fullscreen_values[] =
	{
		"0",
		"1",
		"2",
		0
	};
	static const char *refreshrate_names[] = 
	{
		"default",
		"60Hz",
		"70Hz",
		"72Hz",
		"75Hz",
		"85Hz ",
		"100Hz",
		"110Hz",
		"120Hz",
		"144Hz",
		"150Hz",
		"160Hz",
		"165Hz",
		"180Hz",
		"200Hz",
		"240Hz",
		"360Hz",
		"500Hz",
		0
	};
	static const char *refreshrate_values[] =
	{
		"0",
		"60",
		"70",
		"72",
		"75",
		"85",
		"100",
		"110",
		"120",
		"144",
		"150",
		"160",
		"165",
		"180",
		"200",
		"240",
		"360",
		"500",
		0
	};
	static const char *texfilter_names[] =
	{
		"none",
		"nearest",
		"linear",
		"bilinear",
		"trilinear",
		0
	};
	static const char *texfilter_values[] =
	{
		"GL_NEAREST",
		"GL_NEAREST_MIPMAP_NEAREST",
		"GL_LINEAR",
		"GL_LINEAR_MIPMAP_NEAREST",
		"GL_LINEAR_MIPMAP_LINEAR",
		0
	};
	static const char *lmh_names[] =
	{
		"lowest",
		"low",
		"medium",
		"high",
		"highest",
		0
	};
	static const char *lmh_values[] =
	{
		"4",
		"3",
		"2",
		"1",
		"0",
		0
	};

	int		x, y;

	// menu.x = 320, menu.y = 160
	x = SCREEN_WIDTH*0.5;
	y = SCREEN_HEIGHT*0.5 - 8*MENU_LINE_SIZE;

	s_video_menu.x						= 0;
	s_video_menu.y						= 0;
	s_video_menu.nitems					= 0;
	s_video_menu.isPopup				= false;
	s_video_menu.background				= NULL;
	s_video_menu.drawFunc				= UI_DefaultMenuDraw;
	s_video_menu.keyFunc				= UI_DefaultMenuKey;
	s_video_menu.canOpenFunc			= NULL;
	s_video_menu.defaultsFunc			= M_ResetVideoDefaults;
	s_video_menu.defaultsMessage		= "Reset all Video settings to defaults?";
	s_video_menu.applyChangesFunc		= M_ApplyVideoChanges;
	s_video_menu.applyChangesMessage[0]	= "This will restart the video system to";
	s_video_menu.applyChangesMessage[1]	= "apply settings and return to the menu.";
	s_video_menu.applyChangesMessage[2]	= "Continue?";

	s_video_banner.generic.type		= MTYPE_IMAGE;
	s_video_banner.generic.x		= 0;
	s_video_banner.generic.y		= 9*MENU_LINE_SIZE;
	s_video_banner.width			= 275;
	s_video_banner.height			= 32;
	s_video_banner.imageName		= "/pics/m_banner_video.pcx";
	s_video_banner.alpha			= 255;
	s_video_banner.border			= 0;
	s_video_banner.hCentered		= true;
	s_video_banner.vCentered		= false;
	s_video_banner.useAspectRatio	= false;
	s_video_banner.generic.isHidden	= false;

	s_mode_list.generic.type		= MTYPE_COMBOBOX;
	s_mode_list.generic.name		= "video mode";
	s_mode_list.generic.header		= "mode        aspect";
	s_mode_list.generic.x			= x;
	s_mode_list.generic.y			= y;
	s_mode_list.itemNames			= ui_resolution_names;
	s_mode_list.itemValues			= ui_video_modes;
	s_mode_list.items_y				= 12;
	s_mode_list.itemWidth			= 20;
	s_mode_list.itemSpacing			= 1;
	s_mode_list.itemTextSize		= 8;
	s_mode_list.border				= 1;
	s_mode_list.borderColor[0]		= 60;
	s_mode_list.borderColor[1]		= 60;
	s_mode_list.borderColor[2]		= 60;
	s_mode_list.borderColor[3]		= 255;
	s_mode_list.backColor[0]		= 0;
	s_mode_list.backColor[1]		= 0;
	s_mode_list.backColor[2]		= 0;
	s_mode_list.backColor[3]		= 192;
	s_mode_list.generic.cvar		= "r_mode";
	s_mode_list.generic.cvarNoSave	= true;
	s_mode_list.generic.cvarClamp	= false;
	s_mode_list.generic.statusbar	= "changes screen resolution";
	s_mode_list.generic.callback	= M_ModeListCallback;

	s_customwidth_field.generic.type			= MTYPE_FIELD;
	s_customwidth_field.generic.textSize		= MENU_FONT_SIZE;
	s_customwidth_field.generic.header			= "custom width";
	s_customwidth_field.generic.flags			= QMF_NUMBERSONLY;
	s_customwidth_field.generic.callback		= 0;
	s_customwidth_field.generic.x				= x + -14*MENU_FONT_SIZE;
	s_customwidth_field.generic.y				= y + 3.5*MENU_LINE_SIZE;	// was 3*MENU_LINE_SIZE
	s_customwidth_field.generic.statusbar		= "";
	s_customwidth_field.generic.cvar			= "r_customwidth";
	s_customwidth_field.generic.cvarClamp		= true;
	s_customwidth_field.generic.cvarMin			= 640;
	s_customwidth_field.generic.cvarMax			= 99999;
	s_customwidth_field.generic.cvarNoSave		= true;
	s_customwidth_field.generic.callback		= M_ShowApplyChanges;
	s_customwidth_field.length					= 5;
	s_customwidth_field.visible_length			= 6;

	s_customheight_field.generic.type			= MTYPE_FIELD;
	s_customheight_field.generic.textSize		= MENU_FONT_SIZE;
	s_customheight_field.generic.header			= "custom height";
	s_customheight_field.generic.flags			= QMF_NUMBERSONLY;
	s_customheight_field.generic.callback		= 0;
	s_customheight_field.generic.x				= x + 2*MENU_FONT_SIZE;
	s_customheight_field.generic.y				= y + 3.5*MENU_LINE_SIZE;	// was 3*MENU_LINE_SIZE
	s_customheight_field.generic.statusbar		= "";
	s_customheight_field.generic.cvar			= "r_customheight";
	s_customheight_field.generic.cvarClamp		= true;
	s_customheight_field.generic.cvarMin		= 480;
	s_customheight_field.generic.cvarMax		= 99999;
	s_customheight_field.generic.cvarNoSave		= true;
	s_customheight_field.generic.callback		= M_ShowApplyChanges;
	s_customheight_field.length					= 5;
	s_customheight_field.visible_length			= 6;

	s_fs_box.generic.type			= MTYPE_COMBOBOX;
	s_fs_box.generic.x				= x;
	s_fs_box.generic.y				= y += 5.5*MENU_LINE_SIZE;
	s_fs_box.generic.name			= "display type";
	s_fs_box.itemNames				= fullscreen_names;
	s_fs_box.itemValues				= fullscreen_values;
	s_fs_box.items_y				= 3;
	s_fs_box.itemWidth				= 11;
	s_fs_box.itemSpacing			= 1;
	s_fs_box.itemTextSize			= 8;
	s_fs_box.border					= 1;
	s_fs_box.borderColor[0]			= 60;
	s_fs_box.borderColor[1]			= 60;
	s_fs_box.borderColor[2]			= 60;
	s_fs_box.borderColor[3]			= 255;
	s_fs_box.backColor[0]			= 0;
	s_fs_box.backColor[1]			= 0;
	s_fs_box.backColor[2]			= 0;
	s_fs_box.backColor[3]			= 192;
	s_fs_box.generic.cvar			= "vid_fullscreen";
	s_fs_box.generic.cvarNoSave		= true;
	s_fs_box.generic.cvarClamp		= true;
	s_fs_box.generic.cvarMin		= 0;
	s_fs_box.generic.cvarMax		= 2;
	s_fs_box.generic.statusbar		= "changes between fullscreen, borderless window, and windowed display";
	s_fs_box.generic.callback		= M_ShowApplyChanges;

	s_brightness_slider.generic.type		= MTYPE_SLIDER;
	s_brightness_slider.generic.textSize	= MENU_FONT_SIZE;
	s_brightness_slider.generic.x			= x;
	s_brightness_slider.generic.y			= y += 2*MENU_LINE_SIZE;	// was MENU_LINE_SIZE
	s_brightness_slider.generic.name		= "brightness";
	s_brightness_slider.maxPos				= 20;
	s_brightness_slider.baseValue			= 1.3f;
	s_brightness_slider.increment			= -0.05f;
	s_brightness_slider.displayAsPercent	= false;
	s_brightness_slider.generic.cvar		= "vid_gamma";
	s_brightness_slider.generic.cvarClamp	= false;
	s_brightness_slider.generic.statusbar	= "changes display brightness";

	s_refresh_box.generic.type			= MTYPE_COMBOBOX;
	s_refresh_box.generic.x				= x;
	s_refresh_box.generic.y				= y += 1.5*MENU_LINE_SIZE;
	s_refresh_box.generic.name			= "refresh rate";
	s_refresh_box.itemNames				= refreshrate_names;
	s_refresh_box.itemValues			= refreshrate_values;
	s_refresh_box.items_y				= 10;
	s_refresh_box.itemWidth				= 8;
	s_refresh_box.itemSpacing			= 1;
	s_refresh_box.itemTextSize			= 8;
	s_refresh_box.border				= 1;
	s_refresh_box.borderColor[0]		= 60;
	s_refresh_box.borderColor[1]		= 60;
	s_refresh_box.borderColor[2]		= 60;
	s_refresh_box.borderColor[3]		= 255;
	s_refresh_box.backColor[0]			= 0;
	s_refresh_box.backColor[1]			= 0;
	s_refresh_box.backColor[2]			= 0;
	s_refresh_box.backColor[3]			= 192;
	s_refresh_box.generic.cvar			= "r_displayrefresh";
	s_refresh_box.generic.cvarNoSave	= true;
	s_refresh_box.generic.cvarClamp		= false;
	s_refresh_box.generic.statusbar		= "sets refresh rate for fullscreen modes";
	s_refresh_box.generic.callback		= M_ShowApplyChanges;

	s_texfilter_box.generic.type		= MTYPE_COMBOBOX;
	s_texfilter_box.generic.x			= x;
	s_texfilter_box.generic.y			= y += 2.5*MENU_LINE_SIZE;
	s_texfilter_box.generic.name		= "texture filter";
	s_texfilter_box.itemNames			= texfilter_names;
	s_texfilter_box.itemValues			= texfilter_values;
	s_texfilter_box.items_y				= 5;
	s_texfilter_box.itemWidth			= 10;
	s_texfilter_box.itemSpacing			= 1;
	s_texfilter_box.itemTextSize		= 8;
	s_texfilter_box.border				= 1;
	s_texfilter_box.borderColor[0]		= 60;
	s_texfilter_box.borderColor[1]		= 60;
	s_texfilter_box.borderColor[2]		= 60;
	s_texfilter_box.borderColor[3]		= 255;
	s_texfilter_box.backColor[0]		= 0;
	s_texfilter_box.backColor[1]		= 0;
	s_texfilter_box.backColor[2]		= 0;
	s_texfilter_box.backColor[3]		= 192;
	s_texfilter_box.generic.cvar		= "r_texturemode";
	s_texfilter_box.generic.cvarClamp	= false;
	s_texfilter_box.generic.statusbar	= "changes texture filtering mode";

	s_aniso_box.generic.type		= MTYPE_COMBOBOX;
	s_aniso_box.generic.x			= x;
	s_aniso_box.generic.y			= y += 1.5*MENU_LINE_SIZE;
	s_aniso_box.generic.name		= "anisotropic filter";
	s_aniso_box.itemNames			= ui_aniso_names;
	s_aniso_box.itemValues			= ui_aniso_values;
	s_aniso_box.items_y				= 5;
	s_aniso_box.itemWidth			= 4;
	s_aniso_box.itemSpacing			= 1;
	s_aniso_box.itemTextSize		= 8;
	s_aniso_box.border				= 1;
	s_aniso_box.borderColor[0]		= 60;
	s_aniso_box.borderColor[1]		= 60;
	s_aniso_box.borderColor[2]		= 60;
	s_aniso_box.borderColor[3]		= 255;
	s_aniso_box.backColor[0]		= 0;
	s_aniso_box.backColor[1]		= 0;
	s_aniso_box.backColor[2]		= 0;
	s_aniso_box.backColor[3]		= 192;
	s_aniso_box.generic.cvar		= "r_anisotropic";
	s_aniso_box.generic.cvarClamp	= false;
	s_aniso_box.generic.statusbar	= "changes level of anisotropic mipmap filtering";

	s_texqual_box.generic.type			= MTYPE_COMBOBOX;
	s_texqual_box.generic.x				= x;
	s_texqual_box.generic.y				= y += 1.5*MENU_LINE_SIZE;
	s_texqual_box.generic.name			= "texture quality";
	s_texqual_box.itemNames				= lmh_names;
	s_texqual_box.itemValues			= lmh_values;
	s_texqual_box.items_y				= 5;
	s_texqual_box.itemWidth				= 8;
	s_texqual_box.itemSpacing			= 1;
	s_texqual_box.itemTextSize			= 8;
	s_texqual_box.border				= 1;
	s_texqual_box.borderColor[0]		= 60;
	s_texqual_box.borderColor[1]		= 60;
	s_texqual_box.borderColor[2]		= 60;
	s_texqual_box.borderColor[3]		= 255;
	s_texqual_box.backColor[0]			= 0;
	s_texqual_box.backColor[1]			= 0;
	s_texqual_box.backColor[2]			= 0;
	s_texqual_box.backColor[3]			= 192;
	s_texqual_box.generic.cvar			= "r_picmip";
	s_texqual_box.generic.cvarNoSave	= true;
	s_texqual_box.generic.cvarClamp		= false;
	s_texqual_box.generic.statusbar		= "changes maximum texture size (highest = no limit)";
	s_texqual_box.generic.callback		= M_ShowApplyChanges;
/*
	s_texcompress_box.generic.type			= MTYPE_PICKER;
	s_texcompress_box.generic.textSize		= MENU_FONT_SIZE;
	s_texcompress_box.generic.x				= x;
	s_texcompress_box.generic.y				= y += 1.5*MENU_LINE_SIZE;
	s_texcompress_box.generic.name			= "texture compression";
	s_texcompress_box.itemNames				= yesno_names;
	s_texcompress_box.generic.cvar			= "r_ext_texture_compression";
	s_texcompress_box.generic.cvarNoSave	= true;
	s_texcompress_box.generic.statusbar		= "reduces quality, increases performance (leave off unless needed)";
	s_texcompress_box.generic.callback		= M_ShowApplyChanges;
*/
	s_vsync_box.generic.type			= MTYPE_PICKER;
	s_vsync_box.generic.textSize		= MENU_FONT_SIZE;
	s_vsync_box.generic.x				= x;
	s_vsync_box.generic.y				= y += 2*MENU_LINE_SIZE;
	s_vsync_box.generic.name			= "video sync";
	s_vsync_box.itemNames				= yesno_names;
	s_vsync_box.generic.cvar			= "r_swapinterval";
	s_vsync_box.generic.statusbar		= "sync framerate with monitor refresh";

	s_adjust_fov_box.generic.type		= MTYPE_PICKER;
	s_adjust_fov_box.generic.textSize	= MENU_FONT_SIZE;
	s_adjust_fov_box.generic.x			= x;
	s_adjust_fov_box.generic.y			= y += MENU_LINE_SIZE;
	s_adjust_fov_box.generic.name		= "fov autoscaling";
	s_adjust_fov_box.itemNames			= yesno_names;
	s_adjust_fov_box.generic.cvar		= "cl_widescreen_fov";
	s_adjust_fov_box.generic.statusbar	= "automatic scaling of fov for widescreen modes";

	s_async_box.generic.type			= MTYPE_PICKER;
	s_async_box.generic.textSize		= MENU_FONT_SIZE;
	s_async_box.generic.x				= x;
	s_async_box.generic.y				= y += MENU_LINE_SIZE;
	s_async_box.generic.name			= "async refresh";
	s_async_box.itemNames				= yesno_names;
	s_async_box.generic.cvar			= "cl_async";
	s_async_box.generic.statusbar		= "decouples network framerate from render framerate";

	s_advanced_action.generic.type		= MTYPE_ACTION;
	s_advanced_action.generic.textSize	= MENU_FONT_SIZE;
	s_advanced_action.generic.name		= "Advanced Options";
	s_advanced_action.generic.x			= x;
	s_advanced_action.generic.y			= y += 3*MENU_LINE_SIZE;
	s_advanced_action.generic.callback	= M_AdvancedVideoOptionsFunc;

	s_defaults_action.generic.type		= MTYPE_ACTION;
	s_defaults_action.generic.textSize	= MENU_FONT_SIZE;
	s_defaults_action.generic.name		= "Reset to Defaults";
	s_defaults_action.generic.x			= x;
	s_defaults_action.generic.y			= y += 3*MENU_LINE_SIZE;
	s_defaults_action.generic.callback	= UI_Defaults_Popup;	// M_ResetVideoDefaults
	s_defaults_action.generic.statusbar	= "resets all video settings to internal defaults";

	// changed cancel to apply changes, thanx to MrG
	s_apply_action.generic.type			= MTYPE_ACTION;
	s_apply_action.generic.textSize		= MENU_FONT_SIZE;
	s_apply_action.generic.name			= "Apply Changes";
	s_apply_action.generic.x			= x;
	s_apply_action.generic.y			= y += 2*MENU_LINE_SIZE;
	s_apply_action.generic.callback		= UI_ApplyChanges_Popup;	// M_ApplyVideoChanges;
	s_apply_action.generic.statusbar	= "applies changes that require a video restart";
	s_apply_action.generic.isHidden		= true;

	s_backmain_action.generic.type		= MTYPE_ACTION;
	s_backmain_action.generic.textSize	= MENU_FONT_SIZE;
	s_backmain_action.generic.name		= "Back to Main";
	s_backmain_action.generic.x			= x;
	s_backmain_action.generic.y			= y += 2*MENU_LINE_SIZE;
	s_backmain_action.generic.callback	= UI_BackMenu;

	UI_AddMenuItem (&s_video_menu, (void *) &s_video_banner);
	UI_AddMenuItem (&s_video_menu, (void *) &s_mode_list);

	UI_AddMenuItem (&s_video_menu, (void *) &s_customwidth_field);
	UI_AddMenuItem (&s_video_menu, (void *) &s_customheight_field);

	UI_AddMenuItem (&s_video_menu, (void *) &s_fs_box);
	UI_AddMenuItem (&s_video_menu, (void *) &s_brightness_slider);
	UI_AddMenuItem (&s_video_menu, (void *) &s_refresh_box);
	UI_AddMenuItem (&s_video_menu, (void *) &s_texfilter_box);
	UI_AddMenuItem (&s_video_menu, (void *) &s_aniso_box);
	UI_AddMenuItem (&s_video_menu, (void *) &s_texqual_box);
//	UI_AddMenuItem (&s_video_menu, (void *) &s_texcompress_box);
	UI_AddMenuItem (&s_video_menu, (void *) &s_vsync_box);
	UI_AddMenuItem (&s_video_menu, (void *) &s_adjust_fov_box);
	UI_AddMenuItem (&s_video_menu, (void *) &s_async_box);

	UI_AddMenuItem (&s_video_menu, (void *) &s_advanced_action);
	UI_AddMenuItem (&s_video_menu, (void *) &s_defaults_action);
	UI_AddMenuItem (&s_video_menu, (void *) &s_apply_action);
	UI_AddMenuItem (&s_video_menu, (void *) &s_backmain_action);
}


void Menu_Video_f (void)
{
	Menu_Video_Init ();
	UI_PushMenu (&s_video_menu);
	M_ShowCustomFields ();
}
