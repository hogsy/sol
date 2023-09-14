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

// r_misc.c - particle image loading, and screenshots

#include "r_local.h"

#ifdef _WIN32

#include "../libraries/jpeg/jpeglib.h"
#ifdef PNG_SUPPORT
#include "../libraries/png/png.h"
#endif	// PNG_SUPPORT

#else	// _WIN32

#include <jpeglib.h>
#ifdef PNG_SUPPORT
#include <png.h>
#endif	// PNG_SUPPORT

#endif	// _WIN32

/*
==================
R_CreateNullTexture
==================
*/
#define NULLTEX_SIZE 16
image_t * R_CreateNullTexture (void)
{
	byte	nulltex[NULLTEX_SIZE][NULLTEX_SIZE][4];
	int		x;

	memset (nulltex, 32, sizeof(nulltex));
	for (x = 0; x < NULLTEX_SIZE; x++)
	{
		nulltex[0][x][0]=
		nulltex[0][x][1]=
		nulltex[0][x][2]=
		nulltex[0][x][3]= 255;

		nulltex[x][0][0]=
		nulltex[x][0][1]=
		nulltex[x][0][2]=
		nulltex[x][0][3]= 255;

		nulltex[NULLTEX_SIZE-1][x][0]=
		nulltex[NULLTEX_SIZE-1][x][1]=
		nulltex[NULLTEX_SIZE-1][x][2]=
		nulltex[NULLTEX_SIZE-1][x][3]= 255;

		nulltex[x][NULLTEX_SIZE-1][0]=
		nulltex[x][NULLTEX_SIZE-1][1]=
		nulltex[x][NULLTEX_SIZE-1][2]=
		nulltex[x][NULLTEX_SIZE-1][3]= 255;
	}

	if (r_debug_media->integer)
		R_WriteTGA (&nulltex[0][0][0], NULLTEX_SIZE, NULLTEX_SIZE, 4, "debug_tex/null_texture.tga", false);

	return R_LoadPic ("*notexture", (byte *)nulltex, NULLTEX_SIZE, NULLTEX_SIZE, it_wall, 32);
}


/*
==================
R_CreateWhiteTexture
==================
*/
image_t *R_CreateWhiteTexture (void)
{
	byte	whitetex[NULLTEX_SIZE][NULLTEX_SIZE][4];
	
	memset (whitetex, 255, sizeof(whitetex));

	if (r_debug_media->integer)
		R_WriteTGA (&whitetex[0][0][0], NULLTEX_SIZE, NULLTEX_SIZE, 4, "debug_tex/white_texture.tga", false);

	return R_LoadPic ("*whitetexture", (byte *)whitetex, NULLTEX_SIZE, NULLTEX_SIZE, it_wall, 32);
}


/*
==================
R_CreateDistTextureARB
==================
*/
#define DIST_SIZE 16
image_t *R_CreateDistTextureARB (void)
{
	byte	dist[DIST_SIZE][DIST_SIZE][4];
	int		x, y;
	image_t	*image;

	srand(Sys_TickCount());
	for (x = 0; x < DIST_SIZE; x++)
		for (y = 0; y < DIST_SIZE; y++) {
			dist[x][y][0] = rand()%255;
			dist[x][y][1] = rand()%255;
			dist[x][y][2] = rand()%48;
			dist[x][y][3] = rand()%48;
		}
	image = R_LoadPic ("*disttexture", (byte *)dist, DIST_SIZE, DIST_SIZE, it_wall, 32);

	qglBindTexture(GL_TEXTURE_2D, image->texnum);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
	qglTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

	if (r_debug_media->integer)
		R_WriteTGA (&dist[0][0][0], DIST_SIZE, DIST_SIZE, 4, "debug_tex/dist_texture_arb.tga", false);

	return image;
}


/*
==================
R_CreateCelShadeTexture
==================
*/
#define CEL_SHADE_SIZE 32
const byte cel_tex_colors[CEL_SHADE_SIZE][2] = 
{
	// + 3 = 3
	0,		255,
	0,		255, 
	0,		255,

	// + 5 = 8
	0,		170,
	0,		170,
	0,		170,
	0,		170,
	0,		170,

	// + 8 = 16
	0,		85,
	0,		85,
	0,		85,
	0,		85,
	0,		85,
	0,		85,
	0,		85,
	0,		85,

	// + 8 = 24
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,

	// + 8 = 32
	255,	0,
	255,	0,
	255,	0,
	255,	0,
	255,	0,
	255,	0,
	255,	0,
	255,	0,
};

