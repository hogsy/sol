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

// r_image.c

#include "r_local.h"

#ifdef _WIN32

#include "../libraries/jpeg/jpeglib.h"
#ifdef PNG_SUPPORT
#include "../libraries/png/png.h"
#endif	// PNG_SUPPORT

#else // _WIN32

#include <jpeglib.h>
#ifdef PNG_SUPPORT
#include <png.h>
#endif	// PNG_SUPPORT

#endif // _WIN32

#ifdef _WIN32 // Let's not use GLU on Linux/macOS -flibit
#define USE_GLU_MIPMAP
#endif	// _WIN32

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures;
int			base_textureid;		// gltextures[i] = base_textureid+i

static byte			 intensitytable[256];
static unsigned char gammatable[256];

cvar_t		*r_intensity;

unsigned int	d_8to24table[256];			// palette loaded from colormap.pcx
unsigned int	d_quakePalette[256];		// palette for MDL models
const byte		default_quakePal[768] = {	// default internal palette for MDL models
#include "../qcommon/q1palette.h"
};

qboolean GL_Upload8 (byte *data, int width, int height, imagetype_t type, qboolean useQuakePalette);
qboolean GL_Upload32 (unsigned int *data, int width, int height, imagetype_t type);

int		gl_filter_min;
int		gl_filter_max;

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t gl_modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(gl_modes) / sizeof (glmode_t))


/*
===============
GL_UpdateAnisoMode
===============
*/
void GL_UpdateAnisoMode (void)
{
	int		i;
	image_t	*glt;

	// clamp selected anisotropy
	if (glConfig.anisotropic)
	{
		if (r_anisotropic->value > glConfig.max_anisotropy)
			Cvar_SetValue("r_anisotropic", glConfig.max_anisotropy);
		else if (r_anisotropic->value < 1.0)
			Cvar_SetValue("r_anisotropic", 1.0);
	}

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures; i<numgltextures; i++, glt++)
	{
		if ( (glt->type != it_pic) && (glt->type != it_font) && (glt->type != it_scrap) && (glt->type != it_sky) )
		{
			GL_Bind (glt->texnum);
			// Set anisotropic filter if supported and enabled
			if (glConfig.anisotropic && r_anisotropic->value) {
				qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_anisotropic->value);
			}
		}
	}
}


/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode (char *string)
{
	int		i;
	image_t	*glt;

	for (i=0; i< NUM_GL_MODES; i++)
	{
		if ( !Q_stricmp( gl_modes[ i ].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		VID_Printf ( PRINT_ALL, "bad filter name\n" );
		return;
	}

	gl_filter_min = gl_modes[i].minimize;
	gl_filter_max = gl_modes[i].maximize;

	// clamp selected anisotropy
	if (glConfig.anisotropic)
	{
		if (r_anisotropic->value > glConfig.max_anisotropy)
			Cvar_SetValue("r_anisotropic", glConfig.max_anisotropy);
		else if (r_anisotropic->value < 1.0)
			Cvar_SetValue("r_anisotropic", 1.0);
	}

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures; i<numgltextures; i++, glt++)
	{
		if ( (glt->type != it_pic) && (glt->type != it_font) && (glt->type != it_scrap) && (glt->type != it_sky) )
		{
			GL_Bind (glt->texnum);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			// Set anisotropic filter if supported and enabled
			if (glConfig.anisotropic && r_anisotropic->value) {
				qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_anisotropic->value);
			}
		}
	}
}

