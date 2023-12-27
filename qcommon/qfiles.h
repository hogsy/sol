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

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

/*
========================================================================

The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#define IDPAKHEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')

typedef struct
{
	char				name[56];
	unsigned int		filepos, filelen;
} dpackfile_t;

typedef struct
{
	unsigned int		ident;		// == IDPAKHEADER
	unsigned int		dirofs;
	unsigned int		dirlen;
} dpackheader_t;

#define	MAX_FILES_IN_PACK	16384	// Knightmare- was 4096, increased for Q2 re-release pak

/*
========================================================================

PCX files are used for as many images as possible

========================================================================
*/

typedef struct
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned char	data;			// unbounded
} pcx_t;

/*
========================================================================

.MDL triangle model file format, from Quake source

========================================================================
*/

#define IDMDLHEADER		(('O'<<24)+('P'<<16)+('D'<<8)+'I')
#define MDL_ALIAS_VERSION	6
#define MDL_ONSEAM			0x0020

#define MDL_MAX_TRIANGLES	2048
#define MDL_MAX_VERTS		1024
#define MDL_MAX_FRAMES		256
#define MDL_MAX_SKINS		32

typedef enum { ST_SYNC = 0, ST_RAND } mdl_synctype_t;

typedef enum { MDL_SINGLE = 0, MDL_GROUP } mdl_frametype_t;

typedef enum { MDL_SKIN_SINGLE = 0, MDL_SKIN_GROUP } mdl_skintype_t;

typedef struct {
	int		onSeam;
	int		s;
	int		t;
} dmdl_stvert_t;

typedef struct {
	byte	v[3];
	byte	lightnormalindex;
} dmdl_trivertx_t;

typedef struct dtriangle_s {
	int		facesFront;
	int		vertIndex[3];
} dmdl_triangle_t;

typedef struct {
	mdl_frametype_t	type;
} dmdl_frametype_t;

typedef struct {
	dmdl_trivertx_t	bbox_min;	// lightnormal isn't used
	dmdl_trivertx_t	bbox_max;	// lightnormal isn't used
	char			name[16];		// frame name from grabbing
} dmdl_frame_t;

typedef struct {
	int				num_frames;
	dmdl_trivertx_t	bbox_min;	// lightnormal isn't used
	dmdl_trivertx_t	bbox_max;	// lightnormal isn't used
} dmdl_group_t;

typedef struct {
	float	interval;
} dmdl_interval_t;

typedef struct {
	mdl_skintype_t	type;
} dmdl_skintype_t;

typedef struct {
	int			num_skins;
} dmdl_skingroup_t;

typedef struct {
	float	interval;
} dmdl_skininterval_t;

typedef struct {
	int				ident;
	int				version;

	vec3_t			scale;
	vec3_t			scale_origin;
	float			bounding_radius;
	vec3_t			eye_position;

	int				num_skins;
	int				skin_width;
	int				skin_height;
	int				num_verts;
	int				num_tris;
	int				num_frames;
	mdl_synctype_t	sync_type;
	int				flags;
	float			size;
} dmdl_t;

/*
========================================================================

.MD2 triangle model file format

========================================================================
*/

#define IDMD2HEADER		(('2'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD2_ALIAS_VERSION	8

#define	MD2_MAX_TRIANGLES	4096
#define MD2_MAX_VERTS		2048
#define MD2_MAX_FRAMES		512
#define MD2_MAX_SKINS		32
#define	MD2_MAX_SKINNAME	64

typedef struct
{
	short	s;
	short	t;
} dmd2coord_t;

typedef struct 
{
	short	index_xyz[3];
	short	index_st[3];
} dmd2triangle_t;

typedef struct
{
	byte	v[3];			// scaled byte to fit in frame mins/maxs
	byte	lightnormalindex;
} dmd2vertex_t;

#define DTRIVERTX_V0   0
#define DTRIVERTX_V1   1
#define DTRIVERTX_V2   2
#define DTRIVERTX_LNI  3
#define DTRIVERTX_SIZE 4

typedef struct
{
	float			scale[3];		// multiply byte verts by this
	float			translate[3];	// then add this
	char			name[16];		// frame name from grabbing
	dmd2vertex_t	verts[1];		// variable sized
} dmd2frame_t;


// the glcmd format:
// a positive integer starts a tristrip command, followed by that many
// vertex structures.
// a negative integer starts a trifan command, followed by -x vertexes
// a zero indicates the end of the command list.
// a vertex consists of a floating point s, a floating point t,
// and an integer vertex index.


typedef struct
{
	int			ident;
	int			version;

	int			skinwidth;
	int			skinheight;
	int			framesize;		// byte size of each frame

	int			num_skins;
	int			num_xyz;
	int			num_st;			// greater than num_xyz for seams
	int			num_tris;
	int			num_glcmds;		// dwords in strip/fan command list
	int			num_frames;

	int			ofs_skins;		// each skin is a MD2_MAX_SKINNAME string
	int			ofs_st;			// byte offset from start for stverts
	int			ofs_tris;		// offset for dtriangles
	int			ofs_frames;		// offset for first frame
	int			ofs_glcmds;	
	int			ofs_end;		// end of file

} dmd2_t;

