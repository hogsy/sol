/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2000-2002 Mr. Hyde and Mad Dog

This file is part of Lazarus Quake 2 Mod source code.

Lazarus Quake 2 Mod source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Lazarus Quake 2 Mod source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lazarus Quake 2 Mod source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"
#include "game_entity_manager.h"

void SP_item_health (edict_t *self);
void SP_item_health_small (edict_t *self);
void SP_item_health_large (edict_t *self);
void SP_item_health_mega (edict_t *self);

void SP_info_player_start (edict_t *ent);
void SP_info_player_deathmatch (edict_t *ent);
void SP_info_player_coop (edict_t *ent);
void SP_info_player_intermission (edict_t *ent);

void SP_func_plat (edict_t *ent);
void SP_func_plat2 (edict_t *ent);	// Knightmare added
void SP_func_rotating (edict_t *ent);
void SP_func_button (edict_t *ent);
void SP_func_door (edict_t *ent);
void SP_func_door_secret (edict_t *ent);
void SP_func_door_secret2 (edict_t *ent);	// Knightmare added
void SP_func_door_rotating (edict_t *ent);
void SP_func_water (edict_t *ent);
void SP_func_train (edict_t *ent);
void SP_func_conveyor (edict_t *self);
void SP_func_wall (edict_t *self);
void SP_func_object (edict_t *self);
void SP_func_explosive (edict_t *self);
void SP_func_timer (edict_t *self);
void SP_func_areaportal (edict_t *ent);
void SP_func_clock (edict_t *ent);
void SP_func_killbox (edict_t *ent);

void SP_trigger_always (edict_t *ent);
void SP_trigger_once (edict_t *ent);
void SP_trigger_multiple (edict_t *ent);
void SP_trigger_relay (edict_t *ent);
void SP_trigger_push (edict_t *ent);
void SP_trigger_push_bbox (edict_t *ent);
void SP_trigger_hurt (edict_t *ent);
void SP_trigger_hurt_bbox (edict_t *ent);
void SP_trigger_key (edict_t *ent);
void SP_trigger_counter (edict_t *ent);
void SP_trigger_elevator (edict_t *ent);
void SP_trigger_gravity (edict_t *ent);
void SP_trigger_gravity_bbox (edict_t *ent);
void SP_trigger_monsterjump (edict_t *ent);
void SP_trigger_monsterjump_bbox (edict_t *ent);

void SP_target_temp_entity (edict_t *ent);
void SP_target_speaker (edict_t *ent);
void SP_target_explosion (edict_t *ent);
void SP_target_changelevel (edict_t *ent);
void SP_target_secret (edict_t *ent);
void SP_target_goal (edict_t *ent);
void SP_target_splash (edict_t *ent);
void SP_target_spawner (edict_t *ent);
void SP_target_blaster (edict_t *ent);
void SP_target_crosslevel_trigger (edict_t *ent);
void SP_target_crosslevel_target (edict_t *ent);
void SP_target_laser (edict_t *self);
void SP_target_help (edict_t *ent);
void SP_target_actor (edict_t *ent);
void SP_target_lightramp (edict_t *self);
void SP_target_earthquake (edict_t *ent);
void SP_target_character (edict_t *ent);
void SP_target_string (edict_t *ent);

void SP_worldspawn (edict_t *ent);
void SP_viewthing (edict_t *ent);

void SP_light (edict_t *self);
void SP_light_mine1 (edict_t *ent);
void SP_light_mine2 (edict_t *ent);
void SP_info_null (edict_t *self);
void SP_info_notnull (edict_t *self);
void SP_path_corner (edict_t *self);
void SP_point_combat (edict_t *self);

void SP_misc_explobox (edict_t *self);
void SP_misc_banner (edict_t *self);
void SP_misc_satellite_dish (edict_t *self);
void SP_misc_actor (edict_t *self);
void SP_misc_gib_arm (edict_t *self);
void SP_misc_gib_leg (edict_t *self);
void SP_misc_gib_head (edict_t *self);
void SP_misc_insane (edict_t *self);
void SP_misc_deadsoldier (edict_t *self);
void SP_misc_viper (edict_t *self);
void SP_misc_viper_bomb (edict_t *self);
void SP_misc_bigviper (edict_t *self);
void SP_misc_strogg_ship (edict_t *self);
void SP_misc_teleporter (edict_t *self);
void SP_misc_teleporter_dest (edict_t *self);
void SP_misc_blackhole (edict_t *self);
void SP_misc_eastertank (edict_t *self);
void SP_misc_easterchick (edict_t *self);
void SP_misc_easterchick2 (edict_t *self);

void SP_misc_flare (edict_t *self);

void SP_monster_berserk (edict_t *self);
void SP_monster_gladiator (edict_t *self);
void SP_monster_gunner (edict_t *self);
void SP_monster_infantry (edict_t *self);
void SP_monster_soldier_light (edict_t *self);
void SP_monster_soldier (edict_t *self);
void SP_monster_soldier_ss (edict_t *self);
void SP_monster_tank (edict_t *self);
void SP_monster_medic (edict_t *self);
void SP_monster_flipper (edict_t *self);
void SP_monster_chick (edict_t *self);
void SP_monster_parasite (edict_t *self);
void SP_monster_flyer (edict_t *self);
void SP_monster_brain (edict_t *self);
void SP_monster_floater (edict_t *self);
void SP_monster_hover (edict_t *self);
void SP_monster_mutant (edict_t *self);
void SP_monster_supertank (edict_t *self);
void SP_monster_boss2 (edict_t *self);
void SP_monster_jorg (edict_t *self);
void SP_monster_boss3_stand (edict_t *self);

void SP_monster_commander_body (edict_t *self);

void SP_turret_breach (edict_t *self);
void SP_turret_base (edict_t *self);
void SP_turret_driver (edict_t *self);

// Lazarus
void SP_crane_beam (edict_t *self);
void SP_crane_hoist (edict_t *self);
void SP_crane_hook (edict_t *self);
void SP_crane_control (edict_t *self);
void SP_crane_reset (edict_t *self);
void SP_hint_path (edict_t *self);
void SP_func_bobbingwater (edict_t *self);
void SP_func_door_rot_dh (edict_t *self);
void SP_func_door_swinging (edict_t *self);

