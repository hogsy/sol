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

// menu_main.c -- the main menu & support functions
 
#include "../client/client.h"
#include "ui_local.h"

#define MAIN_MENU_USE_WIDGETS

#ifndef MAIN_MENU_USE_WIDGETS
static int	m_main_cursor;
int			MainMenuMouseHover;

// for checking if quad cursor model is available
#define QUAD_CURSOR_MODEL	"models/ui/quad_cursor.md2"
qboolean	quadModel_loaded;
#endif	// MAIN_MENU_USE_WIDGETS

/*
=======================================================================

MAIN MENU

=======================================================================
*/

#define MAIN_MENU_ITEM_SPACE 40

menuFramework_s			s_main_menu;
static menuImage_s		s_main_plaque;
static menuImage_s		s_main_logo;
static menuButton_s		s_main_game_button;
static menuButton_s		s_main_multiplayer_button;
static menuButton_s		s_main_options_button;
static menuButton_s		s_main_video_button;
//static menuButton_s		s_main_mods_button;
static menuButton_s		s_main_quit_button;
static menuModelView_s	s_main_cursor;

#ifndef MAIN_MENU_USE_WIDGETS

#define	MAIN_ITEMS	5

char *main_names[] =
{
	"m_main_game",
	"m_main_multiplayer",
	"m_main_options",
	"m_main_video",
	"m_main_quit",
	0
};


/*
=============
FindMenuCoords
=============
*/
void FindMenuCoords (int *xoffset, int *ystart, int *totalheight, int *widest)
{
	int w, h, i;

	*totalheight = 0;
	*widest = -1;

	for (i = 0; main_names[i] != 0; i++)
	{
		R_DrawGetPicSize (&w, &h, main_names[i]);
		if (w > *widest)
			*widest = w;
		*totalheight += (h + 12);
	}

	*xoffset = (SCREEN_WIDTH - *widest + 70) * 0.5;
	*ystart = SCREEN_HEIGHT*0.5 - 100;
}


/*
=============
M_DrawMainCursor

Draws an animating cursor with the point at
x,y.  The pic will extend to the left of x,
and both above and below y.
=============
*/
void M_DrawMainCursor (int x, int y, int f)
{
	char	cursorname[80];
	static	qboolean cached;
	int		w,h;

	if (!cached)
	{
		int i;

		for (i = 0; i < NUM_MAINMENU_CURSOR_FRAMES; i++) {
			Com_sprintf (cursorname, sizeof(cursorname), "m_cursor%d", i);
			R_DrawFindPic (cursorname);
		}
		cached = true;
	}

	Com_sprintf (cursorname, sizeof(cursorname), "m_cursor%d", f);
	R_DrawGetPicSize (&w, &h, cursorname);
	UI_DrawPic (x, y, w, h, ALIGN_CENTER, false, cursorname, 1.0);
}


/*
=============
M_DrawMainCursor3D

Draws a rotating quad damage model.
=============
*/
void M_DrawMainCursor3D (int x, int y)
{
	refdef_t	refdef;
	entity_t	quadEnt, *ent;
	float		rx, ry, rw, rh;
	int			yaw;

	yaw = anglemod(cl.time/10);

	memset(&refdef, 0, sizeof(refdef));
	memset (&quadEnt, 0, sizeof(quadEnt));

	// size 24x34
	rx = x;				ry = y;
	rw = 24;			rh = 34;
	SCR_ScaleCoords (&rx, &ry, &rw, &rh, ALIGN_CENTER);
	refdef.x = rx;		refdef.y = ry;
	refdef.width = rw;	refdef.height = rh;
	refdef.fov_x = 40;
	refdef.fov_y = CalcFov (refdef.fov_x, refdef.width, refdef.height);
	refdef.time = cls.realtime*0.001;
	refdef.areabits = 0;
	refdef.lightstyles = 0;
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.num_entities = 0;
	refdef.entities = &quadEnt;

	ent = &quadEnt;
	ent->model = R_RegisterModel (QUAD_CURSOR_MODEL);
	ent->flags = RF_FULLBRIGHT|RF_NOSHADOW|RF_DEPTHHACK;
	VectorSet (ent->origin, 40, 0, -18);
	VectorCopy( ent->origin, ent->oldorigin );
	ent->frame = 0;
	ent->oldframe = 0;
	ent->backlerp = 0.0;
	ent->angles[1] = yaw;
	refdef.num_entities++;

	R_RenderFrame( &refdef );
}