/*
========================================================================

.MD3 model file format

========================================================================
*/

#define IDMD3HEADER		(('3'<<24)+('P'<<16)+('D'<<8)+'I')

#define MD3_ALIAS_VERSION	15
#define MD3_ALIAS_MAX_LODS	4

#define	MD3_MAX_TRIANGLES	8192	// per mesh
#define MD3_MAX_VERTS		4096	// per mesh
#define MD3_MAX_SHADERS		256		// per mesh
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_MESHES		32		// per model
#define MD3_MAX_TAGS		16		// per frame
#define MD3_MAX_PATH		64

#ifndef M_TWOPI
#define M_TWOPI		6.28318530717958647692
#endif

// vertex scales
#define	MD3_XYZ_SCALE		(1.0/64)

typedef unsigned int index_t;

typedef struct
{
	float			st[2];
} dmd3coord_t;

typedef struct
{
	short			point[3];
	short			norm;
} dmd3vertex_t;

typedef struct
{
    vec3_t			mins;
	vec3_t			maxs;
    vec3_t			translate;
    float			radius;
    char			creator[16];
} dmd3frame_t;

typedef struct 
{
	vec3_t			origin;
	float			axis[3][3];
} dorientation_t;

typedef struct
{
	char			name[MD3_MAX_PATH];		// tag name
	float			origin[3];
	dorientation_t	orient;
} dmd3tag_t;

typedef struct 
{
	char			name[MD3_MAX_PATH];
	int				unused;					// shader
} dmd3skin_t;

typedef struct
{
    char			id[4];

    char			name[MD3_MAX_PATH];

	int				flags;

    int				num_frames;
    int				num_skins;
    int				num_verts;
    int				num_tris;

    int				ofs_tris;
    int				ofs_skins;
    int				ofs_tcs;
    int				ofs_verts;

    int				meshsize;
} dmd3mesh_t;

typedef struct
{
    int				id;
    int				version;

    char			filename[MD3_MAX_PATH];

	int				flags;

    int				num_frames;
    int				num_tags;
    int				num_meshes;
    int				num_skins;

    int				ofs_frames;
    int				ofs_tags;
    int				ofs_meshes;
    int				ofs_end;
} dmd3_t;

/*
========================================================================

.SPR sprite file format, from Quake source

========================================================================
*/

#define IDSPRHEADER		(('P'<<24)+('S'<<16)+('D'<<8)+'I')
		// little-endian "IDSP"
#define SPR_VERSION		1
#define SPR32_VERSION	32

#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4

typedef enum { SPR_SINGLE=0, SPR_GROUP } spr1_frametype_t;

typedef struct
{
	spr1_frametype_t	type;
} dspr1_frametype_t;

typedef struct
{
	int		origin[2];
	int		width, height;
} dspr1_frame_t;

typedef struct
{
	int		num_frames;
} dspr1_group_t;

typedef struct
{
	float	interval;
} dspr1_interval_t;

typedef struct
{
	int				ident;
	int				version;
	int				type;
	float			bounding_radius;
	int				width, height;
	int				num_frames;
	float			beam_length;
	mdl_synctype_t	synctype;
} dspr1_t;

/*
========================================================================

.SP2 sprite file format

========================================================================
*/

#define IDSP2HEADER	(('2'<<24)+('S'<<16)+('D'<<8)+'I')
		// little-endian "IDS2"
#define SP2_VERSION		2

typedef struct
{
	int				width, height;
	int				origin_x, origin_y;		// raster coordinates inside pic
	char			name[MD2_MAX_SKINNAME];	// name of pcx file
} dspr2frame_t;

typedef struct {
	int				ident;
	int				version;
	int				numframes;
	dspr2frame_t	frames[1];				// variable sized
} dspr2_t;

/*
==============================================================================

  .WAL texture file format

==============================================================================
*/

#define	MIPLEVELS	4
typedef struct miptex_s
{
	char		name[32];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
	char		animname[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
} miptex_t;

/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define IDBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'I')
		// little-endian "IBSP"

#define Q2_BSPVERSION	38

// upper design bounds
// leaffaces, leafbrushes, planes, and verts are bounded by
// 16 bit short limits
#define	MAX_MAP_MODELS		1024

#define	MAX_MAP_BRUSHES		16384	// was 8192

#define	MAX_MAP_ENTITIES	2048	// unused by engine

#define	MAX_MAP_ENTSTRING	0x80000	// was 0x40000

