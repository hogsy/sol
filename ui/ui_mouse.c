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

#define USE_WIDGET_CLICK_HANDLER

/*
=======================================================================

Menu Mouse Cursor - psychospaz

=======================================================================
*/

/*
=================
UI_RefreshCursorButtons
From Q2max
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


#ifndef USE_WIDGET_CLICK_HANDLER
/*
=================
UI_SliderValueForX
=================
*/
int UI_SliderValueForX (menuSlider_s *s, int x)
{
	float	newValue, sliderbase;
	int		newValueInt;
	int		pos;

	if (!s)
		return 0;

//	sliderbase = s->generic.x + s->generic.parent->x + MENU_FONT_SIZE + RCOLUMN_OFFSET;
	sliderbase = s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET + SLIDER_ENDCAP_WIDTH;
	SCR_ScaleCoords (&sliderbase, NULL, NULL, NULL, ALIGN_CENTER);
	pos = x - sliderbase;
//	pos = x - SCR_ScaledScreen(s->generic.x + s->generic.parent->x + MENU_FONT_SIZE + RCOLUMN_OFFSET);

//	newValue = ((float)pos) / ((SLIDER_RANGE-1) * SCR_ScaledScreen(MENU_FONT_SIZE));
	newValue = ((float)pos) / (SCR_ScaledScreen(SLIDER_RANGE*SLIDER_SECTION_WIDTH));
	newValue = min(newValue, 1.0f);
	newValueInt = newValue * (float)(s->maxPos);

	return newValueInt;
}


/*
=================
UI_Slider_CheckSlide
=================
*/
void UI_Slider_CheckSlide (menuSlider_s *s)
{
	if (!s)
		return;

	s->curPos = min(max(s->curPos, 0), s->maxPos);

	if (s->generic.callback)
		s->generic.callback (s);
}


/*
=================
UI_DragSlideItem
=================
*/
void UI_DragSlideItem (menuFramework_s *menu, void *menuitem)
{
	menuSlider_s *slider;

	if (!menu || !menuitem)
		return;

	slider = (menuSlider_s *) menuitem;

	slider->curPos = UI_SliderValueForX(slider, ui_mousecursor.x);
	UI_Slider_CheckSlide (slider);
}


/*
=================
UI_ClickSlideItem
=================
*/
void UI_ClickSlideItem (menuFramework_s *menu, void *menuitem)
{
	menuSlider_s	*slider;
	int				sliderPos, min, max;
	float			x, w, range;
	
	if (!menu || !menuitem)
		return;

	slider = (menuSlider_s *)menuitem;

	range = min(max((float)slider->curPos / (float)slider->maxPos, 0), 1);
//	sliderPos = (int)(slider->generic.x + slider->generic.parent->x + RCOLUMN_OFFSET + MENU_FONT_SIZE + (float)SLIDER_RANGE*(float)MENU_FONT_SIZE*range);
	sliderPos = (int)(slider->generic.x + slider->generic.parent->x + RCOLUMN_OFFSET + SLIDER_ENDCAP_WIDTH + (float)SLIDER_RANGE*(float)SLIDER_SECTION_WIDTH*range);

//	x = menu->x + item->x + sliderPos - 4;
//	w = 8;
	x = sliderPos - (SLIDER_KNOB_WIDTH/2);
	w = SLIDER_KNOB_WIDTH;
	SCR_ScaleCoords (&x, NULL, &w, NULL, ALIGN_CENTER);
	min = x;	max = x + w;

	if (ui_mousecursor.x < min)
		UI_SlideMenuItem (menu, -1);
	if (ui_mousecursor.x > max)
		UI_SlideMenuItem (menu, 1);
}


