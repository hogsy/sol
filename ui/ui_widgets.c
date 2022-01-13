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

// ui_widgets.c -- supporting code for menu widgets

#include "../client/client.h"
#include "ui_local.h"

static	void	UI_MenuAction_DoEnter (menuAction_s *a);
static	void	UI_MenuAction_Draw (menuAction_s *a);
static	void	UI_MenuLabel_Draw (menuLabel_s *s);
static	void	UI_MenuSlider_DoSlide (menuSlider_s *s, int dir);
static	void	UI_MenuSlider_Draw (menuSlider_s *s);
static	void	UI_MenuPicker_DoEnter (menuPicker_s *p);
static	void	UI_MenuPicker_Draw (menuPicker_s *p);
static	void	UI_MenuPicker_DoSlide (menuPicker_s *p, int dir);

#define RCOLUMN_OFFSET  MENU_FONT_SIZE*2	// was 16
#define LCOLUMN_OFFSET -MENU_FONT_SIZE*2	// was -16

vec4_t		stCoord_arrow_left = {0.0, 0.0, 0.25, 0.25};
vec4_t		stCoord_arrow_right = {0.25, 0.0, 0.5, 0.25};
vec4_t		stCoord_arrow_up = {0.5, 0.0, 0.75, 0.25};
vec4_t		stCoord_arrow_down = {0.75, 0.0, 1, 0.25};
vec4_t		stCoord_scrollKnob_h = {0.0, 0.75, 0.25, 1.0};
vec4_t		stCoord_scrollKnob_v = {0.25, 0.75, 0.5, 1.0};
vec4_t		stCoord_field_left = {0.0, 0.0, 0.25, 1.0};
vec4_t		stCoord_field_center = {0.25, 0.0, 0.5, 1.0};
vec4_t		stCoord_field_right = {0.5, 0.0, 0.75, 1.0};
vec4_t		stCoord_slider_left = {0.0, 0.0, 0.125, 1.0};
vec4_t		stCoord_slider_center = {0.125, 0.0, 0.375, 1.0};
vec4_t		stCoord_slider_right = {0.375, 0.0, 0.5, 1.0};
vec4_t		stCoord_slider_knob = {0.5, 0.0, 0.625, 1.0};

extern viddef_t viddef;

#define VID_WIDTH viddef.width
#define VID_HEIGHT viddef.height

//======================================================

void UI_MenuCommon_DrawItemName (menuCommon_s *c, int nameX, int nameY, int headerX, int headerY, int hoverAlpha)
{
	qboolean	usesName = (c->name && strlen(c->name) > 0);
	qboolean	usesHeader = (c->header && strlen(c->header) > 0);

	if (usesName)
		UI_DrawMenuString (c->topLeft[0] + nameX, c->topLeft[1] +nameY,
							MENU_FONT_SIZE, c->scrAlign, c->name, hoverAlpha, true, true);

	if (usesHeader)
		UI_DrawMenuString (c->topLeft[0] + headerX, c->topLeft[1] + headerY,
							MENU_FONT_SIZE, c->scrAlign, c->header, (!usesName) ? hoverAlpha : 255, false, true);
}

//======================================================

void UI_MenuAction_DoEnter (menuAction_s *a)
{
	if (!a) return;

	if (a->generic.callback)
		a->generic.callback (a);
}

char *UI_MenuAction_Click (menuAction_s *a, qboolean mouse2)
{
	// return if it's just a mouse2 click
	if (!a->usesMouse2 && mouse2)
		return ui_menu_null_sound;

//	UI_MenuAction_DoEnter (a);
	if ( mouse2 && a->generic.mouse2Callback )
		a->generic.mouse2Callback (a);
	else if (a->generic.callback)
		a->generic.callback (a);

	return ui_menu_move_sound;
}

void UI_MenuAction_Draw (menuAction_s *a)
{
	int		alpha;

	if (!a) return;

	alpha = UI_MouseOverAlpha(&a->generic);

	if (a->generic.flags & QMF_LEFT_JUSTIFY)
	{
		if (a->generic.flags & QMF_GRAYED)
			UI_DrawMenuString (a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET,
			a->generic.y + a->generic.parent->y, a->generic.textSize, a->generic.scrAlign, a->generic.name, alpha, false, true);
		else
			UI_DrawMenuString (a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET,
							a->generic.y + a->generic.parent->y, a->generic.textSize, a->generic.scrAlign, a->generic.name, alpha, false, false);
	}
	else
	{
		if (a->generic.flags & QMF_GRAYED)
			UI_DrawMenuString (a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET,
								a->generic.y + a->generic.parent->y, a->generic.textSize, a->generic.scrAlign, a->generic.name, alpha, true, true);
		else
			UI_DrawMenuString (a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET,
								a->generic.y + a->generic.parent->y, a->generic.textSize, a->generic.scrAlign, a->generic.name, alpha, true, false);
	}
	if (a->generic.ownerdraw)
		a->generic.ownerdraw(a);
}

void UI_MenuAction_Setup (menuAction_s *a)
{
	menuFramework_s	*menu = a->generic.parent;
	int				len;

	len = (a->generic.name) ? (int)strlen(a->generic.name) : 0;
	// set min and max coords
	if (a->generic.flags & QMF_LEFT_JUSTIFY) {
		a->generic.topLeft[0] = menu->x + a->generic.x + LCOLUMN_OFFSET;
		a->generic.botRight[0] = a->generic.topLeft[0] + len*a->generic.textSize;
	}
	else {
		a->generic.topLeft[0] = menu->x + a->generic.x - (len+2)*a->generic.textSize;
		a->generic.botRight[0] = a->generic.topLeft[0] + (len+2)*a->generic.textSize;
	}
	a->generic.topLeft[1] = menu->y + a->generic.y;
	a->generic.botRight[1] = a->generic.topLeft[1] + a->generic.textSize;
	a->generic.dynamicWidth = 0;
	a->generic.dynamicHeight = 0;
	a->generic.isExtended = false;
//	a->generic.valueChanged = false;
}

//=========================================================

void UI_MenuKeyBind_DoEnter (menuKeyBind_s *k)
{
	menuFramework_s	*menu = k->generic.parent;

	if (!menu)	return;

	UI_FindKeysForCommand (k->commandName, k->keys);

//	if (k->keys[1] != -1)
//		UI_UnbindCommand (k->commandName);

	UI_SetGrabBindItem (menu, (menuCommon_s *)k);
		
	if (k->generic.callback)
		k->generic.callback (k);
}

char *UI_MenuKeyBind_Click (menuKeyBind_s *k, qboolean mouse2)
{
	// return if it's just a mouse2 click
	if (mouse2)
		return ui_menu_null_sound;

	UI_MenuKeyBind_DoEnter (k);

	return ui_menu_move_sound;
}

void UI_MenuKeyBind_Draw (menuKeyBind_s *k)
{
	menuFramework_s	*menu = k->generic.parent;
	int				x, alpha = UI_MouseOverAlpha(&k->generic);
	const char		*keyName1, *keyName2;

	UI_DrawMenuString (menu->x + k->generic.x + LCOLUMN_OFFSET,
						menu->y + k->generic.y, MENU_FONT_SIZE, k->generic.scrAlign, k->generic.name, alpha,
						!(k->generic.flags & QMF_LEFT_JUSTIFY), (k->generic.flags & QMF_ALTCOLOR));

	if (k->commandName)
	{
		UI_FindKeysForCommand (k->commandName, k->keys);

		if (k->keys[0] == -1)
		{
			UI_DrawMenuString (menu->x + k->generic.x + RCOLUMN_OFFSET,
								menu->y + k->generic.y, MENU_FONT_SIZE, k->generic.scrAlign, "???", alpha, false, false);
		}
		else
		{
			keyName1 = Key_KeynumToString (k->keys[0]);
			UI_DrawMenuString (menu->x + k->generic.x + RCOLUMN_OFFSET,
								menu->y + k->generic.y, MENU_FONT_SIZE, k->generic.scrAlign, keyName1, alpha, false, false);
			if (k->keys[1] != -1)
			{
				x = (int)strlen(keyName1) * MENU_FONT_SIZE;
				keyName2 = Key_KeynumToString (k->keys[1]);
				UI_DrawMenuString (menu->x + k->generic.x + MENU_FONT_SIZE*3 + x,
									menu->y + k->generic.y, MENU_FONT_SIZE, k->generic.scrAlign, "or", alpha, false, false);
				UI_DrawMenuString (menu->x + k->generic.x + MENU_FONT_SIZE*6 + x,
									menu->y + k->generic.y, MENU_FONT_SIZE, k->generic.scrAlign, keyName2, alpha, false, false);
			}
		}
	}
	else
		Com_Printf ("UI_MenuKeyBind_Draw: keybind has no commandName!\n");

//	if (k->generic.ownerdraw)
//		k->generic.ownerdraw(k);
}

void UI_MenuKeyBind_SetDynamicSize (menuKeyBind_s *k)
{
	k->generic.dynamicWidth = 0;
	if (k->commandName)
	{
		UI_FindKeysForCommand (k->commandName, k->keys);
		k->generic.dynamicWidth += RCOLUMN_OFFSET;

		if (k->keys[0] == -1)
			k->generic.dynamicWidth += MENU_FONT_SIZE*3; // "???"
		else {
			k->generic.dynamicWidth += MENU_FONT_SIZE*strlen(Key_KeynumToString(k->keys[0])); // key 1
			if (k->keys[1] != -1) // " or " + key2
				k->generic.dynamicWidth += MENU_FONT_SIZE*4 + MENU_FONT_SIZE*strlen(Key_KeynumToString(k->keys[1]));
		}
	}
}