image_t *R_CreateCelShadeTexture (void)
{
	byte	cel_tex[CEL_SHADE_SIZE][CEL_SHADE_SIZE][4];
	int		x, y;
	image_t	*image;

	for (x = 0; x < CEL_SHADE_SIZE; x++)
		for (y = 0; y < CEL_SHADE_SIZE; y++) {
			cel_tex[x][y][0] = (byte)cel_tex_colors[y][0];
			cel_tex[x][y][1] = (byte)cel_tex_colors[y][0];
			cel_tex[x][y][2] = (byte)cel_tex_colors[y][0];
			cel_tex[x][y][3] = (byte)cel_tex_colors[y][1];
		}

	image = R_LoadPic ("*celshadetexture", (byte *)cel_tex, CEL_SHADE_SIZE, CEL_SHADE_SIZE, it_pic, 32);

	qglBindTexture(GL_TEXTURE_2D, image->texnum);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (r_debug_media->integer)
		R_WriteTGA (&cel_tex[0][0][0], CEL_SHADE_SIZE, CEL_SHADE_SIZE, 4, "debug_tex/celshade_texture.tga", false);

	return image;
}


#ifdef ROQ_SUPPORT
/*
==================
R_CreateRawTexture
==================
*/
#define RAW_TEX_SIZE 256
image_t *R_CreateRawTexture (void)
{
	byte	raw_tex[RAW_TEX_SIZE * RAW_TEX_SIZE * 4]; // Raw texture

	memset (raw_tex, 255, sizeof(raw_tex));
	return R_LoadPic ("*rawtexture", raw_tex, RAW_TEX_SIZE, RAW_TEX_SIZE, it_pic, 32);
}
#endif // ROQ_SUPPORT


/*
==================
R_LoadPartImg
==================
*/
image_t *R_LoadPartImg (char *name, imagetype_t type)
{
	image_t *image = R_FindImage(name, type);
	if (!image) image = glMedia.noTexture;
	return image;
}


/*
==================
R_SetParticleImg
==================
*/
void R_SetParticleImg (int num, char *name)
{
	glMedia.particleTextures[num] = R_LoadPartImg (name, it_part);
}


/*
==================
R_CreateDisplayLists
==================
*/
void R_CreateDisplayLists (void)
{
	int	i;

	for (i=0; i<NUM_DISPLAY_LISTS; i++) {
		if (i == 0)
			glMedia.displayLists[i] = qglGenLists(NUM_DISPLAY_LISTS);
		else	
			glMedia.displayLists[i] = glMedia.displayLists[i-1] + 1;
	}
	
	qglNewList(glMedia.displayLists[DL_NULLMODEL1], GL_COMPILE);
		qglBegin (GL_TRIANGLE_FAN);
		qglVertex3f (0, 0, -16);
		qglVertex3f (16*cos(0*M_PI*0.5f), 16*sin(0*M_PI*0.5f), 0);
		qglVertex3f (16*cos(1*M_PI*0.5f), 16*sin(1*M_PI*0.5f), 0);
		qglVertex3f (16*cos(2*M_PI*0.5f), 16*sin(2*M_PI*0.5f), 0);
		qglVertex3f (16*cos(3*M_PI*0.5f), 16*sin(3*M_PI*0.5f), 0);
		qglVertex3f (16*cos(4*M_PI*0.5f), 16*sin(4*M_PI*0.5f), 0);
		qglEnd ();

		qglBegin (GL_TRIANGLE_FAN);
		qglVertex3f (0, 0, 16);
		qglVertex3f (16*cos(4*M_PI*0.5f), 16*sin(4*M_PI*0.5f), 0);
		qglVertex3f (16*cos(3*M_PI*0.5f), 16*sin(3*M_PI*0.5f), 0);
		qglVertex3f (16*cos(2*M_PI*0.5f), 16*sin(2*M_PI*0.5f), 0);
		qglVertex3f (16*cos(1*M_PI*0.5f), 16*sin(1*M_PI*0.5f), 0);
		qglVertex3f (16*cos(0*M_PI*0.5f), 16*sin(0*M_PI*0.5f), 0);
		qglEnd ();
		qglColor4f (1,1,1,1);
	qglEndList();

	qglNewList(glMedia.displayLists[DL_NULLMODEL2], GL_COMPILE);
		qglLineWidth(6.0f);
		qglBegin(GL_LINES);

		qglColor4ub(255, 0, 0, 255);
		qglVertex3f(0, 0, 0);
		qglVertex3f(16, 0, 0);

		qglColor4ub(64, 0, 0, 255);
		qglVertex3f(0, 0, 0);
		qglVertex3f(-16, 0, 0);

		qglColor4ub(0, 255, 0, 255);
		qglVertex3f(0, 0, 0);
		qglVertex3f(0, 16, 0);

		qglColor4ub(0, 64, 0, 255);
		qglVertex3f(0, 0, 0);
		qglVertex3f(0, -16, 0);

		qglColor4ub(0, 0, 255, 255);
		qglVertex3f(0, 0, 0);
		qglVertex3f(0, 0, 16);

		qglColor4ub(0, 0, 64, 255);
		qglVertex3f(0, 0, 0);
		qglVertex3f(0, 0, -16);

		qglEnd();
		qglLineWidth(1.0f);
		qglColor4f (1,1,1,1);
	qglEndList();
}


