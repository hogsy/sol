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

// ui_dmoptions.c -- DM options menu 

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client/client.h"
#include "ui_local.h"

/*
=============================================================================

DMOPTIONS BOOK MENU

=============================================================================
*/
static char dmoptions_statusbar[128];

static menuframework_s s_dmoptions_menu;

static menulist_s	s_friendlyfire_box;
static menulist_s	s_falls_box;
static menulist_s	s_weapons_stay_box;
static menulist_s	s_instant_powerups_box;
static menulist_s	s_powerups_box;
static menulist_s	s_health_box;
static menulist_s	s_spawn_farthest_box;
static menulist_s	s_teamplay_box;
static menulist_s	s_samelevel_box;
static menulist_s	s_force_respawn_box;
static menulist_s	s_armor_box;
static menulist_s	s_allow_exit_box;
static menulist_s	s_infinite_ammo_box;
static menulist_s	s_fixed_fov_box;
static menulist_s	s_quad_drop_box;

//Xatrix
static menulist_s	s_quadfire_drop_box;

//ROGUE
static menulist_s	s_no_mines_box;
static menulist_s	s_no_nukes_box;
static menulist_s	s_stack_double_box;
static menulist_s	s_no_spheres_box;

// CTF
static menulist_s	s_ctf_forceteam_box;
static menulist_s	s_ctf_armor_protect_box;
static menulist_s	s_ctf_notechs_box;

static menuaction_s	s_dmoptions_back_action;
extern	menulist_s	s_rules_box;


#define DF_CTF_FORCEJOIN	131072	
#define DF_ARMOR_PROTECT	262144
#define DF_CTF_NO_TECH      524288

qboolean CTF_menumode (void)
{
	if ( (FS_RoguePath() && s_rules_box.curValue >= 3)
		|| (!FS_RoguePath() && s_rules_box.curValue >= 2) )
		return true;
	return false;
}

static void DMFlagCallback (void *self)
{
	menulist_s *f = ( menulist_s * ) self;
	int flags;
	int bit = 0;

	flags = Cvar_VariableValue( "dmflags" );

	if ( f == &s_friendlyfire_box )
	{
		if ( f->curValue )
			flags &= ~DF_NO_FRIENDLY_FIRE;
		else
			flags |= DF_NO_FRIENDLY_FIRE;
		goto setvalue;
	}
	else if ( f == &s_falls_box )
	{
		if ( f->curValue )
			flags &= ~DF_NO_FALLING;
		else
			flags |= DF_NO_FALLING;
		goto setvalue;
	}
	else if ( f == &s_weapons_stay_box ) 
	{
		bit = DF_WEAPONS_STAY;
	}
	else if ( f == &s_instant_powerups_box )
	{
		bit = DF_INSTANT_ITEMS;
	}
	else if ( f == &s_allow_exit_box )
	{
		bit = DF_ALLOW_EXIT;
	}
	else if ( f == &s_powerups_box )
	{
		if ( f->curValue )
			flags &= ~DF_NO_ITEMS;
		else
			flags |= DF_NO_ITEMS;
		goto setvalue;
	}
	else if ( f == &s_health_box )
	{
		if ( f->curValue )
			flags &= ~DF_NO_HEALTH;
		else
			flags |= DF_NO_HEALTH;
		goto setvalue;
	}
	else if ( f == &s_spawn_farthest_box )
	{
		bit = DF_SPAWN_FARTHEST;
	}
	else if ( f == &s_teamplay_box )
	{
		if ( f->curValue == 1 )
		{
			flags |=  DF_SKINTEAMS;
			flags &= ~DF_MODELTEAMS;
		}
		else if ( f->curValue == 2 )
		{
			flags |=  DF_MODELTEAMS;
			flags &= ~DF_SKINTEAMS;
		}
		else
		{
			flags &= ~( DF_MODELTEAMS | DF_SKINTEAMS );
		}

		goto setvalue;
	}
	else if ( f == &s_samelevel_box )
	{
		bit = DF_SAME_LEVEL;
	}
	else if ( f == &s_force_respawn_box )
	{
		bit = DF_FORCE_RESPAWN;
	}
	else if ( f == &s_armor_box )
	{
		if ( f->curValue )
			flags &= ~DF_NO_ARMOR;
		else
			flags |= DF_NO_ARMOR;
		goto setvalue;
	}
	else if ( f == &s_infinite_ammo_box )
	{
		bit = DF_INFINITE_AMMO;
	}
	else if ( f == &s_fixed_fov_box )
	{
		bit = DF_FIXED_FOV;
	}
	else if ( f == &s_quad_drop_box )
	{
		bit = DF_QUAD_DROP;
	}
	// Knightmare added
	else if ( FS_XatrixPath() )
	{
		if ( f == &s_quadfire_drop_box)
		{
			bit = DF_QUADFIRE_DROP;
		}
	}
//=======
//ROGUE
	else if ( FS_RoguePath() )
	{
		if ( f == &s_no_mines_box)
		{
			bit = DF_NO_MINES;
		}
		else if ( f == &s_no_nukes_box)
		{
			bit = DF_NO_NUKES;
		}
		else if ( f == &s_stack_double_box)
		{
			bit = DF_NO_STACK_DOUBLE;
		}
		else if ( f == &s_no_spheres_box)
		{
			bit = DF_NO_SPHERES;
		}
	}
//ROGUE
//=======
	// Knightmare added-  CTF flags
	else if ( CTF_menumode() )
	{
		if ( f == &s_ctf_forceteam_box)
		{
			bit = DF_CTF_FORCEJOIN;
		}
		else if ( f == &s_ctf_armor_protect_box)
		{
			bit = DF_ARMOR_PROTECT;
		}
		else if ( f == &s_ctf_notechs_box)
		{
			bit = DF_CTF_NO_TECH;
		}
	}

	if ( f )
	{
		if ( f->curValue == 0 )
			flags &= ~bit;
		else
			flags |= bit;
	}

setvalue:
	Cvar_SetValue ("dmflags", flags);
	Com_sprintf( dmoptions_statusbar, sizeof( dmoptions_statusbar ), "dmflags = %d", flags );
}