void SP_func_breakaway (edict_t *self); // Knightmare added
void SP_func_force_wall(edict_t *ent);
void SP_func_monitor (edict_t *self);
void SP_func_pendulum (edict_t *self);
void SP_func_pivot (edict_t *self);
void SP_func_pushable (edict_t *self);
void SP_func_reflect (edict_t *self);
void SP_func_rotating_dh (edict_t *self);
void SP_func_trackchange (edict_t *self);
void SP_func_tracktrain (edict_t *self);
void SP_func_trainbutton (edict_t *self);
void SP_func_vehicle (edict_t *self);
void SP_info_train_start (edict_t *self);
void SP_misc_light (edict_t *self);
void SP_model_spawn (edict_t *self);
void SP_model_train (edict_t *self);
void SP_model_turret (edict_t *self);
void SP_monster_makron (edict_t *self);
void SP_path_track (edict_t *self);
void SP_target_anger (edict_t *self);
void SP_target_animation (edict_t *self);
void SP_target_attractor (edict_t *self);
void SP_target_CD (edict_t *self);
void SP_target_change (edict_t *self);
void SP_target_clone (edict_t *self);
void SP_target_effect (edict_t *self);
void SP_target_fade (edict_t *self);
void SP_target_failure (edict_t *self);
void SP_target_fog (edict_t *self);
void SP_target_fountain (edict_t *self);
void SP_target_lightswitch (edict_t *self);
void SP_target_locator (edict_t *self);
void SP_target_lock (edict_t *self);
void SP_target_lock_clue (edict_t *self);
void SP_target_lock_code (edict_t *self);
void SP_target_lock_digit (edict_t *self);
void SP_target_monitor (edict_t *ent);
void SP_target_monsterbattle (edict_t *self);
void SP_target_movewith (edict_t *self);
void SP_target_precipitation (edict_t *self);
void SP_target_rocks (edict_t *self);
void SP_target_rotation (edict_t *self);
void SP_target_command (edict_t *self);
void SP_target_set_effect (edict_t *self);
void SP_target_skill (edict_t *self);
void SP_target_sky (edict_t *self);
void SP_target_text (edict_t *self);
void SP_thing (edict_t *self);
void SP_tremor_trigger_multiple (edict_t *self);
void SP_trigger_bbox (edict_t *self);
void SP_trigger_disguise (edict_t *self);
void SP_trigger_fog (edict_t *self);
void SP_trigger_fog_bbox (edict_t *self);
void SP_trigger_inside (edict_t *self);
void SP_trigger_inside_bbox (edict_t *self);
void SP_trigger_look (edict_t *self);
void SP_trigger_mass (edict_t *self);
void SP_trigger_mass_bbox (edict_t *self);
void SP_trigger_scales (edict_t *self);
void SP_trigger_scales_bbox (edict_t *self);
void SP_trigger_switch (edict_t *self);
void SP_trigger_speaker (edict_t *self);
void SP_trigger_teleporter (edict_t *self);
void SP_trigger_teleporter_bbox (edict_t *self);
void SP_trigger_transition (edict_t *self);
void SP_trigger_transition_bbox (edict_t *self);

// Knightmare- entities that use origin-based train pathing
void SP_func_train_origin (edict_t *self);
void SP_model_train_origin (edict_t *self);
void SP_misc_viper_origin (edict_t *ent);
void SP_misc_strogg_ship_origin (edict_t *ent);

// transition entities
void SP_bolt (edict_t *self);
void SP_debris (edict_t *self);
void SP_gib (edict_t *self);
void SP_gibhead (edict_t *self);
void SP_grenade (edict_t *self);
void SP_handgrenade (edict_t *self);
void SP_rocket (edict_t *self);
//
// end Lazarus

