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

// cl_screen.c -- master for refresh, status bar, console, chat, notify, etc

/*
  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  cinematics
  full screen image for quit and victory

  end of unit intermissions
  */

#include "client.h"
#include "../ui/ui_local.h"

float		scr_con_current;	// aproaches scr_conlines at scr_conspeed
float		scr_conlines;		// 0.0 to 1.0 lines of console to display

float		scr_letterbox_current;	// aproaches scr_lboxlines at scr_conspeed
float		scr_letterbox_lines;		// 0.0 to 1.0 lines of letterbox to display
qboolean	scr_letterbox_active;
qboolean	scr_hidehud;

qboolean	scr_initialized = false;	// ready to draw

int			scr_draw_loading;
char		*scr_load_saveshot;

int			scr_fps = 0;	// global FPS counter variable, used by SCR_ShowFPS() and SCR_DrawDebugGraph()

vrect_t		scr_vrect;		// position of render window on screen

cvar_t		*scr_viewsize;
cvar_t		*scr_conspeed;
cvar_t		*scr_conalpha;		// Psychospaz's transparent console
cvar_t		*scr_newconback;	// whether to use new console background
cvar_t		*scr_oldconbar;		// whether to draw bottom bar on old console
//cvar_t		*scr_conheight;	// how far the console drops down
cvar_t		*scr_letterbox;
cvar_t		*scr_centertime;
//cvar_t		*scr_showturtle;	// unused
cvar_t		*scr_showpause;
//cvar_t		*scr_printspeed;	// unused

cvar_t		*scr_netgraph;
cvar_t		*scr_netgraph_pos;
cvar_t		*scr_timegraph;
cvar_t		*scr_debuggraph;
//cvar_t		*scr_graphheight;	// unused
cvar_t		*scr_graphscale;
cvar_t		*scr_graphshift;
//cvar_t		*scr_drawall;		// unused

cvar_t		*scr_simple_loadscreen;	// whether to use reduced load screen

cvar_t		*scr_surroundlayout;	// whether to keep HUD/menu elements on center screen in triple-wide video modes
cvar_t		*scr_surroundleft;		// left placement of HUD/menu elements on center screen in triple-wide video modes
cvar_t		*scr_surroundright;		// right placement of HUD/menu elements on center screen in triple-wide video modes
cvar_t		*scr_surroundthreshold;	// minimum aspect ratio to trigger surround layout

cvar_t		*scr_hudsize;
cvar_t		*scr_hudalpha;
cvar_t		*scr_hudsqueezedigits;

cvar_t		*crosshair;
cvar_t		*crosshair_scale; // Psychospaz's scalable corsshair
cvar_t		*crosshair_red;
cvar_t		*crosshair_green;
cvar_t		*crosshair_blue;
cvar_t		*crosshair_alpha;
cvar_t		*crosshair_pulse;

// Knightmare 12/28/2001- BramBo's FPS counter
cvar_t		*cl_drawfps;
cvar_t		*cl_drawhud;			// HUD disable cvar
cvar_t		*cl_demomessage;
//cvar_t		*cl_loadpercent;	// unused
cvar_t		*cl_hud;
cvar_t		*cl_hud_variant;

color_t		color_identity = {255, 255, 255, 255};
vec4_t		vec4_identity = {1.0f, 1.0f, 1.0f, 1.0f};
vec4_t		stCoord_identity = {1.0f, 1.0f, 0.0f, 0.0f};
vec4_t		stCoord_default = {-1.0f, -1.0f, -1.0f, -1.0f};
vec4_t		stCoord_tile = {-2.0f, -2.0f, -2.0f, -2.0f};

float		scr_screenScale;
float		scr_hudScale;
float		scr_screenAspect;

char		crosshair_pic[MAX_QPATH];
int			crosshair_width, crosshair_height;

void SCR_TimeRefresh_f (void);
void SCR_Loading_f (void);

/*
===============================================================

HUD SCALING

===============================================================
*/

viddef_t hudScale_list[] =
{
	{ -1, -1 },
	{ 1024, 768 },
	{ 960, 720 },
	{ 800, 600 },
	{ 720, 540 },
	{ 640, 480 },
	{ 512, 384 },
	{ 400, 300 },
	{ 320, 240 }
};

#define HUDSCALE_NUM_SIZES ( sizeof(hudScale_list) / sizeof(hudScale_list[0]) )

/*
=================
SCR_InitHudScale
=================
*/
void SCR_InitHudScale (void)
{
	float	refWidth, refHeight;
	int		sizeIndex;

	sizeIndex = min(max(scr_hudsize->integer, 0), HUDSCALE_NUM_SIZES-1);

	if (sizeIndex == 0) {
		refWidth = viddef.width;
		refHeight = viddef.height;
	}
	else {
		refWidth = hudScale_list[sizeIndex].width;
		refHeight = hudScale_list[sizeIndex].height;
	}

	// don't scale if < refWidth, then it would be smaller
	if ( viddef.width > refWidth && viddef.height > refHeight ) {
		scr_hudScale = min( ((float)viddef.width / refWidth), ((float)viddef.height / refHeight) );
	}
	else {
		scr_hudScale = 1.0f;
	}
}

float SCR_ScaledHud (float param)
{
	return param * scr_hudScale;
}

float SCR_GetHudScale (void)
{
	return scr_hudScale;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{	
//	Cvar_SetValue ("viewsize", scr_viewsize->value+10);

	// now handles HUD scale
	int hudscale = Cvar_VariableValue("scr_hudsize")+1;
	if (hudscale > HUDSCALE_NUM_SIZES-1) hudscale = HUDSCALE_NUM_SIZES-1;
	Cvar_SetValue ("scr_hudsize", hudscale);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
//	Cvar_SetValue ("viewsize", scr_viewsize->value-10);

	// now handles HUD scale
	int hudscale = Cvar_VariableValue("scr_hudsize")-1;
	if (hudscale < 0) hudscale = 0;
	Cvar_SetValue ("scr_hudsize", hudscale);
}

/*
===============================================================================

SCREEN SCALING

===============================================================================
*/

/*
================
SCR_InitScreenScale
================
*/
void SCR_InitScreenScale (void)
{
	scr_screenScale = min( ((float)viddef.width / SCREEN_WIDTH), ((float)viddef.height / SCREEN_HEIGHT) );
	scr_screenAspect = (float)viddef.width / (float)viddef.height;
}


float SCR_ScaledScreen (float param)
{
	return param * scr_screenScale;
}


float SCR_GetScreenScale (void)
{
	return scr_screenScale;
}


/*
================
SCR_ScaleCoords

Scales virtual 640x480 coords to screen resolution
================
*/
void SCR_ScaleCoords (float *x, float *y, float *w, float *h, scralign_t align)
{
	float	xscale, lb_xscale, yscale, minscale, vertscale;
	float	tmp_x, tmp_y, tmp_w, tmp_h;
	float	xleft, xright;


	// for eyefinity/surround setups, keep everything on the center monitor
	if ( scr_surroundlayout && scr_surroundlayout->integer && (scr_screenAspect >= scr_surroundthreshold->value) ) // 3.6f
	{
		if (scr_surroundleft && scr_surroundleft->value > 0.0f && scr_surroundleft->value < 1.0f)
			xleft = (float)viddef.width * scr_surroundleft->value;
		else
			xleft = (float)viddef.width / 3.0f;
		if (scr_surroundright && scr_surroundright->value > 0.0f && scr_surroundright->value < 1.0f)
			xright = (float)viddef.width * scr_surroundright->value;
		else
			xright = (float)viddef.width * (2.0f / 3.0f);
		xscale = (xright - xleft) / SCREEN_WIDTH;
	}
	else {
		xleft = 0.0f;
		xright = (float)viddef.width;
		xscale = (float)viddef.width / SCREEN_WIDTH;
	}

	lb_xscale = (float)viddef.width / SCREEN_WIDTH;
	yscale = (float)viddef.height / SCREEN_HEIGHT;
	minscale = min(xscale, yscale);

	// aspect-ratio independent scaling
	switch (align)
	{
	case ALIGN_CENTER:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = (tmp_x - (0.5 * SCREEN_WIDTH)) * minscale + (0.5 * viddef.width);
		}
		if (y) {
			tmp_y = *y;
			*y = (tmp_y - (0.5 * SCREEN_HEIGHT)) * minscale + (0.5 * viddef.height);
		}
		break;
	case ALIGN_LETTERBOX:
		// special case: video mode (eyefinity?) is wider than object
		if ( w != NULL && h != NULL && ((float)viddef.width / (float)viddef.height > *w / *h) )
		{
			tmp_h = *h;
			vertscale = viddef.height / tmp_h;
			if (x != NULL && w != NULL) {
				tmp_x = *x;
				tmp_w = *w;
				*x = tmp_x * lb_xscale - (0.5 * (tmp_w * vertscale - tmp_w * lb_xscale));
			}
			if (y)
				*y = 0;
			if (w) 
				*w *= vertscale;
			if (h)
				*h *= vertscale;
		}
		else
		{
			if (x)
				*x *= xscale;
			if (y != NULL && h != NULL)  {
				tmp_y = *y;
				tmp_h = *h;
				*y = tmp_y * yscale - (0.5 * (tmp_h * xscale - tmp_h * yscale));
			}
			if (w) 
				*w *= xscale;
			if (h)
				*h *= xscale;
		}
		break;
	case ALIGN_TOP:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = (tmp_x - (0.5 * SCREEN_WIDTH)) * minscale + (0.5 * viddef.width);
		}
		if (y)
			*y *= minscale;
		break;
	case ALIGN_BOTTOM:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = (tmp_x - (0.5 * SCREEN_WIDTH)) * minscale + (0.5 * viddef.width);
		}
		if (y) {
			tmp_y = *y;
			*y = (tmp_y - SCREEN_HEIGHT) * minscale + viddef.height;
		}
		break;
	case ALIGN_RIGHT:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = (tmp_x - SCREEN_WIDTH) * minscale + xright;
		}
		if (y) {
			tmp_y = *y;
			*y = (tmp_y - (0.5 * SCREEN_HEIGHT)) * minscale + (0.5 * viddef.height);
		}
		break;
	case ALIGN_LEFT:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = tmp_x * minscale + xleft;
		}
		if (y) {
			tmp_y = *y;
			*y = (tmp_y - (0.5 * SCREEN_HEIGHT)) * minscale + (0.5 * viddef.height);
		}
		break;
	case ALIGN_TOPRIGHT:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = (tmp_x - SCREEN_WIDTH) * minscale + xright;
		}
		if (y)
			*y *= minscale;
		break;
	case ALIGN_TOPLEFT:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = tmp_x * minscale + xleft;
		}
		if (y)
			*y *= minscale;
		break;
	case ALIGN_BOTTOMRIGHT:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = (tmp_x - SCREEN_WIDTH) * minscale + xright;
		}
		if (y) {
			tmp_y = *y;
			*y = (tmp_y - SCREEN_HEIGHT) * minscale + viddef.height;
		}
		break;
	case ALIGN_BOTTOMLEFT:
		if (w) 
			*w *= minscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = tmp_x * minscale + xleft;
		}
		if (y) {
			tmp_y = *y;
			*y = (tmp_y - SCREEN_HEIGHT) * minscale + viddef.height;
		}
		break;
	case ALIGN_TOP_STRETCH:
		if (w) 
			*w *= xscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = tmp_x * xscale + xleft;
		}
		if (y)
			*y *= minscale;
		break;
	case ALIGN_BOTTOM_STRETCH:
		if (w) 
			*w *= xscale;
		if (h)
			*h *= minscale;
		if (x) {
			tmp_x = *x;
			*x = tmp_x * xscale + xleft;
		}
		if (y) {
			tmp_y = *y;
			*y = (tmp_y - SCREEN_HEIGHT) * minscale + viddef.height;
		}
		break;
	case ALIGN_STRETCH_ALL:
		if (x)
			*x *= lb_xscale;
		if (y) 
			*y *= yscale;
		if (w) 
			*w *= lb_xscale;
		if (h)
			*h *= yscale;
		break;
	case ALIGN_STRETCH:
	default:
		if (x) {
			tmp_x = *x;
			*x = tmp_x * xscale + xleft;
		}
		if (y) 
			*y *= yscale;
		if (w) 
			*w *= xscale;
		if (h)
			*h *= yscale;
		break;
	}
}