void UI_MenuKeyBind_Setup (menuKeyBind_s *k)
{
	menuFramework_s	*menu = k->generic.parent;
	int				len;

	if (!menu)	return;

	len = (k->generic.name) ? (int)strlen(k->generic.name) : 0;
	// set min and max coords
	if (k->generic.flags & QMF_LEFT_JUSTIFY) {
		k->generic.topLeft[0] = menu->x + k->generic.x + LCOLUMN_OFFSET;
		k->generic.botRight[0] = k->generic.topLeft[0] + len*MENU_FONT_SIZE;
	}
	else {
		k->generic.topLeft[0] = menu->x + k->generic.x - (len+2)*MENU_FONT_SIZE;
		k->generic.botRight[0] = k->generic.topLeft[0] + (len+2)*MENU_FONT_SIZE;
	}
	k->generic.topLeft[1] = menu->y + k->generic.y;
	k->generic.botRight[1] = k->generic.topLeft[1] + MENU_FONT_SIZE;
	k->generic.dynamicWidth = 0;
	k->generic.dynamicHeight = 0;
	k->generic.isExtended = false;
//	k->generic.valueChanged = false;

	UI_MenuKeyBind_SetDynamicSize (k);
	k->grabBind = false;
}

const char *UI_MenuKeyBind_Key (menuKeyBind_s *k, int key)
{
	menuFramework_s	*menu = k->generic.parent;

	// pressing mouse1 to pick a new bind wont force bind/unbind itself - spaz
	if (UI_HasValidGrabBindItem(menu) && k->grabBind
		&& !(ui_mousecursor.buttonused[MOUSEBUTTON1] && key == K_MOUSE1))
	{
		// grab key here
		if (key != K_ESCAPE && key != '`')
		{
			char cmd[1024];

			if (k->keys[1] != -1)	// if two keys are already bound to this, clear them
				UI_UnbindCommand (k->commandName);

			Com_sprintf (cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(key), k->commandName);
			Cbuf_InsertText (cmd);
		}

		// don't let selecting with mouse buttons screw everything up
		UI_RefreshCursorButtons();
		if (key == K_MOUSE1)
			ui_mousecursor.buttonclicks[MOUSEBUTTON1] = -1;

		if (menu)
			UI_ClearGrabBindItem (menu);

		return ui_menu_out_sound;
	}

	switch (key)
	{
	case K_ESCAPE:
		UI_PopMenu ();
		return ui_menu_out_sound;
	case K_ENTER:
	case K_KP_ENTER:
		UI_MenuKeyBind_DoEnter (k);
		return ui_menu_in_sound;
	case K_BACKSPACE:
	case K_DEL:
	case K_KP_DEL:
		UI_UnbindCommand (k->commandName); // delete bindings
		return ui_menu_out_sound;
	default:
		return ui_menu_null_sound;
	}
}

//=========================================================

qboolean UI_MenuField_DoEnter (menuField_s *f)
{
	if (!f) return false;

	if (f->generic.callback)
	{
		f->generic.callback (f);
		return true;
	}
	return false;
}

char *UI_MenuField_Click (menuField_s *f, qboolean mouse2)
{
	// return if it's just a mouse2 click
	if (mouse2)
		return ui_menu_null_sound;

	if (f->generic.callback)
	{
		f->generic.callback (f);
		return ui_menu_move_sound;
	}
	return ui_menu_null_sound;
}

void UI_MenuField_Draw (menuField_s *f)
{
	menuFramework_s	*menu = f->generic.parent;
	int				i, hoverAlpha = UI_MouseOverAlpha(&f->generic), xtra;
	char			tempbuffer[128]="";
	int				offset;

//	if (f->generic.name)
//		UI_DrawMenuString (f->generic.x + f->generic.parent->x + LCOLUMN_OFFSET,
//								f->generic.y + f->generic.parent->y, f->generic.textSize, f->generic.scrAlign, f->generic.name, 255, true, true);
	// name
	UI_MenuCommon_DrawItemName (&f->generic, -(2*RCOLUMN_OFFSET), 0, 0, -(FIELD_VOFFSET+MENU_LINE_SIZE), hoverAlpha);

	if (xtra = stringLengthExtra(f->buffer))
	{
		strncpy( tempbuffer, f->buffer + f->visible_offset, f->visible_length );
		offset = (int)strlen(tempbuffer) - xtra;

		if (offset > f->visible_length)
		{
			f->visible_offset = offset - f->visible_length;
			strncpy (tempbuffer, f->buffer + f->visible_offset - xtra, f->visible_length + xtra);
			offset = f->visible_offset;
		}
	}
	else
	{
		strncpy (tempbuffer, f->buffer + f->visible_offset, f->visible_length);
		offset = (int)strlen(tempbuffer);
	}

	if (ui_new_textfield->integer)
	{
		UI_DrawPicST (f->generic.x + f->generic.parent->x + RCOLUMN_OFFSET,
						f->generic.y + f->generic.parent->y - 4, f->generic.textSize, f->generic.textSize*2, stCoord_field_left,
						f->generic.scrAlign, true, color_identity, UI_FIELD_PIC);
		UI_DrawPicST (f->generic.x + f->generic.parent->x + (1+f->visible_length)*f->generic.textSize + RCOLUMN_OFFSET,
						f->generic.y + f->generic.parent->y - 4, f->generic.textSize, f->generic.textSize*2, stCoord_field_right,
						f->generic.scrAlign, true, color_identity, UI_FIELD_PIC);
	}
	else
	{
		UI_DrawChar (f->generic.x + f->generic.parent->x + RCOLUMN_OFFSET,
					f->generic.y + f->generic.parent->y - 4, f->generic.textSize, f->generic.scrAlign, 18, 255, 255, 255, 255, false, false);
		UI_DrawChar (f->generic.x + f->generic.parent->x + RCOLUMN_OFFSET,
					f->generic.y + f->generic.parent->y + 4, f->generic.textSize, f->generic.scrAlign, 24, 255, 255, 255, 255, false, false);
		UI_DrawChar (f->generic.x + f->generic.parent->x + (1+f->visible_length)*f->generic.textSize + RCOLUMN_OFFSET,
					f->generic.y + f->generic.parent->y - 4, f->generic.textSize, f->generic.scrAlign, 20, 255, 255, 255, 255, false, false);
		UI_DrawChar (f->generic.x + f->generic.parent->x + (1+f->visible_length)*f->generic.textSize + RCOLUMN_OFFSET,
					f->generic.y + f->generic.parent->y + 4, f->generic.textSize, f->generic.scrAlign, 26, 255, 255, 255, 255, false, false);
	}

	for (i = 0; i < f->visible_length; i++)
	{
		if (ui_new_textfield->integer) {
			UI_DrawPicST (f->generic.x + f->generic.parent->x + (1+i)*f->generic.textSize + RCOLUMN_OFFSET,
							f->generic.y + f->generic.parent->y - 4, f->generic.textSize, f->generic.textSize*2, stCoord_field_center,
							f->generic.scrAlign, true, color_identity, UI_FIELD_PIC);
		}
		else {
			UI_DrawChar (f->generic.x + f->generic.parent->x + (1+i)*f->generic.textSize + RCOLUMN_OFFSET,
						f->generic.y + f->generic.parent->y - 4, f->generic.textSize, f->generic.scrAlign, 19, 255, 255, 255, 255, false, false);
			UI_DrawChar (f->generic.x + f->generic.parent->x + (1+i)*f->generic.textSize + RCOLUMN_OFFSET,
						f->generic.y + f->generic.parent->y + 4, f->generic.textSize, f->generic.scrAlign, 25, 255, 255, 255, 255, false, (i==(f->visible_length-1)));
		}
	}

	// add cursor thingie
	if ( (UI_ItemAtMenuCursor(f->generic.parent) == f)  && ((int)(Sys_Milliseconds()/250))&1 )
		Com_sprintf (tempbuffer, sizeof(tempbuffer), "%s%c", tempbuffer, 11);

	UI_DrawMenuString (f->generic.x + f->generic.parent->x + f->generic.textSize*3,
						f->generic.y + f->generic.parent->y, f->generic.textSize, f->generic.scrAlign, tempbuffer, hoverAlpha, false, false);
}

void UI_MenuField_Setup (menuField_s *f)
{
	menuFramework_s	*menu = f->generic.parent;

	// set min and max coords
	f->generic.topLeft[0] = menu->x + f->generic.x + RCOLUMN_OFFSET;
	f->generic.topLeft[1] = menu->y + f->generic.y;
	f->generic.botRight[0] = f->generic.topLeft[0] + (f->visible_length+2)*MENU_FONT_SIZE;
	f->generic.botRight[1] = f->generic.topLeft[1] + MENU_FONT_SIZE;
	f->generic.dynamicWidth = 0;
	f->generic.dynamicHeight = 0;
	f->generic.isExtended = false;
//	f->generic.valueChanged = false;
}