/*
=================
UI_CheckSlider_Mouseover
=================
*/
qboolean UI_CheckSlider_Mouseover (menuFramework_s *menu, void *menuitem)
{
	int				min[2], max[2];
	float			x1, y1, x2, y2;
	menuSlider_s	*s;

	if (!menu || !menuitem)
		return false;

	s = (menuSlider_s *)menuitem;

	x1 = s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET;
	y1 = s->generic.y + s->generic.parent->y;
	x2 = x1 + SLIDER_ENDCAP_WIDTH + SLIDER_RANGE*SLIDER_SECTION_WIDTH + SLIDER_ENDCAP_WIDTH;
	y2 = y1 + SLIDER_HEIGHT;

	SCR_ScaleCoords (&x1, &y1, NULL, NULL, ALIGN_CENTER);
	SCR_ScaleCoords (&x2, &y2, NULL, NULL, ALIGN_CENTER);
	min[0] = x1;	max[0] = x2;
	min[1] = y1;	max[1] = y2;

	if ( ui_mousecursor.x >= min[0] && ui_mousecursor.x <= max[0] 
		&& ui_mousecursor.y >= min[1] &&  ui_mousecursor.y <= max[1] )
		return true;
	else
		return false;
}
#endif	// USE_WIDGET_CLICK_HANDLER


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

		for (i = menu->nitems; i >= 0 ; i--)
		{
#if 1
			if ( UI_Mouseover_CheckItem (menu, i, lastitem) )
				break;
#else
			int		type, len;
			int		min[2], max[2];
			float	x1, y1, w1, h1;
			menuCommon_s	*item;

			item = ((menuCommon_s * )menu->items[i]);

			if (!item || item->type == MTYPE_LABEL)
				continue;

			x1 = menu->x + item->x + RCOLUMN_OFFSET; // + 2 chars for space + cursor
			y1 = menu->y + item->y;
			w1 = 0;			h1 = item->textSize;	// MENU_FONT_SIZE
			SCR_ScaleCoords (&x1, &y1, &w1, &h1, ALIGN_CENTER);
			min[0] = x1;	max[0] = x1 + w1;
			min[1] = y1;	max[1] = y1 + h1;
		//	max[0] = min[0] = SCR_ScaledScreen(menu->x + item->x + RCOLUMN_OFFSET); //+ 2 chars for space + cursor
		//	max[1] = min[1] = SCR_ScaledScreen(menu->y + item->y);
		//	max[1] += SCR_ScaledScreen(MENU_FONT_SIZE);

			switch (item->type)
			{
				case MTYPE_ACTION:
					{
						len = (int)strlen(item->name);
						
						if (item->flags & QMF_LEFT_JUSTIFY) {
							min[0] += SCR_ScaledScreen(LCOLUMN_OFFSET*2);
							max[0] = min[0] + SCR_ScaledScreen(len*item->textSize);
						}
						else
							min[0] -= SCR_ScaledScreen(len*item->textSize + item->textSize*4);

						type = MENUITEM_ACTION;
					}
					break;
				case MTYPE_SLIDER:
					{
						if (item->name) {
							len = (int)strlen(item->name);
							min[0] -= SCR_ScaledScreen(len*item->textSize - LCOLUMN_OFFSET*2);
						}
						else
							min[0] -= SCR_ScaledScreen(16);
						max[0] += SCR_ScaledScreen((SLIDER_RANGE + 4) * item->textSize);
						type = MENUITEM_SLIDER;
					}
					break;
				case MTYPE_PICKER:
					{
						int len;
						menuPicker_s *spin = menu->items[i];

						if (item->name) {
							len = (int)strlen(item->name);
							min[0] -= SCR_ScaledScreen(len*item->textSize - LCOLUMN_OFFSET*2);
						}

						len = (int)strlen(spin->itemNames[spin->curValue]);
						max[0] += SCR_ScaledScreen(len*item->textSize);

						type = MENUITEM_PICKER;
					}
					break;
				case MTYPE_FIELD:
					{
						menuField_s *text = menu->items[i];

						len = text->visible_length + 2;

						max[0] += SCR_ScaledScreen(len*item->textSize);
						type = MENUITEM_TEXT;
					}
					break;
				case MTYPE_KEYBIND:
					{
						menuKeyBind_s *k = menu->items[i];
						len = (int)strlen(item->name);
						
						if (item->flags & QMF_LEFT_JUSTIFY) {
							min[0] += SCR_ScaledScreen(LCOLUMN_OFFSET*2);
						}
						else {
							min[0] -= SCR_ScaledScreen(len*item->textSize + item->textSize*4);
						}
						max[0] = min[0] + SCR_ScaledScreen(len*item->textSize);

						if (k->commandName)
						{
							UI_FindKeysForCommand (k->commandName, k->keys);
							max[0] += SCR_ScaledScreen(MENU_FONT_SIZE*4);
							if (k->keys[0] == -1)
								max[0] += SCR_ScaledScreen(MENU_FONT_SIZE*3); // "???"
							else {
								max[0] += SCR_ScaledScreen( item->textSize*(int)strlen(Key_KeynumToString(k->keys[0])) ); // key 1
								if (k->keys[1] != -1) // " or " + key2
									max[0] += SCR_ScaledScreen( MENU_FONT_SIZE*4 + item->textSize*(int)strlen(Key_KeynumToString(k->keys[1])) );
							}
						}
						type = MENUITEM_ACTION;
					}
					break;
				default:
					continue;
			}

			if (ui_mousecursor.x >= min[0] && 
				ui_mousecursor.x <= max[0] &&
				ui_mousecursor.y >= min[1] && 
				ui_mousecursor.y <= max[1])
			{
				// new item
				if (lastitem != item)
				{
					int j;

					for (j=0; j<MENU_CURSOR_BUTTON_MAX; j++)
					{
						ui_mousecursor.buttonclicks[j] = 0;
						ui_mousecursor.buttontime[j] = 0;
					}
				}

				ui_mousecursor.menuitem = item;
				ui_mousecursor.menuitemtype = type;
				
				menu->cursor = i;

				break;
			}
#endif	// USE_WIDGET_CLICK_HANDLER
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

/*	if (ui_menuState.draw == Menu_Main_Draw) // have to hack for main menu :p
	{
		UI_CheckMainMenuMouse ();
		return;
	}
	if (ui_menuState.draw == Menu_Credits_Draw) // have to hack for credits :p
	{
		if (ui_mousecursor.buttonclicks[MOUSEBUTTON2])
		{
			ui_mousecursor.buttonused[MOUSEBUTTON2] = true;
			ui_mousecursor.buttonclicks[MOUSEBUTTON2] = 0;
			ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
			ui_mousecursor.buttonclicks[MOUSEBUTTON1] = 0;
			S_StartLocalSound (ui_menu_out_sound);
			if (creditsBuffer)
				FS_FreeFile (creditsBuffer);
			UI_PopMenu();
			return;
		}
	} */

	if (!m)
		return;

	if (ui_mousecursor.menuitem)
	{
#ifdef USE_WIDGET_CLICK_HANDLER
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
#else
		// MOUSE1
		if (ui_mousecursor.buttondown[MOUSEBUTTON1])
		{
			if (ui_mousecursor.menuitemtype == MENUITEM_SLIDER && !ui_mousecursor.buttonused[MOUSEBUTTON1])
			{
				if ( UI_CheckSlider_Mouseover(m, ui_mousecursor.menuitem) ) {
					UI_DragSlideItem (m, ui_mousecursor.menuitem);
					sound = ui_menu_drag_sound;
				}
				else {
					UI_SlideMenuItem (m, 1);
					sound = ui_menu_move_sound;
					ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
				}
			}
			else if (!ui_mousecursor.buttonused[MOUSEBUTTON1] && ui_mousecursor.buttonclicks[MOUSEBUTTON1])
			{
				if (ui_mousecursor.menuitemtype == MENUITEM_PICKER)
				{
					if (ui_item_rotate->integer)					
						UI_SlideMenuItem (m, -1);
					else			
						UI_SlideMenuItem (m, 1);

					sound = ui_menu_move_sound;
					ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
				}
				else
				{
					ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
					UI_MouseSelectItem (ui_mousecursor.menuitem);
					sound = ui_menu_move_sound;
				}
			}
		}
		// MOUSE2
		if (ui_mousecursor.buttondown[MOUSEBUTTON2] && ui_mousecursor.buttonclicks[MOUSEBUTTON2])
		{
			if (ui_mousecursor.menuitemtype == MENUITEM_SLIDER && !ui_mousecursor.buttonused[MOUSEBUTTON2])
			{
				if ( UI_CheckSlider_Mouseover(m, ui_mousecursor.menuitem) ) {
					UI_ClickSlideItem (m, ui_mousecursor.menuitem);
				}
				else {
					UI_SlideMenuItem (m, -1);
				}
				sound = ui_menu_move_sound;
				ui_mousecursor.buttonused[MOUSEBUTTON2] = true;
			}
			else if (!ui_mousecursor.buttonused[MOUSEBUTTON2])
			{
				if (ui_mousecursor.menuitemtype == MENUITEM_PICKER)
				{
					if (ui_item_rotate->integer)					
						UI_SlideMenuItem (m, 1);
					else			
						UI_SlideMenuItem (m, -1);

					sound = ui_menu_move_sound;
					ui_mousecursor.buttonused[MOUSEBUTTON2] = true;
				}
			}
		}
#endif
	}
	else if (!ui_mousecursor.buttonused[MOUSEBUTTON2] && (ui_mousecursor.buttonclicks[MOUSEBUTTON2] == 2)
		&& ui_mousecursor.buttondown[MOUSEBUTTON2])
	{	// Exit with double click 2nd mouse button
		// We need to manually save changes for playerconfig menu here
		if (ui_menuState.draw == Menu_PlayerConfig_Draw)
			Menu_PConfigSaveChanges ();

		UI_PopMenu ();

		sound = ui_menu_out_sound;
		ui_mousecursor.buttonused[MOUSEBUTTON2] = true;
		ui_mousecursor.buttonclicks[MOUSEBUTTON2] = 0;
		ui_mousecursor.buttonused[MOUSEBUTTON1] = true;
		ui_mousecursor.buttonclicks[MOUSEBUTTON1] = 0;
	}

	// clicking on the player model menu...
	if (ui_menuState.draw == Menu_PlayerConfig_Draw)
		Menu_PlayerConfig_MouseClick ();
	// clicking on the screen menu
//	if (ui_menuState.draw == Menu_Options_Screen_Draw)
//		Menu_Options_Screen_Crosshair_MouseClick ();

	if (sound)
		S_StartLocalSound (sound);
}