/*
================
SCR_DrawFill
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawFill (float x, float y, float width, float height, scralign_t align, qboolean roundOut, int red, int green, int blue, int alpha)
{
	SCR_ScaleCoords (&x, &y, &width, &height, align);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}
	R_DrawFill (x, y, width, height, red, green, blue, alpha);
}

/*
================
SCR_DrawBorder
Draws a solid color border
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawBorder (float x, float y, float width, float height, float borderSize, scralign_t align, qboolean roundOut, int red, int green, int blue, int alpha)
{
	float	bSize = floor(SCR_ScaledScreen(borderSize));
	SCR_ScaleCoords (&x, &y, &width, &height, align);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	// top
	R_DrawFill (x-bSize, y-bSize, width+bSize*2, bSize, red, green, blue, alpha);
	// bottom
	R_DrawFill (x-bSize, y+height, width+bSize*2, bSize, red, green, blue, alpha);
	// left
	R_DrawFill (x-bSize, y, bSize, height, red, green, blue, alpha);
	// right
	R_DrawFill (x+width, y, bSize, height, red, green, blue, alpha);
}


/*
================
SCR_DrawPic
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic (float x, float y, float width, float height, scralign_t align, qboolean roundOut, char *pic, float alpha)
{
	vec4_t			outColor;
	drawStruct_t	ds = { 0 };

	SCR_ScaleCoords (&x, &y, &width, &height, align);
	Vector4Set (outColor, 1.0f, 1.0f, 1.0f, alpha);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	ds.pic = pic;
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	Vector2Copy (vec2_origin, ds.offset);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_ScaledPic
Coordinates are actual values
=================
*/
void SCR_DrawScaledPic (float x, float y, float scale, qboolean centerCoords, qboolean roundOut, char *pic, float alpha)
{
	int				w, h;
	float			width, height;
	vec4_t			outColor;
	drawStruct_t	ds = { 0 };

	Vector4Set (outColor, 1.0f, 1.0f, 1.0f, alpha);
	R_DrawGetPicSize (&w, &h, pic);
	width = (float)w * scale;
	height = (float)h * scale;
	if (centerCoords) {
		x -= (width * 0.5f);
		y -= (height * 0.5f);
	}
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	ds.pic = pic;
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	Vector2Copy (vec2_origin, ds.offset);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawLegacyPic
=================
*/
void SCR_DrawLegacyPic (float x, float y, float scale, char *pic, float alpha)
{
	vec4_t			outColor;
	vec4_t			stCoords;
	drawStruct_t	ds = { 0 };

	Vector4Set (outColor, 1.0f, 1.0f, 1.0f, alpha);
	Vector4Set (stCoords, -3.0f, scale, 0, 0);

	ds.pic = pic;
	ds.x = x;	ds.y = y;	ds.w = 24;	ds.h = 24;
	ds.scale[0] = ds.scale[1] = scale;
	ds.flags |= DSFLAG_SCALED;
	Vector2Copy (vec2_origin, ds.offset);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawColoredPic
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawColoredPic (float x, float y, float width, float height, scralign_t align, qboolean roundOut, color_t color, char *pic)
{
	vec4_t			outColor;
	drawStruct_t	ds = { 0 };

	SCR_ScaleCoords (&x, &y, &width, &height, align);
	Vector4Set (outColor, (float)color[0]*DIV255, (float)color[1]*DIV255, (float)color[2]*DIV255, (float)color[3]*DIV255);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	ds.pic = pic;
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	Vector2Copy (vec2_origin, ds.offset);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawOffsetPic
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawOffsetPic (float x, float y, float width, float height, vec2_t offset, scralign_t align, qboolean roundOut, color_t color, char *pic)
{
	vec4_t			outColor;
	vec2_t			scaledOffset;
	drawStruct_t	ds = { 0 };

	Vector2Copy (offset, scaledOffset);
	SCR_ScaleCoords (&x, &y, &width, &height, align);
	SCR_ScaleCoords (NULL, NULL, &scaledOffset[0], &scaledOffset[1], align);
	Vector4Set (outColor, (float)color[0]*DIV255, (float)color[1]*DIV255, (float)color[2]*DIV255, (float)color[3]*DIV255);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	ds.pic = pic;
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	Vector2Copy (scaledOffset, ds.offset);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawOffsetPicST
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawOffsetPicST (float x, float y, float width, float height, vec2_t offset, vec4_t texCorners, scralign_t align, qboolean roundOut, color_t color, char *pic)
{
	vec4_t			outColor;
	vec2_t			scaledOffset;
	drawStruct_t	ds = { 0 };

	Vector2Copy (offset, scaledOffset);
	SCR_ScaleCoords (&x, &y, &width, &height, align);
	SCR_ScaleCoords (NULL, NULL, &scaledOffset[0], &scaledOffset[1], align);
	Vector4Set (outColor, (float)color[0]*DIV255, (float)color[1]*DIV255, (float)color[2]*DIV255, (float)color[3]*DIV255);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	ds.pic = pic;
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	ds.flags |= DSFLAG_USESTCOORDS;
	Vector2Copy (scaledOffset, ds.offset);
	Vector4Copy (texCorners, ds.stCoords);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawScrollPic
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawScrollPic (float x, float y, float width, float height, vec2_t offset, vec4_t texCorners, vec2_t scroll, scralign_t align, qboolean roundOut, color_t color, char *pic)
{
	vec4_t			outColor;
	vec2_t			scaledOffset;
	drawStruct_t	ds = { 0 };

	Vector2Copy (offset, scaledOffset);
	SCR_ScaleCoords (&x, &y, &width, &height, align);
	SCR_ScaleCoords (NULL, NULL, &scaledOffset[0], &scaledOffset[1], align);
	Vector4Set (outColor, (float)color[0]*DIV255, (float)color[1]*DIV255, (float)color[2]*DIV255, (float)color[3]*DIV255);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	ds.pic = pic;
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	ds.flags |= DSFLAG_USESTCOORDS;
	Vector2Copy (scroll, ds.scroll);
	Vector2Copy (scaledOffset, ds.offset);
	Vector4Copy (texCorners, ds.stCoords);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawMaskedPic
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawMaskedPic (float x, float y, float width, float height, vec2_t offset, vec4_t texCorners, vec2_t scroll, scralign_t align, qboolean roundOut, color_t color, char *pic, char *maskPic)
{
	vec4_t			outColor;
	vec2_t			scaledOffset;
	drawStruct_t	ds = { 0 };

	Vector2Copy (offset, scaledOffset);
	SCR_ScaleCoords (&x, &y, &width, &height, align);
	SCR_ScaleCoords (NULL, NULL, &scaledOffset[0], &scaledOffset[1], align);
	Vector4Set (outColor, (float)color[0]*DIV255, (float)color[1]*DIV255, (float)color[2]*DIV255, (float)color[3]*DIV255);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	ds.pic = pic;
	ds.maskPic = maskPic;
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	ds.flags |= DSFLAG_USESTCOORDS|DSFLAG_MASKED;
	Vector2Copy (scroll, ds.scroll);
	Vector2Copy (scaledOffset, ds.offset);
	Vector4Copy (texCorners, ds.stCoords);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawPicFull
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPicFull (float x, float y, float width, float height, vec2_t offset, vec4_t texCorners, vec2_t scroll, scralign_t align, qboolean roundOut, color_t color, qboolean additive, char *pic, char *maskPic)
{
	vec4_t			outColor;
	vec2_t			scaledOffset;
	drawStruct_t	ds = { 0 };
	qboolean		useMask = ( (maskPic != NULL) && (strlen(maskPic) > 0) );

	Vector2Copy (offset, scaledOffset);
	SCR_ScaleCoords (&x, &y, &width, &height, align);
	SCR_ScaleCoords (NULL, NULL, &scaledOffset[0], &scaledOffset[1], align);
	Vector4Set (outColor, (float)color[0]*DIV255, (float)color[1]*DIV255, (float)color[2]*DIV255, (float)color[3]*DIV255);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}
	if (additive) {
		ds.flags |= DSFLAG_ADDITIVE;
	}

	ds.pic = pic;
	if ( useMask ) {
		ds.maskPic = maskPic;
	}
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	ds.flags |= (useMask) ? DSFLAG_USESTCOORDS|DSFLAG_MASKED : DSFLAG_USESTCOORDS;
	Vector2Copy (scroll, ds.scroll);
	Vector2Copy (scaledOffset, ds.offset);
	Vector4Copy (texCorners, ds.stCoords);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawTiledPic
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawTiledPic (float x, float y, float width, float height, scralign_t align, qboolean roundOut, char *pic, float alpha)
{
	vec4_t			outColor;
	drawStruct_t	ds = { 0 };

	SCR_ScaleCoords (&x, &y, &width, &height, align);
	Vector4Set (outColor, 1.0f, 1.0f, 1.0f, alpha);
	if (roundOut) {
		x = floor(x);	y = floor(y);	width = ceil(width);	height = ceil(height);
	}

	ds.pic = pic;
	ds.x = x;	ds.y = y;	ds.w = width;	ds.h = height;
	ds.flags |= DSFLAG_TILED;
	Vector2Copy (vec2_origin, ds.offset);
	Vector4Copy (outColor, ds.color);
	R_DrawPic (&ds);
}


/*
================
SCR_DrawChar
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawChar (float x, float y, scralign_t align, int num, fontslot_t font, int red, int green, int blue, int alpha, qboolean italic, qboolean last)
{
	float	scale = SCR_GetScreenScale();

	SCR_ScaleCoords (&x, &y, NULL, NULL, align);
	R_DrawChar(x, y, num, font, scale, red, green, blue, alpha, italic, last);
}


/*
================
SCR_DrawSizedChar
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawSizedChar (float x, float y, int size, scralign_t align, int num, fontslot_t font, int red, int green, int blue, int alpha, qboolean italic, qboolean last)
{
	float	scale = SCR_ScaledScreen((float)size / (float)MENU_FONT_SIZE);	// SCR_GetScreenScale()

	SCR_ScaleCoords (&x, &y, NULL, NULL, align);
	R_DrawChar (x, y, num, font, scale, red, green, blue, alpha, italic, last);
}


/*
================
SCR_DrawString
Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawString (float x, float y, int size, scralign_t align, const char *string, fontslot_t font, int alpha)
{
	SCR_ScaleCoords (&x, &y, NULL, NULL, align);
	CL_DrawStringGeneric (x, y, string, font, alpha, size, SCALETYPE_MENU, false);
}

//===============================================================================

/*
================
SCR_CalcFPS

FPS counter, code combined from BramBo and Q2E
================
*/
#define FPS_FRAMES		4
static void SCR_CalcFPS (void)
{
	static int	previousTimes[FPS_FRAMES];
	static int	previousTime, fpscounter;
	static unsigned int	index;
	int			i, time, total, fps;

	if (cls.state != ca_active)
		return;

	if ((cl.time + 1000) < fpscounter) {
		fpscounter = cl.time + 100;
	}

	time = Sys_Milliseconds();
	previousTimes[index % FPS_FRAMES] = time - previousTime;
	previousTime = time;
	index++;

	if (index <= FPS_FRAMES) {
		scr_fps = 0;
		return;
	}

	// Average multiple frames together to smooth changes out a bit
	total = 0;
	for (i = 0; i < FPS_FRAMES; i++)
		total += previousTimes[i];
	total = max (total, 1);
	fps = 1000 * FPS_FRAMES / total;

	if (cl.time > fpscounter) {
		scr_fps = fps;
		fpscounter = cl.time + 100;
	}
}


/*
================
SCR_ShowFPS

FPS counter drawing
================
*/
static void SCR_ShowFPS (void)
{
	static char	fpsText[32];
	int			x, y, fragsSize;
	float		scrLeft;

	if ((cls.state != ca_active) || !(cl_drawfps->integer))
		return;

	SCR_InitHudScale ();

	Com_sprintf (fpsText, sizeof(fpsText), S_COLOR_BOLD S_COLOR_SHADOW"%3ifps", scr_fps);

	// leave space for 3-digit frag counter
	scrLeft = SCREEN_WIDTH;
	SCR_ScaleCoords (&scrLeft, NULL, NULL, NULL, ALIGN_STRETCH);
	fragsSize = SCR_GetHudScale() * 3 * (HUD_CHAR_WIDTH+2);
//	x = (viddef.width - stringLen(fpsText)*MENU_FONT_SIZE*SCR_GetScreenScale() - max(fragsSize, SCR_ScaledScreen(68)));
	x = (scrLeft - stringLen(fpsText)*MENU_FONT_SIZE*SCR_GetScreenScale() - max(fragsSize, SCR_ScaledScreen(68)));
	y = MENU_FONT_SIZE / 2;
	CL_DrawStringGeneric (x, y, fpsText, FONT_SCREEN, 255, MENU_FONT_SIZE, SCALETYPE_MENU, false); // SCALETYPE_CONSOLE
}

/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/

int currentping;
void CL_AddNetgraph (void)
{
	int		i;
	int		in;
	int		ping;

	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP-1);
	currentping = cls.realtime - cl.cmd_time[in];

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->integer || scr_timegraph->integer)
		return;

	for (i=0 ; i<cls.netchan.dropped ; i++)
		SCR_DebugGraph (30, 0x40);

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph (30, 0xdf);

	// see what the latency was on this packet
	ping = currentping/30;
	//ping = cls.realtime - cl.cmd_time[in];
	//ping /= 30;
	if (ping > 30)
		ping = 30;
	SCR_DebugGraph (ping, 0xd0);
}


typedef struct
{
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
	int				a, x, y, w, i, h, min, max;
	float			v, scrLeft, scrWidth, scrRight;
	int				color;
	static	float	lasttime = 0;
	static	int		/*fps,*/ ping;

	h = (2*FONT_SIZE > 40)?60+2*FONT_SIZE:100;
	w = (9*FONT_SIZE>100)?9*FONT_SIZE:100;

	scrLeft = 0;
	scrWidth = SCREEN_WIDTH;
	SCR_ScaleCoords (&scrLeft, NULL, &scrWidth, NULL, ALIGN_STRETCH);
	scrRight = scrLeft + scrWidth;

	if (scr_netgraph_pos->integer == 0) // bottom right
	{
	//	x = scr_vrect.width - (w+2) - 1;
		x = scrRight - (w+2) - 1;
		y = scr_vrect.height - (h+2) - 1;
	}	
	else // bottom left
	{
	//	x = 0;
		x = scrLeft;
		y = scr_vrect.height - (h+2) - 1;
	}	

	// trans background
	R_DrawFill (x, y, w+2, h+2, 255, 255, 255, 90);

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v*scr_graphscale->value;
		
		if (v < 1)
			v += h * (1+(int)(-v/h));

		max = (int)v % h + 1;
		min = y + h - max - scr_graphshift->value;

		// bind to box!
		if (min<y+1) min = y+1;
		if (min>y+h) min = y+h;
		if (min+max > y+h) max = y+h-max;

		R_DrawFill (x+w-a, min, 1, max, Q2PalColorRed(color), Q2PalColorGreen(color), Q2PalColorBlue(color), 255);
	}

	if (cls.realtime - lasttime > 50)
	{
		lasttime = cls.realtime;
	//	fps = (cls.renderFrameTime)? 1/cls.renderFrameTime: 0;
		ping = currentping;
	}

	CL_DrawStringGeneric (x, y + 5, va(S_COLOR_SHADOW"fps: %3i", scr_fps), FONT_SCREEN, 255, FONT_SIZE, SCALETYPE_CONSOLE, false);
	CL_DrawStringGeneric (x, y + 5 + FONT_SIZE, va(S_COLOR_SHADOW"ping:%3i", ping), FONT_SCREEN, 255, FONT_SIZE, SCALETYPE_CONSOLE, false);

	// draw border
	R_DrawFill (x,			y,			(w+2),	1,		0, 0, 0, 255);
	R_DrawFill (x,			y+(h+2),	(w+2),	1,		0, 0, 0, 255);
	R_DrawFill (x,			y,			1,		(h+2),	0, 0, 0, 255);
	R_DrawFill (x+(w+2),	y,			1,		(h+2),	0, 0, 0, 255);
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
float		scr_centertime_end;
int			scr_center_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	char	*s;
	char	line[64];
	int		i, j, l, il, vl;

	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_end = scr_centertime_off;
	scr_centertime_start = cl.time;

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;
		s++;
	}

	// echo it to the console
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	s = str;
	do	
	{
	// scan the width of the line
		for (l=0, il=0 ; l<40 ; l++) {
			if ((s[l] == '^') || (l>0 && s[l-1] == '^'))
				il++;
			if (s[l] == '\n' || !s[l])
				break;
		}
		vl = l - il;
		for (i=0 ; i<(40-vl)/2 ; i++)
			line[i] = ' ';

		for (j=0 ; j<l ; j++)
		{
			line[i++] = s[j];
		}

		line[i] = '\n';
		line[i+1] = 0;

		Com_Printf ("%s", line);

		while (*s && *s != '\n')
			s++;

		if (!*s)
			break;
		s++;		// skip the \n
	} while (1);
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_ClearNotify ();
}


void SCR_DrawCenterString (void)
{
	char	*start, line[512];
	int		l;
	int		j;
	int		y; //, x;
	int		remaining;
	// added Psychospaz's fading centerstrings
	int		alpha = 255 * ( 1 - (((cl.time + (scr_centertime->value-1) - scr_centertime_start) / 1000.0) / (scr_centertime_end)));		

	// the finale prints the characters one at a time
	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = viddef.height*0.35;
	else
		y = 48;

	do
	{	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;

		Com_sprintf (line, sizeof(line), "");
		for (j=0 ; j<l ; j++)
		{
			
			Com_sprintf (line, sizeof(line), "%s%c", line, start[j]);
			
			if (!remaining--)
				return;
		}
		CL_DrawStringGeneric ( (int)((viddef.width-stringLen(line)*FONT_SIZE)*0.5), y, line, FONT_SCREEN, alpha, FONT_SIZE, SCALETYPE_CONSOLE, false);
		y += FONT_SIZE;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_centertime_off -= cls.renderFrameTime;
	
	if (scr_centertime_off <= 0)
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect (void)
{
	int		size;

	// bound viewsize
//	if (scr_viewsize->value < 40)
	if (scr_viewsize->integer < 40)
		Cvar_Set ("viewsize","40");
//	if (scr_viewsize->value > 100)
	if (scr_viewsize->integer > 100)
		Cvar_Set ("viewsize","100");

//	size = scr_viewsize->value;
	size = scr_viewsize->integer;

	scr_vrect.width = viddef.width*size/100;
	scr_vrect.width &= ~1;	// Knightmare- was ~7, fixes undersized viewport at 1366x768

	scr_vrect.height = viddef.height*size/100;
	scr_vrect.height &= ~1;

	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	scr_vrect.y = (viddef.height - scr_vrect.height)/2;
}


/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void SCR_Sky_f (void)
{
	float	rotate;
	vec3_t	axis;
	char	blankName[8] = {0};

	if (Cmd_Argc() < 2)
	{
		Com_Printf ("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (Cmd_Argc() > 2)
		rotate = atof(Cmd_Argv(2));
	else
		rotate = 0;
	if (Cmd_Argc() == 6)
	{
		axis[0] = atof(Cmd_Argv(3));
		axis[1] = atof(Cmd_Argv(4));
		axis[2] = atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

//	R_SetSky (Cmd_Argv(1), rotate, axis);
	R_SetSky (Cmd_Argv(1), blankName, rotate, axis, 0, vec2_origin, vec3_origin, vec3_origin, vec3_origin);
}


/*
=================
SCR_SkyClouds_f

Set a specific sky, clouds name, rotation speed, axis, lightning freq, etc.
=================
*/
void SCR_SkyClouds_f (void)
{
	float	rotate;
	vec3_t	axis;
	float	lightningFreq = 0.0f;
	vec2_t	cloudDir = {0};
	vec3_t	cloudTile = {0}, cloudSpeed = {0}, cloudAlpha = {0};

	if (Cmd_Argc() < 3) {
		Com_Printf ("Usage: sky <skyboxName> <cloudsName> <rotate> <axis x y z> <lightningFreq> <cloudDir x y> <cloudTile 1 2 3> <cloudSpeed 1 2 3> <cloudAlpha 1 2 3>\n");
		return;
	}

	if (Cmd_Argc() > 3)
		rotate = atof(Cmd_Argv(3));
	else
		rotate = 0.0f;

	if (Cmd_Argc() > 6) {
		axis[0] = atof(Cmd_Argv(4));
		axis[1] = atof(Cmd_Argv(5));
		axis[2] = atof(Cmd_Argv(6));
	}
	else {
		axis[0] = 0.0f;
		axis[1] = 0.0f;
		axis[2] = 1.0f;
	}

	if (Cmd_Argc() > 7)
		lightningFreq = atof(Cmd_Argv(7));
	else
		lightningFreq = 0.25f;

	if (Cmd_Argc() > 9) {
		cloudDir[0] = atof(Cmd_Argv(8));
		cloudDir[1] = atof(Cmd_Argv(9));
	}
	else {
		Vector2Set (cloudDir, 1.0f, 0.8f);
	}

	if (Cmd_Argc() > 12) {
		cloudTile[0] = atof(Cmd_Argv(10));
		cloudTile[1] = atof(Cmd_Argv(11));
		cloudTile[2] = atof(Cmd_Argv(12));
	}
	else {
		VectorSet (cloudTile, 8.0f, 2.0f, 1.0f);
	}

	if (Cmd_Argc() > 15) {
		cloudSpeed[0] = atof(Cmd_Argv(13));
		cloudSpeed[1] = atof(Cmd_Argv(14));
		cloudSpeed[2] = atof(Cmd_Argv(15));
	}
	else {
		VectorSet (cloudSpeed, 1.0f, 4.0f, 8.0f);
	}

	if (Cmd_Argc() > 18) {
		cloudAlpha[0] = atof(Cmd_Argv(16));
		cloudAlpha[1] = atof(Cmd_Argv(17));
		cloudAlpha[2] = atof(Cmd_Argv(18));
	}
	else {
		VectorSet (cloudAlpha, 0.9f, 0.6f, 0.0f);
	}

	R_SetSky (Cmd_Argv(1), Cmd_Argv(2), rotate, axis, lightningFreq, cloudDir, cloudTile, cloudSpeed, cloudAlpha);
}


/*
================
SCR_SetHud_f
================
*/
void SCR_SetHud_f (void)
{
	if (Cmd_Argc() < 2)
	{
		Com_Printf ("Usage: sethud <hudname>\n");
		return;
	}

	CL_SetHud (Cmd_Argv(1));
}


/*
================
SCR_DumpStatusLayout_f

Saves the statusbar layout to a file
================
*/
void SCR_DumpStatusLayout_f (void)
{
	char	buffer[2048], rawLine[MAX_QPATH+1], formatLine[80], statLine[32];
	int		i, j, cs_general, bufcount = 0;
	FILE	*f;
	char	*p;
	char	name[MAX_OSPATH];

	// starting index is based on protocol for CS_GENERAL section
	if ( LegacyProtocol() )
		cs_general = OLD_CS_GENERAL;
	else
		cs_general = CS_GENERAL;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: dumpstatuslayout <filename>\n");
		return;
	}

	Com_sprintf (name, sizeof(name), "%s/%s.txt", FS_Savegamedir(), Cmd_Argv(1));	// was FS_Gamedir()

	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// statusbar layout is in multiple configstrings
	// starting at CS_STATUSBAR and ending at CS_AIRACCEL
	Com_sprintf (formatLine, sizeof(formatLine), "\nFormatted Dump\n--------------\n");
	Q_strncatz (buffer, sizeof(buffer), formatLine);
	bufcount += (int)strlen(formatLine);
	fwrite(&buffer, 1, bufcount, f);
	buffer[0] = 0;
	bufcount = 0;
	p = &buffer[0];
	bufcount = 0;
	for (i=CS_STATUSBAR; i<CS_AIRACCEL; i++)
	{
		for (j=0; j<MAX_QPATH; j++)
		{
			// check for end
			if (cl.configstrings[i][j] == '\0')
				break;

			*p = cl.configstrings[i][j];
			if (*p == '\0')	*p = ' ';
			if (*p == '\t')	*p = ' ';

			// check for "endif", insert newline after
			if (*p == 'f' && *(p-1) == 'i' && *(p-2) == 'd' && *(p-3) == 'n' && *(p-4) == 'e')
			{
				p++;
				bufcount++;
				*p = '\n';
			}
			p++;
			bufcount++;
		}
	}
	fwrite(&buffer, 1, bufcount, f);
	buffer[0] = 0;
	bufcount = 0;

	// write out the raw dump
	Com_sprintf (formatLine, sizeof(formatLine), "\nRaw Dump\n--------\n");
	Q_strncatz (buffer, sizeof(buffer), formatLine);
	bufcount += (int)strlen(formatLine);
	fwrite(&buffer, 1, bufcount, f);
	buffer[0] = 0;
	bufcount = 0;
	for (i=CS_STATUSBAR; i<CS_AIRACCEL; i++)
	{
		memset(rawLine, 0, sizeof(rawLine));
		for (j=0; j<MAX_QPATH; j++) {
			rawLine[j] = cl.configstrings[i][j];
			if (rawLine[j] == '\0')	rawLine[j] = ' ';
			if (rawLine[j] == '\t')	rawLine[j] = ' ';
		}
		rawLine[MAX_QPATH] = '\n';
		fwrite(&rawLine, 1, sizeof(rawLine), f);
	}

	// write out the stat values for debugging
	Com_sprintf (formatLine, sizeof(formatLine), "\nStat Values\n-----------\n");
	Q_strncatz (buffer, sizeof(buffer), formatLine);
	bufcount += (int)strlen(formatLine);
	for (i=0; i<MAX_STATS; i++)
	{
		Com_sprintf (statLine, sizeof(statLine), "%i: %i\n", i, cl.frame.playerstate.stats[i]);
		// prevent overflow of buffer
		if ( (bufcount + strlen(statLine)) >= sizeof(buffer) ) {
			fwrite(&buffer, 1, bufcount, f);
			buffer[0] = 0;
			bufcount = 0;
		}
		Q_strncatz (buffer, sizeof(buffer), statLine);
		bufcount += (int)strlen(statLine);
	}
	fwrite(&buffer, 1, bufcount, f);
	buffer[0] = 0;
	bufcount = 0;

	// write out CS_GENERAL for stat_string tokens
	Com_sprintf (formatLine, sizeof(formatLine), "\nGeneral Configstrings\n---------------------\n");
	Q_strncatz (buffer, sizeof(buffer), formatLine);
	bufcount += (int)strlen(formatLine);
	for (i = cs_general; i < (cs_general + MAX_GENERAL); i++)
	{
		Com_sprintf (formatLine, sizeof(formatLine), "%i: %s\n", i, cl.configstrings[i]);
		// prevent overflow of buffer
		if ( (bufcount + strlen(formatLine)) >= sizeof(buffer) ) {
			fwrite(&buffer, 1, bufcount, f);
			buffer[0] = 0;
			bufcount = 0;
		}
		Q_strncatz (buffer, sizeof(buffer), formatLine);
		bufcount += (int)strlen(formatLine);
	}
	fwrite(&buffer, 1, bufcount, f);
	buffer[0] = 0;
	bufcount = 0;

	fclose (f);

	Com_Printf ("Dumped statusbar layout to %s.\n", name);
}


/*
================
SCR_DumpGeneralLayout_f

Saves the general layout to a file
================
*/
void SCR_DumpGeneralLayout_f (void)
{
	char	buffer[2048];
	int		i, bufcount;
	FILE	*f;
	char	*p;
	char	name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: dumpgenerallayout <filename>\n");
		return;
	}

	Com_sprintf (name, sizeof(name), "%s/%s.txt", FS_Savegamedir(), Cmd_Argv(1));	// was FS_Gamedir()

	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// general layout is in cl.layout
	p = &buffer[0];
	bufcount = 0;

	for (i=0; i<sizeof(cl.layout); i++)
	{
		// check for end
		if (cl.layout[i] == '\0')
			break;

		*p = cl.layout[i];

		// check for "endif", insert newline after
		if (*p == 'f' && *(p-1) == 'i' && *(p-2) == 'd' && *(p-3) == 'n' && *(p-4) == 'e')
		{
			p++;
			bufcount++;
			*p = '\n';
		}
		p++;
		bufcount++;
	}

	fwrite(&buffer, 1, bufcount, f);
	fclose (f);

	Com_Printf ("Dumped general layout to %s.\n", name);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	cl_drawfps = Cvar_Get ("cl_drawfps", "0", CVAR_ARCHIVE);
	Cvar_SetDescription ("cl_drawfps", "Enables drawing of FPS counter on the screen.");
	cl_drawhud = Cvar_Get ("cl_drawhud", "1", CVAR_ARCHIVE);;	// HUD disable cvar
	Cvar_SetDescription ("cl_drawhud", "Toggles drawing of HUD.");
	cl_demomessage = Cvar_Get ("cl_demomessage", "1", CVAR_ARCHIVE);
	Cvar_SetDescription ("cl_demomessage", "Enables display of \"Running Demo\" message when a demo is playing.");
//	cl_loadpercent = Cvar_Get ("cl_loadpercent", "0", CVAR_ARCHIVE);	// unused
	cl_hud = Cvar_Get ("cl_hud", "default", CVAR_ARCHIVE|CVAR_NOSET);
	Cvar_SetDescription ("cl_hud", "The currently loaded HUD script.  Cannot be set from console, use the \"sethud\" command to change this.");
	cl_hud_variant = Cvar_Get ("cl_hud_variant", "default", CVAR_ARCHIVE);
	Cvar_SetDescription ("cl_hud_variant", "Overrides CS_HUDVARIANT to set the layout variant of the currently loaded HUD script.  Change from \"default\" if you need a specific game mode.  Leading '*' will be skipped.");

	scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);
	Cvar_SetDescription ("viewsize", "Draw size of screen in percent, from 40 to 100.");
	scr_conspeed = Cvar_Get ("scr_conspeed", "3", 0);
	Cvar_SetDescription ("scr_conspeed", "Scrolling speed of the console.");
	scr_conalpha = Cvar_Get ("scr_conalpha", "0.5", CVAR_ARCHIVE);		// Psychospaz's transparent console
	Cvar_SetDescription ("scr_conalpha", "Opacity of console background.");
	scr_newconback = Cvar_Get ("scr_newconback", "0", CVAR_ARCHIVE);	// whether to use new console background
	Cvar_SetDescription ("scr_newconback", "Toggles use of new console background.");
	scr_oldconbar = Cvar_Get ("scr_oldconbar", "1", CVAR_ARCHIVE);		// whether to draw bottom bar on old console
	Cvar_SetDescription ("scr_oldconbar", "Toggles drawing of solid color bottom bar on console with standard conback image.");
//	scr_conheight = Cvar_Get ("scr_conheight", "0.5", CVAR_ARCHIVE);	// how far the console drops down
	scr_letterbox = Cvar_Get ("scr_letterbox", "1", CVAR_ARCHIVE);
	Cvar_SetDescription ("scr_letterbox", "Enables letterbox effect for cameras and cutscenes.");
//	scr_showturtle = Cvar_Get ("scr_showturtle", "0", 0);	// unused
	scr_showpause = Cvar_Get ("scr_showpause", "1", 0);
	Cvar_SetDescription ("scr_showpause", "Toggles drawing of pause plaque.");
	scr_centertime = Cvar_Get ("scr_centertime", "3.5", 0);	// increased for fade
	Cvar_SetDescription ("scr_centertime", "Time in seconds for centerprints to fade away.");
//	scr_printspeed = Cvar_Get ("scr_printspeed", "8", 0);	// unused

	scr_netgraph = Cvar_Get ("netgraph", "0", 0);
	Cvar_SetDescription ("netgraph", "Enables drawing of network activity graph.");
	scr_netgraph_pos = Cvar_Get ("netgraph_pos", "0", CVAR_ARCHIVE);
	Cvar_SetDescription ("netgraph_pos", "Sets draw position of network activity graph.  0 = bottom right, 1 = bottom left.");
	scr_timegraph = Cvar_Get ("timegraph", "0", 0);
	Cvar_SetDescription ("timegraph", "Enables drawing of frame time graph.");
	scr_debuggraph = Cvar_Get ("debuggraph", "0", 0);		// does not show anything
	Cvar_SetDescription ("debuggraph", "Enables drawing of debug graph.");
//	scr_graphheight = Cvar_Get ("graphheight", "32", 0);	// unused
	scr_graphscale = Cvar_Get ("graphscale", "1", 0);
	Cvar_SetDescription ("graphscale", "Sets vertical scale of activity graph.");
	scr_graphshift = Cvar_Get ("graphshift", "0", 0);
	Cvar_SetDescription ("graphshift", "Sets vertical shift of activity graph.");
//	scr_drawall = Cvar_Get ("scr_drawall", "0", 0);	// unused

	crosshair = Cvar_Get ("crosshair", "1", CVAR_ARCHIVE);
	Cvar_SetDescription ("crosshair", "Sets crosshair image.  0 = no crosshair");
	crosshair_scale = Cvar_Get ("crosshair_scale", "1", CVAR_ARCHIVE);
	Cvar_SetDescription ("crosshair_scale", "Sets crosshair size.  Min = 0.25, Max = 15.");
	crosshair_red = Cvar_Get ("crosshair_red", "255", CVAR_ARCHIVE);
	Cvar_SetDescription ("crosshair_red", "Sets red color component of crosshair.");
	crosshair_green = Cvar_Get ("crosshair_green", "255", CVAR_ARCHIVE);
	Cvar_SetDescription ("crosshair_green", "Sets green color component of crosshair.");
	crosshair_blue = Cvar_Get ("crosshair_blue", "255", CVAR_ARCHIVE);
	Cvar_SetDescription ("crosshair_blue", "Sets blue color component of crosshair.");
	crosshair_alpha = Cvar_Get ("crosshair_alpha", "1", CVAR_ARCHIVE);
	Cvar_SetDescription ("crosshair_alpha", "Sets base opacity of crosshair.");
	crosshair_pulse = Cvar_Get ("crosshair_pulse", "0.25", CVAR_ARCHIVE);
	Cvar_SetDescription ("crosshair_pulse", "Sets opacity pulse amplitude of crosshair.");

	scr_simple_loadscreen = Cvar_Get ("scr_simple_loadscreen", "1", CVAR_ARCHIVE);
	Cvar_SetDescription ("scr_simple_loadscreen", "Toggles use of simplified load screen.  Value of 0 enables Quake2Max-style verbose load screen.");

	scr_surroundlayout = Cvar_Get ("scr_surroundlayout", "1", CVAR_ARCHIVE);	// whether to keep HUD/menu elements on center screen in triple-wide video modes
	Cvar_SetDescription ("scr_surroundlayout", "Enables use of triple-monitor surround layout with all HUD/menu graphics on center monitor.");
	scr_surroundleft = Cvar_Get ("scr_surroundleft", "0.333333333333", CVAR_ARCHIVE);		// left placement of HUD/menu elements on center screen in triple-wide video modes
	Cvar_SetDescription ("scr_surroundleft", "Changes left boundary for center monitor in triple-monitor surround layout.  Only change if you have a surround setup with different size monitors.");
	scr_surroundright = Cvar_Get ("scr_surroundright", "0.666666666667", CVAR_ARCHIVE);		// right placement of HUD/menu elements on center screen in triple-wide video modes
	Cvar_SetDescription ("scr_surroundright", "Changes right boundary for center monitor in triple-monitor surround layout.  Only change if you have a surround setup with different size monitors.");
	scr_surroundthreshold = Cvar_Get ("scr_surroundthreshold", "3.6", CVAR_ARCHIVE);		// minimum aspect ratio to trigger surround layout
	Cvar_SetDescription ("scr_surroundthreshold", "Sets minimum aspect ratio to trigger triple-monitor surround layout scaling.");

	scr_hudsize = Cvar_Get ("scr_hudsize", "5", CVAR_ARCHIVE);
	Cvar_SetDescription ("scr_hudsize", "Sets scale for default (server-set) HUD.");
	scr_hudalpha = Cvar_Get ("scr_hudalpha", "1", CVAR_ARCHIVE);
	Cvar_SetDescription ("scr_hudalpha", "Sets opacity of HUD.");
	scr_hudsqueezedigits = Cvar_Get ("scr_hudsqueezedigits", "1", CVAR_ARCHIVE);
	Cvar_SetDescription ("scr_hudsqueezedigits", "Enables display of 4 and 5-digit numbers on HUD.");

//
// register our commands
//
	Cmd_AddCommand ("timerefresh", SCR_TimeRefresh_f);
	Cmd_AddCommand ("loading", SCR_Loading_f);
	Cmd_AddCommand ("sizeup", SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown", SCR_SizeDown_f);
	Cmd_AddCommand ("sky", SCR_Sky_f);
	Cmd_AddCommand ("skyclouds", SCR_SkyClouds_f);
	Cmd_AddCommand ("sethud", SCR_SetHud_f);
	Cmd_AddCommand ("dumpstatuslayout", SCR_DumpStatusLayout_f);
	Cmd_AddCommand ("dumpgenerallayout", SCR_DumpGeneralLayout_f);

	SCR_InitScreenScale ();
	SCR_InitHudScale ();
	CL_LoadHud (true);	// Custom HUD

	scr_initialized = true;
}


/*
==================
SCR_Shutdown
==================
*/
void SCR_Shutdown (void)
{
	if (!scr_initialized)
		return;

	Cmd_RemoveCommand ("timerefresh");
	Cmd_RemoveCommand ("loading");
	Cmd_RemoveCommand ("sizeup");
	Cmd_RemoveCommand ("sizedown");
	Cmd_RemoveCommand ("sky");
	Cmd_RemoveCommand ("skyclouds");
	Cmd_RemoveCommand ("sethud");
	Cmd_RemoveCommand ("dumpstatuslayout");
	Cmd_RemoveCommand ("dumpgenerallayout");

	CL_FreeHud ();

	scr_initialized = false;
}


/*
=================
SCR_DrawCrosshair
Moved from cl_view.c, what the hell was it doing there?
=================
*/
#define CROSSHAIR_SIZE 32

// Psychospaz's new crosshair code
void SCR_DrawCrosshair (void)
{	
	float			scaledSize, alpha, pulsealpha, x, y, w, h;
	int				red, green, blue;
	vec4_t			crossColor;
	drawStruct_t	ds = { 0 };

	if ( !crosshair->integer || cl_zoommode->integer || scr_hidehud )
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		SCR_TouchPics ();

		if ( FS_ModType("dday") ) // D-Day has no crosshair (FORCED)
			Cvar_SetValue("crosshair", 0);
	}

	if (crosshair_scale->modified)
	{
		crosshair_scale->modified = false;
		if (crosshair_scale->value > 15)
			Cvar_SetValue("crosshair_scale", 15);
		else if (crosshair_scale->value < 0.25)
			Cvar_SetValue("crosshair_scale", 0.25);
	}

	if (!crosshair_pic[0])
		return;

	scaledSize = crosshair_scale->value * CROSSHAIR_SIZE;
	pulsealpha = crosshair_alpha->value * crosshair_pulse->value;
	alpha = max(min(crosshair_alpha->value - pulsealpha + pulsealpha*sin(anglemod(cl.time*0.005)), 1.0), 0.0);
	red = max(min(crosshair_red->integer, 255), 0);
	green = max(min(crosshair_green->integer, 255), 0);
	blue = max(min(crosshair_blue->integer, 255), 0);
	Vector4Set (crossColor, red*DIV255, green*DIV255, blue*DIV255, alpha);

	x = ((float)SCREEN_WIDTH - scaledSize) * 0.5;
	y = ((float)SCREEN_HEIGHT - scaledSize) * 0.5;
	w = h = scaledSize;
	SCR_ScaleCoords (&x, &y, &w, &h, ALIGN_CENTER);

	ds.pic = crosshair_pic;
	ds.x = x;	ds.y = y;	ds.w = w;	ds.h = h;
	Vector2Copy (vec2_origin, ds.offset);
	Vector4Copy (crossColor, ds.color);
	R_DrawPic (&ds);

//	SCR_DrawPic ( ((float)SCREEN_WIDTH - scaledSize)*0.5, ((float)SCREEN_HEIGHT - scaledSize)*0.5,
//					scaledSize, scaledSize, ALIGN_CENTER, false, crosshair_pic, alpha);
}


/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if ( (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged) < (CMD_BACKUP-1) )
		return;

	SCR_DrawPic (scr_vrect.x+64, scr_vrect.y, 32, 32, ALIGN_TOPLEFT, false, "net", 1.0f);
}


/*
==============
SCR_DrawPause
==============
*/
void SCR_DrawPause (void)
{
	int		w, h, x, y;

	w = 129;	// size of original pause.pcx = 101x25
	h = 32;
	x = (SCREEN_WIDTH - w)*0.5;
	y = (SCREEN_HEIGHT - h)*0.5;

	if (!scr_showpause->integer)		// turn off for screenshots
		return;

	if (!cl_paused->integer)
		return;

	// Knightmare- no need to draw when in menu
	if (cls.key_dest == key_menu)
		return;

//	R_DrawGetPicSize (&w, &h, "pause");
	SCR_DrawPic (x, y, w, h, ALIGN_CENTER, false, "pause", 1.0);
}


/*
==============
SCR_DrawLoadingTagProgress
==============
*/
#define LOADBAR_TIC_SIZE_X 4
#define LOADBAR_TIC_SIZE_Y 4
void SCR_DrawLoadingTagProgress (char *picName, int yOffset, int percent)
{
	int		w, h, x, y, i, barPos;

	w = 160;	// size of loading_bar.tga = 320x80
	h = 40;
	x = (SCREEN_WIDTH - w)*0.5;
	y = (SCREEN_HEIGHT - h)*0.5;
	barPos = min(max(percent, 0), 100) / 4;

	SCR_DrawPic (x, y + yOffset, w, h, ALIGN_CENTER, false, picName, 1.0);

	for (i=0; i<barPos; i++)
		SCR_DrawPic (x + 33 + (i * LOADBAR_TIC_SIZE_X), y + 28 + yOffset, LOADBAR_TIC_SIZE_X, LOADBAR_TIC_SIZE_Y,
					ALIGN_CENTER, false, "loading_led1", 1.0);
}


/*
==============
SCR_DrawDownloadingTag
==============
*/
void SCR_DrawDownloadingTag (int yOffset)
{
	int		w, h, x, y;

	w = 208;	// size of downloading.tga = 416x64
	h = 32;
	x = (SCREEN_WIDTH - w)*0.5;
	y = (SCREEN_HEIGHT - h)*0.5;

	SCR_DrawPic(x, y + yOffset, w, h, ALIGN_CENTER, false, "downloading", 1.0);
}


/*
==============
SCR_DrawLoadingTag
==============
*/
void SCR_DrawLoadingTag (int yOffset)
{
	int		w, h, x, y;

	w = 142;	// size of original loading.pcx = 111x25
	h = 32;
	x = (SCREEN_WIDTH - w)*0.5;
	y = (SCREEN_HEIGHT - h)*0.5;

	SCR_DrawPic(x, y + yOffset, w, h, ALIGN_CENTER, false, "loading", 1.0);
}


/*
==============
SCR_DrawLoadingBar
==============
*/
void SCR_DrawLoadingBar (float x, float y, float w, float h, int indent, int percent)
{	
	int		red, green, blue;

	// changeable download/map load bar color
	CL_TextColor (alt_text_color->integer, &red, &green, &blue);

	SCR_DrawFill (x, y, w, h, ALIGN_STRETCH, false, 255, 255, 255, 90);

	if (percent != 0)
		SCR_DrawFill (x+indent, y+indent, (w-(2*indent))*percent*0.01, h-(2*indent),
						ALIGN_STRETCH, false, red, green, blue, 255);
}


/*
==============
SCR_GetPicPosWidth
Gets virtual 640x480 x-pos and width
for a fullscreen pic of any aspect ratio.
==============
*/
void SCR_GetPicPosWidth (char *pic, int *x, int *w)
{
	int		picWidth, picHeight, virtualWidth, virtual_x;
	float	picAspect;

	if (!pic || !x || !w)	// catch null pointers
		return;

	R_DrawGetPicSize (&picWidth, &picHeight, pic);
	picAspect = (float)picWidth / (float)picHeight;
	virtualWidth = SCREEN_HEIGHT * picAspect;
	virtual_x = (SCREEN_WIDTH - virtualWidth) / 2;
	*x = virtual_x;
	*w = virtualWidth;
}


/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	int			plaqueOffset, picX, picW;
	char		mapfile[64], picName[MAX_QPATH];
	char		*loadMsg;
	qboolean	isMap = false, haveMapPic = false, widescreen;
	qboolean	simplePlaque = (scr_simple_loadscreen->integer != 0);

	if (!scr_draw_loading) {
		cls.loadingPercent = 0;
		return;
	}

	scr_draw_loading = 0;
	widescreen = (((float)viddef.width / (float)viddef.height) > STANDARD_ASPECT_RATIO);

	// loading a map...
	if (cls.loadingMessage && cl.configstrings[CS_MODELS+1][0])
	{
		Q_strncpyz (mapfile, sizeof(mapfile), cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
		mapfile[strlen(mapfile)-4] = 0;		// cut off ".bsp"

		// show saveshot here
		if (scr_load_saveshot && (strlen(scr_load_saveshot) > 8) && R_DrawFindPic(scr_load_saveshot))
		{
			float	screenAspect = (float)viddef.width / (float)viddef.height;

			// saveshot only grabs center screen in surround modes, so draw saveshot on center screen
			if ( scr_surroundlayout->integer && (screenAspect >= scr_surroundthreshold->value) )
				SCR_DrawPic (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_STRETCH, false, scr_load_saveshot, 1.0);
			else
				SCR_DrawPic (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_STRETCH_ALL, false, scr_load_saveshot, 1.0);
			haveMapPic = true;
		}
		// else try levelshot
		else if (/*widescreen &&*/ R_DrawFindPic(va("/levelshots/%s_widescreen.pcx", mapfile)))
		{
			SCR_DrawFill (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_STRETCH_ALL, false, 0, 0, 0, 255);
		//	SCR_DrawPic (-64, 0, SCREEN_WIDTH+128, SCREEN_HEIGHT, ALIGN_CENTER, false, va("/levelshots/%s_widescreen.pcx", mapfile), 1.0);
			// Draw at native aspect, don't stretch to 16:9 or wider
			Com_sprintf (picName, sizeof(picName), "/levelshots/%s_widescreen.pcx", mapfile);
			SCR_GetPicPosWidth (picName, &picX, &picW);
			SCR_DrawPic (picX, 0, picW, SCREEN_HEIGHT, ALIGN_CENTER, false, picName, 1.0);
			haveMapPic = true;
		}
		else if (R_DrawFindPic(va("/levelshots/%s.pcx", mapfile))) {
			// Draw at 4:3 aspect, don't stretch to 16:9 or wider
			SCR_DrawFill (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_STRETCH_ALL, false, 0, 0, 0, 255);
			SCR_DrawPic (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_CENTER, false, va("/levelshots/%s.pcx", mapfile), 1.0); // was ALIGN_STRETCH
			haveMapPic = true;
		}
		// else fall back on loadscreen
		else if (R_DrawFindPic(LOADSCREEN_NAME)) {
			SCR_DrawFill (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_STRETCH_ALL, false, 0, 0, 0, 255);
		//	SCR_DrawPic (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_CENTER, false, LOADSCREEN_NAME, 1.0);
			SCR_GetPicPosWidth (LOADSCREEN_NAME, &picX, &picW);
			SCR_DrawPic (picX, 0, picW, SCREEN_HEIGHT, ALIGN_CENTER, false, LOADSCREEN_NAME, 1.0);
		}
		// else draw black screen
		else
			SCR_DrawFill (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_STRETCH_ALL, false, 0, 0, 0, 255);

		isMap = true;
	}
	else if (R_DrawFindPic(LOADSCREEN_NAME)) {
		SCR_DrawFill (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_STRETCH_ALL, false, 0, 0, 0, 255);
	//	SCR_DrawPic (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_CENTER, false, LOADSCREEN_NAME, 1.0);
		SCR_GetPicPosWidth (LOADSCREEN_NAME, &picX, &picW);
		SCR_DrawPic (picX, 0, picW, SCREEN_HEIGHT, ALIGN_CENTER, false, LOADSCREEN_NAME, 1.0);
	}
	else
		SCR_DrawFill (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ALIGN_STRETCH_ALL, false, 0, 0, 0, 255);

	// Add Download info stuff...
#ifdef USE_CURL	// HTTP downloading from R1Q2
	if ( cls.downloadname[0] && (cls.download || cls.downloadposition) )
#else
	if (cls.download) // download bar...
#endif	// USE_CURL
	{
		if (simplePlaque)
		{
			plaqueOffset = -48;
			if (cls.downloadrate > 0.0f)
				loadMsg = va("Downloading ["S_COLOR_ALT"%s"S_COLOR_WHITE"]: %3d%% (%4.2fKB/s)", cls.downloadname, cls.downloadpercent, cls.downloadrate);
			else
				loadMsg = va("Downloading ["S_COLOR_ALT"%s"S_COLOR_WHITE"]: %3d%%", cls.downloadname, cls.downloadpercent);

			SCR_DrawString (SCREEN_WIDTH*0.5 - MENU_FONT_SIZE*stringLen(loadMsg)*0.5,
							SCREEN_HEIGHT*0.5 + (plaqueOffset + 48), MENU_FONT_SIZE, ALIGN_CENTER, loadMsg, FONT_SCREEN, 255);

			SCR_DrawLoadingTagProgress ("downloading_bar", plaqueOffset, cls.downloadpercent);
		}
		else
		{
			plaqueOffset = -130;
			loadMsg = va("Downloading ["S_COLOR_ALT"%s"S_COLOR_WHITE"]", cls.downloadname);

			SCR_DrawString (SCREEN_WIDTH*0.5 - MENU_FONT_SIZE*stringLen(loadMsg)*0.5,
							SCREEN_HEIGHT*0.5 + MENU_FONT_SIZE*4.5, MENU_FONT_SIZE, ALIGN_CENTER, loadMsg, FONT_SCREEN, 255);

			if (cls.downloadrate > 0.0f)
				loadMsg = va("%3d%% (%4.2fKB/s)", cls.downloadpercent, cls.downloadrate);
			else
				loadMsg = va("%3d%%", cls.downloadpercent);

			SCR_DrawString (SCREEN_WIDTH*0.5 - MENU_FONT_SIZE*stringLen(loadMsg)*0.5,
							SCREEN_HEIGHT*0.5 + MENU_FONT_SIZE*6, MENU_FONT_SIZE, ALIGN_CENTER, loadMsg, FONT_SCREEN, 255);

			SCR_DrawLoadingBar (SCREEN_WIDTH*0.5 - 180, SCREEN_HEIGHT*0.5 + 60, 360, 24, 3, cls.downloadpercent);
			SCR_DrawDownloadingTag (plaqueOffset);
		}
	}
	// Loading message stuff && loading bar...
	else if (isMap)
	{
		qboolean	drawMapName = false, drawLoadingMsg = false;

		if (!simplePlaque) {
			plaqueOffset = -72;
			drawMapName = drawLoadingMsg = true;
		}
		else if (!haveMapPic) {
			plaqueOffset = -48;
			drawMapName = true;
		}
		else
			plaqueOffset = 0;

		if (drawMapName) {
			loadMsg = va(S_COLOR_SHADOW S_COLOR_WHITE"Loading Map ["S_COLOR_ALT"%s"S_COLOR_WHITE"]", cl.configstrings[CS_NAME]);
			SCR_DrawString (SCREEN_WIDTH*0.5 - MENU_FONT_SIZE*stringLen(loadMsg)*0.5,
							SCREEN_HEIGHT*0.5 + (plaqueOffset + 48), MENU_FONT_SIZE, ALIGN_CENTER, loadMsg, FONT_SCREEN, 255);
		}
		if (drawLoadingMsg) {
			loadMsg = va(S_COLOR_SHADOW"%s", cls.loadingMessages);
			SCR_DrawString (SCREEN_WIDTH*0.5 - MENU_FONT_SIZE*stringLen(loadMsg)*0.5,
							SCREEN_HEIGHT*0.5 + (plaqueOffset + 72), MENU_FONT_SIZE, ALIGN_CENTER, loadMsg, FONT_SCREEN, 255);
		}

		if (simplePlaque)
			SCR_DrawLoadingTagProgress ("loading_bar", plaqueOffset, (int)cls.loadingPercent);
		else {
			SCR_DrawLoadingBar (SCREEN_WIDTH*0.5 - 180, SCREEN_HEIGHT - 20, 360, 15, 3, (int)cls.loadingPercent);
			SCR_DrawLoadingTag (plaqueOffset);
		}
	}
	else {// just a plain old loading plaque
		if (simplePlaque)
			SCR_DrawLoadingTagProgress ("loading_bar", 0, (int)cls.loadingPercent);
		else
			SCR_DrawLoadingTag (0);
	}
}

//=============================================================================

/*
==================
SCR_RunLetterbox
==================
*/
#define LETTERBOX_RATIO 0.5625 // 16:9 aspect ratio (inverse)
//#define LETTERBOX_RATIO 0.625f // 16:10 aspect ratio
void SCR_RunLetterbox (void)
{
	// decide on the height of the letterbox
	if (scr_letterbox->integer && (cl.refdef.rdflags & RDF_LETTERBOX)) {
		scr_letterbox_lines = (1 - min(1, viddef.width*LETTERBOX_RATIO/viddef.height)) * 0.5;
		scr_letterbox_active = true;
		scr_hidehud = true;
	}
	else if (cl.refdef.rdflags & RDF_CAMERAEFFECT) {
		scr_letterbox_lines = 0;
		scr_letterbox_active = false;
		scr_hidehud = true;
	}
	else {
		scr_letterbox_lines = 0;
		scr_letterbox_active = false;
		scr_hidehud = false;
	}
	
	if (scr_letterbox_current > scr_letterbox_lines)
	{
		scr_letterbox_current -= 1*cls.renderFrameTime;
		if (scr_letterbox_current < scr_letterbox_lines)
			scr_letterbox_current = scr_letterbox_lines;

	}
	else if (scr_letterbox_current < scr_letterbox_lines)
	{
		scr_letterbox_current += 1*cls.renderFrameTime;
		if (scr_letterbox_current > scr_letterbox_lines)
			scr_letterbox_current = scr_letterbox_lines;
	}
}


/*
==================
SCR_DrawCameraEffect
==================
*/
void SCR_DrawCameraEffect (void)
{
	if (!(cl.refdef.rdflags & RDF_CAMERAEFFECT))
		return;

	R_DrawCameraEffect ();
}


/*
==================
SCR_DrawLetterbox
==================
*/
void SCR_DrawLetterbox (void)
{
	int boxheight, boxalpha;

	if (!scr_letterbox_active)
		return;

	boxheight = scr_letterbox_current * viddef.height;
	boxalpha = scr_letterbox_current / scr_letterbox_lines * 255;
	R_DrawFill (0, 0, viddef.width, boxheight, 0, 0, 0, boxalpha);
	R_DrawFill (0, viddef.height-boxheight, viddef.width, boxheight, 0, 0, 0, boxalpha);
}

//=============================================================================

/*
==================
SCR_RunConsole
Scroll it up or down
==================
*/
void SCR_RunConsole (void)
{
	// Knightmare- clamp console height
//	if (scr_conheight->value < 0.1f || scr_conheight->value > 1.0f)
//		Cvar_SetValue( "scr_conheight", ClampCvar( 0.1, 1, scr_conheight->value ) );

	// decide on the height of the console
	if (cls.consoleActive) // (cls.key_dest == key_console)
	//	scr_conlines = halfconback ? 0.5 : scr_conheight->value; // variable height console
		scr_conlines = 0.5;
	else
		scr_conlines = 0;				// none visible
	
	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value*cls.renderFrameTime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value*cls.renderFrameTime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	Con_CheckResize ();
	
	// clamp console height
//	if (scr_conheight->value < 0.1f || scr_conheight->value > 1.0f)
//		Cvar_SetValue ( "scr_conheight", ClampCvar(0.1, 1, scr_conheight->value) );

/*	if ( (cls.key_dest != key_menu)
		&& (cls.state == ca_disconnected || cls.state == ca_connecting) )
	{
		R_DrawFill (0, 0, viddef.width, viddef.height, 0);
		Con_DrawConsole (scr_conheight->value, false);
		return;
	}

	// connected, but can't render
	if ( (cls.key_dest != key_menu) && (cl.cinematicframe <= 0)
		&& (cls.state != ca_active || !cl.refresh_prepped) 
		&& (Com_ServerState() != 5) ) // fix stuck console over menu		 
	{
		if ((scr_draw_loading))
			Con_DrawConsole (scr_conheight->value, true);
		else
		{
			R_DrawFill (0, 0, viddef.width, viddef.height, 0);
			Con_DrawConsole (scr_conheight->value, false);
		}
		return;
	} */

	// can't render menu or game
	if ( (cls.key_dest != key_menu) &&
		( (cls.state == ca_disconnected || cls.state == ca_connecting) ||
			( (cls.state != ca_active || !cl.refresh_prepped) &&
				(cl.cinematicframe <= 0) &&
				(Com_ServerState() < 3) // was != 5
			)
		) )
	{
		if (!scr_draw_loading) // no background
			R_DrawFill (0, 0, viddef.width, viddef.height, 0, 0, 0, 255);
	//	Con_DrawConsole (halfconback?0.5:scr_conheight->value, false);
		Con_DrawConsole (0.5, false);
		return;
	}

	if (scr_con_current)
		Con_DrawConsole (scr_con_current, true);
	else if (!cls.consoleActive && !cl.cinematictime
		&& (cls.key_dest == key_game || cls.key_dest == key_message))
		Con_DrawNotify (); // only draw notify in game
}

//=============================================================================

qboolean SCR_NeedLoadingPlaque (void)
{
	if (!cls.disable_screen || !scr_draw_loading)
		return true;
	return false;
}

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds ();
	cl.sound_prepped = false;		// don't play ambients
	CDAudio_Stop ();
//	if (cls.disable_screen)
//		return;
	if (developer->integer)
		return;

	cls.consoleActive = false; // Knightmare added

/*	if (cls.state == ca_disconnected)
		return;	// if at console, don't bring up the plaque
	if (cls.key_dest == key_console)
		return; */
	if (cl.cinematictime > 0)
		scr_draw_loading = 2;	// clear to black first
	else
		scr_draw_loading = 1;

	SCR_UpdateScreen ();
	cls.disable_screen = Sys_Milliseconds ();
	cls.disable_servercount = cl.servercount;
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	// make loading saveshot null here
	scr_load_saveshot = NULL;
	cls.disable_screen = 0;
	scr_draw_loading = 0; // Knightmare added
	Con_ClearNotify ();
}

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f (void)
{
	SCR_BeginLoadingPlaque ();
}

/*
================
SCR_TimeRefresh_f
================
*/
void SCR_TimeRefresh_f (void)
{
	int		i;
	int		start, stop;
	float	time;

	if ( cls.state != ca_active )
		return;

	start = Sys_Milliseconds ();

	if (Cmd_Argc() == 2)
	{	// run without page flipping
		R_BeginFrame (0);
		for (i=0 ; i<128 ; i++)
		{
			cl.refdef.viewangles[1] = i/128.0*360.0;
			R_RenderFrame (&cl.refdef);
		}
		R_EndFrame ();
	}
	else
	{
		for (i=0 ; i<128 ; i++)
		{
			cl.refdef.viewangles[1] = i/128.0*360.0;

			R_BeginFrame (0);
			R_RenderFrame (&cl.refdef);
			R_EndFrame ();
		}
	}

	stop = Sys_Milliseconds ();
	time = (stop-start)/1000.0;
	Com_Printf ("%f seconds (%f fps)\n", time, 128/time);
}


/*
==============
SCR_TileClear

Clear around a sized down screen
==============
*/
void SCR_TileClear (void)
{
	int				top, bottom, left, right;
	int				w, h;
	drawStruct_t	ds = { 0 };

	if (scr_con_current == 1.0)
		return;		// full screen console
	if (scr_viewsize->integer == 100)
		return;		// full screen rendering
	if (cl.cinematictime > 0)
		return;		// full screen cinematic

	w = viddef.width;
	h = viddef.height;

	top = cl.refdef.y;
	bottom = top + cl.refdef.height-1;
	left = cl.refdef.x;
	right = left + cl.refdef.width-1;

	ds.pic = "backtile";
	ds.flags |= DSFLAG_TILED;
	Vector2Copy (vec2_origin, ds.offset);
	Vector4Copy (vec4_identity, ds.color);

	// clear above view screen
	ds.x = 0;	ds.y = 0;		ds.w = w;		ds.h = top;
	R_DrawPic (&ds);

	// clear below view screen
	ds.x = 0;	ds.y = bottom;	ds.w = w;		ds.h = h - bottom;
	R_DrawPic (&ds);

	// clear left of view screen
	ds.x = 0;	ds.y = top;		ds.w = left;	ds.h = bottom - top + 1;
	R_DrawPic (&ds);

	// clear right of view screen
	ds.x = right;	ds.y = top;	ds.w = w - right;	ds.h = bottom - top + 1;
	R_DrawPic (&ds);
}


/*
===============
SCR_TouchPics

Allows rendering code to cache all needed status bar graphics
===============
*/
void SCR_TouchPics (void)
{
	int		i, j;

	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			R_DrawFindPic (sb_nums[i][j]);

	if (crosshair->integer)
	{
		if (crosshair->integer > 100 || crosshair->integer < 0) {
		//	Cvar_SetValue ("crosshair", 1);
			Cvar_SetInteger ("crosshair", 1);
		}

	//	Com_sprintf (crosshair_pic, sizeof(crosshair_pic), "ch%i", (int)(crosshair->value));
		Com_sprintf (crosshair_pic, sizeof(crosshair_pic), "ch%i", crosshair->integer);
		R_DrawGetPicSize (&crosshair_width, &crosshair_height, crosshair_pic);
		if (!crosshair_width)
			crosshair_pic[0] = 0;
	}
}

//=======================================================

/*
==================
CL_DrawDemoMessage
==================
*/
void CL_DrawDemoMessage (void)
{
	// running demo message
//	if ( cl.attractloop && !(cl.cinematictime > 0 && cls.realtime - cl.cinematictime > 1000))
	if ( IsRunningDemo() )
	{
		int len;
		char *message = "Running Demo";
		len = (int)strlen(message);

		SCR_DrawFill (0, SCREEN_HEIGHT-(MENU_FONT_SIZE+3), SCREEN_WIDTH, MENU_FONT_SIZE+3, ALIGN_BOTTOM_STRETCH, true, 60, 60, 60, 255);
		SCR_DrawFill (0, SCREEN_HEIGHT-(MENU_FONT_SIZE+3), SCREEN_WIDTH, 1, ALIGN_BOTTOM_STRETCH, false, 0, 0, 0, 255);
		SCR_DrawString (SCREEN_WIDTH/2-(len/2)*MENU_FONT_SIZE, SCREEN_HEIGHT-(MENU_FONT_SIZE+1), MENU_FONT_SIZE, ALIGN_BOTTOM, message, FONT_SCREEN, 255);
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen (void)
{
	int numframes;
	int i;
	float separation[2] = { 0, 0 };

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (cls.download) { // Knightmare- don't time out on downloads
			cls.disable_screen = Sys_Milliseconds ();
		}
		if (Sys_Milliseconds() - cls.disable_screen > 120000
			&& cl.refresh_prepped && !(cl.cinematictime > 0)) // Knightmare- don't time out on vid restart
		{
			cls.disable_screen = 0;
			Com_Printf ("Loading plaque timed out.\n");
			return; // Knightmare- moved here for loading screen
		}
		scr_draw_loading = 2; // Knightmare- added for loading screen
	}

	if (!scr_initialized || !con.initialized) {
		return;				// not initialized yet
	}

	//
	// range check cl_camera_separation so we don't inadvertently fry someone's
	// brain
	//
	if ( cl_stereo_separation->value > 1.0 ) {
		Cvar_SetValue( "cl_stereo_separation", 1.0 );
	}
	else if ( cl_stereo_separation->value < 0 ) {
		Cvar_SetValue( "cl_stereo_separation", 0.0 );
	}

	// Re-init screen scale
	SCR_InitScreenScale ();

	if ( cl_stereo->integer ) {
		numframes = 2;
		separation[0] = -cl_stereo_separation->value / 2;
		separation[1] =  cl_stereo_separation->value / 2;
	}		
	else {
		separation[0] = 0;
		separation[1] = 0;
		numframes = 1;
	}

	for ( i = 0; i < numframes; i++ )
	{
		R_BeginFrame (separation[i]);

		if (scr_draw_loading == 2)
		{	//  loading plaque over black screen
		//	R_SetPalette(NULL);
			// Knightmare- refresh loading screen
			SCR_DrawLoading ();

			// Knightmare- set back for loading screen
			if (cls.disable_screen) {
				scr_draw_loading = 2;
			}
			if (cls.consoleActive) {
				Con_DrawConsole (0.5, false);
			}
			// NO FULLSCREEN CONSOLE!!!
			continue;
		} 
		// if a cinematic is supposed to be running, handle menus
		// and console specially
		else if (cl.cinematictime > 0)
		{
			if (cls.key_dest == key_menu)
			{
			/*	if (cl.cinematicpalette_active) {
					R_SetPalette(NULL);
					cl.cinematicpalette_active = false;
				} */
				UI_Draw ();
			}
			else {
				SCR_DrawCinematic();
			}
		}
		else 
		{
			// Knightmare added- disconnected menu
			if (cls.state == ca_disconnected && cls.key_dest != key_menu && !scr_draw_loading) 
			{
				SCR_EndLoadingPlaque ();	// get rid of loading plaque
			//	cls.consoleActive = true;	// show error in console
				UI_RootMenu ();
			}

			// make sure the game palette is active
		/*	if (cl.cinematicpalette_active) {
				R_SetPalette(NULL);
				cl.cinematicpalette_active = false;
			} */

			// do 3D refresh drawing, and then update the screen
			SCR_CalcVrect ();

			// clear background around sized down view
			SCR_TileClear ();

			V_RenderView ( separation[i] );

			// don't draw crosshair while in menu
			if (cls.key_dest != key_menu) {
				SCR_DrawCrosshair ();
			}

			if (!scr_hidehud)
			{
				// update hud variant if cl_hud_variant is modified
				if (cl_hud_variant->modified) {
					cl_hud_variant->modified = false;
					CL_SetHudVariant ();
				}
				if (cl_drawhud->integer) {
					CL_DrawStatus ();
				}
				if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1) {
					CL_DrawLayout ();
				}
				if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2) {
					CL_DrawInventory ();
				}
			}
			SCR_DrawCameraEffect ();
			SCR_DrawLetterbox ();

			SCR_DrawNet ();
			SCR_CheckDrawCenterString ();

			if (scr_timegraph->integer) {
				SCR_DebugGraph (cls.netFrameTime*300, 0);
			}

			if (scr_debuggraph->integer || scr_timegraph->integer || scr_netgraph->integer) {
				SCR_DrawDebugGraph ();
			}

			SCR_DrawPause ();

			if (cl_demomessage->integer) {
				CL_DrawDemoMessage();
			}

			SCR_CalcFPS ();
			SCR_ShowFPS ();

			UI_Draw ();

			SCR_DrawLoading ();
		}
		SCR_DrawConsole ();
	}
	R_EndFrame ();
}