qboolean UI_MenuField_Key (menuField_s *f, int key)
{
	extern int keydown[];

	if (!f) return false;

	switch ( key )
	{
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	}

	// mxd- This blocked Shift-Ins combo in the next block.
	// Knightmare- allow only the INS key thru, otherwise mouse events end up as text input!
	if (key > 127)
	{
		switch (key)
		{
		case K_INS:
		case K_KP_INS:
			break;
		case K_DEL:
		default:
			return false;
		}
	}


	//
	// support pasting from the clipboard
	//
	if ( ( toupper(key) == 'V' && keydown[K_CTRL] ) ||
		 ( ( (key == K_INS) || (key == K_KP_INS) ) && keydown[K_SHIFT] ) )
	{
		char *cbd;
		
		if ( ( cbd = Sys_GetClipboardData() ) != 0 )
		{
			strtok( cbd, "\n\r\b" );

			strncpy( f->buffer, cbd, f->length - 1 );
			f->cursor = (int)strlen( f->buffer );
			f->visible_offset = f->cursor - f->visible_length;
			if ( f->visible_offset < 0 )
				f->visible_offset = 0;

			free( cbd );
		}
		return true;
	}

	switch ( key )
	{
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_BACKSPACE:
		if ( f->cursor > 0 )
		{
			memmove( &f->buffer[f->cursor-1], &f->buffer[f->cursor], strlen( &f->buffer[f->cursor] ) + 1 );
			f->cursor--;

			if (f->visible_offset)
				f->visible_offset--;
		}
		break;

	case K_KP_DEL:
	case K_DEL:
		memmove( &f->buffer[f->cursor], &f->buffer[f->cursor+1], strlen( &f->buffer[f->cursor+1] ) + 1 );
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_ESCAPE:
	case K_TAB:
		return false;

	case K_SPACE:
	default:
		if ( !isdigit(key) && (f->generic.flags & QMF_NUMBERSONLY) )
			return false;

		if (f->cursor < f->length)
		{
			f->buffer[f->cursor++] = key;
			f->buffer[f->cursor] = 0;

			if (f->cursor > f->visible_length)
				f->visible_offset++;
		}
	}

	return true;
}

//=========================================================

void UI_MenuLabel_Draw (menuLabel_s *l)
{
	int alpha;

	if (!l) return;

	alpha = UI_MouseOverAlpha(&l->generic);

	if (l->generic.name)
		UI_DrawMenuString (l->generic.x + l->generic.parent->x,
								l->generic.y + l->generic.parent->y, l->generic.textSize, l->generic.scrAlign, l->generic.name, alpha, true, true);
}

void UI_MenuLabel_Setup (menuLabel_s *l)
{
	menuFramework_s	*menu = l->generic.parent;
	int				nameWidth;

	nameWidth = (l->generic.name) ? (int)strlen(l->generic.name)*l->generic.textSize : 0;

	// set min and max coords
	if (l->generic.flags & QMF_LEFT_JUSTIFY) {
		l->generic.topLeft[0] = menu->x + l->generic.x + LCOLUMN_OFFSET;
		l->generic.botRight[0] = l->generic.topLeft[0] + nameWidth;
	}
	else {
		l->generic.topLeft[0] = menu->x + l->generic.x - nameWidth;
		l->generic.botRight[0] = l->generic.topLeft[0] + nameWidth;
	}
	l->generic.topLeft[1] = menu->y + l->generic.y;
	l->generic.botRight[1] = l->generic.topLeft[1] + l->generic.textSize;
	l->generic.dynamicWidth = 0;
	l->generic.dynamicHeight = 0;
	l->generic.isExtended = false;
//	l->generic.valueChanged = false;
	l->generic.flags |= QMF_NOINTERACTION;
}

//=========================================================

void UI_MenuSlider_SetValue (menuSlider_s *s, const char *varName, float cvarMin, float cvarMax, qboolean clamp)
{
	if (!s || !varName || varName[0] == '\0')
		return;
	if (!s->increment)
		s->increment = 1.0f;

	if (clamp) {
		UI_ClampCvar (varName, cvarMin, cvarMax);
	}
	s->curPos = (int)ceil((Cvar_VariableValue((char *)varName) - s->baseValue) / s->increment);
	s->curPos = min(max(s->curPos, 0), s->maxPos);
}

void UI_MenuSlider_SaveValue (menuSlider_s *s, const char *varName)
{
	if (!s || !varName || varName[0] == '\0')
		return;

	Cvar_SetValue ((char *)varName, ((float)s->curPos * s->increment) + s->baseValue);
}

float UI_MenuSlider_GetValue (menuSlider_s *s)
{
	if (!s) return 0.0f;

	if (!s->increment)
		s->increment = 1.0f;

	return ((float)s->curPos * s->increment) + s->baseValue;
}

void UI_MenuSlider_CheckSlide (menuSlider_s *s)
{
	if (!s) return;

	s->curPos = min(max(s->curPos, 0), s->maxPos);

/*	if (!s->generic.cvarNoSave)
		UI_MenuSlider_SaveValue (s);
	else
		s->generic.valueChanged = UI_MenuSlider_ValueChanged (s);
*/
	if (s->generic.callback)
		s->generic.callback (s);
}

void UI_MenuSlider_DoSlide (menuSlider_s *s, int dir)
{
	if (!s) return;

	s->curPos += dir;

//	s->curPos = min(max(s->curPos, 0), s->maxPos);

//	if (s->generic.callback)
//		s->generic.callback(s);
	UI_MenuSlider_CheckSlide (s);
}

void UI_MenuSlider_ClickPos (menuFramework_s *menu, void *menuitem)
{
	menuSlider_s	*slider = (menuSlider_s *)menuitem;
	menuCommon_s	*item = (menuCommon_s *)menuitem;
	int				sliderPos, min, max;
	float			x, w, range;

	range = min(max((float)slider->curPos / (float)slider->maxPos, 0), 1);
	sliderPos = (int)(RCOLUMN_OFFSET + SLIDER_ENDCAP_WIDTH
				+ (float)SLIDER_RANGE*(float)SLIDER_SECTION_WIDTH*range);
	x = menu->x + item->x + sliderPos;
	w = SLIDER_KNOB_WIDTH;
	SCR_ScaleCoords (&x, NULL, &w, NULL, item->scrAlign);
	min = x - w/2;
	max = x + w/2;

	if (ui_mousecursor.x < min)
		UI_MenuSlider_DoSlide (slider, -1);
	if (ui_mousecursor.x > max)
		UI_MenuSlider_DoSlide (slider, 1);
}

void UI_MenuSlider_DragPos (menuFramework_s *menu, void *menuitem)
{
	menuSlider_s	*slider = (menuSlider_s *)menuitem;
	float			newValue, sliderbase;

	sliderbase = menu->x + slider->generic.x + RCOLUMN_OFFSET + SLIDER_ENDCAP_WIDTH;
	SCR_ScaleCoords (&sliderbase, NULL, NULL, NULL, slider->generic.scrAlign);
	newValue = (float)(ui_mousecursor.x - sliderbase)
				/ (SLIDER_RANGE * SCR_ScaledScreen(SLIDER_SECTION_WIDTH));
	slider->curPos = newValue * (float)slider->maxPos;

	UI_MenuSlider_CheckSlide (slider);
}

qboolean UI_MenuSlider_Check_Mouseover (menuSlider_s *s)
{
	int				min[2], max[2];
	float			x1, y1, x2, y2;

	x1 = s->barTopLeft[0];
	y1 = s->barTopLeft[1];
	x2 = s->generic.botRight[0];
	y2 = s->generic.botRight[1];

	SCR_ScaleCoords (&x1, &y1, NULL, NULL, s->generic.scrAlign);
	SCR_ScaleCoords (&x2, &y2, NULL, NULL, s->generic.scrAlign);
	min[0] = x1;	max[0] = x2;
	min[1] = y1;	max[1] = y2;

	if ( ui_mousecursor.x >= min[0] && ui_mousecursor.x <= max[0] 
		&& ui_mousecursor.y >= min[1] &&  ui_mousecursor.y <= max[1] )
		return true;
	else
		return false;
}

char *UI_MenuSlider_Click (menuSlider_s *s, qboolean mouse2)
{
	if (mouse2)
	{
		if ( UI_MenuSlider_Check_Mouseover(s) )
			UI_MenuSlider_ClickPos (s->generic.parent, s);
		else
			UI_MenuSlider_DoSlide (s, -1);
		return ui_menu_move_sound;
	}
	else
	{
		if ( UI_MenuSlider_Check_Mouseover(s) ) {
			UI_MenuSlider_DragPos (s->generic.parent, s);
			return ui_menu_drag_sound;
		}
		else {
			UI_MenuSlider_DoSlide (s, 1);
			return ui_menu_move_sound;
		}
	}
}