void Menu_DMOptions_Init (void)
{
	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};
	static const char *teamplay_names[] = 
	{
		"disabled", "by skin", "by model", 0
	};
	int dmflags = Cvar_VariableValue( "dmflags" );
	int y = 0;

	s_dmoptions_menu.x = SCREEN_WIDTH*0.5;
//	s_dmoptions_menu.x = viddef.width * 0.50;
	s_dmoptions_menu.y = SCREEN_HEIGHT*0.5 - 80;
	s_dmoptions_menu.nitems = 0;

	s_falls_box.generic.type = MTYPE_SPINCONTROL;
	s_falls_box.generic.textSize = MENU_FONT_SIZE;
	s_falls_box.generic.x	= 0;
	s_falls_box.generic.y	= y; // 0
	s_falls_box.generic.name	= "falling damage";
	s_falls_box.generic.callback = DMFlagCallback;
	s_falls_box.itemNames = yes_no_names;
	s_falls_box.curValue = ( dmflags & DF_NO_FALLING ) == 0;

	s_weapons_stay_box.generic.type = MTYPE_SPINCONTROL;
	s_weapons_stay_box.generic.textSize = MENU_FONT_SIZE;
	s_weapons_stay_box.generic.x	= 0;
	s_weapons_stay_box.generic.y	= y += MENU_LINE_SIZE;
	s_weapons_stay_box.generic.name	= "weapons stay";
	s_weapons_stay_box.generic.callback = DMFlagCallback;
	s_weapons_stay_box.itemNames = yes_no_names;
	s_weapons_stay_box.curValue = ( dmflags & DF_WEAPONS_STAY ) != 0;

	s_instant_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_instant_powerups_box.generic.textSize = MENU_FONT_SIZE;
	s_instant_powerups_box.generic.x	= 0;
	s_instant_powerups_box.generic.y	= y += MENU_LINE_SIZE;
	s_instant_powerups_box.generic.name	= "instant powerups";
	s_instant_powerups_box.generic.callback = DMFlagCallback;
	s_instant_powerups_box.itemNames = yes_no_names;
	s_instant_powerups_box.curValue = ( dmflags & DF_INSTANT_ITEMS ) != 0;

	s_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_powerups_box.generic.textSize = MENU_FONT_SIZE;
	s_powerups_box.generic.x	= 0;
	s_powerups_box.generic.y	= y += MENU_LINE_SIZE;
	s_powerups_box.generic.name	= "allow powerups";
	s_powerups_box.generic.callback = DMFlagCallback;
	s_powerups_box.itemNames = yes_no_names;
	s_powerups_box.curValue = ( dmflags & DF_NO_ITEMS ) == 0;

	s_health_box.generic.type = MTYPE_SPINCONTROL;
	s_health_box.generic.textSize = MENU_FONT_SIZE;
	s_health_box.generic.x	= 0;
	s_health_box.generic.y	= y += MENU_LINE_SIZE;
	s_health_box.generic.callback = DMFlagCallback;
	s_health_box.generic.name	= "allow health";
	s_health_box.itemNames = yes_no_names;
	s_health_box.curValue = ( dmflags & DF_NO_HEALTH ) == 0;

	s_armor_box.generic.type = MTYPE_SPINCONTROL;
	s_armor_box.generic.textSize = MENU_FONT_SIZE;
	s_armor_box.generic.x	= 0;
	s_armor_box.generic.y	= y += MENU_LINE_SIZE;
	s_armor_box.generic.name	= "allow armor";
	s_armor_box.generic.callback = DMFlagCallback;
	s_armor_box.itemNames = yes_no_names;
	s_armor_box.curValue = ( dmflags & DF_NO_ARMOR ) == 0;

	s_spawn_farthest_box.generic.type = MTYPE_SPINCONTROL;
	s_spawn_farthest_box.generic.textSize = MENU_FONT_SIZE;
	s_spawn_farthest_box.generic.x	= 0;
	s_spawn_farthest_box.generic.y	= y += MENU_LINE_SIZE;
	s_spawn_farthest_box.generic.name	= "spawn farthest";
	s_spawn_farthest_box.generic.callback = DMFlagCallback;
	s_spawn_farthest_box.itemNames = yes_no_names;
	s_spawn_farthest_box.curValue = ( dmflags & DF_SPAWN_FARTHEST ) != 0;

	s_samelevel_box.generic.type = MTYPE_SPINCONTROL;
	s_samelevel_box.generic.textSize = MENU_FONT_SIZE;
	s_samelevel_box.generic.x	= 0;
	s_samelevel_box.generic.y	= y += MENU_LINE_SIZE;
	s_samelevel_box.generic.name	= "same map";
	s_samelevel_box.generic.callback = DMFlagCallback;
	s_samelevel_box.itemNames = yes_no_names;
	s_samelevel_box.curValue = ( dmflags & DF_SAME_LEVEL ) != 0;

	s_force_respawn_box.generic.type = MTYPE_SPINCONTROL;
	s_force_respawn_box.generic.textSize = MENU_FONT_SIZE;
	s_force_respawn_box.generic.x	= 0;
	s_force_respawn_box.generic.y	= y += MENU_LINE_SIZE;
	s_force_respawn_box.generic.name	= "force respawn";
	s_force_respawn_box.generic.callback = DMFlagCallback;
	s_force_respawn_box.itemNames = yes_no_names;
	s_force_respawn_box.curValue = ( dmflags & DF_FORCE_RESPAWN ) != 0;

	s_teamplay_box.generic.type = MTYPE_SPINCONTROL;
	s_teamplay_box.generic.textSize = MENU_FONT_SIZE;
	s_teamplay_box.generic.x	= 0;
	s_teamplay_box.generic.y	= y += MENU_LINE_SIZE;
	s_teamplay_box.generic.name	= "teamplay";
	s_teamplay_box.generic.callback = DMFlagCallback;
	s_teamplay_box.itemNames = teamplay_names;
	s_teamplay_box.curValue = (dmflags & DF_SKINTEAMS) ? 1 : ((dmflags & DF_MODELTEAMS) ? 2 : 0);

	s_allow_exit_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_exit_box.generic.textSize = MENU_FONT_SIZE;
	s_allow_exit_box.generic.x	= 0;
	s_allow_exit_box.generic.y	= y += MENU_LINE_SIZE;
	s_allow_exit_box.generic.name	= "allow exit";
	s_allow_exit_box.generic.callback = DMFlagCallback;
	s_allow_exit_box.itemNames = yes_no_names;
	s_allow_exit_box.curValue = ( dmflags & DF_ALLOW_EXIT ) != 0;

	s_infinite_ammo_box.generic.type = MTYPE_SPINCONTROL;
	s_infinite_ammo_box.generic.textSize = MENU_FONT_SIZE;
	s_infinite_ammo_box.generic.x	= 0;
	s_infinite_ammo_box.generic.y	= y += MENU_LINE_SIZE;
	s_infinite_ammo_box.generic.name	= "infinite ammo";
	s_infinite_ammo_box.generic.callback = DMFlagCallback;
	s_infinite_ammo_box.itemNames = yes_no_names;
	s_infinite_ammo_box.curValue = ( dmflags & DF_INFINITE_AMMO ) != 0;

	s_fixed_fov_box.generic.type = MTYPE_SPINCONTROL;
	s_fixed_fov_box.generic.textSize = MENU_FONT_SIZE;
	s_fixed_fov_box.generic.x	= 0;
	s_fixed_fov_box.generic.y	= y += MENU_LINE_SIZE;
	s_fixed_fov_box.generic.name	= "fixed FOV";
	s_fixed_fov_box.generic.callback = DMFlagCallback;
	s_fixed_fov_box.itemNames = yes_no_names;
	s_fixed_fov_box.curValue = ( dmflags & DF_FIXED_FOV ) != 0;

	s_quad_drop_box.generic.type = MTYPE_SPINCONTROL;
	s_quad_drop_box.generic.textSize = MENU_FONT_SIZE;
	s_quad_drop_box.generic.x	= 0;
	s_quad_drop_box.generic.y	= y += MENU_LINE_SIZE;
	s_quad_drop_box.generic.name	= "quad drop";
	s_quad_drop_box.generic.callback = DMFlagCallback;
	s_quad_drop_box.itemNames = yes_no_names;
	s_quad_drop_box.curValue = ( dmflags & DF_QUAD_DROP ) != 0;

	s_friendlyfire_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_box.generic.textSize = MENU_FONT_SIZE;
	s_friendlyfire_box.generic.x	= 0;
	s_friendlyfire_box.generic.y	= y += MENU_LINE_SIZE;
	s_friendlyfire_box.generic.name	= "friendly fire";
	s_friendlyfire_box.generic.callback = DMFlagCallback;
	s_friendlyfire_box.itemNames = yes_no_names;
	s_friendlyfire_box.curValue = ( dmflags & DF_NO_FRIENDLY_FIRE ) == 0;

	// Knightmare added
	if ( FS_XatrixPath() )
	{
		s_quadfire_drop_box.generic.type = MTYPE_SPINCONTROL;
		s_quadfire_drop_box.generic.textSize = MENU_FONT_SIZE;
		s_quadfire_drop_box.generic.x	= 0;
		s_quadfire_drop_box.generic.y	= y += MENU_LINE_SIZE;
		s_quadfire_drop_box.generic.name	= "dualfire drop";
		s_quadfire_drop_box.generic.callback = DMFlagCallback;
		s_quadfire_drop_box.itemNames = yes_no_names;
		s_quadfire_drop_box.curValue = ( dmflags & DF_QUADFIRE_DROP ) != 0;
	}
