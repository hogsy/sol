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
 
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client/client.h"
#include "ui_local.h"

static int	m_main_cursor;
int			MainMenuMouseHover;

// for checking if quad cursor model is available
#define QUAD_CURSOR_MODEL	"models/ui/quad_cursor.md2"
qboolean	quadModel_loaded;


/*
=======================================================================

MAIN MENU

=======================================================================
*/

menuFramework_s			s_main_menu;

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
UI_DrawMainCursor

Draws an animating cursor with the point at
x,y.  The pic will extend to the left of x,
and both above and below y.
=============
*/
void UI_DrawMainCursor (int x, int y, int f)
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
UI_DrawMainCursor3D

Draws a rotating quad damage model.
=============
*/
void UI_DrawMainCursor3D (int x, int y)
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


/*
=============
Menu_Main_Draw
=============
*/
void Menu_Main_Draw (void)
{
	int i;
	int w, h, last_h;
	int ystart;
	int	xoffset;
	int widest = -1;
	int totalheight = 0;
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
		UI_DrawMainCursor3D (xoffset-27, ystart+(m_main_cursor*40+1));
	else
		UI_DrawMainCursor (xoffset-25, ystart+(m_main_cursor*40+1), (int)(cls.realtime/100)%NUM_MAINMENU_CURSOR_FRAMES);

	R_DrawGetPicSize (&w, &h, "m_main_plaque");
	UI_DrawPic (xoffset-(w/2+50), ystart, w, h, ALIGN_CENTER, false, "m_main_plaque", 1.0);
	last_h = h;

	R_DrawGetPicSize (&w, &h, "m_main_logo");
	UI_DrawPic (xoffset-(w/2+50), ystart+last_h+20, w, h, ALIGN_CENTER, false, "m_main_logo", 1.0);
}


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


/*
=============
Menu_Main_Key
=============
*/
const char *Menu_Main_Key (int key)
{
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
}

void Menu_Main_Init (void)
{
	int		x, y;

	// menu.x = 247, menu.y = 140
	x = 247;
	y = 143;

	s_main_menu.x							= 0;	// was 247
	s_main_menu.y							= 0;	// was 140
	s_main_menu.nitems						= 0;
//	s_main_menu.hide_statusbar				= true;
//	s_main_menu.isPopup						= false;
//	s_main_menu.keyFunc						= UI_DefaultMenuKey;
//	s_main_menu.canOpenFunc					= NULL;
//	s_main_menu.cursordraw					= M_DrawMainCursor;
//	s_main_menu.cursorItem					= &s_main_cursor;
}


void Menu_Main_f (void)
{
	Menu_Main_Init ();
	UI_CheckQuadModel ();
	UI_PushMenu (&s_main_menu, Menu_Main_Draw, Menu_Main_Key);
}
