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

// ui_draw.c -- object drawing functions for the menus

#include "../client/client.h"
#include "ui_local.h"

/*
=======================================================================

	MISC DRAWING FUNCS

=======================================================================
*/

/*
==========================
UI_DrawMenuString
==========================
*/
void UI_DrawMenuString (int x, int y, int size, scralign_t align, const char *string, int alpha, qboolean R2L, qboolean altColor)
{
	char	newstring[1024];

	if (altColor)
		snprintf (newstring, sizeof(newstring), S_COLOR_ALT"%s", string);
	else
		snprintf (newstring, sizeof(newstring), "%s", string);

	if (R2L)
		x -= stringLen(string)*size;
	UI_DrawString (x, y, size, align, newstring, FONT_UI, alpha);
}


/*
==========================
UI_DrawMenuStatusBar
==========================
*/
void UI_DrawMenuStatusBar (const char *string)
{
	if (string)
	{
		int l = (int)strlen(string);

		UI_DrawFill (0, SCREEN_HEIGHT-(MENU_FONT_SIZE+3), SCREEN_WIDTH, MENU_FONT_SIZE+3, ALIGN_BOTTOM_STRETCH, true, 60,60,60,255);
		UI_DrawFill (0, SCREEN_HEIGHT-(MENU_FONT_SIZE+3), SCREEN_WIDTH, 1, ALIGN_BOTTOM_STRETCH, false, 0,0,0,255);
		UI_DrawString (SCREEN_WIDTH/2-(l/2)*MENU_FONT_SIZE, SCREEN_HEIGHT-(MENU_FONT_SIZE+1), MENU_FONT_SIZE, ALIGN_BOTTOM, string, FONT_UI, 255);
	}
	else
		UI_DrawFill (0, SCREEN_HEIGHT-(MENU_FONT_SIZE+3), SCREEN_WIDTH, MENU_FONT_SIZE+3, ALIGN_BOTTOM_STRETCH, true, 0,0,0,255);
}


/*
==========================
UI_DrawMenuNullCursor
==========================
*/
void UI_DrawMenuNullCursor (void *self)
{
}