/*
=============
UI_CheckQuadModel

Checks for quad damage model.
=============
*/
void UI_CheckQuadModel (void)
{
	struct model_s *quadModel;

	quadModel = R_RegisterModel (QUAD_CURSOR_MODEL);
	
	quadModel_loaded = (quadModel != NULL);
}
#endif	// MAIN_MENU_USE_WIDGETS

//=======================================================================

/*
=============
Menu_Main_Draw
=============
*/
void Menu_Main_Draw (void)
{
#ifdef MAIN_MENU_USE_WIDGETS
	UI_AdjustMenuCursor (&s_main_menu, 1);
	UI_DrawMenu (&s_main_menu);
#else
	int	xoffset, ystart;
	int widest = -1, totalheight = 0;
	int i, w, h, last_h;
	char litname[80];

	FindMenuCoords (&xoffset, &ystart, &totalheight, &widest);

	for (i = 0; main_names[i] != 0; i++)
		if (i != m_main_cursor) {
			R_DrawGetPicSize (&w, &h, main_names[i]);
			UI_DrawPic (xoffset, (ystart + i*40+3), w, h, ALIGN_CENTER, false, main_names[i], 1.0);
		}

//	strncpy (litname, main_names[m_main_cursor]);
//	strncat (litname, "_sel");
	Q_strncpyz (litname, sizeof(litname), main_names[m_main_cursor]);
	Q_strncatz (litname, sizeof(litname), "_sel");
	R_DrawGetPicSize (&w, &h, litname);
	UI_DrawPic (xoffset-1, (ystart + m_main_cursor*40+2), w+2, h+2, ALIGN_CENTER, false, litname, 1.0);

	// Draw our nifty quad damage model as a cursor if it's loaded.
	if (quadModel_loaded)
		M_DrawMainCursor3D (xoffset-27, ystart+(m_main_cursor*40+1));
	else
		M_DrawMainCursor (xoffset-25, ystart+(m_main_cursor*40+1), (int)(cls.realtime/100)%NUM_MAINMENU_CURSOR_FRAMES);

	R_DrawGetPicSize (&w, &h, "m_main_plaque");
	UI_DrawPic (xoffset-(w/2+50), ystart, w, h, ALIGN_CENTER, false, "m_main_plaque", 1.0);
	last_h = h;

	R_DrawGetPicSize (&w, &h, "m_main_logo");
	UI_DrawPic (xoffset-(w/2+50), ystart+last_h+20, w, h, ALIGN_CENTER, false, "m_main_logo", 1.0);
#endif	// MAIN_MENU_USE_WIDGETS
}


#ifndef MAIN_MENU_USE_WIDGETS
/*
=============
OpenMenuFromMain
=============
*/
void OpenMenuFromMain (void)
{
	switch (m_main_cursor)
	{
		case 0:
			Menu_Game_f ();
			break;

		case 1:
			Menu_Multiplayer_f();
			break;

		case 2:
			Menu_Options_f ();
			break;

		case 3:
			Menu_Video_f ();
			break;

		case 4:
			Menu_Quit_f ();
			break;
	}
}