void UI_MenuSlider_Draw (menuSlider_s *s)
{
	int		i, x, y, hoverAlpha;
	float	tmpValue;
	char	valueText[8];

	if (!s) return;

	hoverAlpha = UI_MouseOverAlpha(&s->generic);

//	UI_DrawMenuString (s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET,
//							s->generic.y + s->generic.parent->y, s->generic.textSize, s->generic.scrAlign, s->generic.name, hoverAlpha, true, true);
	// name and header
	UI_MenuCommon_DrawItemName (&s->generic, LCOLUMN_OFFSET, 0, RCOLUMN_OFFSET, -MENU_LINE_SIZE, hoverAlpha);

	if (!s->maxPos)
		s->maxPos = 1;
	if (!s->increment)
		s->increment = 1.0f;

	s->range = (float)s->curPos / (float)s->maxPos;

	if (s->range < 0)
		s->range = 0;
	if (s->range > 1)
		s->range = 1;

	x = s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET;
	y = s->generic.y + s->generic.parent->y;

	// draw left
	UI_DrawPicST (x, y, SLIDER_ENDCAP_WIDTH, SLIDER_HEIGHT,
						stCoord_slider_left, s->generic.scrAlign, true, color_identity, UI_SLIDER_PIC);
//	UI_DrawChar (s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET,
//				s->generic.y + s->generic.parent->y, s->generic.textSize, s->generic.scrAlign, 128, 255,255,255,255, false, false);

	// draw center
	x += SLIDER_ENDCAP_WIDTH;
	for (i = 0; i < SLIDER_RANGE; i++) {
		UI_DrawPicST (x + i*SLIDER_SECTION_WIDTH, y, SLIDER_SECTION_WIDTH, SLIDER_HEIGHT,
							stCoord_slider_center, s->generic.scrAlign, true, color_identity, UI_SLIDER_PIC);
	//	UI_DrawChar (s->generic.x + s->generic.parent->x + (i+1)*s->generic.textSize + RCOLUMN_OFFSET,
	//				s->generic.y + s->generic.parent->y, s->generic.textSize, s->generic.scrAlign, 129, 255,255,255,255, false, false);
	}

	// draw right
	UI_DrawPicST (x + i*SLIDER_SECTION_WIDTH, y, SLIDER_ENDCAP_WIDTH, SLIDER_HEIGHT,
						stCoord_slider_right, s->generic.scrAlign, true, color_identity, UI_SLIDER_PIC);
//	UI_DrawChar (s->generic.x + s->generic.parent->x + (i+1)*s->generic.textSize + RCOLUMN_OFFSET,
//				s->generic.y + s->generic.parent->y, s->generic.textSize, s->generic.scrAlign, 130, 255,255,255,255, false, false);

	// draw knob
	UI_DrawPicST (x + SLIDER_RANGE*SLIDER_SECTION_WIDTH*s->range - (SLIDER_KNOB_WIDTH/2), y, SLIDER_KNOB_WIDTH, SLIDER_HEIGHT,
						stCoord_slider_knob, s->generic.scrAlign, true, color_identity, UI_SLIDER_PIC);
//	UI_DrawChar (s->generic.x + s->generic.parent->x + s->generic.textSize*((SLIDER_RANGE-1)*s->range+1) + RCOLUMN_OFFSET,
//				s->generic.y + s->generic.parent->y, s->generic.textSize, s->generic.scrAlign, 131, 255,255,255,255, false, true);

	// draw value
	tmpValue = s->curPos * s->increment + s->baseValue;
	if (s->displayAsPercent) {
		tmpValue *= 100.0f;
		Com_sprintf (valueText, sizeof(valueText), "%.0f%%", tmpValue);
	}
	else {
		if (fabs((int)tmpValue - tmpValue) < 0.01f)
			Com_sprintf (valueText, sizeof(valueText), "%i", (int)tmpValue);
		else
			Com_sprintf (valueText, sizeof(valueText), "%4.2f", tmpValue);
	}
	UI_DrawMenuString (s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET + 2*SLIDER_ENDCAP_WIDTH + i*SLIDER_SECTION_WIDTH + MENU_FONT_SIZE/2,
						s->generic.y + s->generic.parent->y + 1, MENU_FONT_SIZE-2, s->generic.scrAlign, valueText, hoverAlpha, false, false);
//	UI_DrawMenuString (s->generic.x + s->generic.parent->x + s->generic.textSize*SLIDER_RANGE + RCOLUMN_OFFSET + 2.5*MENU_FONT_SIZE,
//						s->generic.y + s->generic.parent->y + 1, MENU_FONT_SIZE-2, s->generic.scrAlign, valueText, hoverAlpha, false, false);
}

void UI_MenuSlider_Setup (menuSlider_s *s)
{
	menuFramework_s	*menu = s->generic.parent;
//	int				nameWidth;

//	nameWidth = (s->generic.name) ? (int)strlen(s->generic.name)*s->generic.textSize : 0;
	if (!s->baseValue)	s->baseValue = 0.0f;
	if (!s->increment)	s->increment = 1.0f;

	// set min and max coords
	s->generic.topLeft[0] = menu->x + s->generic.x;
	s->generic.topLeft[1] = menu->y + s->generic.y;
	s->generic.botRight[0] = s->generic.topLeft[0] + RCOLUMN_OFFSET + SLIDER_RANGE*SLIDER_SECTION_WIDTH + 2*SLIDER_ENDCAP_WIDTH;
	s->generic.botRight[1] = s->generic.topLeft[1] + SLIDER_HEIGHT;
	s->barTopLeft[0] = s->generic.topLeft[0] + RCOLUMN_OFFSET;
	s->barTopLeft[1] = s->generic.topLeft[1];
	s->generic.dynamicWidth = 0;
	s->generic.dynamicHeight = 0;
	s->generic.isExtended = false;
//	s->generic.valueChanged = false;
}

//=========================================================

void UI_MenuPicker_SetValue (menuPicker_s *p, const char *varName, float cvarMin, float cvarMax, qboolean clamp)
{
	if (!p || !varName || varName[0] == '\0')
		return;

	if (clamp) {
		UI_ClampCvar (varName, cvarMin, cvarMax);
	}
	if (p->itemValues) {
		p->curValue = UI_GetIndexForStringValue(p->itemValues, Cvar_VariableString((char *)varName));
	}
	else
	{
		if (p->invertValue) {
			p->curValue = (Cvar_VariableValue((char *)varName) < 0);
		}
		else {
			p->curValue = (int)min(max(Cvar_VariableValue((char *)varName), cvarMin), cvarMax);
		}
	}
}

void UI_MenuPicker_SaveValue (menuPicker_s *p, const char *varName)
{
	if (!p || !varName || varName[0] == '\0')
		return;
	if (!p->numItems) {
		Com_Printf (S_COLOR_YELLOW"UI_MenuPicker_SaveValue: not initialized!\n");
		return;
	}
	if ( (p->curValue < 0) || (p->curValue >= p->numItems) ) {
		Com_Printf (S_COLOR_YELLOW"UI_MenuPicker_SaveValue: curvalue out of bounds!\n");
		return;
	}

	if (p->itemValues) {
		// Don't save to cvar if this itemvalue is the wildcard
		if ( Q_stricmp(va("%s", p->itemValues[p->curValue]), UI_ITEMVALUE_WILDCARD) != 0 )
			Cvar_Set ((char *)varName, va("%s", p->itemValues[p->curValue]));
	}
	else
	{
		if (p->invertValue) {
			Cvar_SetValue ((char *)varName, Cvar_VariableValue((char *)varName) * -1 );
		}
		else {
			Cvar_SetInteger ((char *)varName, p->curValue);
		}
	}
}

const char *UI_MenuPicker_GetValue (menuPicker_s *p)
{
	const char *value;

	if (!p)
		return NULL;

	if (!p->numItems) {
		Com_Printf (S_COLOR_YELLOW"UI_MenuPicker_GetValue: not initialized!\n");
		return NULL;
	}
	if ( (p->curValue < 0) || (p->curValue >= p->numItems) ) {
		Com_Printf (S_COLOR_YELLOW"UI_MenuPicker_GetValue: curvalue out of bounds!\n");
		return NULL;
	}

	if (p->itemValues) {
		value = p->itemValues[p->curValue];
	}
	else {
		value = va("%d", p->curValue);
	}

	return value;
}

void UI_MenuPicker_DoEnter (menuPicker_s *p)
{
	if (!p || !p->itemNames || !p->numItems)
		return;

	p->curValue++;
	if (p->itemNames[p->curValue] == 0)
		p->curValue = 0;

	if (p->generic.callback)
		p->generic.callback (p);
}

char *UI_MenuPicker_Click (menuPicker_s *p, qboolean mouse2)
{
	if (!p || !p->itemNames || !p->numItems)
		return ui_menu_null_sound;

	if (mouse2) {
		UI_MenuPicker_DoSlide (p, -1);
	}
	else {
		UI_MenuPicker_DoSlide (p, 1);
	}

	return ui_menu_move_sound;
}

void UI_MenuPicker_DoSlide (menuPicker_s *p, int dir)
{
	if (!p || !p->itemNames || !p->numItems)
		return;

	p->curValue += dir;

	if (p->generic.flags & QMF_SKINLIST) // don't allow looping around for skin lists
	{
		if (p->curValue < 0)
			p->curValue = 0;
		else if (p->itemNames[p->curValue] == 0)
			p->curValue--;
	}
	else {
		if (p->curValue < 0)
			p->curValue = p->numItems-1; // was 0
		else if (p->itemNames[p->curValue] == 0)
			p->curValue = 0; // was --
	}

	if (p->generic.callback)
		p->generic.callback (p);
}
 