//============
//ROGUE
	// Knightmare 12/23/2001
	else if ( FS_RoguePath() )
	{
		s_no_mines_box.generic.type = MTYPE_SPINCONTROL;
		s_no_mines_box.generic.textSize = MENU_FONT_SIZE;
		s_no_mines_box.generic.x	= 0;
		s_no_mines_box.generic.y	= y += MENU_LINE_SIZE;
		s_no_mines_box.generic.name	= "remove mines";
		s_no_mines_box.generic.callback = DMFlagCallback;
		s_no_mines_box.itemNames = yes_no_names;
		s_no_mines_box.curValue = ( dmflags & DF_NO_MINES ) != 0;

		s_no_nukes_box.generic.type = MTYPE_SPINCONTROL;
		s_no_nukes_box.generic.textSize = MENU_FONT_SIZE;
		s_no_nukes_box.generic.x	= 0;
		s_no_nukes_box.generic.y	= y += MENU_LINE_SIZE;
		s_no_nukes_box.generic.name	= "remove nukes";
		s_no_nukes_box.generic.callback = DMFlagCallback;
		s_no_nukes_box.itemNames = yes_no_names;
		s_no_nukes_box.curValue = ( dmflags & DF_NO_NUKES ) != 0;

		s_stack_double_box.generic.type = MTYPE_SPINCONTROL;
		s_stack_double_box.generic.textSize = MENU_FONT_SIZE;
		s_stack_double_box.generic.x	= 0;
		s_stack_double_box.generic.y	= y += MENU_LINE_SIZE;
		s_stack_double_box.generic.name	= "2x/4x stacking off";
		s_stack_double_box.generic.callback = DMFlagCallback;
		s_stack_double_box.itemNames = yes_no_names;
		s_stack_double_box.curValue = ( dmflags & DF_NO_STACK_DOUBLE ) != 0;

		s_no_spheres_box.generic.type = MTYPE_SPINCONTROL;
		s_no_spheres_box.generic.textSize = MENU_FONT_SIZE;
		s_no_spheres_box.generic.x	= 0;
		s_no_spheres_box.generic.y	= y += MENU_LINE_SIZE;
		s_no_spheres_box.generic.name	= "remove spheres";
		s_no_spheres_box.generic.callback = DMFlagCallback;
		s_no_spheres_box.itemNames = yes_no_names;
		s_no_spheres_box.curValue = ( dmflags & DF_NO_SPHERES ) != 0;

	}