#define	MAX_MAP_TEXINFO		65536	// was 8192, 16384
#define	MAX_MAP_AREAS		256
#define	MAX_MAP_AREAPORTALS	1024
#define	MAX_MAP_PLANES		65536
#define	MAX_MAP_NODES		65536
#define	MAX_MAP_BRUSHSIDES	65536
#define	MAX_MAP_LEAFS		65536
#define	MAX_MAP_VERTS		65536
#define	MAX_MAP_FACES		65536
#define	MAX_MAP_LEAFFACES	65536
#define	MAX_MAP_LEAFBRUSHES 65536
#define	MAX_MAP_PORTALS		65536
#define	MAX_MAP_EDGES		128000
#define	MAX_MAP_SURFEDGES	256000
#define	MAX_MAP_LIGHTING	0x2000000	// Knightmare increased, was 0x200000
#define	MAX_MAP_VISIBILITY	0x1000000	// Knightmare increased, was 0x100000


// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================

typedef struct
{
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP			16
#define	LUMP_AREAS			17
#define	LUMP_AREAPORTALS	18
#define	HEADER_LUMPS		19

typedef struct
{
	int			ident;
	int			version;	
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// submodels just draw faces
										// without walking the bsp tree
} dmodel_t;


typedef struct
{
	float	point[3];
} dvertex_t;


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

// planes (x&~1) and (x&~1)+1 are always opposites

typedef struct
{
	float	normal[3];
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// these definitions also need to be in q_shared.h!

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_MIST			64
#define	CONTENTS_FOG			1024	// fog
#define	LAST_VISIBLE_CONTENTS	1024	// was 64

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000



#define	SURF_LIGHT		0x1		// value will hold the light strength

#define	SURF_SLICK		0x2		// effects game physics

#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	// scroll towards angle
#define	SURF_NODRAW		0x80	// don't bother referencing the texture

#define SURF_NOLIGHTENV	0x01000000	// no lightmap or envmap
#define SURF_ALPHATEST	0x02000000	// alpha test flag
#define SURF_FOGPLANE	0x04000000	// fog surface


typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling // change to int
	short		maxs[3];								// change to int
	unsigned short	firstface;				// change to int
	unsigned short	numfaces;	// counting both sides //change to int
} dnode_t;


typedef struct texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			flags;			// miptex flags + overrides
	int			value;			// light emission, etc
	char		texture[32];	// texture name (textures/*.wal)
	int			nexttexinfo;	// for animations, -1 = end of chain
} texinfo_t;


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers //change to int
} dedge_t;

#define	MAXLIGHTMAPS	4
typedef struct
{
	unsigned short	planenum;	// change to int
	short		side;

	int			firstedge;		// we must support > 64k edges
	short		numedges;		
	short		texinfo;

// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
} dface_t;

typedef struct
{
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;
	short			area;

	short			mins[3];			// for frustum culling // change to int
	short			maxs[3];				// change to int

	unsigned short	firstleafface;		// change to int
	unsigned short	numleaffaces;		// change to int

	unsigned short	firstleafbrush;		// change to int
	unsigned short	numleafbrushes;		// change to int
} dleaf_t;

typedef struct
{
	unsigned short	planenum;		// facing out of the leaf	// change to int
	short	texinfo;
} dbrushside_t;

typedef struct
{
	int			firstside;
	int			numsides;
	int			contents;
} dbrush_t;

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
typedef struct
{
	int			numclusters;
	int			bitofs[8][2];	// bitofs[numclusters][2]
} dvis_t;

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
typedef struct
{
	int		portalnum;
	int		otherarea;
} dareaportal_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
} darea_t;


// Knightmare- below is upper bounds for Q3 maps
/*
//#define BSP_VERSION			46
#define Q3_BSP_VERSION 46
#define WOLF_BSP_VERSION 47

// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define MAX_MAP_MODELS 0x400
#define MAX_MAP_BRUSHES 0x8000
#define MAX_MAP_ENTITIES 0x800
#define MAX_MAP_ENTSTRING 0x40000
#define MAX_MAP_SHADERS 0x400

#define MAX_MAP_AREAS 0x100	// MAX_MAP_AREA_BYTES in q_shared must match!
#define MAX_MAP_FOGS 0x100
#define MAX_MAP_PLANES 0x20000
#define MAX_MAP_NODES 0x20000
#define MAX_MAP_BRUSHSIDES 0x40000	//%	0x20000	// ydnar
#define MAX_MAP_LEAFS 0x20000
#define MAX_MAP_LEAFFACES 0x20000
#define MAX_MAP_LEAFBRUSHES 0x40000
#define MAX_MAP_PORTALS 0x20000
#define MAX_MAP_LIGHTING 0x800000
#define MAX_MAP_LIGHTGRID 0x800000
#define MAX_MAP_VISIBILITY 0x200000

#define MAX_MAP_DRAW_SURFS 0x20000
#define MAX_MAP_DRAW_VERTS 0x80000
#define MAX_MAP_DRAW_INDEXES 0x80000

// the editor uses these predefined yaw angles to orient entities up or down
#define ANGLE_UP -1
#define ANGLE_DOWN -2

#define LIGHTMAP_WIDTH 128
#define LIGHTMAP_HEIGHT 128

#define MIN_WORLD_COORD (-65536)
#define MAX_WORLD_COORD (65536)
#define WORLD_SIZE (MAX_WORLD_COORD - MIN_WORLD_COORD)
*/