#if 0
spawn_t	spawns[] = {
	{"item_health", SP_item_health},
	{"item_health_small", SP_item_health_small},
	{"item_health_large", SP_item_health_large},
	{"item_health_mega", SP_item_health_mega},

	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_coop", SP_info_player_coop},
	{"info_player_intermission", SP_info_player_intermission},
//ZOID
	{"info_player_team1", SP_info_player_team1},
	{"info_player_team2", SP_info_player_team2},
	{"info_player_team3", SP_info_player_team3}, // Knightmare added
//ZOID

	{"func_plat", SP_func_plat},
	{"func_plat2", SP_func_plat2},	// Knightmare added
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_door_secret", SP_func_door_secret},
	{"func_door_secret2", SP_func_door_secret2},	// Knightmare added
	{"func_door_rotating", SP_func_door_rotating},
	{"func_rotating", SP_func_rotating},
	{"func_train", SP_func_train},
	{"func_water", SP_func_water},
	{"func_conveyor", SP_func_conveyor},
	{"func_areaportal", SP_func_areaportal},
	{"func_clock", SP_func_clock},
	{"func_reflect", SP_func_reflect},
	{"func_wall", SP_func_wall},
	{"func_object", SP_func_object},
	{"func_timer", SP_func_timer},
	{"func_explosive", SP_func_explosive},
	{"func_killbox", SP_func_killbox},

	{"target_actor", SP_target_actor},
	{"target_animation", SP_target_animation},
	{"target_blaster", SP_target_blaster},
	{"target_changelevel", SP_target_changelevel},
	{"target_character", SP_target_character},
	{"target_crosslevel_target", SP_target_crosslevel_target},
	{"target_crosslevel_trigger", SP_target_crosslevel_trigger},
	{"target_earthquake", SP_target_earthquake},
	{"target_explosion", SP_target_explosion},
	{"target_goal", SP_target_goal},
	{"target_help", SP_target_help},
	{"target_laser", SP_target_laser},
	{"target_lightramp", SP_target_lightramp},
	{"target_secret", SP_target_secret},
	{"target_spawner", SP_target_spawner},
	{"target_speaker", SP_target_speaker},
	{"target_splash", SP_target_splash},
	{"target_string", SP_target_string},
	{"target_temp_entity", SP_target_temp_entity},

	{"trigger_always", SP_trigger_always},
	{"trigger_counter", SP_trigger_counter},
	{"trigger_elevator", SP_trigger_elevator},
	{"trigger_gravity", SP_trigger_gravity},
	{"trigger_gravity_bbox", SP_trigger_gravity_bbox},
	{"trigger_hurt", SP_trigger_hurt},
	{"trigger_hurt_bbox", SP_trigger_hurt_bbox},
	{"trigger_key", SP_trigger_key},
	{"trigger_once", SP_trigger_once},
	{"trigger_monsterjump", SP_trigger_monsterjump},
	{"trigger_monsterjump_bbox", SP_trigger_monsterjump_bbox},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push", SP_trigger_push},
	{"trigger_push_bbox", SP_trigger_push_bbox},
	{"trigger_relay", SP_trigger_relay},

	{"viewthing", SP_viewthing},
	//{"worldspawn", SP_worldspawn},

	{"light", SP_light},
	{"light_mine1", SP_light_mine1},
	{"light_mine2", SP_light_mine2},
	{"info_null", SP_info_null},
	{"func_group", SP_info_null},
	{"info_notnull", SP_info_notnull},
	{"path_corner", SP_path_corner},
	{"point_combat", SP_point_combat},

	{"misc_explobox", SP_misc_explobox},
	{"misc_banner", SP_misc_banner},
//ZOID
	{"misc_ctf_banner", SP_misc_ctf_banner},
	{"misc_ctf_small_banner", SP_misc_ctf_small_banner},
//ZOID
	{"misc_satellite_dish", SP_misc_satellite_dish},
	{"misc_actor", SP_misc_actor},
	{"misc_gib_arm", SP_misc_gib_arm},
	{"misc_gib_leg", SP_misc_gib_leg},
	{"misc_gib_head", SP_misc_gib_head},
	{"misc_insane", SP_misc_insane},
	{"misc_deadsoldier", SP_misc_deadsoldier},
	{"misc_viper", SP_misc_viper},
	{"misc_viper_bomb", SP_misc_viper_bomb},
	{"misc_bigviper", SP_misc_bigviper},
	{"misc_strogg_ship", SP_misc_strogg_ship},
	{"misc_teleporter", SP_misc_teleporter},
	{"misc_teleporter_dest", SP_misc_teleporter_dest},
//ZOID
	{"trigger_teleport", SP_trigger_teleport},
	{"info_teleport_destination", SP_info_teleport_destination},
//ZOID
	{"misc_blackhole", SP_misc_blackhole},
	{"misc_eastertank", SP_misc_eastertank},
	{"misc_easterchick", SP_misc_easterchick},
	{"misc_easterchick2", SP_misc_easterchick2},

	{"misc_flare", SP_misc_flare},

	{"monster_berserk", SP_monster_berserk},
	{"monster_gladiator", SP_monster_gladiator},
	{"monster_gunner", SP_monster_gunner},
	{"monster_infantry", SP_monster_infantry},
	{"monster_soldier_light", SP_monster_soldier_light},
	{"monster_soldier", SP_monster_soldier},
	{"monster_soldier_ss", SP_monster_soldier_ss},
	{"monster_tank", SP_monster_tank},
	{"monster_tank_commander", SP_monster_tank},
	{"monster_medic", SP_monster_medic},
	{"monster_flipper", SP_monster_flipper},
	{"monster_chick", SP_monster_chick},
	{"monster_parasite", SP_monster_parasite},
	{"monster_flyer", SP_monster_flyer},
	{"monster_brain", SP_monster_brain},
	{"monster_floater", SP_monster_floater},
	{"monster_hover", SP_monster_hover},
	{"monster_mutant", SP_monster_mutant},
	{"monster_supertank", SP_monster_supertank},
	{"monster_boss2", SP_monster_boss2},
	{"monster_boss3_stand", SP_monster_boss3_stand},
	{"monster_jorg", SP_monster_jorg},

	{"monster_commander_body", SP_monster_commander_body},

	{"turret_breach", SP_turret_breach},
	{"turret_base", SP_turret_base},
	{"turret_driver", SP_turret_driver},

// Lazarus
	{"crane_beam",   SP_crane_beam},
	{"crane_hoist",  SP_crane_hoist},
	{"crane_hook",   SP_crane_hook},
	{"crane_control",SP_crane_control},
	{"crane_reset",SP_crane_reset},
	{"func_bobbingwater", SP_func_bobbingwater},
	{"func_door_rot_dh", SP_func_door_rot_dh},
	{"func_door_swinging", SP_func_door_swinging},
	{"func_force_wall", SP_func_force_wall},

	{"func_breakaway", SP_func_breakaway}, // Knightmare added
	{"func_monitor", SP_func_monitor},
	{"func_pendulum", SP_func_pendulum},
	{"func_pivot", SP_func_pivot},
	{"func_pushable", SP_func_pushable},
	{"func_rotating_dh", SP_func_rotating_dh},
	{"func_trackchange", SP_func_trackchange},
	{"func_tracktrain", SP_func_tracktrain},
	{"func_trainbutton", SP_func_trainbutton},
	{"func_vehicle", SP_func_vehicle},
	{"hint_path", SP_hint_path},
	{"info_train_start", SP_info_train_start},
	{"misc_light", SP_misc_light},
	{"model_spawn",  SP_model_spawn},
	{"model_train", SP_model_train},
	{"model_turret", SP_model_turret},
	{"monster_makron", SP_monster_makron},
	{"path_track", SP_path_track},
	{"target_anger", SP_target_anger},
	{"target_attractor", SP_target_attractor},
	{"target_bmodel_spawner", SP_target_clone},
	{"target_cd", SP_target_CD},
	{"target_change", SP_target_change},
	{"target_clone", SP_target_clone},
	{"target_effect", SP_target_effect},
	{"target_fade", SP_target_fade},
	{"target_failure", SP_target_failure},
	{"target_fog", SP_target_fog},
	{"target_fountain", SP_target_fountain},
	{"target_lightswitch", SP_target_lightswitch},
	{"target_locator", SP_target_locator},
	{"target_lock", SP_target_lock},
	{"target_lock_clue", SP_target_lock_clue},
	{"target_lock_code", SP_target_lock_code},
	{"target_lock_digit", SP_target_lock_digit},
	{"target_monitor", SP_target_monitor},
	{"target_monsterbattle", SP_target_monsterbattle},
	{"target_movewith", SP_target_movewith},
	{"target_precipitation", SP_target_precipitation},
	{"target_rocks", SP_target_rocks},
	{"target_rotation", SP_target_rotation},
	{"target_command", SP_target_command},
	{"target_set_effect", SP_target_set_effect},
	{"target_skill", SP_target_skill},
	{"target_sky", SP_target_sky},
	{"target_text", SP_target_text},
	{"thing", SP_thing},
	{"tremor_trigger_multiple", SP_tremor_trigger_multiple},
	{"trigger_bbox", SP_trigger_bbox},
	{"trigger_disguise", SP_trigger_disguise},
	{"trigger_fog", SP_trigger_fog},
	{"trigger_fog_bbox", SP_trigger_fog_bbox},
	{"trigger_inside", SP_trigger_inside},
	{"trigger_inside_bbox", SP_trigger_inside_bbox},
	{"trigger_look", SP_trigger_look},
	{"trigger_mass", SP_trigger_mass},
	{"trigger_mass_bbox", SP_trigger_mass_bbox},
	{"trigger_scales", SP_trigger_scales},
	{"trigger_scales_bbox", SP_trigger_scales_bbox},
	{"trigger_speaker", SP_trigger_speaker},
	{"trigger_switch", SP_trigger_switch},
	{"trigger_teleporter", SP_trigger_teleporter},
	{"trigger_teleporter_bbox", SP_trigger_teleporter_bbox},
	{"trigger_transition", SP_trigger_transition},
	{"trigger_transition_bbox", SP_trigger_transition_bbox},

// Knightmare- entities that use origin-based train pathing
	{"func_train_origin", SP_func_train_origin},
	{"model_train_origin", SP_model_train_origin},
	{"misc_viper_origin", SP_misc_viper_origin},
	{"misc_strogg_ship_origin", SP_misc_strogg_ship_origin},

// transition entities
	{"bolt", SP_bolt},
	{"debris", SP_debris},
	{"gib", SP_gib},
	{"gibhead", SP_gibhead},
	{"grenade", SP_grenade},
	{"hgrenade", SP_handgrenade},
	{"rocket", SP_rocket},
	{"homing rocket", SP_rocket},
// end Lazarus
	{nullptr,                     nullptr                     }
};