/*
=============
UI_CheckMainMenuMouse
=============
*/
void UI_CheckMainMenuMouse (void)
{
	int ystart;
	int	xoffset;
	int widest;
	int totalheight;
	int i, oldhover;
	char *sound = NULL;
	mainmenuobject_t buttons[MAIN_ITEMS];

	oldhover = MainMenuMouseHover;
	MainMenuMouseHover = 0;

	FindMenuCoords(&xoffset, &ystart, &totalheight, &widest);
	for (i = 0; main_names[i] != 0; i++)
		UI_AddMainButton (&buttons[i], i, xoffset, ystart+(i*40+3), main_names[i]);

	// Exit with double click 2nd mouse button
	if (!ui_mousecursor.buttonused[MOUSEBUTTON2] && ui_mousecursor.buttonclicks[MOUSEBUTTON2]==2)
	{
		UI_PopMenu();
		sound = ui_menu_out_sound;
		ui_mousecursor.buttonused[MOUSEBUTTON2] = true;
		ui_mousecursor.buttonclicks[MOUSEBUTTON2] = 0;
	}

	for (i=MAIN_ITEMS-1; i>=0; i--)
	{
		if ( (ui_mousecursor.x >= buttons[i].min[0]) && (ui_mousecursor.x <= buttons[i].max[0]) &&
			(ui_mousecursor.y >= buttons[i].min[1]) && (ui_mousecursor.y <= buttons[i].max[1]) )
		{
			if (ui_mousecursor.mouseaction)
				m_main_cursor = i;

			MainMenuMouseHover = 1 + i;

			if (oldhover == MainMenuMouseHover && MainMenuMouseHover-1 == m_main_cursor &&
				!ui_mousecursor.buttonused[MOUSEBUTTON1] && ui_mousecursor.buttonclicks[MOUSEBUTTON1]==1)
			{
				OpenMenuFromMain();
				sound = ui_menu_move_sound;
				ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
				ui_mousecursor.buttonclicks[MOUSEBUTTON1] = 0;
			}
			break;
		}
	}

	if (!MainMenuMouseHover)
	{
		ui_mousecursor.buttonused[MOUSEBUTTON1] = false;
		ui_mousecursor.buttonclicks[MOUSEBUTTON1] = 0;
		ui_mousecursor.buttontime[MOUSEBUTTON1] = 0;
	}

	ui_mousecursor.mouseaction = false;

	if (sound)
		S_StartLocalSound(sound);
}
#endif	// MAIN_MENU_USE_WIDGETS


/*
=============
Menu_Main_Key
=============
*/
const char *Menu_Main_Key (int key)
{
#ifdef MAIN_MENU_USE_WIDGETS
	return UI_DefaultMenuKey (&s_main_menu, key);
#else
	const char *sound = ui_menu_move_sound;

	switch (key)
	{
	case K_ESCAPE:
#ifdef ERASER_COMPAT_BUILD // special hack for Eraser build
		if (cls.state == ca_disconnected)
			Menu_Quit_f ();
		else
			UI_PopMenu ();
#else	// can't exit menu if disconnected,
		// so restart demo loop
		if (cls.state == ca_disconnected)
			Cbuf_AddText ("d1\n");
		UI_PopMenu ();
#endif
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		return sound;

	case K_KP_UPARROW:
	case K_UPARROW:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		return sound;

	case K_KP_ENTER:
	case K_ENTER:
		ui_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			Menu_Game_f ();
			break;

		case 1:
			Menu_Multiplayer_f();
			break;

		case 2:
			Menu_Options_f ();
			break;

		case 3:
			Menu_Video_f ();
			break;

		case 4:
			Menu_Quit_f ();
			break;
		}
	}
	return NULL;
#endif	// MAIN_MENU_USE_WIDGETS
}

//=======================================================================

static void M_MainGameFunc (void *unused)
{
	Menu_Game_f ();
}

static void M_MainMultiplayerFunc (void *unused)
{
	Menu_Multiplayer_f ();
}

static void M_MainOptionsFunc (void *unused)
{
	Menu_Options_f ();
}

static void M_MainVideoFunc (void *unused)
{
	Menu_Video_f ();
}

/*static void M_MainModsFunc (void *unused)
{
	Menu_Mods_f ();
} */

static void M_MainQuitFunc (void *unused)
{
	Menu_Quit_f ();
}

//=======================================================================