/*
==================
R_ClearDisplayLists
==================
*/
void R_ClearDisplayLists (void)
{
	int		i;

	if (glMedia.displayLists[0] != 0)	// clear only if not null
		qglDeleteLists (glMedia.displayLists[0], NUM_DISPLAY_LISTS);

	for (i=0; i<NUM_DISPLAY_LISTS; i++)
		glMedia.displayLists[i] = 0;
}


/*
==================
R_InitMedia
==================
*/
void R_InitMedia (void)
{
	int		i;

	glMedia.noTexture = R_CreateNullTexture ();				// Generate null texture
	glMedia.whiteTexture = R_CreateWhiteTexture ();			// Generate white texture
	glMedia.distTextureARB = R_CreateDistTextureARB ();		// Generate warp distortion texture
	glMedia.celShadeTexture = R_CreateCelShadeTexture ();	// Generate cel shading texture

#ifdef ROQ_SUPPORT
	glMedia.rawTexture = R_CreateRawTexture ();				// Generate raw texture for cinematics
#endif // ROQ_SUPPORT
	
	glMedia.envMapTexture = R_LoadPartImg ("gfx/effects/envmap.tga", it_wall);
	glMedia.sphereMapTexture = R_LoadPartImg ("gfx/effects/spheremap.tga", it_skin);
	glMedia.shellTexture = R_LoadPartImg ("gfx/effects/shell_generic.tga", it_skin);
	glMedia.flareTexture = R_LoadPartImg ("gfx/effects/flare.tga", it_skin);
	glMedia.causticWaterTexture = R_LoadPartImg ("gfx/water/caustic_water.tga", it_wall);
	glMedia.causticSlimeTexture = R_LoadPartImg ("gfx/water/caustic_slime.tga", it_wall);
	glMedia.causticLavaTexture = R_LoadPartImg ("gfx/water/caustic_lava.tga", it_wall);
	glMedia.particleBeam = R_LoadPartImg ("gfx/particles/beam.tga", it_part);

	// Psychospaz's enhanced particles
	// These are loaded in CL_PrepRefresh
	for (i=0; i<PARTICLE_TYPES; i++)
		glMedia.particleTextures[i] = NULL;

	for (i=0; i<NUM_DISPLAY_LISTS; i++) 
		glMedia.displayLists[i] = 0;

	R_CreateDisplayLists ();
}


/*
==================
R_ShutdownMedia
==================
*/
void R_ShutdownMedia (void)
{
	int		i;

	glMedia.noTexture = NULL;
	glMedia.whiteTexture = NULL;
	glMedia.distTextureARB = NULL;
	glMedia.celShadeTexture = NULL;
	glMedia.rawTexture = NULL;
	glMedia.envMapTexture = NULL;
	glMedia.sphereMapTexture = NULL;
	glMedia.shellTexture = NULL;
	glMedia.flareTexture = NULL;
	glMedia.causticWaterTexture = NULL;
	glMedia.causticSlimeTexture = NULL;
	glMedia.causticLavaTexture = NULL;
	glMedia.particleBeam = NULL;

	for (i=0; i<PARTICLE_TYPES; i++)
		glMedia.particleTextures[i] = NULL;

	R_ClearDisplayLists ();
}