// Knightmare- sound precache functions
void monster_berserk_soundcache (edict_t *self);
void monster_boss2_soundcache (edict_t *self);
void monster_jorg_soundcache (edict_t *self);
void monster_makron_soundcache (edict_t *self);
void monster_brain_soundcache (edict_t *self);
void monster_chick_soundcache (edict_t *self);
void monster_flipper_soundcache (edict_t *self);
void monster_floater_soundcache (edict_t *self);
void monster_flyer_soundcache (edict_t *self);
void monster_gladiator_soundcache (edict_t *self);
void monster_gunner_soundcache (edict_t *self);
void monster_hover_soundcache (edict_t *self);
void monster_infantry_soundcache (edict_t *self);
void misc_insane_soundcache (edict_t *self);
void monster_medic_soundcache (edict_t *self);
void monster_mutant_soundcache (edict_t *self);
void monster_parasite_soundcache (edict_t *self);
void monster_soldier_x_soundcache (edict_t *self);
void monster_supertank_soundcache (edict_t *self);
void monster_tank_soundcache (edict_t *self);

// Knightmare- sound precache table
soundcache_t	soundcaches[] = {
	{"monster_berserk", monster_berserk_soundcache},
	{"monster_boss2", monster_boss2_soundcache},
	{"monster_jorg", monster_jorg_soundcache},
	{"monster_makron", monster_makron_soundcache},
	{"monster_brain", monster_brain_soundcache},
	{"monster_chick", monster_chick_soundcache},
	{"monster_flipper", monster_flipper_soundcache},
	{"monster_floater", monster_floater_soundcache},
	{"monster_flyer", monster_flyer_soundcache},
	{"monster_gladiator", monster_gladiator_soundcache},
	{"monster_gunner", monster_gunner_soundcache},
	{"monster_hover", monster_hover_soundcache},
	{"monster_infantry", monster_infantry_soundcache},
	{"misc_insane", misc_insane_soundcache},
	{"monster_medic", monster_medic_soundcache},
	{"monster_mutant", monster_mutant_soundcache},
	{"monster_parasite", monster_parasite_soundcache},
	{"monster_soldier_light", monster_soldier_x_soundcache},
	{"monster_soldier", monster_soldier_x_soundcache},
	{"monster_soldier_ss", monster_soldier_x_soundcache},
	{"monster_supertank",  monster_supertank_soundcache},
	{"monster_tank", monster_tank_soundcache},
	{"monster_tank_commander", monster_tank_soundcache},

	{nullptr,                  nullptr                     }
};
// end Knightmare
#endif

// Knightmare- global pointer for the entity alias script
// The file should be loaded into memory, because we can't
// re-open and read it for every entity we parse
char	*alias_data;
int		alias_data_size;
#ifndef KMQUAKE2_ENGINE_MOD
qboolean alias_from_pak;
#endif

void ReInitialize_Entity (edict_t *ent)
{

	// check normal spawn functions
	for ( const spawn_t *s = spawns; s->name; s++)
	{
		if (!strcmp(s->name, ent->classname.c_str()))
		{	// found it
			s->spawn (ent);
			return;
		}
	}
}

/*
=============
ED_NewString
=============
*/
char *ED_NewString ( const char *string)
{

	int l = ( int ) strlen( string ) + 1;

	char *newb = static_cast< char * >( gi.TagMalloc( l, TAG_LEVEL ) );

	char *new_p = newb;

	for ( int i = 0 ; i< l ; i++)
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}

/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
void ED_ParseField( char *key, char *value, edict_t *ent )
{
	byte  *b;
	float  v;
	vec3_t vec = { 0.0f, 0.0f, 0.0f };

	for ( const field_t *f = fields; f->name; f++ )
	{
		if ( !( f->flags & FFL_NOSPAWN ) && !Q_stricmp( f->name, key ) )
		{// found it
			if ( f->flags & FFL_SPAWNTEMP )
			{
				b = reinterpret_cast< byte * >( &st );
			}
			else
			{
				b = reinterpret_cast< byte * >( ent );
			}

			switch ( f->type )
			{
				default:
					break;
				case F_DSTRING:
				{
					auto *s = reinterpret_cast< std::string * >( b + f->ofs );
					*s      = value;
					break;
				}
				case F_LSTRING:
					*reinterpret_cast< char ** >( b + f->ofs ) = ED_NewString( value );
					break;
				case F_VECTOR:
					//	sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
					if ( sscanf( value, "%f %f %f", &vec[ 0 ], &vec[ 1 ], &vec[ 2 ] ) != 3 )
					{
						gi.dprintf( "ED_ParseField: map '%s' has invalid vector '%s' for key '%s'.\n", level.mapname, value, key );
					}
					reinterpret_cast< float * >( b + f->ofs )[ 0 ] = vec[ 0 ];
					reinterpret_cast< float * >( b + f->ofs )[ 1 ] = vec[ 1 ];
					reinterpret_cast< float * >( b + f->ofs )[ 2 ] = vec[ 2 ];
					break;
				case F_INT:
					*reinterpret_cast< int * >( b + f->ofs ) = std::strtol( value, nullptr, 10 );
					break;
				case F_FLOAT:
					*reinterpret_cast< float * >( b + f->ofs ) = std::strtof( value, nullptr );
					break;
				case F_ANGLEHACK:
					v                                              = std::strtof( value, nullptr );
					reinterpret_cast< float * >( b + f->ofs )[ 0 ] = 0;
					reinterpret_cast< float * >( b + f->ofs )[ 1 ] = v;
					reinterpret_cast< float * >( b + f->ofs )[ 2 ] = 0;
					break;
				case F_IGNORE:
					break;
			}
			return;
		}
	}
	gi.dprintf( "%s is not a field\n", key );
}