void UI_MenuPicker_Draw (menuPicker_s *p)
{
	int		alpha;
	char	buffer[100];

	if (!p)	return;

	alpha = UI_MouseOverAlpha(&p->generic);

	if (p->generic.name)
	{
		UI_DrawMenuString (p->generic.x + p->generic.parent->x + LCOLUMN_OFFSET,
							p->generic.y + p->generic.parent->y, p->generic.textSize, p->generic.scrAlign, p->generic.name, alpha, true, true);
	}
	if (!strchr(p->itemNames[p->curValue], '\n'))
	{
		UI_DrawMenuString (p->generic.x + p->generic.parent->x + RCOLUMN_OFFSET,
							p->generic.y + p->generic.parent->y, p->generic.textSize, p->generic.scrAlign, p->itemNames[p->curValue], alpha, false, false);
	}
	else
	{
	//	strncpy(buffer, p->itemnames[p->curvalue]);
		Q_strncpyz (buffer, sizeof(buffer), p->itemNames[p->curValue]);
		*strchr(buffer, '\n') = 0;
		UI_DrawMenuString (p->generic.x + p->generic.parent->x + RCOLUMN_OFFSET,
						p->generic.y + p->generic.parent->y, p->generic.textSize, p->generic.scrAlign, buffer, alpha, false, false);
	//	strncpy(buffer, strchr( p->itemnames[p->curvalue], '\n' ) + 1 );
		Q_strncpyz (buffer, sizeof(buffer), strchr( p->itemNames[p->curValue], '\n' ) + 1);
		UI_DrawMenuString (p->generic.x + p->generic.parent->x + RCOLUMN_OFFSET,
						p->generic.y + p->generic.parent->y + MENU_LINE_SIZE, p->generic.textSize, p->generic.scrAlign, buffer, alpha, false, false);
	}
}

void UI_MenuPicker_SetDynamicSize (menuPicker_s *p)
{
	if (!p)	return;

	p->generic.dynamicWidth = 0;
	p->generic.dynamicWidth += RCOLUMN_OFFSET;
	if ( (p->curValue < p->numItems) && p->itemNames[p->curValue] && (strlen(p->itemNames[p->curValue]) > 0) )
	{
		p->generic.dynamicWidth += (int)strlen(p->itemNames[p->curValue])*p->generic.textSize;
	}
}

void UI_MenuPicker_Setup (menuPicker_s *p)
{
	menuFramework_s	*menu = p->generic.parent;
	int				i, j, len;

	for (i=0; p->itemNames[i]; i++);
	p->numItems = i;

	if (p->itemValues)	// Check if itemvalues count matches itemnames
	{
		for (j=0; p->itemValues[j]; j++);
		if (j != i) {
			Com_Printf (S_COLOR_YELLOW"UI_MenuPicker_Setup: itemvalues size mismatch for %s!\n",
						(p->generic.name && (p->generic.name[0] != 0)) ? p->generic.name : "<noname>");
		}
	}

	// set min and max coords
	len = (p->generic.name) ? (int)strlen(p->generic.name) : 0;
	p->generic.topLeft[0] = menu->x + p->generic.x - (len+2)*p->generic.textSize;
	p->generic.topLeft[1] = menu->y + p->generic.y;
	p->generic.botRight[0] = p->generic.topLeft[0] + (len+2)*p->generic.textSize;
	p->generic.botRight[1] = p->generic.topLeft[1] + p->generic.textSize;
	p->generic.dynamicWidth = 0;
	p->generic.dynamicHeight = 0;
	p->generic.isExtended = false;
//	p->generic.valueChanged = false;

	UI_MenuPicker_SetDynamicSize (p);
}

//=========================================================

void UI_MenuImage_Draw (menuImage_s *i)
{
//	menuFramework_s	*menu = i->generic.parent;
	byte	*bc = i->borderColor;

	if (i->border > 0)
	{
		if (i->alpha == 255) // just fill whole area for border if not trans
			UI_DrawFill (i->generic.topLeft[0]-i->border, i->generic.topLeft[1]-i->border,
							i->width+(i->border*2), i->height+(i->border*2), i->generic.scrAlign, false, bc[0],bc[1],bc[2],bc[3]);
		else // have to do each side
			UI_DrawBorder ((float)i->generic.topLeft[0], (float)i->generic.topLeft[1], (float)i->width, (float)i->height,
							(float)i->border, i->generic.scrAlign, false, bc[0],bc[1],bc[2],bc[3]);
	}
	if (i->imageName && strlen(i->imageName) > 0) {
		if (i->overrideColor)
			UI_DrawColoredPic (i->generic.topLeft[0], i->generic.topLeft[1], i->width, i->height, i->generic.scrAlign, false, i->imageColor, i->imageName);
		else
			UI_DrawPic (i->generic.topLeft[0], i->generic.topLeft[1], i->width, i->height, i->generic.scrAlign, false, i->imageName, i->alpha);
	}
	else
		UI_DrawFill (i->generic.topLeft[0], i->generic.topLeft[1], i->width, i->height, i->generic.scrAlign, false, 0,0,0,255);
}

void UI_MenuImage_UpdateCoords (menuImage_s *i)
{
	menuFramework_s	*menu = i->generic.parent;

	i->generic.topLeft[0] = menu->x + i->generic.x;
	i->generic.topLeft[1] = menu->y + i->generic.y;
	i->generic.botRight[0] = i->generic.topLeft[0] + i->width;
	i->generic.botRight[1] = i->generic.topLeft[1] + i->height;
}

void UI_MenuImage_Setup (menuImage_s *i)
{
	menuFramework_s	*menu = i->generic.parent;

	// automatic sizing
	if (i->width == -1 || i->height == -1)
	{
		int w, h;
		R_DrawGetPicSize (&w, &h, i->imageName);
		if (i->width == -1)
			i->width = w;
		if (i->height == -1)
			i->height = h;
	}

	i->width = max(i->width, 1);
	i->height = max(i->height, 1);
	i->border = max(i->border, 0);

	// automatic centering
	if (i->hCentered)
		i->generic.x = (SCREEN_WIDTH/2 - i->width/2) - menu->x;
	if (i->vCentered)
		i->generic.y = (SCREEN_HEIGHT/2 - i->height/2) - menu->y;

	// set min and max coords
	UI_MenuImage_UpdateCoords (i);
	i->generic.dynamicWidth = 0;
	i->generic.dynamicHeight = 0;
	i->generic.isExtended = false;
//	i->generic.valueChanged = false;
	i->generic.flags |= QMF_NOINTERACTION;

/*	if (R_DrawFindPic(i->imageName))
		i->imageValid = true;
	else
		i->imageValid = false;*/
}

//=========================================================

void UI_MenuButton_DoEnter (menuButton_s *b)
{
	if (b->generic.flags & QMF_MOUSEONLY)
		return;

	if (b->generic.callback)
		b->generic.callback (b);
}

char *UI_MenuButton_Click (menuButton_s *b, qboolean mouse2)
{
	// skip if it's not mouse2-enabled and mouse1 wasn't clicked
	if (!b->usesMouse2 && mouse2)
		return ui_menu_null_sound;

	if ( mouse2 && b->generic.mouse2Callback )
		b->generic.mouse2Callback (b);
	else if (b->generic.callback)
		b->generic.callback (b);

	return ui_menu_move_sound;
}

void UI_MenuButton_Draw (menuButton_s *b)
{
	menuFramework_s	*menu = b->generic.parent;
	byte	*bc = b->borderColor;

	if (b->border > 0)
	{
		if (b->alpha == 255) // just fill whole area for border if not trans
			UI_DrawFill (b->generic.topLeft[0]-b->border, b->generic.topLeft[1]-b->border,
							b->width+(b->border*2), b->height+(b->border*2), b->generic.scrAlign, false, bc[0],bc[1],bc[2],bc[3]);
		else // have to do each side
			UI_DrawBorder ((float)b->generic.topLeft[0], (float)b->generic.topLeft[1], (float)b->width, (float)b->height,
							(float)b->border, b->generic.scrAlign, false, bc[0],bc[1],bc[2],bc[3]);
	}
	if ( (b == ui_mousecursor.menuitem || b == UI_ItemAtMenuCursor(menu))
		&& (b->hoverImageName && strlen(b->hoverImageName) > 0) ) {
		if (b->overrideColor)
			UI_DrawColoredPic (b->generic.topLeft[0], b->generic.topLeft[1], b->width, b->height, b->generic.scrAlign, false, b->imageColor, b->hoverImageName);
		else
			UI_DrawPic (b->generic.topLeft[0], b->generic.topLeft[1], b->width, b->height, b->generic.scrAlign, false, b->hoverImageName, b->alpha);
	}
	else if (b->imageName && strlen(b->imageName) > 0) {
		if (b->overrideColor)
			UI_DrawColoredPic (b->generic.topLeft[0], b->generic.topLeft[1], b->width, b->height, b->generic.scrAlign, false, b->imageColor, b->imageName);
		else
			UI_DrawPic (b->generic.topLeft[0], b->generic.topLeft[1], b->width, b->height, b->generic.scrAlign, false, b->imageName, b->alpha);
	}
	else
		UI_DrawFill (b->generic.topLeft[0], b->generic.topLeft[1], b->width, b->height, b->generic.scrAlign, false, 0,0,0,255);
}

void UI_MenuButton_UpdateCoords (menuButton_s *b)
{
	menuFramework_s	*menu = b->generic.parent;

	b->generic.topLeft[0] = menu->x + b->generic.x;
	b->generic.topLeft[1] = menu->y + b->generic.y;
	b->generic.botRight[0] = b->generic.topLeft[0] + b->width;
	b->generic.botRight[1] = b->generic.topLeft[1] + b->height;
}