/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
================
R_ScreenShot_Read_Buffer
from Daikatana v1.3
================
*/
void R_ScreenShot_Read_Buffer (int grab_x, int grab_width, byte *buffer) /* FS */
{
	float g = 0.0f;

	if (!buffer)
		return;

	if (!r_screenshot_gamma_correct->integer)
	{
		qglReadPixels (grab_x, 0, grab_width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
		return;
	}

	g = 1.0f - (vid_gamma->value - 1.0f);
	qglPushAttrib(GL_PIXEL_MODE_BIT);
	qglPixelTransferf(GL_RED_SCALE, g);
	qglPixelTransferf(GL_GREEN_SCALE, g);
	qglPixelTransferf(GL_BLUE_SCALE, g);
	qglReadPixels (grab_x, 0, grab_width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	qglPopAttrib();
}


/*
================
R_ResampleShotLerpLine
from DarkPlaces
================
*/
void R_ResampleShotLerpLine (byte *in, byte *out, int inwidth, int outwidth) 
{ 
	int j, xi, oldx = 0, f, fstep, l1, l2, endx;

	fstep = (int) (inwidth*65536.0f/outwidth); 
	endx = (inwidth-1); 
	for (j = 0,f = 0; j < outwidth; j++, f += fstep) 
	{ 
		xi = (int) f >> 16; 
		if (xi != oldx) 
		{ 
			in += (xi - oldx) * 3; 
			oldx = xi; 
		} 
		if (xi < endx) 
		{ 
			l2 = f & 0xFFFF; 
			l1 = 0x10000 - l2; 
			*out++ = (byte) ((in[0] * l1 + in[3] * l2) >> 16);
			*out++ = (byte) ((in[1] * l1 + in[4] * l2) >> 16); 
			*out++ = (byte) ((in[2] * l1 + in[5] * l2) >> 16); 
		} 
		else // last pixel of the line has no pixel to lerp to 
		{ 
			*out++ = in[0]; 
			*out++ = in[1]; 
			*out++ = in[2]; 
		} 
	} 
}


/*
================
R_ResampleShot
================
*/
void R_ResampleShot (void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight) 
{ 
	int i, j, yi, oldy, f, fstep, l1, l2, endy = (inheight-1);
	
	byte *inrow, *out, *row1, *row2; 
	out = outdata; 
	fstep = (int) (inheight*65536.0f/outheight); 

	row1 = malloc(outwidth*3); 
	row2 = malloc(outwidth*3); 
	inrow = indata; 
	oldy = 0; 
	R_ResampleShotLerpLine (inrow, row1, inwidth, outwidth); 
	R_ResampleShotLerpLine (inrow + inwidth*3, row2, inwidth, outwidth); 
	for (i = 0, f = 0; i < outheight; i++,f += fstep) 
	{ 
		yi = f >> 16; 
		if (yi != oldy) 
		{ 
			inrow = (byte *)indata + inwidth*3*yi; 
			if (yi == oldy+1) 
				memcpy(row1, row2, outwidth*3); 
			else 
				R_ResampleShotLerpLine (inrow, row1, inwidth, outwidth);

			if (yi < endy) 
				R_ResampleShotLerpLine (inrow + inwidth*3, row2, inwidth, outwidth); 
			else 
				memcpy(row2, row1, outwidth*3); 
			oldy = yi; 
		} 
		if (yi < endy) 
		{ 
			l2 = f & 0xFFFF; 
			l1 = 0x10000 - l2; 
			for (j = 0;j < outwidth;j++) 
			{ 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
			} 
			row1 -= outwidth*3; 
			row2 -= outwidth*3; 
		} 
		else // last line has no pixels to lerp to 
		{ 
			for (j = 0;j < outwidth;j++) 
			{ 
				*out++ = *row1++; 
				*out++ = *row1++; 
				*out++ = *row1++; 
			} 
			row1 -= outwidth*3; 
		} 
	} 
	free(row1); 
	free(row2); 
} 


/* 
================== 
R_ScaledScreenshot
by Knightmare
================== 
*/
saveShot_t	r_saveShot;

void R_ScaledScreenshot (char *name)
{
	struct jpeg_compress_struct		cinfo;
	struct jpeg_error_mgr			jerr;
	JSAMPROW						s[1];
	FILE							*file;
	char							shotname[MAX_OSPATH];
	int								saveshotWidth, saveshotHeight, offset;
	byte							*jpgdata;

	if (!r_saveShot.buffer)
		return;

	saveshotWidth = saveshotHeight = 256;
	if (r_saveShot.width >= 2048)
		saveshotWidth = 2048;
	else if (r_saveShot.width >= 1024)
		saveshotWidth = 1024;
	else if (r_saveShot.width >= 512)
		saveshotWidth = 512;

	if (r_saveShot.height >= 2048)
		saveshotHeight = 2048;
	else if (r_saveShot.height >= 1024)
		saveshotHeight = 1024;
	else if (r_saveShot.height >= 512)
		saveshotHeight = 512;

	// Allocate room for reduced screenshot
	jpgdata = malloc(saveshotWidth * saveshotHeight * 3);
	if (!jpgdata)
		return;

	// Resize grabbed screen
	R_ResampleShot (r_saveShot.buffer, r_saveShot.width, r_saveShot.height, jpgdata, saveshotWidth, saveshotHeight);

	// Open the file for Binary Output
	Com_sprintf (shotname, sizeof(shotname), "%s", name);
	file = fopen(shotname, "wb");
	if (!file)
	{
		VID_Printf (PRINT_ALL, "Menu_ScreenShot: Couldn't create %s\n", name); 
		return;
 	}

	// Initialise the JPEG compression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest (&cinfo, file);

	// Setup JPEG Parameters
	cinfo.image_width = saveshotWidth; //256;
	cinfo.image_height = saveshotHeight; //256;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, 85, TRUE); // was 100

	// Start Compression
	jpeg_start_compress (&cinfo, true);

	// Feed Scanline data
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while (cinfo.next_scanline < cinfo.image_height)
	{
		s[0] = &jpgdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines (&cinfo, s, 1);
	}

	// Finish Compression
	jpeg_finish_compress (&cinfo);

	// Destroy JPEG object
	jpeg_destroy_compress (&cinfo);

	// Close File
	fclose (file);

	// Free Reduced screenshot
	free (jpgdata);
}


/* 
================== 
R_GrabScreen
by Knightmare
================== 
*/
void R_GrabScreen (void)
{	
	int		grab_width, grab_x;
	float	screenAspect;

	// Free saveshot buffer first
	if (r_saveShot.buffer) {
		Z_Free (r_saveShot.buffer);
		r_saveShot.buffer = NULL;
	}

	// Round down width to nearest multiple of 4
	// Properly handle surround modes
	screenAspect = (float)vid.width / (float)vid.height;
	if ( (Cvar_VariableInteger("scr_surroundlayout") != 0) && (screenAspect >= Cvar_VariableValue("scr_surroundthreshold")) ) {
		grab_width = (float)vid.width * (Cvar_VariableValue("scr_surroundright") - Cvar_VariableValue("scr_surroundleft"));
		grab_width &= ~3;
	}
	else {
		grab_width = vid.width & ~3;
	}
	grab_x = (vid.width - grab_width) / 2;

	// Allocate room for a copy of the framebuffer
	r_saveShot.buffer = Z_Malloc(grab_width * vid.height * 3);
	if (!r_saveShot.buffer)	return;

	// Read the framebuffer into our storage and store dimensions
	qglReadPixels (grab_x, 0, grab_width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, r_saveShot.buffer);
	r_saveShot.width = grab_width;
	r_saveShot.height = vid.height;
}


/* 
================== 
R_ScreenShot_JPG
By Robert 'Heffo' Heffernan
================== 
*/
void R_ScreenShot_JPG (qboolean silent)
{
	byte							*rgbdata;
	FILE							*file;
	char							picname[80], mapname[MAX_QPATH], checkname[MAX_OSPATH];
	int								i, offset, grab_width, grab_x;
	struct jpeg_compress_struct		cinfo;
	struct jpeg_error_mgr			jerr;
	JSAMPROW						s[1];

	// Create the screenshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/screenshots", FS_Savegamedir());	// was FS_Gamedir()
	Sys_Mkdir (checkname);

	// Copy mapname to buffer
	Q_strncpyz (mapname, sizeof(mapname), Cvar_VariableString("mapname"));

	// Find a file name to save it to 
	// Knightmare- changed screenshot filenames, up to 10000 screenies
	for (i=0; i<=9999; i++) 
	{ 
		int one, ten, hundred, thousand;

		thousand = i*0.001;
		hundred = (i - thousand*1000)*0.01;
		ten = (i - thousand*1000 - hundred*100)*0.1;
		one = i - thousand*1000 - hundred*100 - ten*10;

		// Include mapname in filename if enabled
		if ( r_screenshot_use_mapname->integer && (r_worldmodel != NULL) && (mapname[0] != 0) )
			Com_sprintf (picname, sizeof(picname), "%s_%i%i%i%i.jpg", mapname, thousand, hundred, ten, one);
		else
			Com_sprintf (picname, sizeof(picname), "kmquake2_%i%i%i%i.jpg", thousand, hundred, ten, one);
		Com_sprintf (checkname, sizeof(checkname), "%s/screenshots/%s", FS_Savegamedir(), picname);	// was FS_Gamedir()
		file = fopen (checkname, "rb");
		if (!file)
			break;	// file doesn't exist
		fclose (file);
	} 
	if (i == 10000) 
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_JPG: Screenshots directory is full!\n"); 
		return;
 	}

	// Open the file for Binary Output
	file = fopen(checkname, "wb");
	if (!file)
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_JPG: Couldn't create a file\n"); 
		return;
 	}

	// Round down width to nearest multiple of 4
	grab_width = vid.width & ~3;
	grab_x = (vid.width - grab_width) / 2;

	// Allocate room for a copy of the framebuffer
	rgbdata = malloc(grab_width * vid.height * 3);
	if (!rgbdata)
	{
		fclose(file);
		return;
	}

	// Read the framebuffer into our storage
//	qglReadPixels(grab_x, 0, grab_width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);
	R_ScreenShot_Read_Buffer (grab_x, grab_width, rgbdata);

	// Initialise the JPEG compression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest (&cinfo, file);

	// Setup JPEG Parameters
	cinfo.image_width = grab_width;
	cinfo.image_height = vid.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults (&cinfo);
	if ((r_screenshot_jpeg_quality->integer >= 101) || (r_screenshot_jpeg_quality->integer <= 0))
		Cvar_Set ("r_screenshot_jpeg_quality", "85");
	jpeg_set_quality (&cinfo, r_screenshot_jpeg_quality->integer, TRUE);

	// Start Compression
	jpeg_start_compress (&cinfo, true);

	// Feed Scanline data
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while (cinfo.next_scanline < cinfo.image_height)
	{
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines (&cinfo, s, 1);
	}

	// Finish Compression
	jpeg_finish_compress (&cinfo);

	// Destroy JPEG object
	jpeg_destroy_compress (&cinfo);

	// Close File
	fclose (file);

	// Free Temp Framebuffer
	free (rgbdata);

	// Done!
	if (!silent)
		VID_Printf (PRINT_ALL, "Wrote %s\n", picname);
}