//ROGUE
//============
	// Knightmare added
	else if ( CTF_menumode() )
	{
		s_ctf_forceteam_box.generic.type = MTYPE_SPINCONTROL;
		s_ctf_forceteam_box.generic.textSize = MENU_FONT_SIZE;
		s_ctf_forceteam_box.generic.x	= 0;
		s_ctf_forceteam_box.generic.y	= y += MENU_LINE_SIZE;
		s_ctf_forceteam_box.generic.name	= "force team join";
		s_ctf_forceteam_box.generic.callback = DMFlagCallback;
		s_ctf_forceteam_box.itemNames = yes_no_names;
		s_ctf_forceteam_box.curValue = ( dmflags & DF_CTF_FORCEJOIN ) != 0;

		s_ctf_armor_protect_box.generic.type = MTYPE_SPINCONTROL;
		s_ctf_armor_protect_box.generic.textSize = MENU_FONT_SIZE;
		s_ctf_armor_protect_box.generic.x	= 0;
		s_ctf_armor_protect_box.generic.y	= y += MENU_LINE_SIZE;
		s_ctf_armor_protect_box.generic.name	= "team armor protect";
		s_ctf_armor_protect_box.generic.callback = DMFlagCallback;
		s_ctf_armor_protect_box.itemNames = yes_no_names;
		s_ctf_armor_protect_box.curValue = ( dmflags & DF_ARMOR_PROTECT ) != 0;

		s_ctf_notechs_box.generic.type = MTYPE_SPINCONTROL;
		s_ctf_notechs_box.generic.textSize = MENU_FONT_SIZE;
		s_ctf_notechs_box.generic.x	= 0;
		s_ctf_notechs_box.generic.y	= y += MENU_LINE_SIZE;
		s_ctf_notechs_box.generic.name	= "disable techs";
		s_ctf_notechs_box.generic.callback = DMFlagCallback;
		s_ctf_notechs_box.itemNames = yes_no_names;
		s_ctf_notechs_box.curValue = ( dmflags & DF_CTF_NO_TECH ) != 0;
	}

	s_dmoptions_back_action.generic.type = MTYPE_ACTION;
	s_dmoptions_back_action.generic.textSize = MENU_FONT_SIZE;
	s_dmoptions_back_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_dmoptions_back_action.generic.x	= 0;
	s_dmoptions_back_action.generic.y	= y += 3*MENU_LINE_SIZE;
	s_dmoptions_back_action.generic.name	= " back";
	s_dmoptions_back_action.generic.callback = UI_BackMenu;

	UI_AddMenuItem (&s_dmoptions_menu, &s_falls_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_weapons_stay_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_instant_powerups_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_powerups_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_health_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_armor_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_spawn_farthest_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_samelevel_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_force_respawn_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_teamplay_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_allow_exit_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_infinite_ammo_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_fixed_fov_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_quad_drop_box);
	UI_AddMenuItem (&s_dmoptions_menu, &s_friendlyfire_box);

	// Xatrix
	if ( FS_XatrixPath() )
	{
		UI_AddMenuItem (&s_dmoptions_menu, &s_quadfire_drop_box);
	}

	// Rogue
	else if ( FS_RoguePath() )
	{
		UI_AddMenuItem (&s_dmoptions_menu, &s_no_mines_box);
		UI_AddMenuItem (&s_dmoptions_menu, &s_no_nukes_box);
		UI_AddMenuItem (&s_dmoptions_menu, &s_stack_double_box);
		UI_AddMenuItem (&s_dmoptions_menu, &s_no_spheres_box);
	}

	// CTF
	else if ( CTF_menumode() )
	{
		UI_AddMenuItem (&s_dmoptions_menu, &s_ctf_forceteam_box);
		UI_AddMenuItem (&s_dmoptions_menu, &s_ctf_armor_protect_box);
		UI_AddMenuItem (&s_dmoptions_menu, &s_ctf_notechs_box);
	}
	UI_AddMenuItem (&s_dmoptions_menu, &s_dmoptions_back_action);

//	UI_CenterMenu (&s_dmoptions_menu);

	// set the original dmflags statusbar
	DMFlagCallback (0);
	UI_SetMenuStatusBar (&s_dmoptions_menu, dmoptions_statusbar);
}

void Menu_DMOptions_Draw (void)
{
	UI_DrawBanner ("m_banner_start_server"); // added
	UI_DrawMenu (&s_dmoptions_menu);
}

const char *Menu_DMOptions_Key (int key)
{
	return UI_DefaultMenuKey (&s_dmoptions_menu, key);
}

void Menu_DMOptions_f (void)
{
	Menu_DMOptions_Init ();
	UI_PushMenu (Menu_DMOptions_Draw, Menu_DMOptions_Key);
}