/*
==========================
UI_DrawMenuTextBox
==========================
*/
void UI_DrawMenuTextBox (int x, int y, int width, int lines)
{
	int		cx, cy;
	int		n;
	vec4_t	stCoord_textBox[9];

	Vector4Set (stCoord_textBox[0], 0.0, 0.0, 0.25, 0.25);
	Vector4Set (stCoord_textBox[1], 0.0, 0.25, 0.25, 0.5);
	Vector4Set (stCoord_textBox[2], 0.0, 0.5, 0.25, 0.75);
	Vector4Set (stCoord_textBox[3], 0.25, 0.0, 0.5, 0.25);
	Vector4Set (stCoord_textBox[4], 0.25, 0.25, 0.5, 0.5);
	Vector4Set (stCoord_textBox[5], 0.25, 0.5, 0.5, 0.75);
	Vector4Set (stCoord_textBox[6], 0.5, 0.0, 0.75, 0.25);
	Vector4Set (stCoord_textBox[7], 0.5, 0.25, 0.75, 0.5);
	Vector4Set (stCoord_textBox[8], 0.5, 0.5, 0.75, 0.75);

	// draw left side
	cx = x;
	cy = y;
	if (ui_new_textbox->integer)
		UI_DrawPicST (cx, cy, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[0], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
	else
		UI_DrawChar (cx, cy, MENU_FONT_SIZE, ALIGN_CENTER, 1, 255, 255, 255, 255, false, false);
	for (n = 0; n < lines; n++)
	{
		cy += MENU_FONT_SIZE;
		if (ui_new_textbox->integer)
			UI_DrawPicST (cx, cy, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[1], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
		else
			UI_DrawChar (cx, cy, MENU_FONT_SIZE, ALIGN_CENTER, 4, 255, 255, 255, 255, false, false);
	}
	if (ui_new_textbox->integer)
		UI_DrawPicST (cx, cy+MENU_FONT_SIZE, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[2], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
	else
		UI_DrawChar (cx, cy+MENU_FONT_SIZE, MENU_FONT_SIZE, ALIGN_CENTER, 7, 255, 255, 255, 255, false, false);

	// draw middle
	cx += MENU_FONT_SIZE;
	while (width > 0)
	{
		cy = y;
		if (ui_new_textbox->integer)
			UI_DrawPicST (cx, cy, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[3], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
		else
			UI_DrawChar (cx, cy, MENU_FONT_SIZE, ALIGN_CENTER, 2, 255, 255, 255, 255, false, false);
		for (n = 0; n < lines; n++)
		{
			cy += MENU_FONT_SIZE;
			if (ui_new_textbox->integer)
				UI_DrawPicST (cx, cy, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[4], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
			else
				UI_DrawChar (cx, cy, MENU_FONT_SIZE, ALIGN_CENTER, 5, 255, 255, 255, 255, false, false);
		}
		if (ui_new_textbox->integer)
			UI_DrawPicST (cx, cy+MENU_FONT_SIZE, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[5], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
		else
			UI_DrawChar (cx, cy+MENU_FONT_SIZE, MENU_FONT_SIZE, ALIGN_CENTER, 8, 255, 255, 255, 255, false, false);
		width -= 1;
		cx += MENU_FONT_SIZE;
	}

	// draw right side
	cy = y;
	if (ui_new_textbox->integer)
		UI_DrawPicST (cx, cy, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[6], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
	else
		UI_DrawChar (cx, cy, MENU_FONT_SIZE, ALIGN_CENTER, 3, 255, 255, 255, 255, false, false);
	for (n = 0; n < lines; n++)
	{
		cy += MENU_FONT_SIZE;
		if (ui_new_textbox->integer)
			UI_DrawPicST (cx, cy, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[7], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
		else
			UI_DrawChar (cx, cy, MENU_FONT_SIZE, ALIGN_CENTER, 6, 255, 255, 255, 255, false, false);
	}
	if (ui_new_textbox->integer)
		UI_DrawPicST (cx, cy+MENU_FONT_SIZE, MENU_FONT_SIZE, MENU_FONT_SIZE, stCoord_textBox[8], ALIGN_CENTER, true, color_identity, UI_TEXTBOX_PIC);
	else
		UI_DrawChar (cx, cy+MENU_FONT_SIZE, MENU_FONT_SIZE, ALIGN_CENTER, 9, 255, 255, 255, 255, false, true);
}


/*
==========================
UI_DrawPopupMessage
==========================
*/
void UI_DrawPopupMessage (char *message)
{
	int		i, x, y, numlines=0, width, widest=0, padding, boxwidth;
	char	buffer[1024], *parse, *tok;

	if (!message)	return;

	memset (&buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), message);

	// get max width and num of lines
	parse = buffer;
	while (*parse)
	{
		width = 0;
		while (*parse && *parse != '\n') {
			width++;	parse++;
		}
		numlines++;
		widest = max(widest, width);

		if (*parse)
			parse++;	// skip the \n
	}
	padding = (widest % 2 == 0) ? 2 : 3;
	boxwidth = min(widest + padding, SCREEN_WIDTH/MENU_FONT_SIZE-2);
	numlines = min(numlines, SCREEN_HEIGHT/MENU_FONT_SIZE-2);

	// draw the box
	x = SCREEN_WIDTH/2 - ((boxwidth+2)*MENU_FONT_SIZE/2);
	y = SCREEN_HEIGHT/2 -((numlines+2)*MENU_FONT_SIZE/2);
	UI_DrawMenuTextBox (x, y, boxwidth, numlines);

	// draw the text
	parse = buffer;
	for (i=0, tok = strtok (parse, "\n"); i<numlines, tok != NULL; i++, tok = strtok (NULL, "\n"))
		UI_DrawMenuString (x+((padding+2)*MENU_FONT_SIZE/2), y+MENU_FONT_SIZE*(i+1), MENU_FONT_SIZE,
							ALIGN_CENTER, va("%s%s",S_COLOR_ALT,tok), 255, false, false);

	// the text box won't show up unless we do a buffer swap
	R_EndFrame ();
}


/*
=================
UI_Draw_Cursor
=================
*/
#if 1
void UI_Draw_Cursor (void)
{
	float	scale = SCR_ScaledScreen(ui_cursor_scale->value); // 0.4
	char	*cur_img = UI_MOUSECURSOR_PIC;

	SCR_DrawScaledPic (ui_mousecursor.x, ui_mousecursor.y, scale, true, false, cur_img, 1.0f);
}
#else
void UI_Draw_Cursor (void)
{
	float	alpha = 1.0f, scale = SCR_ScaledScreen(0.66f);
	char	*overlay = NULL;
	char	*cur_img = NULL;

	if (ui_mousecursor.menuitem)
	{
	/*if (ui_mousecursor.menuitemtype == MENUITEM_TEXT)
			cur_img = UI_MOUSECURSOR_TEXT_PIC;
		else
		{*/
			if ( ((ui_mousecursor.buttonused[0] && ui_mousecursor.buttonclicks[0])
				|| (ui_mousecursor.buttonused[1] && ui_mousecursor.buttonclicks[1]))
				&& ui_mousecursor.menuitemtype != MENUITEM_TEXT)
			{
				cur_img = UI_MOUSECURSOR_CLICK_PIC;
				alpha = 0.85 + 0.15*sin(anglemod(cl.time*0.005f));
			}
			else
			{
				cur_img = UI_MOUSECURSOR_HOVER_PIC;
				alpha = 0.85 + 0.15*sin(anglemod(cl.time*0.005f));
			}
			overlay = UI_MOUSECURSOR_OVER_PIC;
	//	}
	}
	else
	{
		cur_img = UI_MOUSECURSOR_MAIN_PIC;
		overlay = UI_MOUSECURSOR_OVER_PIC;
	}

	if (cur_img)
	{
		SCR_DrawScaledPic (ui_mousecursor.x, ui_mousecursor.y, scale, true, false, cur_img, alpha);
		if (overlay)
			SCR_DrawScaledPic (ui_mousecursor.x, ui_mousecursor.y, scale, true, false, overlay, 1.0f);
	}
}
#endif