#ifdef PNG_SUPPORT

// fix for old libpng on MSVC6
//#if defined (PNG_LIBPNG_VER) && (PNG_LIBPNG_VER < 10209)
/*#ifndef png_jmpbuf
#define png_jmpbuf(a)	((a)->jmpbuf)
#endif */

/* 
================== 
R_ScreenShot_PNG
================== 
*/
void R_ScreenShot_PNG (qboolean silent)
{
	char		picname[80], mapname[MAX_QPATH], checkname[MAX_OSPATH];
	int			i, grab_width, grab_x;
	byte		*rgbdata;
	FILE		*file;
	png_structp	png_sptr;
	png_infop	png_infoptr;
	void		*lineptr;

	// create the screenshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/screenshots", FS_Savegamedir());	// was FS_Gamedir()
	Sys_Mkdir (checkname);

	// Copy mapname to buffer
	Q_strncpyz (mapname, sizeof(mapname), Cvar_VariableString("mapname"));

	// Find a file name to save it to 
	// Knightmare- changed screenshot filenames, up to 10000 screenies
	for (i=0; i<=9999; i++) 
	{ 
		int one, ten, hundred, thousand;

		thousand = i*0.001;
		hundred = (i - thousand*1000)*0.01;
		ten = (i - thousand*1000 - hundred*100)*0.1;
		one = i - thousand*1000 - hundred*100 - ten*10;

		// Include mapname in filename if enabled
		if ( r_screenshot_use_mapname->integer && (r_worldmodel != NULL) && (mapname[0] != 0) )
			Com_sprintf (picname, sizeof(picname), "%s_%i%i%i%i.png", mapname, thousand, hundred, ten, one);
		else
			Com_sprintf (picname, sizeof(picname), "kmquake2_%i%i%i%i.png", thousand, hundred, ten, one);
		Com_sprintf (checkname, sizeof(checkname), "%s/screenshots/%s", FS_Savegamedir(), picname);	// was FS_Gamedir()
		file = fopen (checkname, "rb");
		if (!file)
			break;	// file doesn't exist
		fclose (file);
	} 
	if (i == 10000) 
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_PNG: Screenshots directory is full!\n"); 
		return;
 	}

	// Round down width to nearest multiple of 4
	grab_width = vid.width & ~3;
	grab_x = (vid.width - grab_width) / 2;

	// Allocate room for a copy of the framebuffer
	rgbdata = malloc(grab_width * vid.height * 3);
	if (!rgbdata)
	{
		return;
	}

	// Read the framebuffer into our storage
