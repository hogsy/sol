#include "g_local.h"

/******* Knightmare's cvar code file ***********/

// enable/disable options
/*
cvar_t	*mega_gibs;					// whether to spawn extra gibs, default to 0
cvar_t	*mp_monster_replace;		// whether to replace monsters with mission pack modified variants
cvar_t	*mp_monster_ammo_replace;	// whether to replace monsters' ammo items if above is enabled
cvar_t	*kamikaze_flyer_replace;	// whether to replace some flyers with kamikaze flyers, probability 0-1
cvar_t	*allow_player_use_abandoned_turret;	// whether to allow player to use turrets in exisiting maps
cvar_t	*turn_rider;				// whether to turn player on rotating object
cvar_t	*adjust_train_corners;		// whether to subtract (1,1,1) from train path corners to fix misalignments

cvar_t	*add_velocity_throw;		// whether to add player's velocity to thrown objects
cvar_t	*falling_armor_damage;		// whether player's armor absorbs damage from falling
cvar_t	*player_jump_sounds;		// whether to play that STUPID grunting sound when the player jumps
cvar_t	*tpp_auto;					// whether to automatically go into third-person when pushing a pushable
cvar_t	*use_vwep;
*/
// Server-side speed control stuff
cvar_t	*player_max_speed;
cvar_t	*player_crouch_speed;
cvar_t	*player_accel;
cvar_t	*player_stopspeed;

// weapon balancing
cvar_t	*sk_blaster_damage;
cvar_t	*sk_blaster_damage_dm;
cvar_t	*sk_blaster_speed;
cvar_t	*sk_blaster_color;		// blaster color- 1=yellow, 2=green, 3=blue, 4=red

cvar_t	*sk_shotgun_damage;
cvar_t	*sk_shotgun_count;
cvar_t	*sk_shotgun_hspread;
cvar_t	*sk_shotgun_vspread;

cvar_t	*sk_sshotgun_damage;
cvar_t	*sk_sshotgun_count;
cvar_t	*sk_sshotgun_hspread;
cvar_t	*sk_sshotgun_vspread;

cvar_t	*sk_machinegun_damage;
cvar_t	*sk_machinegun_hspread;
cvar_t	*sk_machinegun_vspread;

cvar_t	*sk_chaingun_damage;
cvar_t	*sk_chaingun_damage_dm;
cvar_t	*sk_chaingun_hspread;
cvar_t	*sk_chaingun_vspread;

cvar_t	*sk_grenade_damage;
cvar_t	*sk_grenade_radius;
cvar_t	*sk_grenade_speed;

cvar_t	*sk_hand_grenade_damage;
cvar_t	*sk_hand_grenade_radius;

cvar_t	*sk_rocket_damage;
cvar_t	*sk_rocket_damage2;
cvar_t	*sk_rocket_rdamage;
cvar_t	*sk_rocket_radius;
cvar_t	*sk_rocket_speed;
cvar_t	*sk_rocket_lockon_speed;

cvar_t	*sk_hyperblaster_damage;
cvar_t	*sk_hyperblaster_damage_dm;
cvar_t	*sk_hyperblaster_speed;
cvar_t	*sk_hyperblaster_color;		// hyperblaster color- 1=yellow, 2=green, 3=blue, 4=red

cvar_t	*sk_railgun_damage;
cvar_t	*sk_railgun_damage_dm;
cvar_t	*sk_railgun_skin;

cvar_t	*sk_bfg_damage;
cvar_t	*sk_bfg_damage_dm;
cvar_t	*sk_bfg_damage2;
cvar_t	*sk_bfg_damage2_dm;
cvar_t	*sk_bfg_rdamage;
cvar_t	*sk_bfg_radius;
cvar_t	*sk_bfg_speed;

//cvar_t	*sk_jump_kick_damage;

cvar_t	*sk_ionripper_damage;
cvar_t	*sk_ionripper_damage_dm;
cvar_t	*sk_ionripper_kick;
cvar_t	*sk_ionripper_kick_dm;
cvar_t	*sk_ionripper_speed;
cvar_t	*sk_ionripper_extra_sounds;

cvar_t	*sk_phalanx_damage;
cvar_t	*sk_phalanx_damage2;
cvar_t	*sk_phalanx_radius_damage;
cvar_t	*sk_phalanx_radius;
cvar_t	*sk_phalanx_speed;

