/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2024 Mark E. Sowden <hogsy@oldtimes-software.com>

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

#include "../g_local.h"
#include "../game_entity_manager.h"
#include "../game_entity.h"

static const char *single_statusbar =
        "yb	-24 "

        // health
        "xv	0 "
        "hnum "
        "xv	50 "
        "pic 0 "

        // ammo
        "if 2 "
        "	xv	100 "
        "	anum "
        "	xv	150 "
        "	pic 2 "
        "endif "

        // armor
        "if 4 "
        "	xv	200 "
        "	rnum "
        "	xv	250 "
        "	pic 4 "
        "endif "

        // selected item
        "if 6 "
        "	xv	296 "
        "	pic 6 "
        "endif "

        "yb	-50 "

        // picked up item
        "if 7 "
        "	xv	0 "
        "	pic 7 "
        "	xv	26 "
        "	yb	-42 "
        "	stat_string 8 "
        "	yb	-50 "
        "endif "

        // timer (was xv 262)
        "if 9 "
        "	xv	230 "
        "	num	4 10 "
        "	xv	296 "
        "	pic	9 "
        "endif "

        //  help / weapon icon
        "if 11 "
        "	xv	148 "
        "	pic	11 "
        "endif "

        // vehicle speed
        "if 22 "
        "	yb -90 "
        "	xv 128 "
        "	pic 22 "
        "endif "

        // zoom
        "if 23 "
        "   yv 0 "
        "   xv 0 "
        "   pic 23 "
        "endif ";

static const char *dm_statusbar =
        "yb	-24 "

        // health
        "xv	0 "
        "hnum "
        "xv	50 "
        "pic 0 "

        // ammo
        "if 2 "
        "	xv	100 "
        "	anum "
        "	xv	150 "
        "	pic 2 "
        "endif "

        // armor
        "if 4 "
        "	xv	200 "
        "	rnum "
        "	xv	250 "
        "	pic 4 "
        "endif "

        // selected item
        "if 6 "
        "	xv	296 "
        "	pic 6 "
        "endif "

        "yb	-50 "

        // picked up item
        "if 7 "
        "	xv	0 "
        "	pic 7 "
        "	xv	26 "
        "	yb	-42 "
        "	stat_string 8 "
        "	yb	-50 "
        "endif "

        // timer
        "if 9 "
        "	xv	230 "
        "	num	4 10 "
        "	xv	296 "
        "	pic	9 "
        "endif "

        // help / weapon icon
        "if 11 "
        "	xv	148 "
        "	pic	11 "
        "endif "

        // frags
        "xr	-50 "
        "yt 2 "
        "num 3 14 "

        // tech
        "yb -75 "
        "if 26 "
        "xr -26 "
        "pic 26 "
        "endif "

        // spectator
        "if 17 "
        "xv 0 "
        "yb -58 "
        "string2 \"SPECTATOR MODE\" "
        "endif "

        // chase camera
        "if 16 "
        "xv 0 "
        "yb -68 "
        "string \"Chasing\" "
        "xv 64 "
        "stat_string 16 "
        "endif "

        // vehicle speed
        "if 22 "
        "	yb -90 "
        "	xv 128 "
        "	pic 22 "
        "endif ";

class WorldSpawn : public Entity
{
	IMPLEMENT_ENTITY( WorldSpawn, Entity )

public:
	WorldSpawn() = default;