void UI_MenuButton_Setup (menuButton_s *b)
{
	menuFramework_s	*menu = b->generic.parent;

	// automatic sizing
	if (b->width == -1 || b->height == -1)
	{
		int w, h;
		R_DrawGetPicSize (&w, &h, b->imageName);
		if (b->width == -1)
			b->width = w;
		if (b->height == -1)
			b->height = h;
	}

	b->width = max(b->width, 1);
	b->height = max(b->height, 1);
	b->border = max(b->border, 0);

	// automatic centering
	if (b->hCentered)
		b->generic.x = (SCREEN_WIDTH/2 - b->width/2) - menu->x;
	if (b->vCentered)
		b->generic.y = (SCREEN_HEIGHT/2 - b->height/2) - menu->y;

	// set min and max coords
	UI_MenuButton_UpdateCoords (b);
	b->generic.dynamicWidth = 0;
	b->generic.dynamicHeight = 0;
	b->generic.isExtended = false;
//	b->generic.valueChanged = false;
}

//=========================================================

void UI_MenuRectangle_Draw (menuRectangle_s *r)
{
//	menuFramework_s	*menu = r->generic.parent;
	byte	*bc = r->borderColor;
	byte	*c = r->color;

	if (r->border > 0)
	{
		if (c[3] == 255) // just fill whole area for border if not trans
			UI_DrawFill (r->generic.topLeft[0]-r->border, r->generic.topLeft[1]-r->border,
							r->width+(r->border*2), r->height+(r->border*2), r->generic.scrAlign, false, bc[0],bc[1],bc[2],bc[3]);
		else // have to do each side
			UI_DrawBorder ((float)r->generic.topLeft[0], (float)r->generic.topLeft[1], (float)r->width, (float)r->height,
							(float)r->border, r->generic.scrAlign, false, bc[0],bc[1],bc[2],bc[3]);
	}
	UI_DrawFill (r->generic.topLeft[0], r->generic.topLeft[1], r->width, r->height, r->generic.scrAlign, false, c[0],c[1],c[2],c[3]);
}

void UI_MenuRectangle_UpdateCoords (menuRectangle_s *r)
{
	menuFramework_s	*menu = r->generic.parent;

	r->generic.topLeft[0] = menu->x + r->generic.x;
	r->generic.topLeft[1] = menu->y + r->generic.y;
	r->generic.botRight[0] = r->generic.topLeft[0] + r->width;
	r->generic.botRight[1] = r->generic.topLeft[1] + r->height;
}

void UI_MenuRectangle_Setup (menuRectangle_s *r)
{
	menuFramework_s	*menu = r->generic.parent;

	r->width =	max(r->width, 1);
	r->height =	max(r->height, 1);
	r->border = max(r->border, 0);

	// automatic centering
	if (r->hCentered)
		r->generic.x = (SCREEN_WIDTH/2 - r->width/2) - menu->x;
	if (r->vCentered)
		r->generic.y = (SCREEN_HEIGHT/2 - r->height/2) - menu->y;

	// set min and max coords
	UI_MenuRectangle_UpdateCoords (r);
	r->generic.dynamicWidth = 0;
	r->generic.dynamicHeight = 0;
	r->generic.isExtended = false;
//	r->generic.valueChanged = false;
	r->generic.flags |= QMF_NOINTERACTION;
}

//=========================================================

void UI_MenuTextScroll_Draw (menuTextScroll_s *t)
{
//	menuFramework_s	*menu = t->generic.parent;
	float			y, alpha;
	int				i, x, len, stringoffset;
	qboolean		bold;

	if (!t->initialized)
		return;

	if ( ((float)t->height - ((float)(cls.realtime - t->start_time)*t->time_scale)
		+ (float)(t->start_line * t->lineSize)) < 0 )
	{
		t->start_line++;
		if (!t->scrollText[t->start_line])
		{
			t->start_line = 0;
			t->start_time = cls.realtime;
		}
	}

	for (i=t->start_line, y=(float)t->generic.botRight[1] - ((float)(cls.realtime - t->start_time)*t->time_scale) + t->start_line * t->lineSize;
		t->scrollText[i] && y < t->generic.botRight[1]; y += (float)t->lineSize, i++)
	{
		stringoffset = 0;
		bold = false;

		if (y <= t->generic.topLeft[1]-t->generic.textSize)
			continue;
		if (y > t->generic.botRight[1])
			continue;

		if (t->scrollText[i][0] == '+')
		{
			bold = true;
			stringoffset = 1;
		}
		else
		{
			bold = false;
			stringoffset = 0;
		}

		if (y > (float)t->height*(7.0f/8.0f))
		{
			float y_test, h_test;
			y_test = y - t->height*(7.0f/8.0f);
			h_test = t->height/8;

			alpha = 1 - (y_test/h_test);

			alpha = max(min(alpha, 1), 0);
		}
		else if (y < (float)t->height/8)
		{
			float y_test, h_test;
			y_test = y;
			h_test = t->height/8;

			alpha = y_test/h_test;

			alpha = max(min(alpha, 1), 0);
		}
		else
			alpha = 1;

		len = (int)strlen(t->scrollText[i]) - stringLengthExtra(t->scrollText[i]);

		x = t->generic.topLeft[0] + (t->width - (len * t->generic.textSize) - (stringoffset * t->generic.textSize)) / 2
			+ stringoffset * t->generic.textSize;
		UI_DrawMenuString (x, (int)floor(y), t->generic.textSize, t->generic.scrAlign, t->scrollText[i], alpha*255, false, false);
	}
}

void UI_MenuTextScroll_UpdateCoords (menuTextScroll_s *t)
{
	menuFramework_s	*menu = t->generic.parent;

	t->generic.topLeft[0] = menu->x + t->generic.x;
	t->generic.topLeft[1] = menu->y + t->generic.y;
	t->generic.botRight[0] = t->generic.topLeft[0] + t->width;
	t->generic.botRight[1] = t->generic.topLeft[1] + t->height;
}

void UI_MenuTextScroll_Setup (menuTextScroll_s *t)
{
//	menuFramework_s	*menu = t->generic.parent;
	int				n, count;
	static char		*lineIndex[256];
	char			*p;

	t->initialized = false;

	// free this if reinitializing
	if (t->fileBuffer) {
		FS_FreeFile (t->fileBuffer);
		t->fileBuffer = NULL;
		t->scrollText = NULL;
	}

	if ( t->fileName && (strlen(t->fileName) > 0)
		&& (count = FS_LoadFile (t->fileName, &t->fileBuffer)) != -1 )
	{
		p = t->fileBuffer;
		for (n = 0; n < 255; n++)
		{
			lineIndex[n] = p;
			while (*p != '\r' && *p != '\n')
			{
				p++;
				if (--count == 0)
					break;
			}
			if (*p == '\r')
			{
				*p++ = 0;
				if (--count == 0)
					break;
			}
			*p++ = 0;
			if (--count == 0)
				break;
		}
		lineIndex[++n] = 0;
		t->scrollText = lineIndex;
		t->initialized = true;
	}
	else if (t->scrollText != NULL)
	{
		t->initialized = true;
	}

	t->generic.textSize = min(max(t->generic.textSize, 4), 32);			// text size must be between 4 and 32 px
	t->lineSize = min(max(t->lineSize, t->generic.textSize+1), t->generic.textSize*2);	// line spacing must be at least 1 px
	t->width = min(max(t->width, t->generic.textSize*4), SCREEN_WIDTH);	// width must be at least 4 columns 
	t->height = min(max(t->height, t->lineSize*8), SCREEN_HEIGHT);		// height must be at least 8 lines
	t->time_scale = min(max(t->time_scale, 0.01f), 0.25f);				// scroll speed must be between 100 and 4 px/s
	t->start_time = cls.realtime;
	t->start_line = 0;

	// set min and max coords
	UI_MenuTextScroll_UpdateCoords (t);
	t->generic.dynamicWidth = 0;
	t->generic.dynamicHeight = 0;
	t->generic.isExtended = false;
//	t->generic.valueChanged = false;

	t->generic.flags |= QMF_NOINTERACTION;
}

//=========================================================

#if 0
void UI_MenuModelView_Reregister (menuModelView_s *m)
{
	int				i;
	char			scratch[MAX_QPATH];

	for (i=0; i<MODELVIEW_MAX_MODELS; i++)
	{
		if (!m->modelValid[i] || !m->model[i])
			continue;

		Com_sprintf( scratch, sizeof(scratch),  m->modelName[i]);
		m->model[i] = R_RegisterModel(scratch);
		if (m->skin[i]) {
			Com_sprintf( scratch, sizeof(scratch),  m->skinName[i]);
			m->skin[i] = R_RegisterSkin (scratch);
		}
	}
}
#endif