//	qglReadPixels(grab_x, 0, grab_width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);
	R_ScreenShot_Read_Buffer (grab_x, grab_width, rgbdata);

	png_sptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_sptr)
	{
		free(rgbdata);
		VID_Printf (PRINT_ALL, "R_ScreenShot_PNG: Couldn't create PNG struct\n"); 
		return;
	}

	png_infoptr = png_create_info_struct(png_sptr);
	if (!png_infoptr)
	{
		png_destroy_write_struct(&png_sptr, 0);
		free(rgbdata);
		VID_Printf (PRINT_ALL, "R_ScreenShot_PNG: Couldn't create info struct\n"); 
		return;
	}

//	if ( setjmp(png_sptr->jmpbuf) )
	if ( setjmp(png_jmpbuf(png_sptr)) )
	{
		png_destroy_info_struct (png_sptr, &png_infoptr);
		png_destroy_write_struct (&png_sptr, 0);
		free (rgbdata);
		VID_Printf (PRINT_ALL, "R_ScreenShot_PNG: bad data\n"); 
		return;
	}

	// open png file
	file = fopen(checkname, "wb");
	if (!file)
	{
		png_destroy_info_struct (png_sptr, &png_infoptr);
		png_destroy_write_struct (&png_sptr, 0);
		free (rgbdata);
		VID_Printf (PRINT_ALL, "R_ScreenShot_PNG: Couldn't create a file\n"); 
		return;
 	}

	// encode and output
	png_init_io (png_sptr, file);
	png_set_IHDR(png_sptr, png_infoptr, grab_width, vid.height, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info (png_sptr, png_infoptr);
	for (i=vid.height-1; i>=0; i--)
	{
		lineptr = rgbdata + i*grab_width*3;
		png_write_row (png_sptr, lineptr);
	}
	png_write_end (png_sptr, png_infoptr);

	// clean up
	fclose (file);
	png_destroy_info_struct (png_sptr, &png_infoptr);
	png_destroy_write_struct (&png_sptr, 0);
	free (rgbdata);

	if (!silent)
		VID_Printf (PRINT_ALL, "Wrote %s\n", picname);
}
#endif	// PNG_SUPPORT