// Knightmare added
/*
===============
ED_SetDefaultFields

Sets the default binary values in an edict
===============
*/
void ED_SetDefaultFields (edict_t *ent)
{
	byte	*b;

	for ( field_t *f = fields ; f->name ; f++)
	{
		if (f->flags & FFL_DEFAULT_NEG)
		{
			if (f->flags & FFL_SPAWNTEMP)
				b = (byte *)&st;
			else
				b = (byte *)ent;

			if (f->type == F_LSTRING)
				*(char **)(b+f->ofs) = ED_NewString ("-1");
			else if ( (f->type == F_VECTOR) || (f->type == F_ANGLEHACK) ) {
				((float *)(b+f->ofs))[0] = -1.0f;
				((float *)(b+f->ofs))[1] = -1.0f;
				((float *)(b+f->ofs))[2] = -1.0f;
			}
			else if (f->type == F_INT)
				*(int *)(b+f->ofs) = -1;
			else if (f->type == F_FLOAT)
				*(float *)(b+f->ofs) = -1.0f;
		}
	}
}
// end Knightmare

/*
==============================================================================

ALIAS SCRIPT LOADING

==============================================================================
*/

#ifndef KMQUAKE2_ENGINE_MOD
/*
====================
ReadTextFile

Called from LoadAliasData to try
loading script file from disk.
====================
*/
char *ReadTextFile (char *filename, int *size)
{
    FILE *fp;
    char *filestring = NULL;
    int len;

    fp = fopen(filename, "r");
    if (!fp)
		return NULL;

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    filestring = gi.TagMalloc(len + 1, TAG_LEVEL);
    if (!filestring)
	{
		fclose(fp);
		return NULL;
    }

    len = fread(filestring, 1, len, fp);
	*size = len;
    filestring[len] = 0;

    fclose(fp);

    return filestring;
}

/*
====================
LoadAliasFile

Tries to load script file from
disk, and then from a pak file.
====================
*/
qboolean LoadAliasFile (char *name)
{
	char aliasfilename[MAX_OSPATH] = "";

	alias_from_pak = false;

	GameDirRelativePath (name, aliasfilename, sizeof(aliasfilename));
    alias_data = ReadTextFile(aliasfilename, &alias_data_size);

	// If file doesn't exist on hard disk, it must be in a pak file
	if (!alias_data)
	{
		cvar_t			*basedir, *gamedir;
		char			gamepath[256];
		char			pakfile[256];
		char			textname[128];
		int				i, k, num, numitems;
		qboolean		in_pak = false;
		FILE			*fpak;
		pak_header_t	pakheader;
		pak_item_t		pakitem;

		basedir = gi.cvar("basedir", "", 0);
		gamedir = gi.cvar("gamedir", "", 0);
		Com_sprintf (textname, sizeof(textname), name);

		if (strlen(gamedir->string))
		//	Q_strncpyz(gamepath, sizeof(gamepath), gamedir->string);
			Com_sprintf (gamepath, sizeof(gamepath), "%s/%s", basedir->string, gamedir->string);
		else
		//	Q_strncpyz(gamepath, sizeof(gamepath), basedir->string);
			Com_sprintf (gamepath, sizeof(gamepath), "%s/baseq2", basedir->string);

		// check all pakfiles in current gamedir
		for (i=0; i<10; i++)
		{
			Com_sprintf (pakfile, sizeof(pakfile), "%s/pak%d.pak", gamepath, i);
			if (NULL != (fpak = fopen(pakfile, "rb")))
			{
				num = fread(&pakheader, 1, sizeof(pak_header_t), fpak);
				if (num >= sizeof(pak_header_t))
				{
					if (pakheader.id[0] == 'P' &&
						pakheader.id[1] == 'A' &&
						pakheader.id[2] == 'C' &&
						pakheader.id[3] == 'K'   )
					{
						numitems = pakheader.dsize / sizeof(pak_item_t);
						fseek(fpak, pakheader.dstart, SEEK_SET);
						for (k=0; k<numitems && !in_pak; k++)
						{
							fread(&pakitem, 1, sizeof(pak_item_t), fpak);
							if (!Q_stricmp(pakitem.name,textname))
							{
								in_pak = true;
								fseek(fpak, pakitem.start, SEEK_SET);
								alias_data = gi.TagMalloc(pakitem.size + 1, TAG_LEVEL);
								if (!alias_data) {
									fclose(fpak);
									gi.dprintf("LoadAliasData: Memory allocation failure for entalias.dat\n");
									return false;
								}
								alias_data_size = fread(alias_data, 1, pakitem.size, fpak);
								alias_data[pakitem.size] = 0; // put end marker
								alias_from_pak = true;
							}
						}
					}
				}
				fclose(fpak);
			}
		}
	}

	if (!alias_data)
		return false;

	return true;
}
#endif

/*
====================
LoadAliasData

Loads entity alias file either on disk
or from a pak file in the game dir.
====================
*/
void LoadAliasData ()
{
#ifdef KMQUAKE2_ENGINE_MOD // use new engine file loading function instead
	// try per-level script file
	alias_data_size = gi.LoadFile(va("ext_data/entalias/%s.alias", level.mapname), (void **)&alias_data);
	if (alias_data_size < 2) // file not found, try global file
		alias_data_size = gi.LoadFile("ext_data/entalias.def", (void **)&alias_data);
	if (alias_data_size < 2) // file still not found, try old filename
		alias_data_size = gi.LoadFile("scripts/entalias.dat", (void **)&alias_data);
#else
	if (!LoadAliasFile(va("ext_data/entalias/%s.alias", level.mapname)))
		if (!LoadAliasFile("ext_data/entalias.def"))
			LoadAliasFile("scripts/entalias.dat");
#endif
}