cvar_t	*sk_trap_life;
cvar_t	*sk_trap_health;
/*
cvar_t	*sk_etf_rifle_damage;
cvar_t	*sk_etf_rifle_damage_dm;
cvar_t	*sk_etf_rifle_radius_damage;
cvar_t	*sk_etf_rifle_radius;
cvar_t	*sk_etf_rifle_speed;

cvar_t	*sk_plasmabeam_damage;
cvar_t	*sk_plasmabeam_damage_dm;

cvar_t	*sk_disruptor_damage;
cvar_t	*sk_disruptor_damage_dm;
cvar_t	*sk_disruptor_speed;

cvar_t	*sk_prox_damage;
cvar_t	*sk_prox_radius;
cvar_t	*sk_prox_speed;
cvar_t	*sk_prox_life;
cvar_t	*sk_prox_health;

cvar_t	*sk_tesla_damage;
cvar_t	*sk_tesla_radius;
cvar_t	*sk_tesla_life;
cvar_t	*sk_tesla_health;

cvar_t	*sk_chainfist_damage;
cvar_t	*sk_chainfist_damage_dm;

cvar_t	*sk_nuke_delay;
cvar_t	*sk_nuke_life;
cvar_t	*sk_nuke_radius;

cvar_t	*sk_nbomb_delay;
cvar_t	*sk_nbomb_life;
cvar_t	*sk_nbomb_radius;
cvar_t	*sk_nbomb_damage;

cvar_t	*sk_shockwave_bounces;
cvar_t	*sk_shockwave_damage;
cvar_t	*sk_shockwave_damage2;
cvar_t	*sk_shockwave_rdamage;
cvar_t	*sk_shockwave_radius;
cvar_t	*sk_shockwave_speed;
cvar_t	*sk_shockwave_effect_damage;
cvar_t	*sk_shockwave_effect_radius;

cvar_t	*sk_plasma_rifle_damage_bounce;
cvar_t	*sk_plasma_rifle_damage_bounce_dm;
cvar_t	*sk_plasma_rifle_damage_spread;
cvar_t	*sk_plasma_rifle_damage_spread_dm;
cvar_t	*sk_plasma_rifle_speed_bounce;
cvar_t	*sk_plasma_rifle_speed_spread;
cvar_t	*sk_plasma_rifle_radius;
cvar_t	*sk_plasma_rifle_life_bounce;
cvar_t	*sk_plasma_rifle_life_spread;
*/

// DM start values
cvar_t	*sk_dm_start_shells;
cvar_t	*sk_dm_start_bullets;
cvar_t	*sk_dm_start_rockets;
cvar_t	*sk_dm_start_homing;
cvar_t	*sk_dm_start_grenades;
cvar_t	*sk_dm_start_cells;
cvar_t	*sk_dm_start_slugs;
cvar_t	*sk_dm_start_magslugs;
cvar_t	*sk_dm_start_traps;
/*
cvar_t	*sk_dm_start_flechettes;
cvar_t	*sk_dm_start_rounds;
cvar_t	*sk_dm_start_prox;
cvar_t	*sk_dm_start_tesla;
cvar_t	*sk_dm_start_shocksphere;
*/

cvar_t	*sk_dm_start_shotgun;
cvar_t	*sk_dm_start_sshotgun;
cvar_t	*sk_dm_start_machinegun;
cvar_t	*sk_dm_start_chaingun;
cvar_t	*sk_dm_start_grenadelauncher;
cvar_t	*sk_dm_start_rocketlauncher;
cvar_t	*sk_dm_start_hyperblaster;
cvar_t	*sk_dm_start_railgun;
cvar_t	*sk_dm_start_bfg;
cvar_t	*sk_dm_start_ionripper;
cvar_t	*sk_dm_start_phalanx;
/*
cvar_t	*sk_dm_start_etfrifle;
cvar_t	*sk_dm_start_proxlauncher;
cvar_t	*sk_dm_start_plasmabeam;
cvar_t	*sk_dm_start_disruptor;
cvar_t	*sk_dm_start_chainfist;
cvar_t	*sk_dm_start_shockwave;
cvar_t	*sk_dm_start_plasmarifle;
*/

// maximum values
cvar_t	*sk_max_health;
cvar_t	*sk_max_health_dm;
cvar_t	*sk_max_foodcube_health;
cvar_t	*sk_max_armor_jacket;
cvar_t	*sk_max_armor_combat;
cvar_t	*sk_max_armor_body;
cvar_t	*sk_max_bullets;	
cvar_t	*sk_max_shells;	
cvar_t	*sk_max_rockets;	
cvar_t	*sk_max_grenades;	
cvar_t	*sk_max_cells;		
cvar_t	*sk_max_slugs;
cvar_t	*sk_max_magslugs;
cvar_t	*sk_max_traps;
/*
cvar_t	*sk_max_prox;
cvar_t	*sk_max_tesla;
cvar_t	*sk_max_flechettes;
cvar_t	*sk_max_rounds;
cvar_t	*sk_max_shocksphere;
*/

// maximum settings if a player gets a bandolier
cvar_t	*sk_bando_bullets;  // 300
cvar_t	*sk_bando_shells;   // 200
cvar_t	*sk_bando_cells;    // 300
cvar_t	*sk_bando_slugs;    // 100
cvar_t	*sk_bando_magslugs;
//cvar_t	*sk_bando_flechettes; // 250
//cvar_t	*sk_bando_rounds; // 150