	void Spawn() override
	{
		movetype     = MOVETYPE_PUSH;
		solid        = SOLID_BSP;
		inuse        = true;// since the world doesn't use G_Spawn()
		s.modelindex = 1;   // world model is always index 1

//		paused = false;

		// reserve some spots for dead player bodies for coop / deathmatch
//		InitBodyQue();

		// set configstrings for items
//		SetItemNames();

//		if ( st.nextmap )
//			Q_strncpyz( level.nextmap, sizeof( level.nextmap ), st.nextmap );


		// make some data visible to the server

		if ( !message.empty() && message[ 0 ] )
		{
			gi.configstring( CS_NAME, message.data() );
//			Q_strncpyz( level.level_name, sizeof( level.level_name ), message.c_str() );
		}
		else
		{
//			Q_strncpyz( level.level_name, sizeof( level.level_name ), level.mapname );
		}

//		if ( st.sky && st.sky[ 0 ] )
//			gi.configstring( CS_SKY, st.sky );
//		else
//			gi.configstring( CS_SKY, "unit1_" );

//		gi.configstring( CS_SKYROTATE, va( "%f", st.skyrotate ) );

//		gi.configstring( CS_SKYAXIS, va( "%f %f %f",
//		                                 st.skyaxis[ 0 ], st.skyaxis[ 1 ], st.skyaxis[ 2 ] ) );

		// Knightmare- configstrings added for DK-style clouds support
#ifdef KMQUAKE2_ENGINE_MOD
//		gi.configstring( CS_SKYDISTANCE, va( "%f", st.skydistance ) );

//		if ( st.cloudname && st.cloudname[ 0 ] )
//			gi.configstring( CS_CLOUDNAME, st.cloudname );
//		else
			gi.configstring( CS_CLOUDNAME, "" );

//		gi.configstring( CS_CLOUDLIGHTFREQ, va( "%f", st.lightningfreq ) );

//		gi.configstring( CS_CLOUDDIR, va( "%f %f", st.cloudxdir, st.cloudydir ) );

//		gi.configstring( CS_CLOUDTILE, va( "%f %f %f", st.cloud1tile, st.cloud2tile, st.cloud3tile ) );

//		gi.configstring( CS_CLOUDSPEED, va( "%f %f %f", st.cloud1speed, st.cloud2speed, st.cloud3speed ) );

//		gi.configstring( CS_CLOUDALPHA, va( "%f %f %f", st.cloud1alpha, st.cloud2alpha, st.cloud3alpha ) );
#endif// KMQUAKE2_ENGINE_MOD \
        // end DK-style clouds support

		// Knightmare- if a named soundtrack is specified, play it instead of from CD
		if ( musictrack && strlen( musictrack ) )
			gi.configstring( CS_CDTRACK, musictrack );
		else
			gi.configstring( CS_CDTRACK, va( "%i", sounds ) );

		gi.configstring( CS_MAXCLIENTS, va( "%i", static_cast< int >( maxclients->value ) ) );

		// Knightmare added
//		if ( ttctf->value )
//		{
//			gi.cvar_forceset( "ctf", "1" );
//			gi.cvar_forceset( "deathmatch", "1" );
//		}
//		else if ( ctf->value )
//		{
//			gi.cvar_forceset( "deathmatch", "1" );
//		}

		// status bar program
		gi.configstring( CS_STATUSBAR, single_statusbar );
#ifdef KMQUAKE2_ENGINE_MOD
		gi.configstring( CS_HUDVARIANT, "default" );// use DM/SP HUD script variant
#endif

		//---------------

		// help icon for statusbar
		gi.imageindex( "i_help" );
//		level.pic_health = gi.imageindex( "i_health" );
		gi.imageindex( "help" );
		gi.imageindex( "field_3" );

//		if ( !st.gravity )
			gi.cvar_set( "sv_gravity", "800" );
//		else
//			gi.cvar_set( "sv_gravity", st.gravity );

//		snd_fry = gi.soundindex( "player/fry.wav" );// standing in lava / slime

//		PrecacheItem( FindItem( "Blaster" ) );

		gi.soundindex( "player/lava1.wav" );
		gi.soundindex( "player/lava2.wav" );

		gi.soundindex( "misc/pc_up.wav" );
		gi.soundindex( "misc/talk1.wav" );

		gi.soundindex( "misc/udeath.wav" );

		// gibs
		gi.soundindex( "items/respawn1.wav" );

		// sexed sounds
		gi.soundindex( "*death1.wav" );
		gi.soundindex( "*death2.wav" );
		gi.soundindex( "*death3.wav" );
		gi.soundindex( "*death4.wav" );
		gi.soundindex( "*fall1.wav" );
		gi.soundindex( "*fall2.wav" );
		gi.soundindex( "*gurp1.wav" );// drowning damage
		gi.soundindex( "*gurp2.wav" );
		gi.soundindex( "*jump1.wav" );// player jump
		gi.soundindex( "*pain25_1.wav" );
		gi.soundindex( "*pain25_2.wav" );
		gi.soundindex( "*pain50_1.wav" );
		gi.soundindex( "*pain50_2.wav" );
		gi.soundindex( "*pain75_1.wav" );
		gi.soundindex( "*pain75_2.wav" );
		gi.soundindex( "*pain100_1.wav" );
		gi.soundindex( "*pain100_2.wav" );

		// sexed models
		// THIS ORDER MUST MATCH THE DEFINES IN g_local.h
		// you can add more, max 64
//		if ( use_vwep->value || deathmatch->value )
//		{
//			gi.modelindex( "#w_blaster.md2" );
//			gi.modelindex( "#w_shotgun.md2" );
//			gi.modelindex( "#w_sshotgun.md2" );
//			gi.modelindex( "#w_machinegun.md2" );
//			gi.modelindex( "#w_chaingun.md2" );
//			gi.modelindex( "#a_grenades.md2" );
//			gi.modelindex( "#w_glauncher.md2" );
//			gi.modelindex( "#w_rlauncher.md2" );
//			gi.modelindex( "#w_hyperblaster.md2" );
//			gi.modelindex( "#w_railgun.md2" );
//			gi.modelindex( "#w_bfg.md2" );
//			if ( ctf->value )
//				gi.modelindex( "#w_grapple.md2" );
//		}

		//-------------------

		gi.soundindex( "player/gasp1.wav" );// gasping for air
		gi.soundindex( "player/gasp2.wav" );// head breaking surface, not gasping

		gi.soundindex( "player/watr_in.wav" ); // feet hitting water
		gi.soundindex( "player/watr_out.wav" );// feet leaving water

		gi.soundindex( "player/watr_un.wav" );// head going underwater

		gi.soundindex( "player/u_breath1.wav" );
		gi.soundindex( "player/u_breath2.wav" );

		gi.soundindex( "items/pkup.wav" );  // bonus item pickup
		gi.soundindex( "world/land.wav" );  // landing thud
		gi.soundindex( "misc/h2ohit1.wav" );// landing splash

		gi.soundindex( "items/damage.wav" );
		gi.soundindex( "items/protect.wav" );
		gi.soundindex( "items/protect4.wav" );
		gi.soundindex( "weapons/noammo.wav" );

		gi.soundindex( "infantry/inflies1.wav" );

//		sm_meat_index = gi.modelindex( "models/objects/gibs/sm_meat/tris.md2" );
		gi.modelindex( "models/objects/gibs/arm/tris.md2" );
		gi.modelindex( "models/objects/gibs/leg/tris.md2" );
		gi.modelindex( "models/objects/gibs/bone/tris.md2" );
		gi.modelindex( "models/objects/gibs/bone2/tris.md2" );
		gi.modelindex( "models/objects/gibs/chest/tris.md2" );
		gi.modelindex( "models/objects/gibs/skull/tris.md2" );
		gi.modelindex( "models/objects/gibs/head2/tris.md2" );

		gi.soundindex( "mud/mud_in2.wav" );
		gi.soundindex( "mud/mud_out1.wav" );
		gi.soundindex( "mud/mud_un1.wav" );
		gi.soundindex( "mud/wade_mud1.wav" );
		gi.soundindex( "mud/wade_mud2.wav" );

//		Lights();

		// Fog clipping - if "fogclip" is non-zero, force gl_clear to a good
		// value for obscuring HOM with fog... "good" is driver-dependent
		if ( fogclip )
		{
			if ( gl_driver && !Q_stricmp( gl_driver->string, "3dfxgl" ) )
				gi.cvar_forceset( GL_CLEAR_CVAR, "0" );
			else
				gi.cvar_forceset( GL_CLEAR_CVAR, "1" );
		}

		// cvar overrides for effects flags:
//		if ( alert_sounds->value )
//			world->effects |= FX_WORLDSPAWN_ALERTSOUNDS;
//		if ( corpse_fade->value )
//			world->effects |= FX_WORLDSPAWN_CORPSEFADE;
//		if ( jump_kick->value )
//			world->effects |= FX_WORLDSPAWN_JUMPKICK;
	}
};

REGISTER_ENTITY_CLASS( worldspawn, WorldSpawn )

// Hud toggle ripped from TPP source

static int nohud = 0;

void Hud_On()
{
	gi.configstring( CS_STATUSBAR, single_statusbar );
	nohud = 0;
}

void Hud_Off()
{
	gi.configstring( CS_STATUSBAR, nullptr );
	nohud = 1;
}

void Cmd_ToggleHud()
{
	if ( nohud )
		Hud_On();
	else
		Hud_Off();
}
