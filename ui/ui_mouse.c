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

// ui_mouse.c -- mouse support code

#include "../client/client.h"
#include "ui_local.h"

cursor_t ui_mousecursor;

/*
=======================================================================

Menu Mouse Cursor - psychospaz

=======================================================================
*/

/*
=================
UI_RefreshCursorButtons
=================
*/
void UI_RefreshCursorButtons (void)
{
	ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
	ui_mousecursor.buttondown[MOUSEBUTTON1] = false;
	ui_mousecursor.buttonclicks[MOUSEBUTTON1] = 0;
	ui_mousecursor.buttonused[MOUSEBUTTON2] = true;
	ui_mousecursor.buttondown[MOUSEBUTTON2] = false;
	ui_mousecursor.buttonclicks[MOUSEBUTTON2] = 0;
}


/*
=================
UI_RefreshCursorMenu
=================
*/
void UI_RefreshCursorMenu (void)
{
	ui_mousecursor.menu = NULL;
}


/*
=================
UI_RefreshCursorLink
=================
*/
void UI_RefreshCursorLink (void)
{
	ui_mousecursor.menuitem = NULL;
}


/*
=================
UI_Mouseover_CheckItem
=================
*/
qboolean UI_Mouseover_CheckItem (menuFramework_s *menu, int i, menuCommon_s	*lastitem)
{
	int				itemType, min[2], max[2];
	float			x1, y1, x2, y2;
	menuCommon_s	*item;

	item = ((menuCommon_s * )menu->items[i]);

	if ( !UI_ItemHasMouseBounds(item) )
		return false;
	if ( item->isHidden )
		return false;

	x1 = item->topLeft[0];
	y1 = item->topLeft[1];
	x2 = item->botRight[0];
	y2 = item->botRight[1];

	// add length and height of current item
	UI_SetMenuItemDynamicSize (menu->items[i]);
	x2 += item->dynamicWidth;
	y2 += item->dynamicHeight;

	SCR_ScaleCoords (&x1, &y1, NULL, NULL, item->scrAlign);
	SCR_ScaleCoords (&x2, &y2, NULL, NULL, item->scrAlign);
	min[0] = x1;	max[0] = x2;
	min[1] = y1;	max[1] = y2;

	itemType = UI_GetItemMouseoverType(item);
	if (itemType == MENUITEM_NONE)
		return false;

	if ( (ui_mousecursor.x >= min[0]) &&  (ui_mousecursor.x <= max[0])
		&& (ui_mousecursor.y >= min[1]) && (ui_mousecursor.y <= max[1]) )
	{	// new item
		if (lastitem != item)
		{
			int j;
			for (j=0; j<MENU_CURSOR_BUTTON_MAX; j++) {
				ui_mousecursor.buttonclicks[j] = 0;
				ui_mousecursor.buttontime[j] = 0;
			}
		}
		ui_mousecursor.menuitem = item;
		ui_mousecursor.menuitemtype = itemType;
		// don't set menu cursor for mouse-only items
		if (!(item->flags & QMF_MOUSEONLY))
			menu->cursor = i;
		return true;
	}
	return false;
}


/*
=================
UI_Mouseover_Check
=================
*/
void UI_Mouseover_Check (menuFramework_s *menu)
{
	int				i;
	menuCommon_s	*lastitem;

	ui_mousecursor.menu = menu;

	// don't allow change in item focus if waiting to grab a key
	if ( UI_HasValidGrabBindItem(menu) ) 
		return;

	if (ui_mousecursor.mouseaction)
	{
		lastitem = ui_mousecursor.menuitem;
		UI_RefreshCursorLink ();

	//	for (i = menu->nitems; i >= 0 ; i--)
		for (i=0; i<=menu->nitems; i++)
		{
			if ( UI_Mouseover_CheckItem (menu, i, lastitem) )
				break;
		}
	}
	ui_mousecursor.mouseaction = false;
}


/*
=================
UI_MouseCursor_Think
=================
*/
void UI_MouseCursor_Think (void)
{
	char * sound = NULL;
	menuFramework_s *m = (menuFramework_s *)ui_mousecursor.menu;

	if (!m)
		return;

	if (ui_mousecursor.menuitem)
	{
		// MOUSE1
		if (ui_mousecursor.buttondown[MOUSEBUTTON1] && ui_mousecursor.buttonclicks[MOUSEBUTTON1]
			&& !ui_mousecursor.buttonused[MOUSEBUTTON1])
		{
			sound = UI_ClickMenuItem (ui_mousecursor.menuitem, false);
			if ( !strcmp(sound, ui_menu_drag_sound) )	// dragging an item does not make sound
				sound = ui_menu_null_sound;
			else
				ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
		}
		// MOUSE2
		if (ui_mousecursor.buttondown[MOUSEBUTTON2] && ui_mousecursor.buttonclicks[MOUSEBUTTON2]
			&& !ui_mousecursor.buttonused[MOUSEBUTTON2])
		{
			sound = UI_ClickMenuItem (ui_mousecursor.menuitem, true);
			ui_mousecursor.buttonused[MOUSEBUTTON2] = true;
		}
	}
	else if (!ui_mousecursor.buttonused[MOUSEBUTTON2] && (ui_mousecursor.buttonclicks[MOUSEBUTTON2] == 2)
		&& ui_mousecursor.buttondown[MOUSEBUTTON2])
	{	// Exit with double click 2nd mouse button
		UI_CheckAndPopMenu (m);

		ui_mousecursor.buttonused[MOUSEBUTTON2] = true;
		ui_mousecursor.buttonclicks[MOUSEBUTTON2] = 0;
		ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
		ui_mousecursor.buttonclicks[MOUSEBUTTON1] = 0;
		sound = ui_menu_out_sound;
	}

	if (sound)
		S_StartLocalSound (sound);
}