// maximum settings if a player gets a pack
cvar_t	*sk_pack_health;
cvar_t	*sk_pack_armor;
cvar_t	*sk_pack_bullets;  // 300
cvar_t	*sk_pack_shells;   // 200
cvar_t	*sk_pack_rockets;  // 100
cvar_t	*sk_pack_grenades; // 100
cvar_t	*sk_pack_cells;    // 300
cvar_t	*sk_pack_slugs;    // 100
cvar_t	*sk_pack_magslugs;
cvar_t	*sk_pack_traps;
/*
cvar_t	*sk_pack_flechettes;
cvar_t	*sk_pack_rounds;
cvar_t	*sk_pack_prox;
cvar_t	*sk_pack_tesla;
cvar_t	*sk_pack_shocksphere;
*/

cvar_t	*sk_pack_give_xatrix_ammo;
//cvar_t	*sk_pack_give_rogue_ammo;

// pickup values
cvar_t	*sk_box_shells; // value of shells
cvar_t	*sk_box_bullets; // value of bullets
cvar_t	*sk_box_grenades; // value of grenade pack
cvar_t	*sk_box_rockets; // value of rocket pack
cvar_t	*sk_box_cells; // value of cell pack
cvar_t	*sk_box_slugs; // value of slug box
cvar_t	*sk_box_magslugs; // value ofmagslug box
cvar_t	*sk_box_flechettes; // value of flechettes
cvar_t	*sk_box_prox; // value of prox
cvar_t	*sk_box_tesla; // value of tesla pack
cvar_t	*sk_box_disruptors; // value of disruptor pack
cvar_t	*sk_box_shocksphere; // value of shocksphere
cvar_t	*sk_box_trap; // value of trap
cvar_t	*sk_box_fuel; // value of fuel
cvar_t	*sk_box_flares;	// value of flares box
cvar_t	*sk_box_tbombs;	// value of IRED box
cvar_t	*sk_box_empnuke;	// value of EMP nuke
cvar_t	*sk_box_plasmashield;	// value of plasma shield

// items/powerups
cvar_t	*sk_armor_bonus_value; // value of armor shards
cvar_t	*sk_health_bonus_value; // value of stimpacks
cvar_t	*sk_powerup_max;
/*
cvar_t	*sk_nuke_max;
cvar_t	*sk_nbomb_max;
cvar_t	*sk_doppleganger_max;
cvar_t	*sk_defender_time;
cvar_t	*sk_defender_blaster_damage;
cvar_t	*sk_defender_blaster_speed;
cvar_t	*sk_vengeance_time;
cvar_t	*sk_vengeance_health_threshold;
cvar_t	*sk_hunter_time;
cvar_t	*sk_doppleganger_time;
*/
cvar_t	*sk_quad_time;
cvar_t	*sk_inv_time;
cvar_t	*sk_breather_time;
cvar_t	*sk_enviro_time;
cvar_t	*sk_silencer_shots;
//cvar_t	*sk_ir_time;
cvar_t	*sk_double_time;
cvar_t	*sk_quad_fire_time;

// CTF stuff
cvar_t	*use_techs;          // enables techs
cvar_t	*use_coloredtechs;   // enable colored techs, otherwise plain CTF Techs
cvar_t	*use_lithiumtechs;   // enable lithium style colored runes, otherwise plain CTF Techs

cvar_t	*ctf_blastercolors;   // enable different blaster colors for each team
cvar_t	*ctf_railcolors;      // enable different railtrail colors for each team

cvar_t	*allow_flagdrop;
cvar_t	*allow_flagpickup;
cvar_t	*allow_techdrop;
cvar_t	*allow_techpickup;

cvar_t	*tech_flags;         // determines which techs will show in the game, add these:
							 // 1 = resist, 2 = strength, 4 = haste, 8 = regen, 16 = vampire, 32 = ammogen
//cvar_t	*tech_spawn;         // chance a rune will spawn from another item respawning
//cvar_t	*tech_perplayer;     // sets techs per player that will appear in map
cvar_t	*tech_life;          // seconds a rune will stay around before disappearing
//cvar_t	*tech_min;           // sets minimum number of techs to be in the game
//cvar_t	*tech_max;           // sets maximum number of techs to be in the game

//cvar_t	*tech_haste; // what should I use this for?
cvar_t	*tech_resist;        // sets how much damage is divided by with resist rune
cvar_t	*tech_strength;      // sets how much damage is multiplied by with strength rune
//cvar_t	*tech_regen;         // sets how fast health is gained back
cvar_t	*tech_regen_armor;
cvar_t	*tech_regen_health_max;      // sets maximum health that can be gained from regen rune
cvar_t	*tech_regen_armor_max;      // sets maximum armor that can be gained from regen rune
cvar_t	*tech_regen_armor_always;      // sets whether armor should be regened regardless of if currently held
cvar_t	*tech_vampire;       // sets percentage of health gained from damage inflicted
cvar_t	*tech_vampiremax;    // sets maximum health that can be gained from vampire rune
// end CTF stuff