/* 
================== 
R_ScreenShot_TGA
================== 
*/  
void R_ScreenShot_TGA (qboolean silent) 
{
	byte		*buffer;
	char		picname[80], mapname[MAX_QPATH], checkname[MAX_OSPATH];
	int			i, c, temp, grab_width, grab_x;
	FILE		*f;

	// create the screenshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/screenshots", FS_Savegamedir());	// was FS_Gamedir()
	Sys_Mkdir (checkname);

	// Copy mapname to buffer
	Q_strncpyz (mapname, sizeof(mapname), Cvar_VariableString("mapname"));

	// Find a file name to save it to 
	// Knightmare- changed screenshot filenames, up to 10000 screenies
	for (i=0; i<=9999; i++) 
	{ 
		int one, ten, hundred, thousand;

		thousand = i*0.001;
		hundred = (i - thousand*1000)*0.01;
		ten = (i - thousand*1000 - hundred*100)*0.1;
		one = i - thousand*1000 - hundred*100 - ten*10;

		// Include mapname in filename if enabled
		if ( r_screenshot_use_mapname->integer && (r_worldmodel != NULL) && (mapname[0] != 0) )
			Com_sprintf (picname, sizeof(picname), "%s_%i%i%i%i.tga", mapname, thousand, hundred, ten, one);
		else
			Com_sprintf (picname, sizeof(picname), "kmquake2_%i%i%i%i.tga", thousand, hundred, ten, one);
		Com_sprintf (checkname, sizeof(checkname), "%s/screenshots/%s", FS_Savegamedir(), picname);	// was FS_Gamedir()
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i == 10000) 
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_TGA: Screenshots directory is full!\n"); 
		return;
 	}

	// Round down width to nearest multiple of 4
	grab_width = vid.width & ~3;
	grab_x = (vid.width - grab_width) / 2;

	buffer = malloc(grab_width*vid.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = grab_width&255;
	buffer[13] = grab_width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

//	qglReadPixels (grab_x, 0, grab_width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 
	R_ScreenShot_Read_Buffer(grab_x, grab_width, buffer+18);

	// swap rgb to bgr
	c = 18+grab_width*vid.height*3;
	for (i = 18; i < c; i += 3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	if (!f) {
		free (buffer);
		VID_Printf (PRINT_ALL, "R_ScreenShot_TGA: Couldn't create a file\n"); 
		return;
 	}
	fwrite (buffer, 1, c, f);
	fclose (f);

	free (buffer);

	if (!silent)
		VID_Printf (PRINT_ALL, "Wrote %s\n", picname);
} 


/* 
================== 
R_ScreenShot_f
================== 
*/  
void R_ScreenShot_f (void) 
{
	if ( !Q_strcasecmp(r_screenshot_format->string, "jpg") )
		R_ScreenShot_JPG (false);
#ifdef PNG_SUPPORT
	else if ( !Q_strcasecmp(r_screenshot_format->string, "png") )
		R_ScreenShot_PNG (false);
#endif	// PNG_SUPPORT
	else
		R_ScreenShot_TGA (false);
}


/* 
================== 
R_ScreenShot_Silent_f
================== 
*/  
void R_ScreenShot_Silent_f (void) 
{
	if ( !Q_strcasecmp(r_screenshot_format->string, "jpg") )
		R_ScreenShot_JPG (true);
#ifdef PNG_SUPPORT
	else if ( !Q_strcasecmp(r_screenshot_format->string, "png") )
		R_ScreenShot_PNG (true);
#endif	// PNG_SUPPORT
	else
		R_ScreenShot_TGA (true);
}


/* 
================== 
R_ScreenShot_TGA_f
================== 
*/  
void R_ScreenShot_TGA_f (void) 
{
	R_ScreenShot_TGA (false);
}


/* 
================== 
R_ScreenShot_TGA_f
================== 
*/  
void R_ScreenShot_JPG_f (void) 
{
	R_ScreenShot_JPG (false);
}


/* 
================== 
R_ScreenShot_PNG_f
================== 
*/  
void R_ScreenShot_PNG_f (void) 
{
	R_ScreenShot_PNG (false);
}

//============================================================================== 


/*
=================
GL_UpdateSwapInterval
=================
*/
void GL_UpdateSwapInterval (void)
{
	static qboolean registering;

	// don't swap interval if loading a map
	if (registering != registration_active)
		r_swapinterval->modified = true;

	if ( r_swapinterval->modified )
	{
		r_swapinterval->modified = false;

		registering = registration_active;

		if ( !glState.stereo_enabled ) 
		{
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( (registration_active) ? 0 : r_swapinterval->integer );
#endif
		}
	}
}