void Menu_Main_Init (void)
{
	int		x, y;

	// menu.x = 247, menu.y = 140
	x = 247;
	y = 143;

	s_main_menu.x							= 0;	// was 247
	s_main_menu.y							= 0;	// was 140
	s_main_menu.nitems						= 0;
	s_main_menu.hide_statusbar				= true;
//	s_main_menu.isPopup						= false;
//	s_main_menu.keyFunc						= UI_DefaultMenuKey;
//	s_main_menu.canOpenFunc					= NULL;
//	s_main_menu.cursordraw					= M_DrawMainCursor;
	s_main_menu.cursorItem					= &s_main_cursor;

#ifdef MAIN_MENU_USE_WIDGETS
	s_main_game_button.generic.type			= MTYPE_BUTTON;
	s_main_game_button.generic.x			= x;
	s_main_game_button.generic.y			= y;
	s_main_game_button.width				= 99;
	s_main_game_button.height				= 32;
	s_main_game_button.imageName			= "/pics/m_main_game.pcx";
	s_main_game_button.hoverImageName		= "/pics/m_main_game_sel.pcx";
	s_main_game_button.alpha				= 255;
	s_main_game_button.border				= 0;
	s_main_game_button.generic.isHidden		= false;
	s_main_game_button.usesMouse2			= false;
	s_main_game_button.generic.callback		= M_MainGameFunc;
//	s_main_game_button.generic.statusbar	= "start a new game, or load/save a game";

	s_main_multiplayer_button.generic.type		= MTYPE_BUTTON;
	s_main_multiplayer_button.generic.x			= x;
	s_main_multiplayer_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_multiplayer_button.width				= 215;
	s_main_multiplayer_button.height			= 32;
	s_main_multiplayer_button.imageName			= "/pics/m_main_multiplayer.pcx";
	s_main_multiplayer_button.hoverImageName	= "/pics/m_main_multiplayer_sel.pcx";
	s_main_multiplayer_button.alpha				= 255;
	s_main_multiplayer_button.border			= 0;
	s_main_multiplayer_button.generic.isHidden	= false;
	s_main_multiplayer_button.usesMouse2		= false;
	s_main_multiplayer_button.generic.callback	= M_MainMultiplayerFunc;
//	s_main_multiplayer_button.generic.statusbar	= "join a server, start a server, or player setup";

	s_main_options_button.generic.type		= MTYPE_BUTTON;
	s_main_options_button.generic.x			= x;
	s_main_options_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_options_button.width				= 150;
	s_main_options_button.height			= 32;
	s_main_options_button.imageName			= "/pics/m_main_options.pcx";
	s_main_options_button.hoverImageName	= "/pics/m_main_options_sel.pcx";
	s_main_options_button.alpha				= 255;
	s_main_options_button.border			= 0;
	s_main_options_button.generic.isHidden	= false;
	s_main_options_button.usesMouse2		= false;
	s_main_options_button.generic.callback	= M_MainOptionsFunc;
//	s_main_options_button.generic.statusbar	= "change sound, controls, HUD, effects, and menu settings";

	s_main_video_button.generic.type		= MTYPE_BUTTON;
	s_main_video_button.generic.x			= x;
	s_main_video_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_video_button.width				= 118;
	s_main_video_button.height				= 32;
	s_main_video_button.imageName			= "/pics/m_main_video.pcx";
	s_main_video_button.hoverImageName		= "/pics/m_main_video_sel.pcx";
	s_main_video_button.alpha				= 255;
	s_main_video_button.border				= 0;
	s_main_video_button.generic.isHidden	= false;
	s_main_video_button.usesMouse2			= false;
	s_main_video_button.generic.callback	= M_MainVideoFunc;
//	s_main_video_button.generic.statusbar	= "change display/graphics settings";
/*
	s_main_mods_button.generic.type			= MTYPE_BUTTON;
	s_main_mods_button.generic.x			= x;
	s_main_mods_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_mods_button.width				= 96;
	s_main_mods_button.height				= 32;
	s_main_mods_button.imageName			= "/pics/m_main_mods.pcx";
	s_main_mods_button.hoverImageName		= "/pics/m_main_mods_sel.pcx";
	s_main_mods_button.alpha				= 255;
	s_main_mods_button.border				= 0;
	s_main_mods_button.generic.isHidden		= false;
	s_main_mods_button.usesMouse2			= false;
	s_main_mods_button.generic.callback		= M_MainModsFunc;
//	s_main_mods_button.generic.statusbar	= "load a modification";
*/
	s_main_quit_button.generic.type			= MTYPE_BUTTON;
	s_main_quit_button.generic.x			= x;
	s_main_quit_button.generic.y			= y+=MAIN_MENU_ITEM_SPACE;
	s_main_quit_button.width				= 100;
	s_main_quit_button.height				= 32;
	s_main_quit_button.imageName			= "/pics/m_main_quit.pcx";
	s_main_quit_button.hoverImageName		= "/pics/m_main_quit_sel.pcx";
	s_main_quit_button.alpha				= 255;
	s_main_quit_button.border				= 0;
	s_main_quit_button.generic.isHidden		= false;
	s_main_quit_button.usesMouse2			= false;
	s_main_quit_button.generic.callback		= M_MainQuitFunc;
//	s_main_quit_button.generic.statusbar	= "exit Quake II";

	s_main_cursor.generic.type				= MTYPE_MODELVIEW;
	s_main_cursor.generic.x					= 220; // -27
	s_main_cursor.generic.y					= 142; // -1
	s_main_cursor.generic.name				= 0;
	s_main_cursor.generic.isCursorItem		= true;
	Vector2Set (s_main_cursor.generic.cursorItemOffset, -27, -1);
	s_main_cursor.width						= 24;
	s_main_cursor.height					= 34;
	s_main_cursor.fov						= 40;
	s_main_cursor.isMirrored				= false;
	s_main_cursor.modelName[0]				= "models/ui/quad_cursor.md2";
	VectorSet (s_main_cursor.modelOrigin[0], 40, 0, -18);
	VectorSet (s_main_cursor.modelBaseAngles[0], 0, 0, 0);
	VectorSet (s_main_cursor.modelRotation[0], 0, 0.1, 0);
	s_main_cursor.modelFrame[0]				= 0;
	s_main_cursor.modelFrameNumbers[0]		= 0;
	s_main_cursor.entFlags[0]				= RF_FULLBRIGHT|RF_NOSHADOW|RF_DEPTHHACK;
	s_main_cursor.generic.isHidden			= false;

	s_main_plaque.generic.type				= MTYPE_IMAGE;
	s_main_plaque.generic.x					= x - 69;
	s_main_plaque.generic.y					= 140;	// y-163, 0
	s_main_plaque.width						= 38;
	s_main_plaque.height					= 166;
	s_main_plaque.imageName					= "/pics/m_main_plaque.pcx";
	s_main_plaque.alpha						= 255;
	s_main_plaque.border					= 0;
	s_main_plaque.hCentered					= false;
	s_main_plaque.vCentered					= false;
	s_main_plaque.generic.isHidden			= false;

	s_main_logo.generic.type				= MTYPE_IMAGE;
	s_main_logo.generic.x					= x - 68;
	s_main_logo.generic.y					= 326; // y+23, 186
	s_main_logo.width						= 36;
	s_main_logo.height						= 42;
	s_main_logo.imageName					= "/pics/m_main_logo.pcx";
	s_main_logo.alpha						= 255;
	s_main_logo.border						= 0;
	s_main_logo.hCentered					= false;
	s_main_logo.vCentered					= false;
	s_main_logo.generic.isHidden			= false;

	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_game_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_multiplayer_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_options_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_video_button);
//	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_mods_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_quit_button);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_cursor);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_plaque);
	UI_AddMenuItem (&s_main_menu, ( void * ) &s_main_logo);
#endif	// MAIN_MENU_USE_WIDGETS
}


void Menu_Main_f (void)
{
	Menu_Main_Init ();
#ifndef MAIN_MENU_USE_WIDGETS
	UI_CheckQuadModel ();
#endif
	UI_PushMenu (&s_main_menu, Menu_Main_Draw, Menu_Main_Key);
}