#if 0	// removed
/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode (char *string)
{
	int		i;

	for (i=0 ; i< NUM_GL_ALPHA_MODES ; i++)
	{
		if ( !Q_stricmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		VID_Printf (PRINT_ALL, "bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode (char *string)
{
	int		i;

	for (i=0 ; i< NUM_GL_SOLID_MODES ; i++)
	{
		if ( !Q_stricmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		VID_Printf (PRINT_ALL, "bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}
#endif

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;
	const char *palstrings[2] =
	{
		"RGB",
		"PAL"
	};

	VID_Printf ( PRINT_ALL, "------------------\n" );
	texels = 0;

	for (i=0, image=gltextures; i<numgltextures; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_skin:
			VID_Printf ( PRINT_ALL, "M" );
			break;
		case it_sprite:
			VID_Printf ( PRINT_ALL, "S" );
			break;
		case it_wall:
			VID_Printf ( PRINT_ALL, "W" );
			break;
		case it_pic:
			VID_Printf ( PRINT_ALL, "P" );
			break;
		case it_font:
			VID_Printf ( PRINT_ALL, "F" );
			break;
		case it_scrap:
			VID_Printf ( PRINT_ALL, "C" );
			break;
		case it_part:
			VID_Printf ( PRINT_ALL, "R" );
			break;
		default:
			VID_Printf ( PRINT_ALL, " " );
			break;
		}

		VID_Printf ( PRINT_ALL, " %3i %3i %s: %s\n",
		            image->upload_width, image->upload_height, palstrings[ image->paletted ], image->name );
	}
	VID_Printf ( PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels );
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

//#define	MAX_SCRAPS		1	// moved to r_local.h
#define	SCRAP_BLOCK_WIDTH	256
#define	SCRAP_BLOCK_HEIGHT	256

int				scrap_allocated[MAX_SCRAPS][SCRAP_BLOCK_WIDTH];
byte			scrap_texels[MAX_SCRAPS][SCRAP_BLOCK_WIDTH*SCRAP_BLOCK_HEIGHT];
//qboolean		scrap_dirty;
qboolean		scrap_dirty[MAX_SCRAPS];
qboolean		scrap_check_dirty;

// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0; texnum<MAX_SCRAPS; texnum++)
	{
		best = SCRAP_BLOCK_HEIGHT;

		for (i=0; i<SCRAP_BLOCK_WIDTH-w; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > SCRAP_BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
//	Sys_Error ("Scrap_AllocBlock: full");
}

int	scrap_uploads;

void Scrap_Upload (void)
{
/*	scrap_uploads++;
	GL_Bind (TEXNUM_SCRAPS);
	GL_Upload8 (scrap_texels[0], SCRAP_BLOCK_WIDTH, SCRAP_BLOCK_HEIGHT, it_scrap, false);	// was it_pic
	scrap_dirty = false;
*/
	int		i;

	// Use array of scraps
	for (i = 0; i < MAX_SCRAPS; i++)
	{
		if (scrap_dirty[i]) {
			scrap_uploads++;
			GL_Bind (TEXNUM_SCRAPS + i);
			GL_Upload8 (scrap_texels[i], SCRAP_BLOCK_WIDTH, SCRAP_BLOCK_HEIGHT, it_scrap, false);	// was it_pic
			scrap_dirty[i] = false;
		}
	}
	scrap_check_dirty = false;
}

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
void LoadPCX (const char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic = nullptr;
	*palette = nullptr;

	//
	// load the file
	//
	len = FS_LoadFile (filename, (void **)&raw);
	if (!raw)
	{	// Knightmare- skip this unless developer >= 2 because it spams the console
		if (developer->integer > 1)
			VID_Printf ( PRINT_DEVELOPER, "Bad pcx file %s\n", filename );
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

    pcx->xmin = LittleShort(pcx->xmin);
    pcx->ymin = LittleShort(pcx->ymin);
    pcx->xmax = LittleShort(pcx->xmax);
    pcx->ymax = LittleShort(pcx->ymax);
    pcx->hres = LittleShort(pcx->hres);
    pcx->vres = LittleShort(pcx->vres);
    pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
    pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
	// Knightmare- increased max supported size to 4096x4096
		|| pcx->xmax >= 4096	// was 640
		|| pcx->ymax >= 4096)	// was 480
	{
		VID_Printf ( PRINT_ALL, "Bad pcx file %s\n", filename );
		return;
	}

	out = malloc ( (pcx->ymax+1) * (pcx->xmax+1) );

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = malloc(768);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len)
	{
		VID_Printf ( PRINT_DEVELOPER, "PCX file %s was malformed", filename );
		free (*pic);
		*pic = nullptr;
	}

	FS_FreeFile (pcx);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

// Definitions for image types
#define TGA_Null		0   // no image data
#define TGA_Map			1   // Uncompressed, color-mapped images
#define TGA_RGB			2   // Uncompressed, RGB images
#define TGA_Mono		3   // Uncompressed, black and white images
#define TGA_RLEMap		9   // Runlength encoded color-mapped images
#define TGA_RLERGB		10   // Runlength encoded RGB images
#define TGA_RLEMono		11   // Compressed, black and white images
#define TGA_CompMap		32   // Compressed color-mapped data, using Huffman, Delta, and runlength encoding
#define TGA_CompMap4	33   // Compressed color-mapped data, using Huffman, Delta, and runlength encoding. 4-pass quadtree-type process
// Definitions for interleave flag
#define TGA_IL_None		0   // non-interleaved
#define TGA_IL_Two		1   // two-way (even/odd) interleaving
#define TGA_IL_Four		2   // four way interleaving
#define TGA_IL_Reserved	3   // reserved
// Definitions for origin flag
#define TGA_O_UPPER		0   // Origin in lower left-hand corner
#define TGA_O_LOWER		1   // Origin in upper left-hand corner
#define MAXCOLORS		16384

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;

#if 1

/*
=============
R_LoadTGA
NiceAss: LoadTGA() from Q2Ice, it supports more formats
=============
*/
void R_LoadTGA (const char *filename, byte **pic, int *width, int *height)
{
	int				w, h, x, y, i, temp1, temp2;
	int				realrow, truerow, baserow, interleave, origin;
	int				pixel_size, map_idx, RLE_count, RLE_flag;
	size_t			picSize;
	qboolean		isColorMapped, RL_encoded;
	TargaHeader		header;
	byte			tmp[2], r, g, b, a, j, k, l;
	byte			*data, *pdata, *dst, *ColorMap;

	if ( !filename || (filename[0] == '\0') )
		return;

	// load file
	FS_LoadFile (filename, (void*)&data);
	if (!data)
	{	// Knightmare- skip this unless developer >= 2 because it spams the console
		if (developer->integer > 1)
			VID_Printf ( PRINT_DEVELOPER, "R_LoadTGA (%s): Bad tga file\n", filename );
		return;
	}

	pdata = data;

	header.id_length = *pdata++;
	header.colormap_type = *pdata++;
	header.image_type = *pdata++;

	tmp[0] = pdata[0];
	tmp[1] = pdata[1];
	header.colormap_index = LittleShort( *((short *)tmp) );
	pdata+=2;
	tmp[0] = pdata[0];
	tmp[1] = pdata[1];
	header.colormap_length = LittleShort( *((short *)tmp) );
	pdata+=2;
	header.colormap_size = *pdata++;
	header.x_origin = LittleShort( *((short *)pdata) );
	pdata+=2;
	header.y_origin = LittleShort( *((short *)pdata) );
	pdata+=2;
	header.width = LittleShort( *((short *)pdata) );
	pdata+=2;
	header.height = LittleShort( *((short *)pdata) );
	pdata+=2;
	header.pixel_size = *pdata++;
	header.attributes = *pdata++;

	if ( header.id_length )
		pdata += header.id_length;

	// validate TGA type
	switch ( header.image_type )
	{
	case TGA_Map:
	case TGA_RGB:
	case TGA_Mono:
	case TGA_RLEMap:
	case TGA_RLERGB:
	case TGA_RLEMono:
		break;
	default:
		FS_FreeFile (data);
	//	VID_Error ( ERR_DROP, "R_LoadTGA: Only type 1 (map), 2 (RGB), 3 (mono), 9 (RLEmap), 10 (RLERGB), 11 (RLEmono) TGA images supported\n" );
		VID_Printf ( PRINT_ALL, "R_LoadTGA (%s): Only type 1 (map), 2 (RGB), 3 (mono), 9 (RLEmap), 10 (RLERGB), 11 (RLEmono) TGA images supported\n", filename );
		return;
	}

	// validate color depth
	switch ( header.pixel_size )
	{
	case 8:
	case 15:
	case 16:
	case 24:
	case 32:
		break;
	default:
		FS_FreeFile (data);
	//	VID_Error ( ERR_DROP, "R_LoadTGA: Only 8, 15, 16, 24 and 32 bit images (with colormaps) supported\n" );
		VID_Printf ( PRINT_ALL, "R_LoadTGA (%s): Only 8, 15, 16, 24 and 32 bit images (with colormaps) supported\n", filename );
		return;
	}

   r = g = b = a = l = 0;

   // if required, read the color map information
   ColorMap = nullptr;
   isColorMapped = ( (header.image_type == TGA_Map) || (header.image_type == TGA_RLEMap) /*|| (header.image_type == TGA_CompMap) || (header.image_type == TGA_CompMap4)*/ )
					&& (header.colormap_type == 1);
   if ( isColorMapped )
   {
		// validate colormap size
		switch( header.colormap_size )
		{
		case 8:
		case 16:
		case 24:
		case 32:
			break;
		default:
			FS_FreeFile (data);
		//	VID_Error ( ERR_DROP, "R_LoadTGA: Only 8, 16, 24 and 32 bit colormaps supported\n" );
			VID_Printf ( PRINT_ALL, "R_LoadTGA (%s): Only 8, 16, 24 and 32 bit colormaps supported\n", filename );
			return;
		}

		temp1 = header.colormap_index;
		temp2 = header.colormap_length;
		if ( (temp1 + temp2 + 1) >= MAXCOLORS ) {
			FS_FreeFile (data);
			VID_Printf ( PRINT_ALL, "R_LoadTGA (%s): Color map exceeds %i colors\n", filename, MAXCOLORS );
			return;
		}
		ColorMap = (byte *)malloc(MAXCOLORS * 4);
		map_idx = 0;
		for ( i = temp1; i < temp1 + temp2; ++i, map_idx += 4 )
		{
			// read appropriate number of bytes, break into rgb & put in map
			switch ( header.colormap_size )
			{
			case 8:
				r = g = b = *pdata++;
				a = 255;
				break;
				case 15:
				j = *pdata++;
				k = *pdata++;
				l = ((unsigned int) k << 8) + j;
				r = (byte) ( ((k & 0x7C) >> 2) << 3 );
				g = (byte) ( (((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3 );
				b = (byte) ( (j & 0x1F) << 3 );
				a = 255;
				break;
			case 16:
				j = *pdata++;
				k = *pdata++;
				l = ((unsigned int) k << 8) + j;
				r = (byte) ( ((k & 0x7C) >> 2) << 3 );
				g = (byte) ( (((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3 );
				b = (byte) ( (j & 0x1F) << 3 );
				a = (k & 0x80) ? 255 : 0;
				break;
			case 24:
				b = *pdata++;
				g = *pdata++;
				r = *pdata++;
				a = 255;
				l = 0;
				break;
			case 32:
				b = *pdata++;
				g = *pdata++;
				r = *pdata++;
				a = *pdata++;
				l = 0;
				break;
			}
			ColorMap[map_idx + 0] = r;
			ColorMap[map_idx + 1] = g;
			ColorMap[map_idx + 2] = b;
			ColorMap[map_idx + 3] = a;
		}
	}

   // check run-length encoding
   RL_encoded = (header.image_type == TGA_RLEMap) || (header.image_type == TGA_RLERGB) || (header.image_type == TGA_RLEMono);
   RLE_count = 0;
   RLE_flag = 0;

   w = header.width;
   h = header.height;

   if ( width )
	   *width = w;
   if ( height )
	   *height = h;

   picSize = w * h * 4;
   *pic = (byte *)malloc(picSize);
   memset (*pic, 0, picSize);

   // read the Targa file body and convert to portable format
   pixel_size = header.pixel_size;
   origin = (header.attributes & 0x20) >> 5;
   interleave = (header.attributes & 0xC0) >> 6;
   truerow = 0;
   baserow = 0;
   for (y = 0; y < h; y++)
   {
	   realrow = truerow;
	   if (origin == TGA_O_UPPER)
		   realrow = h - realrow - 1;

	   dst = *pic + realrow * w * 4;

	   for ( x = 0; x < w; x++ )
	   {
		   // check if run length encoded
		   if ( RL_encoded )
		   {
			   if ( !RLE_count )
			   {
				   // have to restart run
				   i = *pdata++;
				   RLE_flag = (i & 0x80);
				   if ( !RLE_flag ) {
					   // stream of unencoded pixels
					   RLE_count = i + 1;
				   }
				   else {
					   // single pixel replicated
					   RLE_count = i - 127;
				   }
				   // decrement count & get pixel
				   --RLE_count;
			   }
			   else {
				   // have already read count & (at least) first pixel
				   --RLE_count;
				   if( RLE_flag )
					   // replicated pixels
					   goto PixEncode;
			   }
		   }

		 // read appropriate number of bytes, break into RGB
		 switch( pixel_size ) {
			case 8:
				r = g = b = l = *pdata++;
				a = 255;
				break;
			case 15:
				j = *pdata++;
				k = *pdata++;
				l = ((unsigned int) k << 8) + j;
				r = (byte) ( ((k & 0x7C) >> 2) << 3 );
				g = (byte) ( (((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3 );
				b = (byte) ( (j & 0x1F) << 3 );
				a = 255;
				break;
			case 16:
				j = *pdata++;
				k = *pdata++;
				l = ((unsigned int) k << 8) + j;
				r = (byte) ( ((k & 0x7C) >> 2) << 3 );
				g = (byte) ( (((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3 );
				b = (byte) ( (j & 0x1F) << 3 );
				a = 255;
				break;
			case 24:
				b = *pdata++;
				g = *pdata++;
				r = *pdata++;
				a = 255;
				l = 0;
				break;
			case 32:
				b = *pdata++;
				g = *pdata++;
				r = *pdata++;
				a = *pdata++;
				l = 0;
				break;
			default:
				if (isColorMapped)
					free (ColorMap);
				free (pic);
				*pic = nullptr;
				FS_FreeFile (data);
			//	VID_Error ( ERR_DROP, "Illegal pixel_size '%d' in file '%s'\n", pixel_size, filename );
				VID_Printf ( PRINT_ALL, "R_LoadTGA (%s): Illegal pixel_size '%d'\n", filename, pixel_size );
				return;
		 }

PixEncode:
		 if ( isColorMapped )
		 {
			 map_idx = l * 4;
			 *dst++ = ColorMap[map_idx + 0];
			 *dst++ = ColorMap[map_idx + 1];
			 *dst++ = ColorMap[map_idx + 2];
			 *dst++ = ColorMap[map_idx + 3];
		 }
		 else
		 {
			 *dst++ = r;
			 *dst++ = g;
			 *dst++ = b;
			 *dst++ = a;
		 }
	  }

	  if (interleave == TGA_IL_Four)
		  truerow += 4;
	  else if (interleave == TGA_IL_Two)
		  truerow += 2;
	  else
		  truerow++;

	  if (truerow >= h)
		  truerow = ++baserow;
	}

	if (isColorMapped)
		free (ColorMap);

	FS_FreeFile (data);
}

#else

/*
=============
R_LoadTGA
=============
*/
void R_LoadTGA (const char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
	{	// Knightmare- skip this unless developer >= 2 because it spams the console
		if (developer->integer > 1)
			VID_Printf (PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	// Knightmare- check for bad data
	if (!targa_header.width || !targa_header.height) {
		VID_Printf (PRINT_ALL, "Bad tga file %s\n", name);
		FS_FreeFile (buffer);
		return;
	}

	if (targa_header.image_type != 2
		&& targa_header.image_type != 10) {
	//	VID_Error (ERR_DROP, "R_LoadTGA: Only type 2 and 10 targa RGB images supported\n");
		VID_Printf (PRINT_ALL, "R_LoadTGA: %s has wrong image format; only type 2 and 10 targa RGB images supported.\n", name);
		FS_FreeFile (buffer);
		return;
	}

	if (targa_header.colormap_type !=0
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24)) {
	//	VID_Error (ERR_DROP, "R_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
		VID_Printf (PRINT_ALL, "R_LoadTGA: %s has wrong image format; only 32 or 24 bit images supported (no colormaps).\n", name);
		FS_FreeFile (buffer);
		return;
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = malloc (numPixels*4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment

	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) {
					case 24:

							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
					case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
					}

					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
			}
			breakOut:;
		}
	}

	FS_FreeFile (buffer);
}

#endif


/*
==================
R_WriteTGA

Writes a raw image to a TGA file
==================
*/
void R_WriteTGA (byte *rawImage, int width, int height, int nBytes, const char *filename, qboolean relativePath, qboolean checkIfExists)
{
	int		i, temp, nBits;
	size_t	imageSize, c;
	char	checkname[MAX_OSPATH];
	byte	*buffer;
	FILE	*file = nullptr;

	if ( !rawImage || !filename || (width <= 0) || (height <= 0) || (nBytes < 3) || (nBytes > 4) ) {
	//	VID_Printf (PRINT_ALL, "R_WriteTGA: bad parms\n");
		return;
	}

	imageSize = width * height * nBytes;
	nBits = (nBytes == 4) ? 32 : 24;

	if (relativePath)
		snprintf (checkname, sizeof(checkname), "%s/%s", FS_Savegamedir(), filename);
	else
		Q_strncpyz (checkname, sizeof(checkname), filename);
	FS_CreatePath (checkname);
	if (checkIfExists) {
		file = fopen (checkname, "rb");
	}
	if ( !file )
	{
		buffer = malloc(imageSize + 18);
		memset (buffer, 0, 18);
		buffer[2] = 2;		// uncompressed type
		buffer[12] = width & 255;
		buffer[13] = width >> 8;
		buffer[14] = height & 255;
		buffer[15] = height >> 8;
		buffer[16] = nBits;	// pixel size
		memcpy (buffer+18, rawImage, imageSize);

		// swap rgb to bgr
		c = imageSize + 18;
		for (i = 18; i < c; i += nBytes)
		{
			temp = buffer[i];
			buffer[i] = buffer[i+2];
			buffer[i+2] = temp;
		}

		file = fopen (checkname, "wb");
		if (file != nullptr ) {
			fwrite (buffer, 1, c, file);
			fclose (file);
		}
		else
			VID_Printf ( PRINT_ALL, "R_WriteTGA: couldn't open %s for writing\n", checkname );

		free (buffer);
	}
	else {
		fclose (file);
		VID_Printf ( PRINT_ALL, "R_WriteTGA: %s already exists\n", checkname );
	}
}

/*
=================================================================

PNG LOADING

From Quake2Max

=================================================================
*/
#ifdef PNG_SUPPORT

// Knightmare- abstracted function call for png_set_gray_1_2_4_to_8()
#if defined (PNG_LIBPNG_VER) && (PNG_LIBPNG_VER < 10209)
#define PNG_GRAYFUNC_1_2_4_to_8(a)	png_set_gray_1_2_4_to_8(a)
#else
#define PNG_GRAYFUNC_1_2_4_to_8(a)	png_set_expand_gray_1_2_4_to_8(a)
#endif

typedef struct png_handle_s
{
	char	*tmpBuf;
	int		tmpi;
	int /*long*/	fBgColor;		// DL Background color Added 30/05/2000
	int		fTransparent;	// DL Is this Image Transparent?   Added 30/05/2000
	int /*long*/	fRowBytes;		// DL Added 30/05/2000
	double	fGamma;			// DL Added 07/06/2000
	double	fScreenGamma;	// DL Added 07/06/2000
	char	*fRowPtrs;		// DL Changed for consistancy 30/05/2000
	char	*data;			// property data: pByte read fData;
	char	*title;
	char	*author;
	char	*description;
	int		bitDepth;
	int		bytesPerPixel;
	int		colorType;
	int		height;
	int		width;
	int		interlace;
	int		compression;
	int		filter;
	double	lastModified;
	int		transparent;
} png_handle_t;

png_handle_t	*r_png_handle = nullptr;

void R_InitializePNGData (void)
{
	size_t /*long*/*cvaluep; //ub
	int /*long*/	y;

	// Initialize Data and RowPtrs
	if (r_png_handle->data)
	{
		free (r_png_handle->data);
		r_png_handle->data = nullptr;
	}
	if (r_png_handle->fRowPtrs)
	{
		free (r_png_handle->fRowPtrs);
		r_png_handle->fRowPtrs = nullptr;
	}
	r_png_handle->data = malloc(r_png_handle->height * r_png_handle->fRowBytes ); // DL Added 30/5/2000
	r_png_handle->fRowPtrs = malloc(sizeof(void*) * r_png_handle->height);

	if ( (r_png_handle->data) && (r_png_handle->fRowPtrs) )
	{
		cvaluep = (size_t /*long*/*)r_png_handle->fRowPtrs;
		for (y=0; y < r_png_handle->height; y++)
		{
			cvaluep[y] = (size_t /*long*/)r_png_handle->data + ( y * (size_t /*long*/)r_png_handle->fRowBytes ); // DL Added 08/07/2000
		}
	}
}

void R_CreatePNG (void)
{
	if (r_png_handle)
		return;
	r_png_handle = malloc(sizeof(png_handle_t));
	r_png_handle->data    = nullptr;
	r_png_handle->fRowPtrs = nullptr;
	r_png_handle->height  = 0;
	r_png_handle->width   = 0;
	r_png_handle->colorType = PNG_COLOR_TYPE_RGBA;	// was PNG_COLOR_TYPE_RGB
	r_png_handle->interlace = PNG_INTERLACE_NONE;
	r_png_handle->compression = PNG_COMPRESSION_TYPE_DEFAULT;
	r_png_handle->filter = PNG_FILTER_TYPE_DEFAULT;
}

void R_DestroyPNG (qboolean keepData)
{
	if (!r_png_handle)
		return;
	if (r_png_handle->data && !keepData)
		free (r_png_handle->data);
	if (r_png_handle->fRowPtrs)
		free (r_png_handle->fRowPtrs);
	free (r_png_handle);
	r_png_handle = nullptr;
}

void PNGAPI R_ReadPNGData (png_structp png, png_bytep data, png_size_t length)	// called by libpng
{
	int	i;

	for (i=0; i<length; i++)
		data[i] = r_png_handle->tmpBuf[r_png_handle->tmpi++];    // give pnglib a some more bytes
}


/*
==============
R_LoadPNG
==============
*/
void R_LoadPNG (const char *filename, byte **pic, int *width, int *height)
{
	png_structp	png;
	png_infop	pnginfo;
	byte		ioBuffer[8192];
	int			len;
	byte		*raw;

	*pic = nullptr;

	if ( !filename || (filename[0] == '\0') )
		return;

	len = FS_LoadFile (filename, (void **)&raw);

	if (!raw)
	{	// Knightmare- skip this unless developer >= 2 because it spams the console
		if (developer->integer > 1)
			VID_Printf ( PRINT_DEVELOPER, "R_LoadPNG (%s): Bad png file\n", filename );
		return;
	}

	if ( png_sig_cmp(raw, 0, 4) ) {
		FS_FreeFile (raw);
		return;
	}

	png = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png) {
		FS_FreeFile (raw);
		return;
	}

	pnginfo = png_create_info_struct (png);

	if (!pnginfo) {
		png_destroy_read_struct (&png, &pnginfo, nullptr);
		FS_FreeFile (raw);
		return;
	}

	png_set_sig_bytes (png, 0/*sizeof( sig )*/);

	R_CreatePNG (); // creates the r_png_handle struct

	r_png_handle->tmpBuf = raw; //buf = whole file content
	r_png_handle->tmpi = 0;
	png_set_read_fn (png, ioBuffer, R_ReadPNGData);
	png_read_info (png, pnginfo);

	png_get_IHDR (png, pnginfo, &r_png_handle->width, &r_png_handle->height, &r_png_handle->bitDepth,
				&r_png_handle->colorType, &r_png_handle->interlace, &r_png_handle->compression, &r_png_handle->filter);
	// ...removed bgColor code here...

	// conversion of non-RGB types to RGBA is not working, so exclude them for now
	if ( (r_png_handle->colorType != PNG_COLOR_TYPE_RGB) && (r_png_handle->colorType != PNG_COLOR_TYPE_RGB_ALPHA) ) {
		VID_Printf ( PRINT_DEVELOPER, "R_LoadPNG (%s): png type is non-RGB, non-RGBA, aborting load\n", filename );
		png_destroy_read_struct (&png, &pnginfo, nullptr);
		R_DestroyPNG (false);
		FS_FreeFile (raw);
		return;
	}

	if (r_png_handle->colorType == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb (png);
	if ( (r_png_handle->colorType == PNG_COLOR_TYPE_GRAY) && (r_png_handle->bitDepth < 8) )
	//	png_set_gray_1_2_4_to_8 (png);
		PNG_GRAYFUNC_1_2_4_to_8 (png);

	// Add alpha channel if present
	if ( png_get_valid(png, pnginfo, PNG_INFO_tRNS) )
		png_set_tRNS_to_alpha (png);

	// hax: expand 24bit to 32bit
//	if (r_png_handle->bitDepth == 8 && r_png_handle->colorType == PNG_COLOR_TYPE_RGB)
//		png_set_filler (png,255, PNG_FILLER_AFTER);

	// Expand grayscale to RGB
	if ( (r_png_handle->colorType == PNG_COLOR_TYPE_GRAY) || (r_png_handle->colorType == PNG_COLOR_TYPE_GRAY_ALPHA) )
		png_set_gray_to_rgb (png);

	if (r_png_handle->bitDepth < 8)
		png_set_expand (png);

	// hax: expand 24bit to 32bit
	if ( (r_png_handle->bitDepth == 8) && (r_png_handle->colorType == PNG_COLOR_TYPE_RGB) )
		png_set_filler (png, 255, PNG_FILLER_AFTER);

	// update the info structure
	png_read_update_info (png, pnginfo);

	r_png_handle->fRowBytes = (int)png_get_rowbytes (png, pnginfo);
	r_png_handle->bytesPerPixel = png_get_channels (png, pnginfo);  // DL Added 30/08/2000

	R_InitializePNGData ();
	if ( (r_png_handle->data) && (r_png_handle->fRowPtrs) )
		png_read_image (png, (png_bytepp)r_png_handle->fRowPtrs);

	png_read_end (png, pnginfo); // read last information chunks

	png_destroy_read_struct (&png, &pnginfo, nullptr);

	// only load 32 bit by now...
	if (r_png_handle->bitDepth == 8) {
		*pic = r_png_handle->data;
		if (width)
			*width = r_png_handle->width;
		if (height)
			*height = r_png_handle->height;
	}
	else {
		VID_Printf ( PRINT_DEVELOPER, "R_LoadPNG (%s): Bad png color depth\n", filename );
		*pic = nullptr;
		free (r_png_handle->data);
	}

	R_DestroyPNG (true);
	FS_FreeFile (raw);
}


/*
==================
R_WritePNG

Writes a raw image to a PNG file
==================
*/
void R_WritePNG (byte *rawImage, int width, int height, int nBytes, const char *filename, qboolean relativePath, qboolean checkIfExists)
{
	int			i, colorType;
	size_t		imageSize;
	char		checkname[MAX_OSPATH];
	FILE		*file = nullptr;
	png_structp	png_sptr;
	png_infop	png_infoptr;
	void		*lineptr;

	if ( !rawImage || !filename || (width <= 0) || (height <= 0) || (nBytes < 3) || (nBytes > 4) ) {
	//	VID_Printf (PRINT_ALL, "R_WritePNG: bad parms\n");
		return;
	}

	imageSize = width * height * nBytes;

	if (relativePath)
		snprintf (checkname, sizeof(checkname), "%s/%s", FS_Savegamedir(), filename);
	else
		Q_strncpyz (checkname, sizeof(checkname), filename);
	FS_CreatePath (checkname);
	if (checkIfExists) {
		file = fopen (checkname, "rb");
	}
	if ( !file )
	{
		png_sptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if ( !png_sptr ) {
			VID_Printf ( PRINT_ALL, "R_ScreenShot_PNG: Couldn't create PNG struct\n" );
			return;
		}

		png_infoptr = png_create_info_struct (png_sptr);
		if ( !png_infoptr ) {
			png_destroy_write_struct (&png_sptr, nullptr);
			VID_Printf ( PRINT_ALL, "R_ScreenShot_PNG: Couldn't create info struct\n" );
			return;
		}

		if ( setjmp(png_jmpbuf(png_sptr)) ) {
			png_destroy_info_struct (png_sptr, &png_infoptr);
			png_destroy_write_struct (&png_sptr, nullptr);
			VID_Printf ( PRINT_ALL, "R_ScreenShot_PNG: bad data\n" );
			return;
		}

		// open png file
		file = fopen(checkname, "wb");
		if ( !file ) {
			png_destroy_info_struct (png_sptr, &png_infoptr);
			png_destroy_write_struct (&png_sptr, nullptr);
			VID_Printf ( PRINT_ALL, "R_ScreenShot_PNG: Couldn't open %s for writing\n", checkname );
			return;
 		}

		// encode and output
		colorType = (nBytes == 4) ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB;
		png_init_io (png_sptr, file);
		png_set_IHDR (png_sptr, png_infoptr, width, height, 8, colorType,
					PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info (png_sptr, png_infoptr);
		for (i = (vid.height-1); i >= 0; i--) {
			lineptr = rawImage + i * width * nBytes;
			png_write_row (png_sptr, lineptr);
		}
		png_write_end (png_sptr, png_infoptr);

		// clean up
		fclose (file);
		png_destroy_info_struct (png_sptr, &png_infoptr);
		png_destroy_write_struct (&png_sptr, nullptr);

		// close File
		fclose (file);
	}
	else {
		fclose (file);
		VID_Printf ( PRINT_ALL, "R_WritePNG: %s already exists\n", checkname );
	}
}
#endif	// PNG_SUPPORT

/*
=================================================================

JPEG LOADING

By Robert 'Heffo' Heffernan

=================================================================
*/

void jpg_null (j_decompress_ptr cinfo)
{
}

boolean jpg_fill_input_buffer (j_decompress_ptr cinfo)
{
    VID_Printf ( PRINT_ALL, "Premature end of JPEG data\n" );
    return 1;
}

void jpg_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;

    if (cinfo->src->bytes_in_buffer < 0)
		VID_Printf ( PRINT_ALL, "Premature end of JPEG data\n" );
}

//void jpeg_mem_src (j_decompress_ptr cinfo, const byte *mem, unsigned long len)
void jpeg_mem_src (j_decompress_ptr cinfo, const byte *mem, size_t len)
{
    cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    cinfo->src->init_source = jpg_null;
    cinfo->src->fill_input_buffer = jpg_fill_input_buffer;
    cinfo->src->skip_input_data = jpg_skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart;
    cinfo->src->term_source = jpg_null;
    cinfo->src->bytes_in_buffer = len;
    cinfo->src->next_input_byte = mem;
}

#define DSTATE_START	200	/* after create_decompress */
#define DSTATE_INHEADER	201	/* reading header markers, no SOS yet */

/*
==============
R_LoadJPG
==============
*/
void R_LoadJPG (const char *filename, byte **pic, int *width, int *height)
{
	struct jpeg_decompress_struct	cinfo;
	struct jpeg_error_mgr			jerr;
	byte							*rawdata, *rgbadata, *scanline, *p, *q;
	int								rawsize, i;

	if ( !filename || (filename[0] == '\0') )
		return;

	// Load JPEG file into memory
	rawsize = FS_LoadFile(filename, (void **)&rawdata);
	if (!rawdata)
	{	// Knightmare- skip this unless developer >= 2 because it spams the console
		if (developer->integer > 1)
			VID_Printf ( PRINT_DEVELOPER, "R_LoadJPG (%s): Bad jpg file\n", filename );
		return;
	}

	// Knightmare- check for bad data
	if (	rawdata[6] != 'J'
		||	rawdata[7] != 'F'
		||	rawdata[8] != 'I'
		||	rawdata[9] != 'F' ) {
		VID_Printf ( PRINT_ALL, "R_LoadJPG (%s): Bad jpg file\n", filename );
		FS_FreeFile(rawdata);
		return;
	}

	// Initialise libJpeg Object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress (&cinfo);

	// Feed JPEG memory into the libJpeg Object
	jpeg_mem_src (&cinfo, rawdata, rawsize);

	// Process JPEG header
	jpeg_read_header (&cinfo, true);

	// Start Decompression
	jpeg_start_decompress (&cinfo);

	// Check Color Components
	if (cinfo.output_components != 3)
	{
		VID_Printf ( PRINT_ALL, "R_LoadJPG (%s): Invalid JPEG color components\n", filename );
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (rawdata);
		return;
	}

	// Allocate Memory for decompressed image
	rgbadata = malloc(cinfo.output_width * cinfo.output_height * 4);
	if (!rgbadata)
	{
		VID_Printf ( PRINT_ALL, "R_LoadJPG (%s): Insufficient RAM for JPEG buffer\n", filename );
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (rawdata);
		return;
	}

	// Pass sizes to output
	if (width)
		*width = cinfo.output_width;
	if (height)
		*height = cinfo.output_height;

	// Allocate Scanline buffer
	scanline = malloc(cinfo.output_width * 3);
	if (!scanline)
	{
		VID_Printf ( PRINT_ALL, "R_LoadJPG (%s): Insufficient RAM for JPEG scanline buffer\n", filename );
		free (rgbadata);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (rawdata);
		return;
	}

	// Read Scanlines, and expand from RGB to RGBA
	q = rgbadata;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		p = scanline;
		jpeg_read_scanlines (&cinfo, &scanline, 1);

		for (i=0; i<cinfo.output_width; i++)
		{
			q[0] = p[0];
			q[1] = p[1];
			q[2] = p[2];
			q[3] = 255;

			p+=3; q+=4;
		}
	}

	// Free the scanline buffer
	free (scanline);

	// Finish Decompression
	jpeg_finish_decompress (&cinfo);

	// Destroy JPEG object
	jpeg_destroy_decompress (&cinfo);

	// Free raw data buffer
	FS_FreeFile (rawdata);

	// Return the 'rgbadata'
	*pic = rgbadata;
}


/*
==================
R_WriteJPG

Writes a raw image to a JPG file
Based on code by Robert 'Heffo' Heffernan
==================
*/
void R_WriteJPG (byte *rawImage, int width, int height, int nBytes, const char *filename, int quality, qboolean relativePath, qboolean checkIfExists)
{
	size_t						imageSize;
	char						checkname[MAX_OSPATH];
	FILE						*file = nullptr;
	int							offset;
	struct jpeg_compress_struct	cinfo;
	struct jpeg_error_mgr		jerr;
	JSAMPROW					s[1];

	if ( !rawImage || !filename || (width <= 0) || (height <= 0) || (nBytes < 3) || (nBytes > 3) ) {
	//	VID_Printf (PRINT_ALL, "R_WriteJPG: bad parms\n");
		return;
	}

	imageSize = width * height * nBytes;

	if (relativePath)
		snprintf (checkname, sizeof(checkname), "%s/%s", FS_Savegamedir(), filename);
	else
		Q_strncpyz (checkname, sizeof(checkname), filename);
	FS_CreatePath (checkname);
	if (checkIfExists) {
		file = fopen (checkname, "rb");
	}
	if ( !file )
	{
		file = fopen (checkname, "wb");
		if ( !file ) {
			VID_Printf ( PRINT_ALL, "R_WriteJPG: couldn't open %s for writing\n", checkname );
			return;
		}

		// Initialise the JPEG compression object
		cinfo.err = jpeg_std_error (&jerr);
		jpeg_create_compress (&cinfo);
		jpeg_stdio_dest (&cinfo, file);

		// Setup JPEG Parameters
		cinfo.image_width = width;
		cinfo.image_height = height;
		cinfo.in_color_space = JCS_RGB;
		cinfo.input_components = 3;
		jpeg_set_defaults (&cinfo);
		jpeg_set_quality (&cinfo, quality, TRUE);

		// Start Compression
		jpeg_start_compress (&cinfo, true);

		// Feed Scanline data
		offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
		while (cinfo.next_scanline < cinfo.image_height) {
			s[0] = &rawImage[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
			jpeg_write_scanlines (&cinfo, s, 1);
		}

		// Finish Compression
		jpeg_finish_compress (&cinfo);

		// Destroy JPEG object
		jpeg_destroy_compress (&cinfo);

		// Close File
		fclose (file);
	}
	else {
		fclose (file);
		VID_Printf ( PRINT_ALL, "R_WriteJPG: %s already exists\n", checkname );
	}
}

/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
R_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin (byte *skin, int skinwidth, int skinheight, qboolean useQuakePalette)
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	unsigned int		*palette = nullptr;
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	// Knightmare- Quake palette support
	if ( useQuakePalette )
		palette = d_quakePalette;
	else
		palette = d_8to24table;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
		{
		//	if (d_8to24table[i] == (255 << 0) {) // alpha 1.0
			// Knightmare- Quake palette support
			if (palette[i] == (255 << 0)) { // alpha 1.0
				filledcolor = i;
				break;
			}
		}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
	//	VID_Printf (PRINT_ALL, "not filling skin from %d to %d\n", fillcolor, filledcolor);
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}

//=======================================================

/*
================
GL_ResampleTextureLerpLine
from DarkPlaces
================
*/

void GL_ResampleTextureLerpLine (byte *in, byte *out, int inwidth, int outwidth)
{
	int j, xi, oldx = 0, f, fstep, l1, l2, endx;

	fstep = (int) (inwidth*65536.0f/outwidth);
	endx = (inwidth-1);
	for (j = 0,f = 0;j < outwidth;j++, f += fstep)
	{
		xi = (int) f >> 16;
		if (xi != oldx)
		{
			in += (xi - oldx) * 4;
			oldx = xi;
		}
		if (xi < endx)
		{
			l2 = f & 0xFFFF;
			l1 = 0x10000 - l2;
			*out++ = (byte) ((in[0] * l1 + in[4] * l2) >> 16);
			*out++ = (byte) ((in[1] * l1 + in[5] * l2) >> 16);
			*out++ = (byte) ((in[2] * l1 + in[6] * l2) >> 16);
			*out++ = (byte) ((in[3] * l1 + in[7] * l2) >> 16);
		}
		else // last pixel of the line has no pixel to lerp to
		{
			*out++ = in[0];
			*out++ = in[1];
			*out++ = in[2];
			*out++ = in[3];
		}
	}
}

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
	int i, j, yi, oldy, f, fstep, l1, l2, endy = (inheight-1);

	byte *inrow, *out, *row1, *row2;
	out = outdata;
	fstep = (int) (inheight*65536.0f/outheight);

	row1 = malloc(outwidth*4);
	row2 = malloc(outwidth*4);
	inrow = indata;
	oldy = 0;
	GL_ResampleTextureLerpLine (inrow, row1, inwidth, outwidth);
	GL_ResampleTextureLerpLine (inrow + inwidth*4, row2, inwidth, outwidth);
	for (i = 0, f = 0;i < outheight;i++,f += fstep)
	{
		yi = f >> 16;
		if (yi != oldy)
		{
			inrow = (byte *)indata + inwidth*4*yi;
			if (yi == oldy+1)
				memcpy(row1, row2, outwidth*4);
			else
				GL_ResampleTextureLerpLine (inrow, row1, inwidth, outwidth);

			if (yi < endy)
				GL_ResampleTextureLerpLine (inrow + inwidth*4, row2, inwidth, outwidth);
			else
				memcpy(row2, row1, outwidth*4);
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
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16);
			}
			row1 -= outwidth*4;
			row2 -= outwidth*4;
		}
		else // last line has no pixels to lerp to
		{
			for (j = 0;j < outwidth;j++)
			{
				*out++ = *row1++;
				*out++ = *row1++;
				*out++ = *row1++;
				*out++ = *row1++;
			}
			row1 -= outwidth*4;
		}
	}
	free (row1);
	free (row2);
}
/*
void GL_ResampleTexture (unsigned int *in, int inwidth, int inheight, unsigned int *out, int outwidth, int outheight)
{
	int				i, j;
	unsigned int	*inrow, *inrow2;
	unsigned int	frac, fracstep;
	unsigned int	p1[1024], p2[1024];
	byte			*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for (i=0 ; i<outwidth ; i++)
	{
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for (i=0 ; i<outwidth ; i++)
	{
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}*/


#define MAX_UPSAMPLE_FACTOR	6	// limit to 6 (64x scaling)
/*
================
GL_UpscaleTexture
================
*/
void GL_UpscaleTexture (void *indata, int inwidth, int inheight, void *outdata, int scaleFactor, qboolean blend)
{
	if ( !indata || !outdata || (scaleFactor < 1) || (scaleFactor > MAX_UPSAMPLE_FACTOR) )
		return;

//	if ( (r_font_upscale->integer >= 2) && (scaleFactor == 1) ) {
	if ( blend && (scaleFactor == 1) ) {
		R_Upscale2x_Render (outdata, indata, inwidth, inheight);
	}
//	else if ( (r_font_upscale->integer >= 2) && (scaleFactor == 2) ) {
	else if ( blend && (scaleFactor == 2) ) {
		R_Upscale4x_Render (outdata, indata, inwidth, inheight);
	}
	else // if (scaleFactor >= 3)	// just block-copy each pixel
	{
		int				sizeMult = (1 << scaleFactor);
		int				x, y, i, j;
		unsigned int	*inL, *inP, *outL, *outP;

		for (y = 0; y < inheight; y++)
		{
			inL = (unsigned int *)indata + (y * inwidth);
			outL = (unsigned int *)outdata + (y * inwidth * sizeMult * sizeMult);

			for (x = 0; x < inwidth; x++)
			{
				inP = inL + x;
				outP = outL + (x * sizeMult);
				// copy each input pixel to sizeMult^2 pixels
				for (i = 0; i < sizeMult; i++)
				{
					for (j = 0; j < sizeMult; j++)
					{
						*(outP + (i * inwidth * sizeMult) + j) = *inP;
					}
				}
			}
		}
	}
}


/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void GL_LightScaleTexture (unsigned int *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
	}
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0; j<width; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}


#if 0
/*
================
GL_BuildPalettedTexture
================
*/
void GL_BuildPalettedTexture (unsigned char *paletted_texture, unsigned char *scaled, int scaled_width, int scaled_height)
{
	int i;

	for ( i = 0; i < scaled_width * scaled_height; i++ )
	{
		unsigned int r, g, b, c;

		r = ( scaled[0] >> 3 ) & 31;
		g = ( scaled[1] >> 2 ) & 63;
		b = ( scaled[2] >> 3 ) & 31;

		c = r | ( g << 5 ) | ( b << 11 );

		paletted_texture[i] = glState.d_16to8table[c];

		scaled += 4;
	}
}
#endif


/*
===============
here starts modified code
by Heffo/changes by Nexus
===============
*/

static	int		upload_width, upload_height; //** DMP made local to module
static	qboolean uploaded_paletted;			//** DMP ditto

/*static qboolean IsPowerOf2 (int value)
{
	int i = 1;
	while (1) {
		if (value == i)
			return true;
		if (i > value)
			return false;
		i <<= 1;
	}
} */

/*
================
NearestPowerOf2
================
*/
int NearestPowerOf2 (int size)
{
	int i = 2;

	// NeVo - infinite loop bug-fix
//	if (size == 1)
	if (size < 2)	// FS: Better fix
		return size;

	while (1)
	{
		i <<= 1;
		if (size == i)
			return i;
		if ( (size > i) && (size < (i << 1)) )
		{
			if (size >= ((i + (i << 1))/2))
				return i << 1;
			else
				return i;
		}
	};
}


/*
===============
GL_Upload32

Returns has_alpha
===============
*/
qboolean GL_Upload32 (unsigned int *data, int width, int height, imagetype_t type)
{
	unsigned int	*scaled = nullptr;
	int				scaled_width, scaled_height;
	int				i, c, comp, idealImgRes = 256, upscaleFactor = 0;
	qboolean		mipmap, hasAlpha, isNPOT, resampled = false;
	byte			*scan;

	mipmap = ( (type != it_pic) && (type != it_font) && (type != it_scrap) && (type != it_sky) );
	uploaded_paletted = false;

	// get best upscale size for old fonts
	if ( (type == it_font) && (width == 128) && (height == 128) && r_font_upscale->integer ) {
		idealImgRes = NearestPowerOf2(min(vid.width, vid.height));	// was / 2
		idealImgRes = min(idealImgRes, glConfig.max_texsize);
		idealImgRes = max(idealImgRes, 256);
	//	VID_Printf (PRINT_DEVELOPER, "GL_Upload32: ideal font res is %d.\n", idealImgRes);
	}
	// get best upscale size for scrap
	if ( (type == it_scrap) && r_scrap_upscale->integer ) {
		idealImgRes = NearestPowerOf2(min(vid.width, vid.height));
		idealImgRes = min(idealImgRes, glConfig.max_texsize);
		idealImgRes = max(idealImgRes, 256);
	//	VID_Printf (PRINT_DEVELOPER, "GL_Upload32: ideal scrap res is %d.\n", idealImgRes);
	}

	//
	// scan the texture for any non-255 alpha
	//
	c = width*height;
	scan = ((byte *)data) + 3;
	hasAlpha = false;
	for (i = 0; i < c; i++, scan += 4)
	{
		if ( *scan != 255 ) {
			hasAlpha = true;
			break;
		}
	}

	// Heffo - ARB Texture Compression
	qglHint(GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);
	comp = (glState.texture_compression && (type != it_pic) && (type != it_font) && (type != it_scrap)) ? GL_COMPRESSED_RGBA_ARB : GL_RGBA;

	//
	// find sizes to scale to
	//
	if ( glConfig.arbTextureNonPowerOfTwo && (!mipmap || r_nonpoweroftwo_mipmaps->integer) ) {
		scaled_width = width;
		scaled_height = height;
		isNPOT = ( (NearestPowerOf2(width) != width) || (NearestPowerOf2(height) != height) );
	}
	else {
		scaled_width = NearestPowerOf2 (width);
		scaled_height = NearestPowerOf2 (height);
		isNPOT = false;
	}

	if (scaled_width > glConfig.max_texsize)
		scaled_width = glConfig.max_texsize;
	if (scaled_height > glConfig.max_texsize)
		scaled_height = glConfig.max_texsize;

	//
	// allow sampling down of the world textures for speed
	//
	if (mipmap && r_picmip->integer > 0)
	{
		int maxsize;

		if (r_picmip->integer == 1)		// clamp to 1024x1024
			maxsize = 1024;
		else if (r_picmip->integer == 2) // clamp to 512x512
			maxsize = 512;
		else if (r_picmip->integer == 3) // clamp to 256x256
			maxsize = 256;
		else								// clamp to 128x128
			maxsize = 128;

		while (1) {
			if (scaled_width <= maxsize && scaled_height <= maxsize)
				break;
			scaled_width >>= 1;
			scaled_height >>= 1;
		}
	//	scaled_width >>= r_picmip->integer;
	//	scaled_height >>= r_picmip->integer;
	}

	//
	// resample texture if needed
	//
	if ( ( (type == it_font) && r_font_upscale->integer ) || ( (type == it_scrap) && r_scrap_upscale->integer )
		&& (scaled_width == scaled_height) && ((scaled_width * 2) <= idealImgRes) )	// scale up fonts
	{
		while (1)
		{
			if ( ((scaled_width * 2) > idealImgRes) && ((scaled_height * 2) > idealImgRes) )
				break;
			if (upscaleFactor >= MAX_UPSAMPLE_FACTOR)	// don't go past 64x scaling
				break;
			scaled_width <<= 1;
			scaled_height <<= 1;
			upscaleFactor++;
		}
		if (upscaleFactor > 0)
		{
			upscaleFactor = min(upscaleFactor, MAX_UPSAMPLE_FACTOR);	// clamp to max upscale factor
			VID_Printf ( PRINT_DEVELOPER, "GL_Upload32: scaling font or scrap image from %dx%d to %dx%d.\n", width, height, scaled_width, scaled_height );
			scaled = malloc((scaled_width * scaled_height) * 4);
			if (type == it_font)
				GL_UpscaleTexture (data, width, height, scaled, upscaleFactor, (r_font_upscale->integer >= 2));
			else
				GL_UpscaleTexture (data, width, height, scaled, upscaleFactor, (r_scrap_upscale->integer >= 2));
			resampled = true;
		}
		else {
			scaled_width = width;
			scaled_height = height;
			scaled = data;
			resampled = false;
		}
	}
	else if (scaled_width != width || scaled_height != height) {
		scaled = malloc((scaled_width * scaled_height) * 4);
		GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);
		resampled = true;
	}
	else {
		scaled_width = width;
		scaled_height = height;
		scaled = data;
		resampled = false;
	}

	if (!glState.gammaRamp)
		GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap );

	//
	// generate mipmaps and upload
	//
#ifdef USE_GLU_MIPMAP
	if (mipmap)
	{
		if (glState.sgis_mipmap) {
			qglTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, true);
			qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		}
		else
			gluBuild2DMipmaps (GL_TEXTURE_2D, comp, scaled_width, scaled_height, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	}
	else
		qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

#else	// USE_GLU_MIPMAP
	qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	if (mipmap)
	{
		int				mip_width, mip_height, miplevel = 0;
		unsigned int	*mip_scaled = nullptr;
		qboolean		mip_resampled = false;

/*		if (isNPOT)	// resample NPOT tetures to POT dimensions before mipping
		{
			mip_width = NearestPowerOf2 (scaled_width);
			mip_height = NearestPowerOf2 (scaled_height);
			mip_scaled = malloc((mip_width * mip_height) * 4);
			GL_ResampleTexture (scaled, scaled_width, scaled_height, mip_scaled, mip_width, mip_height);
			mip_resampled = true;
		}
		else {
			mip_width = scaled_width;
			mip_height = scaled_height;
			mip_scaled = scaled;
			mip_resampled = false;
		}
*/
		mip_width = scaled_width;	mip_height = scaled_height;
		mip_scaled = scaled;
		while (mip_width > 1 || mip_height > 1)
		{
			GL_MipMap ((byte *)mip_scaled, mip_width, mip_height);
			mip_width = max(mip_width>>1, 1);
			mip_height = max(mip_height>>1, 1);
			miplevel++;
			qglTexImage2D (GL_TEXTURE_2D, miplevel, comp, mip_width, mip_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mip_scaled);
		}

	//	if (mip_resampled)
	//		free (mip_scaled);
	}
#endif	// USE_GLU_MIPMAP

//	if (scaled_width != width || scaled_height != height)
	if (resampled)
		free (scaled);
	scaled = nullptr;

	upload_width = scaled_width;	upload_height = scaled_height;

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap) ? gl_filter_min : gl_filter_max);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	// Set anisotropic filter if supported and enabled
	if (mipmap && glConfig.anisotropic && r_anisotropic->value)
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_anisotropic->value);

	return hasAlpha;
}


/*
===============
GL_Upload8

Returns has_alpha
===============
*/
qboolean GL_Upload8 (byte *data, int width, int height, imagetype_t type, qboolean useQuakePalette)
{
//	unsigned int	trans[512*256];
	unsigned int	*trans = nullptr;	// Knightmare- changed to dynamic allocation
	unsigned int	*palette = nullptr;
	int				i, s;
	int				p;
	qboolean		has_alpha;	// Knightmare added

	s = width*height;

//	if (s > sizeof(trans)/4)
//		VID_Error (ERR_DROP, "GL_Upload8: too large");

	trans = (unsigned int *)malloc(s*4);	// Knightmare- changed to dynamic allocation

	// Knightmare- Quake palette support
	if ( useQuakePalette )
		palette = d_quakePalette;
	else
		palette = d_8to24table;

	for (i=0 ; i<s ; i++)
	{
		p = data[i];
	//	trans[i] = d_8to24table[p];
		// Knightmare- Quake palette support
		trans[i] = palette[p];

		if (p == 255)
		{	// transparent, so scan around for another color
			// to avoid alpha fringes
			// FIXME: do a full flood fill so mips work...
			if (i > width && data[i-width] != 255)
				p = data[i-width];
			else if (i < s-width && data[i+width] != 255)
				p = data[i+width];
			else if (i > 0 && data[i-1] != 255)
				p = data[i-1];
			else if (i < s-1 && data[i+1] != 255)
				p = data[i+1];
			else
				p = 0;
			// copy rgb components
		//	((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
		//	((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
		//	((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
			// Knightmare- Quake palette support
			((byte *)&trans[i])[0] = ((byte *)&palette[p])[0];
			((byte *)&trans[i])[1] = ((byte *)&palette[p])[1];
			((byte *)&trans[i])[2] = ((byte *)&palette[p])[2];
		}
	}

//	return GL_Upload32 (trans, width, height, type);

	// Knightmare- changed to dynamic allocation
	has_alpha = GL_Upload32 (trans, width, height, type);

	free (trans);
	trans = nullptr;

	return has_alpha;
	// end Knightmare
}


/*
================
R_LoadPic

This is also used as an entry point for the generated notexture
Nexus  - changes for hires-textures
================
*/
image_t *R_LoadPic (const char *name, byte *pic, int width, int height, imagetype_t type, int bits)
{
	image_t		*image;
	int			i;
	// Nexus'added vars
	int			nameLen;
	char		refName[128];

	// find a free image_t
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			VID_Error ( ERR_DROP, "MAX_GLTEXTURES" );
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		VID_Error ( ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name );
	Q_strncpyz (image->name, sizeof(image->name), name);
	image->hash = Com_HashFileName(name, 0, false);	// Knightmare added
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;
	image->replace_scale_w = image->replace_scale_h = 1.0f; // Knightmare added

	if ( (type == it_skin) && (bits == 8) )
		R_FloodFillSkin (pic, width, height, false);

	// replacement scaling hack for TGA/JPEG HUD images and skins
	// TODO: replace this with shaders as soon as they are supported
	nameLen = (int)strlen(name);
	Q_strncpyz (refName, sizeof(refName), name);

	// Load .pcx for size refereence, check if we have a tga/jpg/png pic
#ifdef PNG_SUPPORT
	if ( (nameLen >= 5) && ((type == it_pic) || (type == it_font)) && (!strcmp(refName+nameLen-4, ".tga") || !strcmp(refName+nameLen-4, ".png") || !strcmp(refName+nameLen-4, ".jpg")) )
#else	// PNG_SUPPORT
	if ( (nameLen >= 5) && ((type == it_pic) || (type == it_font)) && (!strcmp(refName+nameLen-4, ".tga") || !strcmp(refName+nameLen-4, ".jpg")) )
#endif	// PNG_SUPPORT
	{
		byte	*pcxPic, *pcxPalette;
		int		pcxWidth, pcxHeight;

		refName[nameLen-3] = 'p'; refName[nameLen-2] = 'c'; refName[nameLen-1] = 'x';	// replace extension
		LoadPCX (refName, &pcxPic, &pcxPalette, &pcxWidth, &pcxHeight);					// load .pcx file

		if ( pcxPic && (pcxWidth > 0) && (pcxHeight > 0) ) {
			image->replace_scale_w = (float)pcxWidth / image->width;
			image->replace_scale_h = (float)pcxHeight / image->height;
		}
		if (pcxPic)
			free (pcxPic);
		if (pcxPalette)
			free (pcxPalette);
	}

	// load little pics into the scrap
	if ( (image->type == it_pic) && (bits == 8) && (image->width < 64) && (image->height < 64) )
	{
		int		x, y;
		int		i, j, k;
		int		texnum;

		texnum = Scrap_AllocBlock (image->width, image->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;
	//	scrap_dirty = true;
		scrap_dirty[texnum] = true;
		scrap_check_dirty = true;

		// copy the texels into the scrap block
		k = 0;
		for (i=0 ; i<image->height ; i++)
			for (j=0 ; j<image->width ; j++, k++)
				scrap_texels[texnum][(y+i)*SCRAP_BLOCK_WIDTH + x + j] = pic[k];
		image->texnum = TEXNUM_SCRAPS + texnum;
		image->scrap = true;
		image->has_alpha = true;
		image->sl = (x+0.01)/(float)SCRAP_BLOCK_WIDTH;
		image->sh = (x+image->width-0.01)/(float)SCRAP_BLOCK_WIDTH;
		image->tl = (y+0.01)/(float)SCRAP_BLOCK_WIDTH;
		image->th = (y+image->height-0.01)/(float)SCRAP_BLOCK_WIDTH;
	}
	else
	{
nonscrap:
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + (image - gltextures);
		GL_Bind(image->texnum);
		if (bits == 8)
			image->has_alpha = GL_Upload8 (pic, width, height, image->type, false);
		else
			image->has_alpha = GL_Upload32 ((unsigned int *)pic, width, height, image->type);
		image->upload_width = upload_width;		// after power of 2 and scales
		image->upload_height = upload_height;
		image->paletted = uploaded_paletted;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	return image;
}


/*
================
R_LoadQuakePic

Loads embedded skins/textures using Quake palette
Example filename: models/objects/tree.mdl_0.pcx
================
*/
image_t *R_LoadQuakePic (const char *name, byte *pic, int width, int height, imagetype_t type, int bits)
{
	image_t		*image;
	int			i;
	int			nameLen;
	char		replaceName[128];

	// TGA/PNG/JPG auto-replace
	nameLen = (int)strlen(name);
	if ( (bits == 8) && ( !strcmp(name+nameLen-4, ".pcx") || !strcmp(name+nameLen-4, ".wal") ) )
	{
		Q_strncpyz (replaceName, sizeof(replaceName), name);
		replaceName[nameLen-3] = 't'; replaceName[nameLen-2] = 'g'; replaceName[nameLen-1] = 'a';
		image = R_FindImage (replaceName, type);	// will automatically fall back to .png and .jpg
		if (image != nullptr ) {
			return image;
		}
	}

	// find a free image_t
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			VID_Error ( ERR_DROP, "MAX_GLTEXTURES" );
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		VID_Error ( ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name );
	Q_strncpyz (image->name, sizeof(image->name), name);
	image->hash = Com_HashFileName(name, 0, false);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;
	image->replace_scale_w = image->replace_scale_h = 1.0f;

	if ( (type == it_skin) && (bits == 8) )
		R_FloodFillSkin (pic, width, height, true);

	image->scrap = false;
	image->texnum = TEXNUM_IMAGES + (image - gltextures);
	GL_Bind(image->texnum);
	if (bits == 8)
		image->has_alpha = GL_Upload8 (pic, width, height, image->type, true);
	else
		image->has_alpha = GL_Upload32 ((unsigned int *)pic, width, height, image->type);
	image->upload_width = upload_width;		// after power of 2 and scales
	image->upload_height = upload_height;
	image->paletted = uploaded_paletted;
	image->sl = 0;
	image->sh = 1;
	image->tl = 0;
	image->th = 1;

	return image;
}


// store the names of last images that failed to load
#define NUM_FAIL_IMAGES 1024
char lastFailedImage[NUM_FAIL_IMAGES][MAX_OSPATH];
unsigned int lastFailedImageHash[NUM_FAIL_IMAGES];
static unsigned int failedImgListIndex;

/*
===============
R_InitFailedImgList
===============
*/
void R_InitFailedImgList (void)
{
	int		i;

	for (i=0; i<NUM_FAIL_IMAGES; i++) {
		snprintf(lastFailedImage[i], sizeof(lastFailedImage[i]), "\0");
		lastFailedImageHash[i] = 0;
	}

	failedImgListIndex = 0;
}

/*
===============
R_CheckImgFailed
===============
*/
qboolean R_CheckImgFailed (const char *name)
{
	int				i;
	unsigned int	hash;

	if ( !name || (name[0] == '\0') )
		return true;

	hash = Com_HashFileName(name, 0, false);
	for (i=0; i<NUM_FAIL_IMAGES; i++)
	{
		if (hash == lastFailedImageHash[i]) {	// compare hash first
			if (lastFailedImage[i] && strlen(lastFailedImage[i])
				&& !strcmp(name, lastFailedImage[i]))
			{	// we already tried to load this image, didn't find it
				//VID_Printf (PRINT_ALL, "R_CheckImgFailed: found %s on failed to load list\n", name);
				return true;
			}
		}
	}

	return false;
}

/*
===============
R_AddToFailedImgList
===============
*/
void R_AddToFailedImgList (const char *name)
{
	if ( !name || (name[0] == '\0') )
		return;

	if (!strncmp(name, "save/", 5)) // don't add saveshots
		return;

//	VID_Printf (PRINT_ALL, "R_AddToFailedImgList: adding %s to failed to load list\n", name);

	snprintf(lastFailedImage[failedImgListIndex], sizeof(lastFailedImage[failedImgListIndex]), "%s", name);
	lastFailedImageHash[failedImgListIndex] = Com_HashFileName(name, 0, false);
	failedImgListIndex++;

	// wrap around to start of list
	if (failedImgListIndex >= NUM_FAIL_IMAGES)
		failedImgListIndex = 0;
}

/*
================
R_LoadWal
================
*/
image_t *R_LoadWal (const char *name, imagetype_t type)
{
	miptex_t	*mt;
	int			width, height, ofs;
	image_t		*image;

	if ( !name || (name[0] == '\0') )
		return nullptr;

	FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		if (type == it_wall)
			VID_Printf ( PRINT_ALL, "R_FindImage: can't load %s\n", name );
	//	return glMedia.noTexture;
		return nullptr;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	ofs = LittleLong (mt->offsets[0]);

	image = R_LoadPic (name, (byte *)mt + ofs, width, height, it_wall, 8);

	FS_FreeFile ((void *)mt);

	return image;
}

/*
===============
R_FindImage

Finds or loads the given image
===============
*/
image_t *R_FindImage( const char *rawName, imagetype_t type )
{
	image_t *image;
	int      i;
	byte    *pic, *palette;
	int      width, height;
	char     s[ MAX_OSPATH ];

	if ( !rawName || ( rawName[ 0 ] == '\0' ) )
	{
		return nullptr;
	}

	int len = ( int ) strlen( rawName );
	if ( len < 5 )
	{
		return nullptr;
	}

	std::string path      = FS_SanitizePath( rawName );
	std::string extension = FS_GetExtension( path );
	// Fix some weird edge cases where files requested don't have extensions
	//TODO: fix this at the source, rather than here!
	if ( path[ 0 ] != '*' && extension.empty() )
	{
		path += ".png";
	}

	// look for it
	const char  *name = path.c_str();
	unsigned int hash = Com_HashFileName( name, 0, false );
	for ( i = 0, image = gltextures; i < numgltextures; i++, image++ )
	{
		if ( hash == image->hash )
		{// compare hash first
			if ( !strcmp( name, image->name ) )
			{
				image->registration_sequence = registration_sequence;
				return image;
			}
		}
	}

	// don't try again to load an image that just failed
	if ( R_CheckImgFailed( name ) )
	{
		if ( extension == "tga" )
		{
			// fall back to png
			Q_strncpyz( s, sizeof( s ), name );
			s[ len - 3 ] = 'p';
			s[ len - 2 ] = 'n';
			s[ len - 1 ] = 'g';
			return R_FindImage( s, type );
		}

		if ( extension == "png" )
		{
			// fall back to jpg
			Q_strncpyz( s, sizeof( s ), name );
			s[ len - 3 ] = 'j';
			s[ len - 2 ] = 'p';
			s[ len - 1 ] = 'g';
			return R_FindImage( s, type );
		}

		return nullptr;
	}

	// MrG's automatic JPG & TGA loading
	// search for TGAs or JPGs to replace .pcx and .wal images
	if ( extension == "pcx" || extension == "wal" )
	{
		Q_strncpyz( s, sizeof( s ), name );
		s[ len - 3 ] = 't';
		s[ len - 2 ] = 'g';
		s[ len - 1 ] = 'a';
		image        = R_FindImage( s, type );
		if ( image )
		{
			return image;
		}
	}

	//
	// load the pic from disk
	//
	pic     = nullptr;
	palette = nullptr;

	if ( extension == "pcx" )
	{
		LoadPCX( name, &pic, &palette, &width, &height );
		if ( pic )
		{
			image = R_LoadPic( name, pic, width, height, type, 8 );
		}
		else
		{
			image = nullptr;
		}
	}
	else if ( extension == "wal" )
	{
		image = R_LoadWal( name, type );
	}
	else if ( extension == "tga" )
	{
		R_LoadTGA( name, &pic, &width, &height );
		if ( pic )
		{
			image = R_LoadPic( name, pic, width, height, type, 32 );
		}
		else
		{// fall back to png
			R_AddToFailedImgList( name );
			Q_strncpyz( s, sizeof( s ), name );
			s[ len - 3 ] = 'p';
			s[ len - 2 ] = 'n';
			s[ len - 1 ] = 'g';
			return R_FindImage( s, type );
		}
	}
	else if ( extension == "png" )
	{
		R_LoadPNG( name, &pic, &width, &height );
		if ( pic )
		{
			image = R_LoadPic( name, pic, width, height, type, 32 );
		}
		else
		{// fall back to jpg
			R_AddToFailedImgList( name );
			Q_strncpyz( s, sizeof( s ), name );
			s[ len - 3 ] = 'j';
			s[ len - 2 ] = 'p';
			s[ len - 1 ] = 'g';
			return R_FindImage( s, type );
		}
	}
	else if ( extension == "jpg" )// Heffo - JPEG support
	{
		R_LoadJPG( name, &pic, &width, &height );
		if ( pic )
		{
			image = R_LoadPic( name, pic, width, height, type, 32 );
		}
		else
		{
			image = nullptr;
		}
	}
	else
	{
		image = nullptr;
	}

	if ( !image )
	{
#if !defined( NDEBUG )
		VID_Printf( PRINT_ALL, "Didn't find image (%s)!\n", name );
#endif

		R_AddToFailedImgList( name );
	}

	if ( pic )
	{
		free( pic );
	}
	if ( palette )
	{
		free( palette );
	}

	return image;
}



/*
===============
R_RegisterSkin
===============
*/
image_s *R_RegisterSkin (char *name)
{
	return R_FindImage (name, it_skin);
}


/*
================
R_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void R_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	// never free notexture or particle textures
	glMedia.noTexture->registration_sequence       = registration_sequence;
	glMedia.whiteTexture->registration_sequence    = registration_sequence;
	glMedia.distTextureARB->registration_sequence  = registration_sequence;
	glMedia.celShadeTexture->registration_sequence = registration_sequence;
#ifdef ROQ_SUPPORT
	glMedia.rawTexture->registration_sequence = registration_sequence;
#endif// ROQ_SUPPORT
	glMedia.envMapTexture->registration_sequence       = registration_sequence;
	glMedia.sphereMapTexture->registration_sequence    = registration_sequence;
	glMedia.shellTexture->registration_sequence        = registration_sequence;
	glMedia.flareTexture->registration_sequence        = registration_sequence;
	glMedia.causticWaterTexture->registration_sequence = registration_sequence;
	glMedia.causticSlimeTexture->registration_sequence = registration_sequence;
	glMedia.causticLavaTexture->registration_sequence  = registration_sequence;
	glMedia.particleBeam->registration_sequence        = registration_sequence;

	for (i=0; i<PARTICLE_TYPES; i++)
		if (glMedia.particleTextures[i]) // don't mess with null ones silly :p
			glMedia.particleTextures[i]->registration_sequence = registration_sequence;

	for (i=0, image=gltextures; i<numgltextures; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if ( (image->type == it_pic) || (image->type == it_font) || (image->type == it_scrap) )
			continue;		// don't free pics or fonts

		// free it
		qglDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}


/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette (void)
{
	int				i;
	int				r, g, b;
	unsigned int	v;
	byte			*pic, *pal;
	int				width, height;

	// get the palette

	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		VID_Error ( ERR_FATAL, "Couldn't load pics/colormap.pcx" );

	for (i=0 ; i<256 ; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];

		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

	free (pic);
	free (pal);

	return 0;
}


/*
===============
Draw_GetQuakePalette

Loads Quake Palette for MDL models
===============
*/
void Draw_GetQuakePalette (void)
{
	int				i, len;
	int				r, g, b;
	unsigned int	v;
	byte			*pal;
	qboolean		useDefaultPal = false;

	len = FS_LoadFile ("gfx/palette.lmp", (void*)&pal);
	if ( !pal ) {
		VID_Printf ( PRINT_DEVELOPER, "gfx/palette.lmp not found, using internal default\n" );
		useDefaultPal = true;
	}
	else if (len < 768) {
		VID_Printf ( PRINT_DEVELOPER, "gfx/palette.lmp has wrong size, using internal default\n" );
		FS_FreeFile (pal);
		useDefaultPal = true;
	}

	if ( useDefaultPal ) {
		pal = (byte *)default_quakePal;
	}

	for (i = 0; i < 256; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_quakePalette[i] = LittleLong(v);
	}
	d_quakePalette[255] &= LittleLong(0xffffff);	// 255 is transparent

	if ( !useDefaultPal ) {
		FS_FreeFile (pal);
		VID_Printf ( PRINT_DEVELOPER, "Sucessfully loaded gfx/palette.lmp\n" );
	}
}


/*
===============
R_InitImages
===============
*/
void R_InitImages (void)
{
	int		i, j;
	float	g = vid_gamma->value;

	// Knightmare- reinitialize these after a vid_restart
	// this is needed because the renderer is no longer a DLL
	gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
	gl_filter_max = GL_LINEAR;

	registration_sequence = 1;

	// init intensity conversions
	// added Vic's RGB brightening
	if (glConfig.mtexcombine)
		r_intensity = Cvar_Get ( "r_intensity", "1", 0 );
	else
		r_intensity = Cvar_Get ( "r_intensity", "2", 0 );
	Cvar_SetDescription ( "r_intensity", "Sets intensity value for gamma table.  Value range is >= 1." );

	if ( r_intensity->value <= 1 )
		Cvar_Set( "r_intensity", "1" );

	glState.inverse_intensity = 1 / r_intensity->value;

	R_InitFailedImgList ();		// Knightmare added

	Draw_GetPalette ();

	Draw_GetQuakePalette ();	// Knightmare added

#if 0
	if (qglColorTableEXT)
	{
		FS_LoadFile( "pics/16to8.dat", &glState.d_16to8table );
		if ( !glState.d_16to8table )
			VID_Error( ERR_FATAL, "Couldn't load pics/16to8.pcx");
	}
#endif

	if (glConfig.rendType == GLREND_VOODOO)
	{
		g = 1.0F;
	}

	for ( i = 0; i < 256; i++ )
	{
		if ( g == 1 )
		{
			gammatable[i] = i;
		}
		else
		{
			float inf;

			inf = 255 * pow ( (i+0.5)/255.5 , g ) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i=0 ; i<256 ; i++)
	{
		j = i*r_intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}

	R_Upscale_Init ();

	R_InitBloomTextures (); // BLOOMS
}

/*
===============
R_FreePic
by Knightmare
Frees a single pic
===============
*/
void R_FreePic (char *name)
{
	int		i;
	image_t	*image;

	for (i=0, image=gltextures; i<numgltextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type != it_pic)
			continue;		// only free pics
		if (!strcmp(name, image->name))
		{	// free it
			qglDeleteTextures (1, &image->texnum);
			memset (image, 0, sizeof(*image));
			return; //we're done here
		}
	}
}

/*
===============
R_ShutdownImages
===============
*/
void R_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=gltextures; i<numgltextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot

		// free it
		qglDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}