void UI_MenuModelView_Draw (menuModelView_s *m)
{
//	menuFramework_s	*menu = m->generic.parent;
	int				i, j, entnum;
	refdef_t		refdef;
	float			rx, ry, rw, rh;
	entity_t		entity[MODELVIEW_MAX_MODELS], *ent;
//	qboolean		reRegister;
	char			scratch[MAX_QPATH];

	memset(&refdef, 0, sizeof(refdef));

	rx = m->generic.topLeft[0];		ry = m->generic.topLeft[1];
	rw = m->width;					rh = m->height;
	SCR_ScaleCoords (&rx, &ry, &rw, &rh, m->generic.scrAlign);
	refdef.x = rx;			refdef.y = ry;
	refdef.width = rw;		refdef.height = rh;
	refdef.fov_x = m->fov;
	refdef.fov_y = CalcFov (refdef.fov_x, refdef.width, refdef.height);
	refdef.time = cls.realtime*0.001;
	refdef.areabits = 0;
	refdef.lightstyles = 0;
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.num_entities = 0;
	refdef.entities = entity;
	entnum = 0;

	for (i=0; i<MODELVIEW_MAX_MODELS; i++)
	{
		if (!m->modelValid[i] || !m->model[i])
			continue;

		ent = &entity[entnum];
		memset (&entity[entnum], 0, sizeof(entity[entnum]));

		// model pointer may become invalid after a vid_restart
	//	reRegister = (!R_ModelIsValid(m->model[i]));
	//	if (reRegister) {
			Com_sprintf (scratch, sizeof(scratch),  m->modelName[i]);
			m->model[i] = R_RegisterModel(scratch);
	//	}
		ent->model = m->model[i];
	//	if (m->skin[i]) {
		if (m->skinName[i] && strlen(m->skinName[i])) {
		//	if (reRegister) {
				Com_sprintf (scratch, sizeof(scratch),  m->skinName[i]);
				m->skin[i] = R_RegisterSkin (scratch);
		//	}
			ent->skin = m->skin[i];
		}

		ent->flags = m->entFlags[i];
		VectorCopy (m->modelOrigin[i], ent->origin);
		VectorCopy (ent->origin, ent->oldorigin);
		ent->frame = m->modelFrame[i] + ((int)(cl.time*0.01f) % (m->modelFrameNumbers[i]+1)); // m->modelFrameTime[i]
	//	ent->oldframe = 
		ent->backlerp = 0.0f;
		for (j=0; j<3; j++)
			ent->angles[j] = m->modelBaseAngles[i][j] + cl.time*m->modelRotation[i][j];
		if (m->isMirrored) {
			ent->flags |= RF_MIRRORMODEL;
			ent->angles[1] = 360 - ent->angles[1];
		}

		refdef.num_entities++;
		entnum++;
	}

	if (refdef.num_entities > 0)
		R_RenderFrame (&refdef);
}

void UI_MenuModelView_UpdateCoords (menuModelView_s *m)
{
	menuFramework_s	*menu = m->generic.parent;

	m->generic.topLeft[0] = menu->x + m->generic.x;
	m->generic.topLeft[1] = menu->y + m->generic.y;
	m->generic.botRight[0] = m->generic.topLeft[0] + m->width;
	m->generic.botRight[1] = m->generic.topLeft[1] + m->height;
}

void UI_MenuModelView_Setup (menuModelView_s *m)
{
//	menuFramework_s	*menu = m->generic.parent;
	int				i, j;
	char			scratch[MAX_QPATH];

	m->num_entities = 0;
	for (i=0; i<MODELVIEW_MAX_MODELS; i++) {
		m->modelValid[i] = false;
		m->model[i] = NULL;	m->skin[i] = NULL;
	}
	m->width	= min(max(m->width, 16), SCREEN_WIDTH);		// width must be at least 16 px
	m->height	= min(max(m->height, 16), SCREEN_HEIGHT);	// height must be at least 16 px
	m->fov		= min(max(m->fov, 10), 110);				// fov must be reasonble
	for (i=0; i<MODELVIEW_MAX_MODELS; i++)
	{
		if (!m->modelName[i] || !strlen(m->modelName[i]))
			continue;

		Com_sprintf( scratch, sizeof(scratch),  m->modelName[i]);
		if ( !(m->model[i] = R_RegisterModel(scratch)) )
			continue;

		if (m->skinName[i] && strlen(m->skinName[i])) {
			Com_sprintf( scratch, sizeof(scratch),  m->skinName[i]);
			m->skin[i] = R_RegisterSkin (scratch);
		}
		for (j=0; j<3; j++) {
			m->modelOrigin[i][j]		= min(max(m->modelOrigin[i][j], -1024), 1024);
			m->modelBaseAngles[i][j]	= min(max(m->modelBaseAngles[i][j], 0), 360);
			m->modelRotation[i][j]		= min(max(m->modelRotation[i][j], -1.0f), 1.0f);
		}
		m->modelFrame[i]		= max(m->modelFrame[i], 0);			// catch negative frames
		m->modelFrameNumbers[i]	= max(m->modelFrameNumbers[i], 0);	// catch negative frames
	//	m->modelFrameTime[i]	= min(max(m->modelFrameTime[i], 0.005f), 0.04f);	// must be between 5 and 40 fps
		m->modelValid[i] = true;
		m->num_entities++;
	}

	// set min and max coords
	UI_MenuModelView_UpdateCoords (m);
	m->generic.dynamicWidth = 0;
	m->generic.dynamicHeight = 0;
	m->generic.isExtended = false;
//	m->generic.valueChanged = false;

	m->generic.flags |= QMF_NOINTERACTION;
}

//=========================================================

/*
==========================
UI_UpdateMenuItemCoords
Just updates coords for display items
==========================
*/
void UI_UpdateMenuItemCoords (void *item)
{
	switch ( ((menuCommon_s *)item)->type )
	{
	case MTYPE_IMAGE:
		UI_MenuImage_UpdateCoords ((menuImage_s *)item);
		break;
	case MTYPE_BUTTON:
		UI_MenuButton_UpdateCoords ((menuButton_s *)item);
		break;
	case MTYPE_RECTANGLE:
		UI_MenuRectangle_UpdateCoords ((menuRectangle_s *)item);
		break;
	case MTYPE_TEXTSCROLL:
		UI_MenuTextScroll_UpdateCoords ((menuTextScroll_s *)item);
		break;
	case MTYPE_MODELVIEW:
		UI_MenuModelView_UpdateCoords ((menuModelView_s *)item);
		break;
	default:
		break;
	}
}


/*
==========================
UI_ItemCanBeCursorItem
Checks if an item is of a
valid type to be a cursor item.
==========================
*/
qboolean UI_ItemCanBeCursorItem (void *item)
{
	if (!item)	return false;
	
	switch ( ((menuCommon_s *)item)->type )
	{
	case MTYPE_IMAGE:
	case MTYPE_BUTTON:
	case MTYPE_RECTANGLE:
	case MTYPE_TEXTSCROLL:
	case MTYPE_MODELVIEW:
		return true;
	default:
		return false;
	}
	return false;
}


/*
==========================
UI_ItemIsValidCursorPosition
Checks if an item can be used
as a cursor position.
==========================
*/
qboolean UI_ItemIsValidCursorPosition (void *item)
{
	if (!item)	return false;
	
	if ( (((menuCommon_s *)item)->flags & QMF_NOINTERACTION) || (((menuCommon_s *)item)->flags & QMF_MOUSEONLY) )
		return false;

	if ( ((menuCommon_s *)item)->isHidden )
		return false;

	switch ( ((menuCommon_s *)item)->type )
	{
	case MTYPE_LABEL:
	case MTYPE_IMAGE:
	case MTYPE_RECTANGLE:
	case MTYPE_TEXTSCROLL:
	case MTYPE_MODELVIEW:
		return false;
	default:
		return true;
	}
	return true;
}


/*
==========================
UI_ItemHasMouseBounds
Checks if an item is mouse-interactive.
==========================
*/
qboolean UI_ItemHasMouseBounds (void *item)
{
	if (!item)	return false;

	if (((menuCommon_s *)item)->flags & QMF_NOINTERACTION)
		return false;

	switch ( ((menuCommon_s *)item)->type )
	{
	case MTYPE_LABEL:
	case MTYPE_IMAGE:
	case MTYPE_RECTANGLE:
	case MTYPE_TEXTSCROLL:
	case MTYPE_MODELVIEW:
		return false;
	default:
		return true;
	}
	return true;
}


/*
==========================
UI_DrawMenuItem
Calls draw functions for each item type
==========================
*/
void UI_DrawMenuItem (void *item)
{
	if (!item)	return;

	// skip hidden items
	if ( ((menuCommon_s *)item)->isHidden )
		return;

	switch ( ((menuCommon_s *)item)->type )
	{
	case MTYPE_ACTION:
		UI_MenuAction_Draw ((menuAction_s *)item);
		break;
	case MTYPE_KEYBIND:
		UI_MenuKeyBind_Draw ((menuKeyBind_s *)item);
		break;
/*	case MTYPE_KEYBINDLIST:
		UI_MenuKeyBindList_Draw ((menuKeyBindList_s *)item);
		break; */
	case MTYPE_FIELD:
		UI_MenuField_Draw ((menuField_s *)item);
		break;
	case MTYPE_SLIDER:
		UI_MenuSlider_Draw ((menuSlider_s *)item);
		break;
	case MTYPE_PICKER:
		UI_MenuPicker_Draw ((menuPicker_s *)item);
		break;
/*	case MTYPE_CHECKBOX:
		UI_MenuCheckBox_Draw ((menuCheckBox_s *)item);
		break; */
	case MTYPE_LABEL:
		UI_MenuLabel_Draw ((menuLabel_s *)item);
		break;
	case MTYPE_IMAGE:
		UI_MenuImage_Draw ((menuImage_s *)item);
		break;
	case MTYPE_BUTTON:
		UI_MenuButton_Draw ((menuButton_s *)item);
		break;
	case MTYPE_RECTANGLE:
		UI_MenuRectangle_Draw ((menuRectangle_s *)item);
		break;
/*	case MTYPE_LISTBOX:
		UI_MenuListBox_Draw ((menuListBox_s *)item);
		break;
	case MTYPE_COMBOBOX:
		UI_MenuComboBox_Draw ((menuComboBox_s *)item);
		break;
	case MTYPE_LISTVIEW:
		UI_MenuListView_Draw ((menuListView_s *)item);
		break; */
	case MTYPE_TEXTSCROLL:
		UI_MenuTextScroll_Draw ((menuTextScroll_s *)item);
		break;
	case MTYPE_MODELVIEW:
		UI_MenuModelView_Draw ((menuModelView_s *)item);
		break;
	default:
		break;
	}
}