/*
====================
ED_ParseEntityAlias

Parses an edict, looking for its classname, and then
looks in the entity alias file for an alias, and
loads that if found.
Returns true if an alias was loaded for the given entity.
====================
*/
qboolean ED_ParseEntityAlias (char *data, edict_t *ent)
{
	char		*search_data;
	char		*search_token;
	char		entclassname[256];

	qboolean classname_found = false;
	qboolean alias_loaded    = false;

	if (!alias_data) // If no alias file was loaded, don't bother
		return false;

	search_data = data;  // copy entity data postion
	// go through all the dictionary pairs looking for the classname
	while (true)
	{	// parse keyname
		search_token = COM_Parse (&search_data);
		if (!search_data)
			gi.error ("ED_ParseEntityAlias: end of entity data without closing brace");
		if (search_token[0] == '}')
			break;
		if (!strcmp(search_token, "classname"))
			classname_found = true;

		// parse value
		search_token = COM_Parse (&search_data);
		if (!search_data)
			gi.error ("ED_ParseEntityAlias: end of entity data without closing brace");
		if (search_token[0] == '}')
			gi.error ("ED_ParseEntityAlias: closing brace without entity data");
		// if we've found the classname, exit loop
		if (classname_found) {
			Q_strncpyz (entclassname, sizeof(entclassname), search_token);
			break;
		}
	}
	// then search the entalias.def file for that classname
	if (classname_found)
	{
		qboolean alias_found = false;
		int braceLevel = 0;
		search_data = alias_data;	// copy alias data postion
 		while (search_data < (alias_data + alias_data_size))
		{
			search_token = COM_Parse (&search_data);
			if (!search_data)
				return false;
			if (!search_token)
				break;
			// see if we're inside the braces of an alias definition
			if (search_token[0] == '{') braceLevel++;
			else if (search_token[0] == '}') braceLevel--;
			if (braceLevel < 0) {
				gi.dprintf ("ED_ParseEntityAlias: closing brace without matching opening brace\n");
				return false;
			}
			// matching classname must be outside braces
			if (!strcmp(search_token, entclassname) && (braceLevel == 0)) {
			//	gi.dprintf ("Alias for %s found in alias script file.\n", search_token);
				alias_found = true;
				break;
			}
		}
		// then if that classname is found, load the fields from that alias
		if (alias_found)
		{	// get the opening curly brace
			search_token = COM_Parse (&search_data);
			if (!search_data) {
				gi.dprintf ("ED_ParseEntityAlias: unexpected EOF\n");
				return false;
			}
			if (search_token[0] != '{') {
				gi.dprintf ("ED_ParseEntityAlias: found %s when expecting {\n", search_token);
				return false;
			}
			// go through all the dictionary pairs
			while (search_data < (alias_data + alias_data_size))
			{
				char keyname[ 256 ];
				// parse key
				search_token = COM_Parse (&search_data);
				if (!search_data) {
					gi.dprintf ("ED_ParseEntityAlias: EOF without closing brace\n");
					return false;
				}
				if (search_token[0] == '}')
					break;
				Q_strncpyz (keyname, sizeof(keyname), search_token);

			// parse value
				search_token = COM_Parse (&search_data);
				if (!search_data) {
					gi.dprintf ("ED_ParseEntityAlias: EOF without closing brace\n");
					return false;
				}
				if (search_token[0] == '}') {
					gi.dprintf ("ED_ParseEntityAlias: closing brace without data\n");
					return false;
				}
				ED_ParseField (keyname, search_token, ent);
				alias_loaded = true;
			}
		}
	}
	return alias_loaded;
}
/*
==============================================================================
END ALIAS SCRIPT LOADING
==============================================================================
*/

/*
===========
G_PrecachePlayerInventories

Precaches inventory for all players transitioning
across maps in SP and coop.
============
*/
void G_PrecachePlayerInventories ()
{
	const gclient_t	*client = nullptr;
	gitem_t		*item = nullptr;

	if (deathmatch->value)	// not needed in DM/CTF
		return;

	for ( int i = 0; i < game.maxclients; i++)
	{
		if (&game.clients[i] != nullptr )
		{
		//	gi.dprintf ("PrecachePlayerInventories(): precaching for client %i\n", i);
			client = &game.clients[i];
			for ( int j = 0; j < game.num_items; j++)
			{
				if (client->pers.inventory[j] > 0) {
					item = &itemlist[j];
					if (item != nullptr ) {
					//	gi.dprintf ("PrecachePlayerInventories(): precaching item %i: %s\n", j, item->classname);
						PrecacheItem (item);
					}
				}
			}
		}
	}
}