void InitLithiumVars (void)
{
/*
	mega_gibs = gi.cvar("mega_gibs", "0", CVAR_ARCHIVE);
	mp_monster_replace = gi.cvar("mp_monster_replace", "0", CVAR_LATCH);
	mp_monster_ammo_replace = gi.cvar("mp_monster_ammo_replace", "0", CVAR_ARCHIVE);
	kamikaze_flyer_replace = gi.cvar("kamikaze_flyer_replace", "0", CVAR_ARCHIVE);
	allow_player_use_abandoned_turret = gi.cvar("allow_player_use_abandoned_turret", "0", CVAR_ARCHIVE);
	turn_rider = gi.cvar("turn_rider", "1", CVAR_ARCHIVE);
	adjust_train_corners = gi.cvar("adjust_train_corners", "0", CVAR_ARCHIVE);

	add_velocity_throw = gi.cvar("add_velocity_throw", "0", CVAR_ARCHIVE);
	falling_armor_damage = gi.cvar("falling_armor_damage", "0", CVAR_ARCHIVE);
	player_jump_sounds = gi.cvar("player_jump_sounds", "1", CVAR_ARCHIVE);
	tpp_auto = gi.cvar("tpp_auto", "1", CVAR_ARCHIVE);
	use_vwep = gi.cvar("use_vwep", "1", CVAR_ARCHIVE);
*/
	player_max_speed = gi.cvar("player_max_speed", "300", 0);
	player_crouch_speed = gi.cvar("player_crouch_speed", "100", 0); 
	player_accel = gi.cvar("player_accel", "10", 0);
	player_stopspeed = gi.cvar("player_stopspeed", "100", 0);

	// weapon balancing
	sk_blaster_damage = gi.cvar("sk_blaster_damage", "10", 0);
	sk_blaster_damage_dm = gi.cvar("sk_blaster_damage_dm", "15", 0);
	sk_blaster_speed = gi.cvar("sk_blaster_speed", "1000", 0);
	sk_blaster_color = gi.cvar("sk_blaster_color", "1", 0);

	sk_shotgun_damage = gi.cvar("sk_shotgun_damage", "4", 0);
	sk_shotgun_count = gi.cvar("sk_shotgun_count", "12", 0);
	sk_shotgun_hspread = gi.cvar("sk_shotgun_hspread", "500", 0);
	sk_shotgun_vspread = gi.cvar("sk_shotgun_vspread", "500", 0);

	sk_sshotgun_damage = gi.cvar("sk_sshotgun_damage", "6", 0);
	sk_sshotgun_count = gi.cvar("sk_sshotgun_count", "20", 0);
	sk_sshotgun_hspread = gi.cvar("sk_sshotgun_hspread", "1000", 0);
	sk_sshotgun_vspread = gi.cvar("sk_sshotgun_vspread", "500", 0);

	sk_machinegun_damage = gi.cvar("sk_machinegun_damage", "8", 0);
	sk_machinegun_hspread = gi.cvar("sk_machinegun_hspread", "300", 0);
	sk_machinegun_vspread = gi.cvar("sk_machinegun_vspread", "500", 0);

	sk_chaingun_damage = gi.cvar("sk_chaingun_damage", "8", 0);
	sk_chaingun_damage_dm = gi.cvar("sk_chaingun_damage_dm", "6", 0);
	sk_chaingun_hspread = gi.cvar("sk_chaingun_hspread", "300", 0);
	sk_chaingun_vspread = gi.cvar("sk_chaingun_vspread", "500", 0);

	sk_grenade_damage = gi.cvar("sk_grenade_damage", "120", 0);
	sk_grenade_radius = gi.cvar("sk_grenade_radius", "160", 0);
	sk_grenade_speed = gi.cvar("sk_grenade_speed", "600", 0);

	sk_hand_grenade_damage = gi.cvar("sk_hand_grenade_damage", "125", 0);
	sk_hand_grenade_radius = gi.cvar("sk_hand_grenade_radius", "165", 0);

	sk_rocket_damage = gi.cvar("sk_rocket_damage", "100", 0);
	sk_rocket_damage2 = gi.cvar("sk_rocket_damage2", "20", 0);
	sk_rocket_rdamage = gi.cvar("sk_rocket_rdamage", "120", 0);
	sk_rocket_radius = gi.cvar("sk_rocket_radius", "140", 0);
	sk_rocket_speed = gi.cvar("sk_rocket_speed", "650", 0);
	sk_rocket_lockon_speed = gi.cvar("sk_rocket_lockon_speed", "20", 0);

	sk_hyperblaster_damage = gi.cvar("sk_hyperblaster_damage", "20", 0);
	sk_hyperblaster_damage_dm = gi.cvar("sk_hyperblaster_damage_dm", "15", 0);
	sk_hyperblaster_speed = gi.cvar("sk_hyperblaster_speed", "1000", 0);
	sk_hyperblaster_color = gi.cvar("sk_hyperblaster_color", "1", 0);

	sk_railgun_damage = gi.cvar("sk_railgun_damage", "150", 0);
	sk_railgun_damage_dm = gi.cvar("sk_railgun_damage_dm", "100", 0);
	sk_railgun_skin = gi.cvar("sk_railgun_skin", "0", 0);

	sk_bfg_damage = gi.cvar("sk_bfg_damage", "500", 0);
	sk_bfg_damage_dm = gi.cvar("sk_bfg_damage_dm", "200", 0);
	sk_bfg_damage2 = gi.cvar("sk_bfg_damage2", "10", 0);
	sk_bfg_damage2_dm = gi.cvar("sk_bfg_damage2_dm", "5", 0);
	sk_bfg_rdamage = gi.cvar("sk_bfg_rdamage", "200", 0);
	sk_bfg_radius = gi.cvar("sk_bfg_radius", "1000", 0);
	sk_bfg_speed = gi.cvar("sk_bfg_speed", "400", 0);

//	sk_jump_kick_damage = gi.cvar("sk_jump_kick_damage", "10", 0);

	sk_ionripper_damage = gi.cvar("sk_ionripper_damage", "50", 0);
	sk_ionripper_damage_dm = gi.cvar("sk_ionripper_damage_dm", "30", 0);
	sk_ionripper_kick = gi.cvar("sk_ionripper_kick", "60", 0);
	sk_ionripper_kick_dm = gi.cvar("sk_ionripper_kick_dm", "40", 0);
	sk_ionripper_speed = gi.cvar("sk_ionripper_speed", "500", 0);
	sk_ionripper_extra_sounds = gi.cvar("sk_ionripper_extra_sounds", "0", CVAR_ARCHIVE);

	sk_phalanx_damage = gi.cvar("sk_phalanx_damage", "70", 0);
	sk_phalanx_damage2 = gi.cvar("sk_phalanx_damage2", "10", 0);
	sk_phalanx_radius_damage = gi.cvar("sk_phalanx_radius_damage", "120", 0);
	sk_phalanx_radius = gi.cvar("sk_phalanx_radius", "120", 0);
	sk_phalanx_speed = gi.cvar("sk_phalanx_speed", "725", 0);

	sk_trap_life = gi.cvar("sk_trap_life", "30", 0);
	sk_trap_health = gi.cvar("sk_trap_health", "30", 0);
/*
	sk_etf_rifle_damage = gi.cvar("sk_etf_rifle_damage", "10", 0);
	sk_etf_rifle_damage_dm = gi.cvar("sk_etf_rifle_damage_dm", "10", 0);
	sk_etf_rifle_radius_damage = gi.cvar("sk_etf_rifle_radius_damage", "20", 0);
	sk_etf_rifle_radius = gi.cvar("sk_etf_rifle_radius", "100", 0);
	sk_etf_rifle_speed = gi.cvar("sk_etf_rifle_speed", "850", 0);

	sk_plasmabeam_damage = gi.cvar("sk_plasmabeam_damage", "15", 0);
	sk_plasmabeam_damage_dm = gi.cvar("sk_plasmabeam_damage_dm", "15", 0);

	sk_disruptor_damage = gi.cvar("sk_disruptor_damage", "45", 0);
	sk_disruptor_damage_dm = gi.cvar("sk_disruptor_damage_dm", "30", 0);
	sk_disruptor_speed = gi.cvar("sk_disruptor_speed", "1000", 0);

	sk_prox_damage = gi.cvar("sk_prox_damage", "135", 0);
	sk_prox_radius = gi.cvar("sk_prox_radius", "192", 0);
	sk_prox_speed = gi.cvar("sk_prox_speed", "600", 0);
	sk_prox_life = gi.cvar("sk_prox_life", "600", 0);
	sk_prox_health = gi.cvar("sk_prox_health", "20", 0);

	sk_tesla_damage = gi.cvar("sk_tesla_damage", "3", 0);
	sk_tesla_radius = gi.cvar("sk_tesla_radius", "128", 0);
	sk_tesla_life = gi.cvar("sk_tesla_life", "30", 0);
	sk_tesla_health = gi.cvar("sk_tesla_health", "20", 0);

	sk_chainfist_damage = gi.cvar("sk_chainfist_damage", "15", 0);
	sk_chainfist_damage_dm = gi.cvar("sk_chainfist_damage_dm", "30", 0);

	sk_nuke_delay = gi.cvar("sk_nuke_delay", "4", 0);
	sk_nuke_life = gi.cvar("sk_nuke_life", "6", 0);
	sk_nuke_radius = gi.cvar("sk_nuke_radius", "512", 0);

	sk_nbomb_delay = gi.cvar("sk_nbomb_delay", "4", 0);
	sk_nbomb_life = gi.cvar("sk_nbomb_life", "6", 0);
	sk_nbomb_radius = gi.cvar("sk_nbomb_radius", "256", 0);
	sk_nbomb_damage = gi.cvar("sk_nbomb_damage", "5000", 0);

	sk_shockwave_bounces = gi.cvar("sk_shockwave_bounces", "6", 0);
	sk_shockwave_damage = gi.cvar("sk_shockwave_damage", "80", 0);
	sk_shockwave_damage2 = gi.cvar("sk_shockwave_damage2", "40", 0);
	sk_shockwave_rdamage = gi.cvar("sk_shockwave_rdamage", "120", 0);
	sk_shockwave_radius = gi.cvar("sk_shockwave_radius", "300", 0);
	sk_shockwave_speed = gi.cvar("sk_shockwave_speed", "650", 0);
	sk_shockwave_effect_damage = gi.cvar("sk_shockwave_effect_damage", "100", 0);
	sk_shockwave_effect_radius = gi.cvar("sk_shockwave_effect_radius", "340", 0);

	sk_plasma_rifle_damage_bounce = gi.cvar("sk_plasma_rifle_damage_bounce", "60", 0);
	sk_plasma_rifle_damage_bounce_dm = gi.cvar("sk_plasma_rifle_damage_bounce_dm", "39", 0);
	sk_plasma_rifle_damage_spread = gi.cvar("sk_plasma_rifle_damage_spread", "43", 0);
	sk_plasma_rifle_damage_spread_dm = gi.cvar("sk_plasma_rifle_damage_spread_dm", "28", 0);
	sk_plasma_rifle_speed_bounce = gi.cvar("sk_plasma_rifle_speed_bounce", "1200", 0);
	sk_plasma_rifle_speed_spread = gi.cvar("sk_plasma_rifle_speed_spread", "1200", 0);
	sk_plasma_rifle_radius = gi.cvar("sk_plasma_rifle_radius", "70", 0);
	sk_plasma_rifle_life_bounce = gi.cvar("sk_plasma_rifle_life_bounce", "3", 0);
	sk_plasma_rifle_life_spread = gi.cvar("sk_plasma_rifle_life_spread", "3", 0);
*/
	// DM start values
	sk_dm_start_shells = gi.cvar("sk_dm_start_shells", "0", 0);
	sk_dm_start_bullets = gi.cvar("sk_dm_start_bullets", "0", 0);
	sk_dm_start_rockets = gi.cvar("sk_dm_start_rockets", "0", 0);
	sk_dm_start_homing = gi.cvar("sk_dm_start_homing", "0", 0);
	sk_dm_start_grenades = gi.cvar("sk_dm_start_grenades", "0", 0);
	sk_dm_start_cells = gi.cvar("sk_dm_start_cells", "0", 0);
	sk_dm_start_slugs = gi.cvar("sk_dm_start_slugs", "0", 0);
	sk_dm_start_magslugs = gi.cvar("sk_dm_start_magslugs", "0", 0);
	sk_dm_start_traps = gi.cvar("sk_dm_start_traps", "0", 0);
/*
	sk_dm_start_flechettes = gi.cvar("sk_dm_start_flechettes", "0", 0);
	sk_dm_start_rounds = gi.cvar("sk_dm_start_rounds", "0", 0);
	sk_dm_start_prox = gi.cvar("sk_dm_start_prox", "0", 0);
	sk_dm_start_tesla = gi.cvar("sk_dm_start_tesla", "0", 0);
	sk_dm_start_shocksphere = gi.cvar("sk_dm_start_shocksphere", "0", 0);
*/
	sk_dm_start_shotgun = gi.cvar("sk_dm_start_shotgun", "0", 0);
	sk_dm_start_sshotgun = gi.cvar("sk_dm_start_sshotgun", "0", 0);
	sk_dm_start_machinegun = gi.cvar("sk_dm_start_machinegun", "0", 0);
	sk_dm_start_chaingun = gi.cvar("sk_dm_start_chaingun", "0", 0);
	sk_dm_start_grenadelauncher = gi.cvar("sk_dm_start_grenadelauncher", "0", 0);
	sk_dm_start_rocketlauncher = gi.cvar("sk_dm_start_rocketlauncher", "0", 0);
	sk_dm_start_hyperblaster = gi.cvar("sk_dm_start_hyperblaster", "0", 0);
	sk_dm_start_railgun = gi.cvar("sk_dm_start_railgun", "0", 0);
	sk_dm_start_bfg = gi.cvar("sk_dm_start_bfg", "0", 0);
	sk_dm_start_ionripper = gi.cvar("sk_dm_start_ionripper", "0", 0);
	sk_dm_start_phalanx = gi.cvar("sk_dm_start_phalanx", "0", 0);
/*
	sk_dm_start_etfrifle = gi.cvar("sk_dm_start_etfrifle", "0", 0);
	sk_dm_start_proxlauncher = gi.cvar("sk_dm_start_proxlauncher", "0", 0);
	sk_dm_start_plasmabeam = gi.cvar("sk_dm_start_plasmabeam", "0", 0);
	sk_dm_start_disruptor = gi.cvar("sk_dm_start_disruptor", "0", 0);
	sk_dm_start_chainfist = gi.cvar("sk_dm_start_chainfist", "0", 0);
	sk_dm_start_shockwave = gi.cvar("sk_dm_start_shockwave", "0", 0);
	sk_dm_start_plasmarifle = gi.cvar("sk_dm_start_plasmarifle", "0", 0);
*/
	// maximum values
	sk_max_health = gi.cvar("sk_max_health", "100", 0);
	sk_max_health_dm = gi.cvar("sk_max_health_dm", "120", 0);
	sk_max_foodcube_health = gi.cvar("sk_max_foodcube_health", "300", 0);
	sk_max_armor_jacket = gi.cvar("sk_max_armor_jacket", "50", 0);
	sk_max_armor_combat = gi.cvar("sk_max_armor_combat", "100", 0);
	sk_max_armor_body = gi.cvar("sk_max_armor_body", "200", 0);
	sk_max_bullets = gi.cvar("sk_max_bullets", "200", 0);
	sk_max_shells = gi.cvar("sk_max_shells", "100", 0);
	sk_max_rockets = gi.cvar("sk_max_rockets", "50", 0);
	sk_max_grenades = gi.cvar("sk_max_grenades", "50", 0);
	sk_max_cells = gi.cvar("sk_max_cells", "200", 0);
	sk_max_slugs = gi.cvar("sk_max_slugs", "50", 0);
	sk_max_magslugs = gi.cvar("sk_max_magslugs", "60", 0);
	sk_max_traps = gi.cvar("sk_max_traps", "10", 0);
/*
	sk_max_prox = gi.cvar("sk_max_prox", "50", 0);
	sk_max_tesla = gi.cvar("sk_max_tesla", "50", 0);
	sk_max_flechettes = gi.cvar("sk_max_flechettes", "200", 0);
	sk_max_rounds = gi.cvar("sk_max_rounds", "100", 0);
	sk_max_shocksphere = gi.cvar("sk_max_shocksphere", "10", 0);
*/
	// maximum settings if a player gets a bandolier
	sk_bando_bullets = gi.cvar("sk_bando_bullets", "250", 0);
	sk_bando_shells = gi.cvar("sk_bando_shells", "150", 0);
	sk_bando_cells = gi.cvar("sk_bando_cells", "250", 0);
	sk_bando_slugs = gi.cvar("sk_bando_slugs", "75", 0);
	sk_bando_magslugs = gi.cvar("sk_bando_magslugs", "75", 0);
//	sk_bando_flechettes = gi.cvar("sk_bando_flechettes", "250", 0);
//	sk_bando_rounds = gi.cvar("sk_bando_rounds", "150", 0);

	// maximum settings if a player gets a pack
	sk_pack_health = gi.cvar("sk_pack_health", "120", 0);
	sk_pack_armor = gi.cvar("sk_pack_armor", "250", 0);
	sk_pack_bullets = gi.cvar("sk_pack_bullets", "300", 0);
	sk_pack_shells = gi.cvar("sk_pack_shells", "200", 0);
	sk_pack_rockets = gi.cvar("sk_pack_rockets", "100", 0);
	sk_pack_grenades = gi.cvar("sk_pack_grenades", "100", 0);
	sk_pack_cells = gi.cvar("sk_pack_cells", "300", 0);
	sk_pack_slugs = gi.cvar("sk_pack_slugs", "100", 0);
	sk_pack_magslugs = gi.cvar("sk_pack_magslugs", "100", 0);
	sk_pack_traps = gi.cvar("sk_pack_traps", "50", 0);
/*
	sk_pack_flechettes = gi.cvar("sk_pack_flechettes", "300", 0);
	sk_pack_rounds = gi.cvar("sk_pack_rounds", "200", 0);
	sk_pack_prox = gi.cvar("sk_pack_prox", "100", 0);
	sk_pack_tesla = gi.cvar("sk_pack_tesla", "100", 0);
	sk_pack_shocksphere = gi.cvar("sk_pack_shocksphere", "20", 0);
*/
	sk_pack_give_xatrix_ammo = gi.cvar("sk_pack_give_xatrix_ammo", "1", CVAR_ARCHIVE);
//	sk_pack_give_rogue_ammo = gi.cvar("sk_pack_give_rogue_ammo", "0", CVAR_ARCHIVE);

	// pickup values
	sk_box_shells = gi.cvar("sk_box_shells", "10", 0);
	sk_box_bullets = gi.cvar("sk_box_bullets", "50", 0);
	sk_box_grenades = gi.cvar("sk_box_grenades", "5", 0);
	sk_box_rockets = gi.cvar("sk_box_rockets", "5", 0);
	sk_box_cells = gi.cvar("sk_box_cells", "50", 0);
	sk_box_slugs = gi.cvar("sk_box_slugs", "10", 0);
	sk_box_magslugs = gi.cvar("sk_box_magslugs", "10", 0);
	sk_box_trap = gi.cvar("sk_box_trap", "1", 0);
/*
	sk_box_flechettes = gi.cvar("sk_box_flechettes", "50", 0);
	sk_box_prox = gi.cvar("sk_box_prox", "5", 0);
	sk_box_tesla = gi.cvar("sk_box_tesa", "5", 0);
	sk_box_disruptors = gi.cvar("sk_box_disruptors", "15", 0);
	sk_box_shocksphere = gi.cvar("sk_box_shocksphere", "1", 0);
*/
	// items/powerups
	sk_armor_bonus_value = gi.cvar("sk_armor_bonus_value", "2", 0);
	sk_health_bonus_value = gi.cvar("sk_health_bonus_value", "2", 0);
	sk_powerup_max = gi.cvar("sk_powerup_max", "2", 0);
/*
	sk_doppleganger_max = gi.cvar("sk_doppleganger_max", "1", 0);
	sk_nuke_max = gi.cvar("sk_nuke_max", "1", 0);
	sk_nbomb_max = gi.cvar("sk_nbomb_max", "4", 0);
	sk_defender_time = gi.cvar("sk_defender_time", "60", 0);
	sk_defender_blaster_damage = gi.cvar("sk_defender_blaster_damage", "10", 0);
	sk_defender_blaster_speed = gi.cvar("sk_defender_blaster_speed", "1000", 0);
	sk_vengeance_time = gi.cvar("sk_vengeance_time", "60", 0);
	sk_vengeance_health_threshold = gi.cvar("sk_vengeance_health_threshold", "25", 0);
	sk_hunter_time = gi.cvar("sk_hunter_time", "60", 0);
	sk_doppleganger_time = gi.cvar("sk_doppleganger_time", "30", 0);
*/
	sk_quad_time = gi.cvar("sk_quad_time", "30", 0);
	sk_inv_time = gi.cvar("sk_inv_time", "30", 0);
	sk_breather_time = gi.cvar("sk_breather_time", "30", 0);
	sk_enviro_time = gi.cvar("sk_enviro_time", "30", 0);
	sk_silencer_shots = gi.cvar("sk_silencer_shots", "30", 0);
//	sk_ir_time = gi.cvar("sk_ir_time", "30", 0);
	sk_double_time = gi.cvar("sk_double_time", "30", 0);
	sk_quad_fire_time = gi.cvar("sk_quad_fire_time", "30", 0);

	// CTF stuff
	use_techs = gi.cvar("use_techs", "0", 0);
	use_coloredtechs = gi.cvar("use_coloredtechs", "0", 0);
	use_lithiumtechs = gi.cvar("use_lithiumtechs", "0", 0);

	ctf_blastercolors = gi.cvar("ctf_blastercolors", "0", 0);
	ctf_railcolors = gi.cvar("ctf_railcolors", "0", 0);

	allow_flagdrop = gi.cvar("allow_flagdrop", "1", 0);
	allow_flagpickup = gi.cvar("allow_flagpickup", "1", 0);
	allow_techdrop = gi.cvar("allow_techdrop", "1", 0);
	allow_techpickup = gi.cvar("allow_techpickup", "1", 0);

	tech_flags = gi.cvar("tech_flags", "15", 0);
//	tech_spawn = gi.cvar("tech_spawn", "0.10", 0);
//	tech_perplayer = gi.cvar("tech_perplayer", "0.7", 0);
	tech_life = gi.cvar("tech_life", "20", 0);
//	tech_min = gi.cvar("tech_min", "2", 0);
//	tech_max = gi.cvar("tech_max", "10", 0);

//	tech_haste = gi.cvar("tech_haste", "", 0);
	tech_resist = gi.cvar("tech_resist", "2.0", 0);
	tech_strength = gi.cvar("tech_strength", "2.0", 0);
//	tech_regen = gi.cvar("tech_regen", "0.25", 0);
	tech_regen_armor = gi.cvar("tech_regen_armor", "1", 0);
	tech_regen_health_max = gi.cvar("tech_regen_health_max", "150", 0);
	tech_regen_armor_max = gi.cvar("tech_regen_armor_max", "150", 0);
	tech_regen_armor_always = gi.cvar("tech_regen_armor_always", "0", 0);
	tech_vampire = gi.cvar("tech_vampire", "0.5", 0);
	tech_vampiremax = gi.cvar("tech_vampiremax", "200", 0);
	// end CTF Tech stuff
}