/*
==========================
UI_SetMenuItemDynamicSize
Calls SetDynamicSize functions for each item type
==========================
*/
void UI_SetMenuItemDynamicSize (void *item)
{
	if (!item)	return;

	switch ( ((menuCommon_s *)item)->type )
	{
	case MTYPE_KEYBIND:
		UI_MenuKeyBind_SetDynamicSize ((menuKeyBind_s *)item);
		break;
	case MTYPE_PICKER:
		UI_MenuPicker_SetDynamicSize ((menuPicker_s *)item);
		break;
/*	case MTYPE_COMBOBOX:
		UI_MenuComboBox_SetDynamicSize ((menuComboBox_s *)item);
		break; */
	default:
		break;
	}
}


/*
==========================
UI_GetItemMouseoverType
Returns mouseover type for a menu item
==========================
*/
int UI_GetItemMouseoverType (void *item)
{
	if (!item)
		return MENUITEM_NONE;

	switch ( ((menuCommon_s *)item)->type )
	{
		case MTYPE_ACTION:
		case MTYPE_KEYBIND:
			return MENUITEM_ACTION;
	/*	case MTYPE_KEYBINDLIST:
			return MENUITEM_KEYBINDLIST; */
		case MTYPE_SLIDER:
			return MENUITEM_SLIDER;
		case MTYPE_PICKER:
			return MENUITEM_PICKER;
	/*	case MTYPE_CHECKBOX:
			return MENUITEM_CHECKBOX; */
		case MTYPE_FIELD:
			return MENUITEM_TEXT;
		case MTYPE_BUTTON:
			return MENUITEM_BUTTON;
	/*	case MTYPE_LISTBOX:
			return MENUITEM_LISTBOX;
		case MTYPE_COMBOBOX:
			return MENUITEM_COMBOBOX;
		case MTYPE_LISTVIEW:
			return MENUITEM_LISTVIEW; */
		default:
			return MENUITEM_NONE;
	}
	return MENUITEM_NONE;
}


/*
=================
UI_ClickMenuItem
=================
*/
char *UI_ClickMenuItem (menuCommon_s *item, qboolean mouse2)
{
	char *s;

/*	if ( UI_MouseOverScrollKnob(item) && !mouse2 )
	{
	//	Com_Printf ("Dragging scroll bar\n");
		UI_ClickItemScrollBar (item);
		return ui_menu_drag_sound;
	} */

	if (item)
	{
		switch (item->type)
		{
		case MTYPE_ACTION:
			s = UI_MenuAction_Click ( (menuAction_s *)item, mouse2 );
			break;
		case MTYPE_KEYBIND:
			s = UI_MenuKeyBind_Click ( (menuKeyBind_s *)item, mouse2 );
			break;
	/*	case MTYPE_KEYBINDLIST:
			s = UI_MenuKeyBindList_Click ( (menuKeyBindList_s *)item, mouse2 );
			break; */
		case MTYPE_SLIDER:
			s = UI_MenuSlider_Click ( (menuSlider_s *)item, mouse2 );
			break;
		case MTYPE_PICKER:
			s = UI_MenuPicker_Click ( (menuPicker_s *)item, mouse2 );
			break;
		case MTYPE_FIELD:
			s = UI_MenuField_Click ( (menuField_s *)item, mouse2 );
			break;
	/*	case MTYPE_CHECKBOX:
			s = UI_MenuCheckBox_Click ( (menuCheckBox_s *)item, mouse2 );
			break; */
		case MTYPE_BUTTON:
			s = UI_MenuButton_Click ( (menuButton_s *)item, mouse2 );
			break;
	/*	case MTYPE_LISTBOX:
			s = UI_MenuListBox_Click ( (menuListBox_s *)item, mouse2 );
			break;
		case MTYPE_COMBOBOX:
			s = UI_MenuComboBox_Click ( (menuComboBox_s *)item, mouse2 );
			break;
		case MTYPE_LISTVIEW:
			s = UI_MenuListView_Click ( (menuListView_s *)item, mouse2 );
			break; */
		default:
			s = ui_menu_null_sound;
			break;
		}
		if (!s)
			s = ui_menu_null_sound;
		return s;
	}
	return ui_menu_null_sound;
}


/*
=================
UI_SelectMenuItem
=================
*/
qboolean UI_SelectMenuItem (menuFramework_s *s)
{
	menuCommon_s *item=NULL;

	if (!s)	return false;

	item = (menuCommon_s *)UI_ItemAtMenuCursor(s);

	if (item)
	{
		switch (item->type)
		{
		case MTYPE_ACTION:
			UI_MenuAction_DoEnter ((menuAction_s *)item);
			return true;
		case MTYPE_KEYBIND:
			UI_MenuKeyBind_DoEnter ((menuKeyBind_s *)item);
			return true;
		case MTYPE_FIELD:
			return UI_MenuField_DoEnter ((menuField_s *)item) ;
		case MTYPE_PICKER:
		//	UI_MenuSpinControl_DoEnter ((menuPicker_s *)item);
			return false;
		case MTYPE_BUTTON:
			UI_MenuButton_DoEnter ((menuButton_s *)item);
			break;
		default:
			break;
		}
	}
	return false;
}


/*
=================
UI_MouseSelectItem
=================
*/
qboolean UI_MouseSelectItem (menuCommon_s *item)
{
	if (!item)	return false;

	if (item)
	{
		switch (item->type)
		{
		case MTYPE_ACTION:
			UI_MenuAction_DoEnter ((menuAction_s *)item);
			return true;
		case MTYPE_KEYBIND:
			UI_MenuKeyBind_DoEnter ((menuKeyBind_s *)item);
			return true;
		case MTYPE_FIELD:
			return UI_MenuField_DoEnter ((menuField_s *)item) ;
		case MTYPE_PICKER:
			return false;
		case MTYPE_BUTTON:
			UI_MenuButton_DoEnter ((menuButton_s *)item);
			break;
		default:
			break;
		}
	}
	return false;
}


/*
=================
UI_SlideMenuItem
=================
*/
void UI_SlideMenuItem (menuFramework_s *s, int dir)
{
	menuCommon_s *item=NULL;

	if (!s)	return;

	item = (menuCommon_s *) UI_ItemAtMenuCursor(s);

	if (item)
	{
		switch (item->type)
		{
		case MTYPE_SLIDER:
			UI_MenuSlider_DoSlide ((menuSlider_s *) item, dir);
			break;
		case MTYPE_PICKER:
			UI_MenuPicker_DoSlide ((menuPicker_s *) item, dir);
			break;
		default:
			break;
		}
	}
}


/*
==========================
UI_InitMenuItem
Calls setup functions for menu item type
==========================
*/
void UI_InitMenuItem (void *item)
{
	// only some items can be a cursor item
	if ( !UI_ItemCanBeCursorItem(item) )
		((menuCommon_s *)item)->isCursorItem = false;

	// default alignment is center
	if ( ((menuCommon_s *)item)->scrAlign == ALIGN_UNSET )
		((menuCommon_s *)item)->scrAlign = ALIGN_CENTER;

	// clamp text size
	if (!((menuCommon_s *)item)->textSize)
		((menuCommon_s *)item)->textSize = MENU_FONT_SIZE;
	((menuCommon_s *)item)->textSize = min(max(((menuCommon_s *)item)->textSize, 4), 32);

	switch ( ((menuCommon_s *)item)->type )
	{
	case MTYPE_ACTION:
		UI_MenuAction_Setup ((menuAction_s *)item);
		break;
	case MTYPE_KEYBIND:
		UI_MenuKeyBind_Setup ((menuKeyBind_s *)item);
		break;
/*	case MTYPE_KEYBINDLIST:
		UI_MenuKeyBindList_Setup ((menuKeyBindList_s *)item);
		break; */
	case MTYPE_SLIDER:
		UI_MenuSlider_Setup ((menuSlider_s *)item);
		break;
	case MTYPE_PICKER:
		UI_MenuPicker_Setup ((menuPicker_s *)item);
		break;
/*	case MTYPE_CHECKBOX:
		UI_MenuCheckBox_Setup ((menuCheckBox_s *)item);
		break; */
	case MTYPE_LABEL:
		UI_MenuLabel_Setup ((menuLabel_s *)item);
		break;
	case MTYPE_FIELD:
		UI_MenuField_Setup ((menuField_s *)item);
		break;
	case MTYPE_IMAGE:
		UI_MenuImage_Setup ((menuImage_s *)item);
		break;
	case MTYPE_BUTTON:
		UI_MenuButton_Setup ((menuButton_s *)item);
		break;
	case MTYPE_RECTANGLE:
		UI_MenuRectangle_Setup ((menuRectangle_s *)item);
		break;
/*	case MTYPE_LISTBOX:
		UI_MenuListBox_Setup ((menuListBox_s *)item);
		break;
	case MTYPE_COMBOBOX:
		UI_MenuComboBox_Setup ((menuComboBox_s *)item);
		break;
	case MTYPE_LISTVIEW:
		UI_MenuListView_Setup ((menuListView_s *)item);
		break; */
	case MTYPE_TEXTSCROLL:
		UI_MenuTextScroll_Setup ((menuTextScroll_s *)item);
		break;
	case MTYPE_MODELVIEW:
		UI_MenuModelView_Setup ((menuModelView_s *)item);
		break;
	default:
		break;
	}
}