/*
================
G_FixTeams

Borrowed from Rogue source
================
*/
void G_FixTeams ()
{
#if 0//TODO
	edict_t	*e, *e2;
	int		i, j;

	int c  = 0;
	int c2 = 0;
	for (i=1, e=g_edicts+i ; i < globals.num_edicts ; i++,e++)
	{
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		// Lazarus- ignore bmodel spawner (its team isn't used)
		if (!e->classname.empty() && !Q_stricmp( e->classname.c_str(), "target_bmodel_spawner" ) )
			continue;
		if (!strcmp(e->classname.c_str(), "func_train"))
		{
			if (e->flags & FL_TEAMSLAVE)
			{
				edict_t *chain = e;
				e->teammaster = e;
				e->teamchain = nullptr;
				e->flags &= ~FL_TEAMSLAVE;
				c++;
				c2++;
				for (j=1, e2=g_edicts+j ; j < globals.num_edicts ; j++,e2++)
				{
					if (e2 == e)
						continue;
					if (!e2->inuse)
						continue;
					if (!e2->team)
						continue;
					if (!strcmp(e->team, e2->team))
					{
						c2++;
						chain->teamchain = e2;
						e2->teammaster = e;
						e2->teamchain = nullptr;
						chain = e2;
						e2->flags |= FL_TEAMSLAVE;
						e2->movetype = MOVETYPE_PUSH;
						e2->speed = e->speed;
					}
				}
			}
		}
	}
	gi.dprintf ("%i teams repaired\n", c);
#endif
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams ()
{
#if 0//TODO
	edict_t	*e, *e2;
	int		i, j;

	int c  = 0;
	int c2 = 0;
	for (i=1, e=g_edicts+i ; i < globals.num_edicts ; i++,e++)
	{
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		// Lazarus: some entities may have psuedo-teams that shouldn't be handled here
		if (!e->classname.empty() && !Q_stricmp( e->classname.c_str(), "target_change" ) )
			continue;
		if (!e->classname.empty() && !Q_stricmp( e->classname.c_str(), "target_bmodel_spawner" ) )
			continue;
		if (!e->classname.empty() && !Q_stricmp( e->classname.c_str(), "target_clone" ) )
			continue;
		edict_t *chain = e;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < globals.num_edicts ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	// Knightmare- set up Rogue train teams
	if (level.maptype == MAPTYPE_ROGUE) {
		G_FixTeams ();
	}

	if (level.time < 2)
		gi.dprintf ("%i teams with %i entities\n", c, c2);
#endif
}

void trans_ent_filename (char *filename, size_t filenameSize);
void ReadEdict (FILE *f, edict_t *ent);
void LoadTransitionEnts()
{
#if 0//TODO
	if (developer->value)
		gi.dprintf("==== LoadTransitionEnts ====\n");
	if (game.transition_ents)
	{
		char		t_file[MAX_OSPATH];
		vec3_t		v_spawn;

		VectorClear (v_spawn);
		if (strlen(game.spawnpoint))
		{
			edict_t *spawn = G_Find( nullptr, FOFS( targetname ), game.spawnpoint );
			while (spawn)
			{
				if (!Q_stricmp ( spawn->classname.c_str(), "info_player_start" ) )
				{
					VectorCopy (spawn->s.origin, v_spawn);
					break;
				}
				spawn = G_Find(spawn, FOFS(targetname), game.spawnpoint);
			}
		}
		trans_ent_filename (t_file, sizeof(t_file));
		FILE *f = fopen( t_file, "rb" );
		if (!f)
			gi.error("LoadTransitionEnts: Cannot open %s\n", t_file);
		else
		{
			for ( int i = 0; i<game.transition_ents; i++)
			{
				edict_t *ent = G_Spawn();
				ReadEdict(f,ent);
				// Correction for monsters with health EXACTLY 0
				// If we don't do this, spawn function will bring
				// 'em back to life
				if (ent->svflags & SVF_MONSTER)
				{
					if (!ent->health)
					{
						ent->health = -1;
						ent->deadflag = DEAD_DEAD;
					}
					else if (ent->deadflag == DEAD_DEAD)
					{
						ent->health = min(ent->health, -1);
					}
				}
				VectorAdd (ent->s.origin, v_spawn, ent->s.origin);
				VectorCopy( ent->s.origin, ent->s.old_origin);
				ED_CallSpawn (ent);
				if (ent->owner_id)
				{
					if (ent->owner_id < 0)
					{
						ent->owner = &g_edicts[-ent->owner_id];
					}
					else
					{
						// We KNOW owners precede owned ents in the
						// list because of the way it was constructed
						ent->owner = nullptr;
						for ( int j = game.maxclients + 1; j<globals.num_edicts && !ent->owner; j++)
						{
							if (ent->owner_id == g_edicts[j].id)
								ent->owner = &g_edicts[j];
						}
					}
					ent->owner_id = 0;
				}
				ent->s.renderfx |= RF_IR_VISIBLE;
			}
			fclose(f);
		}
	}
#endif
}


//===================================================

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn( edict_t *ent )
{
#if 0
	gitem_t *item;
	int      i;

	// Lazarus: if this fails, edict is freed.

	if ( ent->classname.empty() )
	{
		gi.dprintf( "ED_CallSpawn: NULL classname\n" );
		G_FreeEdict( ent );
		return;
	}

	// Lazarus: Preserve original angles for movewith stuff
	//          before G_SetMoveDir wipes 'em out
	VectorCopy( ent->s.angles, ent->org_angles );

	// check item spawn functions
	for ( i = 0, item = itemlist; i < game.num_items; i++, item++ )
	{
		if ( !item->classname )
			continue;
		if ( !strcmp( item->classname, ent->classname.c_str() ) )
		{// found it
			SpawnItem( ent, item );
			return;
		}
	}

	// check normal spawn functions
	for ( const spawn_t *s = spawns; s->name; s++ )
	{
		if ( !strcmp( s->name, ent->classname.c_str() ) )
		{// found it
			s->spawn( ent );
			return;
		}
	}

	EntityManager::SpawnEntity( ent );
#else
	assert( 0 );
#endif
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/


/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities( const char *mapname, char *entities, const char *spawnpoint )
{
	if ( developer->value )
		gi.dprintf( "====== SpawnEntities ========\n" );
	float skill_level = std::floor( skill->value );
	if ( skill_level < 0 )
		skill_level = 0;
	if ( skill_level > 3 )
		skill_level = 3;
	if ( skill->value != skill_level )
	{
		gi.cvar_forceset( "skill", va( "%f", skill_level ) );
	}

	SaveClientData();

	memset( &level, 0, sizeof( level ) );
	memset( g_custom_anims, 0, sizeof( g_custom_anims ) );// Knightmare- wipe custom animations

	// Lazarus: these are used to track model and sound indices
	//          in g_main.c:
	max_modelindex = 0;
	max_soundindex = 0;

	// Lazarus: last frame a gib was spawned in
	lastgibframe = 0;

	Q_strncpyz( level.mapname, sizeof( level.mapname ), mapname );
	Q_strncpyz( game.spawnpoint, sizeof( game.spawnpoint ), spawnpoint );

	// set client fields on player ents
#if 0//TODO
	for ( i = 0; i < game.maxclients; i++ )
	{
		g_edicts[ i + 1 ].client = game.clients + i;
	}
#endif

	int inhibit = 0;

	// Knightmare- set maptype for pack-specific changes
	if ( IsIdMap() )
	{
		level.maptype = MAPTYPE_ID;
		//	gi.dprintf ("Maptype is Id.\n");
	}
	else if ( IsXatrixMap() )
	{
		level.maptype = MAPTYPE_XATRIX;
		//	gi.dprintf ("Maptype is Xatrix.\n");
	}
	else if ( IsRogueMap() )
	{
		level.maptype = MAPTYPE_ROGUE;
		//	gi.dprintf ("Maptype is Rogue.\n");
	}
	else if ( IsZaeroMap() )
	{
		level.maptype = MAPTYPE_ZAERO;
		//	gi.dprintf ("Maptype is Zaero.\n");
	}
	else
	{
		level.maptype = MAPTYPE_CUSTOM;
		//	gi.dprintf ("Maptype is Custom.\n");
	}
	// end Knightmare

	// Knightamre- load the entity alias script file
	LoadAliasData();
	//gi.dprintf ("Size of alias data: %i\n", alias_data_size);

	// parse ents
	while ( true )
	{
		// parse the opening brace
		char *com_token = COM_Parse( &entities );
		if ( !entities )
		{
			break;
		}

		if ( com_token[ 0 ] != '{' )
		{
		}

#if 0   //TODO
        // yet another map hack
		if (!Q_stricmp( level.mapname, "command" ) && !Q_stricmp( ent->classname.c_str(), "trigger_once" ) && !Q_stricmp( ent->model, "*27" ) )
			ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;

		// remove things (except the world) from different skill levels or deathmatch
		if (ent != g_edicts)
		{
			if (deathmatch->value)
			{
				if ( ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH )
				{
					G_FreeEdict (ent);
					inhibit++;
					continue;
				}
			}
			else if (coop->value) // Knightmare added- not in coop flag support
			{
				if ( ent->spawnflags & SPAWNFLAG_NOT_COOP )
				{
					G_FreeEdict (ent);
					inhibit++;
					continue;
				}

				// NOT_EASY && NOT_MEDIUM && NOT_HARD && NOT_DM == COOP_ONLY
				if ( (ent->spawnflags & SPAWNFLAG_NOT_EASY)
					&& (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)
					&& (ent->spawnflags & SPAWNFLAG_NOT_HARD)
					&& (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH) )
					goto removeflags;

				if( ((skill->value == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
					((skill->value == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
					((skill->value >= 2) && (ent->spawnflags & SPAWNFLAG_NOT_HARD)) )
					{
						G_FreeEdict (ent);
						inhibit++;
						continue;
					}
			}
			else // single player
			{
				if( ((skill->value == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
					((skill->value == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
					((skill->value >= 2) && (ent->spawnflags & SPAWNFLAG_NOT_HARD)) )
					{
						G_FreeEdict (ent);
						inhibit++;
						continue;
					}
			}
removeflags:
			// Knightmare- remove no coop flag
			ent->spawnflags &= ~(SPAWNFLAG_NOT_EASY|SPAWNFLAG_NOT_MEDIUM|SPAWNFLAG_NOT_HARD|SPAWNFLAG_NOT_DEATHMATCH|SPAWNFLAG_NOT_COOP);
		}
#endif


	}

	// Knightmare- unload the alias script file
	if ( alias_data )
	{                     // If no alias file was loaded, don't bother
#ifdef KMQUAKE2_ENGINE_MOD// use new engine function instead
		gi.FreeFile( alias_data );
#else
		//		if (alias_from_pak)
		gi.TagFree( alias_data );
//		else
//			free(&alias_data);
#endif
	}

	// Knightmare- unload the replacement entity data
	/*	if (newents) // If no alias file was loaded, don't bother
#ifdef KMQUAKE2_ENGINE_MOD // use new engine function instead
		gi.FreeFile(newents);
#else
		free(&newents);
#endif*/

	gi.dprintf( "%i entities inhibited\n", inhibit );

#ifdef DEBUG
	i   = 1;
	ent = EDICT_NUM( i );
	while ( i < globals.num_edicts )
	{
		if ( ent->inuse != 0 || ent->inuse != 1 )
			Com_DPrintf( "Invalid entity %d\n", i );
		i++, ent++;
	}
#endif

	G_FindTeams();

	// DWH
	G_FindCraneParts();

	for ( auto &ent : g_edicts )
	{
		if ( ent->classname == "worldspawn" )
		{
			continue;
		}

		// Get origin offsets (mainly for brush models w/o origin brushes)
		VectorAdd( ent->absmin, ent->absmax, ent->origin_offset );
		VectorScale( ent->origin_offset, 0.5f, ent->origin_offset );
		VectorSubtract( ent->origin_offset, ent->s.origin, ent->origin_offset );

		if ( !ent->movewith || ent->movewith_ent )
		{
			continue;
		}

		ent->movewith_ent = G_Find( nullptr, FOFS( targetname ), ent->movewith );
		// Make sure that we can really "movewith" this guy. This check
		// allows us to have movewith parent with same targetname as
		// other entities
		while ( ent->movewith_ent &&
		        ( Q_stricmp( ent->movewith_ent->classname.c_str(), "func_train" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "model_train" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_door" ) &&

		// Knightmare added
#ifdef POSTTHINK_CHILD_MOVEMENT
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_door_rotating" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_water" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_plat" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_plat2" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_rotating" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_button" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_door_secret" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_door_secret2" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_bobbingwater" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_door_swinging" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_breakaway" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_trackchange" ) &&
#endif// POSTTHINK_CHILD_MOVEMENT \
        // end Knightmare

		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_vehicle" ) &&
		          Q_stricmp( ent->movewith_ent->classname.c_str(), "func_tracktrain" ) ) )
			ent->movewith_ent = G_Find( ent->movewith_ent, FOFS( targetname ), ent->movewith );
		if ( ent->movewith_ent )
			movewith_init( ent->movewith_ent );
	}

	// end DWH

	PlayerTrail_Init();

	//ZOID
	CTFSpawn();
	// Knightmare added
	if ( deathmatch->value && !ctf->value )
		CTFSetupTechSpawn();
	//ZOID

	if ( !deathmatch->value )
		SetupHintPaths();

	if ( game.transition_ents )
		LoadTransitionEnts();

	actor_files();

	// Knightmare- precache transitioning player inventories here
	// Fixes lag when changing weapons after level transition
	G_PrecachePlayerInventories();
}


//===================================================================

// Knightmare added
/*
==============
G_SoundcacheEntities

Reloads static cached sounds for entities using spawns table
==============
*/
void G_SoundcacheEntities()
{
#if 0//TODO
	for ( auto &ent : g_edicts )
	{
		if ( !ent->inuse )
			continue;

		// check normal spawn functions
		for ( const soundcache_t *s = soundcaches; s->name; s++ )
		{
			if ( !strcmp( s->name, ent->classname.c_str() ) )
			{// found it
				if ( s->soundcache != nullptr )
				{
					s->soundcache( ent.get() );
				}
				break;
			}
		}
	}
#endif
}
// end Knightmare

//===================================================================

#if 0
	// cursor positioning
	xl <value>
	xr <value>
	yb <value>
	yt <value>
	xv <value>
	yv <value>

	// drawing
	statpic <name>
	pic <stat>
	num <fieldwidth> <stat>
	string <stat>

	// control
	if <stat>
	ifeq <stat> <value>
	ifbit <stat> <value>
	endif

#endif
