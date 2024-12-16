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
#include "../qcommon/qcommon.h"

#include <memory>

game_locals_t	game;
level_locals_t	level;
spawn_temp_t	st;

int	sm_meat_index;
int	snd_fry;
int meansOfDeath;

cvar_t	*deathmatch;
cvar_t	*coop;
cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
//ZOID
cvar_t	*capturelimit;
cvar_t	*instantweap;
//ZOID
cvar_t	*password;
cvar_t	*spectator_password;
cvar_t	*needpass;
cvar_t	*maxspectators;
cvar_t	*maxentities;
cvar_t	*g_select_empty;

cvar_t	*filterban;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

cvar_t	*sv_rollspeed;
cvar_t	*sv_rollangle;
cvar_t	*gun_x;
cvar_t	*gun_y;
cvar_t	*gun_z;

cvar_t	*run_pitch;
cvar_t	*run_roll;
cvar_t	*bob_up;
cvar_t	*bob_pitch;
cvar_t	*bob_roll;

cvar_t	*sv_cheats;

cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;

cvar_t	*sv_maplist;

cvar_t	*actorchicken;
cvar_t	*actorjump;
cvar_t	*actorscram;
cvar_t	*alert_sounds;
cvar_t	*allow_fog;			// Set to 0 for no fog

// set to 0 to bypass target_changelevel clear inventory flag
// because some user maps have this erroneously set
cvar_t	*allow_clear_inventory;

cvar_t	*bounce_bounce;
cvar_t	*bounce_minv;
cvar_t	*cl_thirdperson; // Knightmare added
cvar_t	*corpse_fade;
cvar_t	*corpse_fadetime;
cvar_t	*footstep_sounds;
cvar_t	*gl_clear;
cvar_t	*gl_driver_fog;
cvar_t	*jetpack_weenie;
cvar_t	*joy_pitchsensitivity;
cvar_t	*joy_yawsensitivity;
cvar_t	*jump_kick;
cvar_t	*lazarus_cd_loop;
cvar_t	*lazarus_cl_gun;
cvar_t	*lazarus_crosshair;
cvar_t	*lazarus_r_clear;
cvar_t	*lazarus_joyp;
cvar_t	*lazarus_joyy;
cvar_t	*lazarus_pitch;
cvar_t	*lazarus_yaw;
cvar_t	*lights;
cvar_t	*lightsmin;
cvar_t	*monsterjump;
cvar_t	*readout;
cvar_t	*rocket_strafe;
cvar_t	*rotate_distance;
cvar_t	*shift_distance;
cvar_t	*sv_maxgibs;
cvar_t	*turn_rider;
cvar_t	*zoomrate;
cvar_t	*zoomsnap;

cvar_t	*sv_stopspeed;	//PGM	 (this was a define in g_phys.c)
cvar_t	*sv_step_fraction;	// Knightmare- this was a define in p_view.c

cvar_t	*g_aimfix;				// Knightmare- from Yamagi Q2
cvar_t	*g_aimfix_min_dist;		// Knightmare- minimum range for aimfix
cvar_t	*g_aimfix_taper_dist;	// Knightmare- transition range for aimfix
cvar_t	*g_nm_maphacks;			// Knightmare- enables hacks for Neil Manke's Q2 maps

cvar_t	*g_showlogic;			// Knightmare added

// Knightmare- simulated pause for deathmatch
qboolean	paused;

void SpawnEntities ( const char *mapname, char *entities, const char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void RunEntity (edict_t *ent);
void WriteGame (char *filename, qboolean autosave);
void ReadGame (char *filename);
void WriteLevel (char *filename);
void ReadLevel (char *filename);
void InitGame (void);
void G_RunFrame (void);

//===================================================================

void ShutdownGame (void)
{
	gi.dprintf ("==== ShutdownGame ====\n");
	if (!deathmatch->value && !coop->value) {
#ifndef KMQUAKE2_ENGINE_MOD // engine has zoom autosensitivity
		gi.cvar_forceset("m_pitch", va("%f",lazarus_pitch->value));
#endif
	//	gi.cvar_forceset("cd_loopcount", va("%d", lazarus_cd_loop->value));
	//	gi.cvar_forceset("gl_clear", va("%d", lazarus_r_clear->value));
	}

#if 0//TODO
	// Lazarus: Turn off fog if it's on
	if (!dedicated->value) {
	//	Fog_Off (true);
		Fog_Off_Global ();
	}
#endif

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}


game_import_t RealFunc;
int	max_modelindex;
int	max_soundindex;


int Debug_Modelindex (const char *name)
{
	int	modelnum;
	modelnum = RealFunc.modelindex(name);
	if (modelnum > max_modelindex)
	{
		gi.dprintf("Model %03d %s\n",modelnum,name);
		max_modelindex = modelnum;
	}
	return modelnum;
}

int Debug_Soundindex (const char *name)
{
	int soundnum;
	soundnum = RealFunc.soundindex(name);
	if (soundnum > max_soundindex)
	{
		gi.dprintf("Sound %03d %s\n",soundnum,name);
		max_soundindex = soundnum;
	}
	return soundnum;
}

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t *GetGameAPI ( const game_import_t *import)
{
	gi = *import;

	gl_driver = gi.cvar ("gl_driver", "", 0);
	vid_ref = gi.cvar ("vid_ref", "", 0);
	gl_driver_fog = gi.cvar ("gl_driver_fog", "opengl32", CVAR_NOSET | CVAR_ARCHIVE);

	Fog_Init();

	developer = gi.cvar("developer", "0", CVAR_SERVERINFO);
	readout   = gi.cvar("readout", "0", CVAR_SERVERINFO);
	if (readout->value)
	{
		max_modelindex = 0;
		max_soundindex = 0;
		RealFunc.modelindex = gi.modelindex;
		gi.modelindex       = Debug_Modelindex;
		RealFunc.soundindex = gi.soundindex;
		gi.soundindex       = Debug_Soundindex;
	}

	return &globals;
}

/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	// calc the player views now that all pushing
	// and damage has been added
	for ( int i = 0 ; i<maxclients->value ; i++)
	{
		edict_t *ent = g_edicts[ 1 + i ].get();
		if (!ent->inuse || !ent->client)
		{
			continue;
		}

		ClientEndServerFrame (ent);
	}

	//reflection stuff -- modified from psychospaz' original code
	if ( level.num_reflectors )
	{
		for ( auto &ent : g_edicts )
		{
			if ( !ent->inuse )
				continue;
			if ( !ent->s.modelindex )
				continue;
			//	if (ent->s.effects & EF_ROTATE)
			//		continue;
			if ( ent->flags & FL_REFLECT )
				continue;
			if ( !ent->client && ( ent->svflags & SVF_NOCLIENT ) )
				continue;
			if ( ent->client && !ent->client->chasetoggle && ( ent->svflags & SVF_NOCLIENT ) )
				continue;
			if ( ent->svflags & SVF_MONSTER && ent->solid != SOLID_BBOX )
				continue;
			if ( ( ent->solid == SOLID_BSP ) && ( ent->movetype != MOVETYPE_PUSHABLE ) )
				continue;
			if ( ent->client && ( ent->client->resp.spectator || ent->health <= 0 || ent->deadflag == DEAD_DEAD ) )
				continue;

			AddReflection( ent.get() );
		}
	}
}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
static edict_t *CreateTargetChangeLevel( char *map )
{
	edict_t *ent = EntityManager::CreateEntity( "target_changelevel" );
	assert( ent != nullptr );
	ent->classname = "target_changelevel";
	snprintf( level.nextmap, sizeof( level.nextmap ), "%s", map );
	ent->map = level.nextmap;

	return ent;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel (void)
{
	static const char *seps = " ,\n\r";

	// stay on same level flag
	if ((int)dmflags->value & DF_SAME_LEVEL)
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}

	// see if it's in the map list
	if (*sv_maplist->string)
	{
		char *s = strdup( sv_maplist->string );
		char *f = nullptr;
		char *t = strtok( s, seps );
		while (t != nullptr )
		{
			if (Q_stricmp( t, level.mapname ) == 0)
			{
				// it's in the list, go to the next one
				t = strtok( nullptr, seps);
				if (t == nullptr ) { // end of list, go to first one
					if (f == nullptr ) // there isn't a first one, same level
						BeginIntermission (CreateTargetChangeLevel (level.mapname) );
					else
						BeginIntermission (CreateTargetChangeLevel (f) );
				}
				else
					BeginIntermission (CreateTargetChangeLevel (t) );
				free(s);
				return;
			}
			if (!f)
				f = t;
			t = strtok( nullptr, seps);
		}
		free(s);
	}

	if (level.nextmap[0]) // go to a specific map
		BeginIntermission (CreateTargetChangeLevel (level.nextmap) );
	else
	{
		// search for a changelevel
		edict_t *ent = G_Find( nullptr, FOFS( classname ), "target_changelevel" );
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			return;
		}
		BeginIntermission (ent);
	}
}


/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass ()
{
	int need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified)
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp( password->string, "none" ) )
			need |= 1;
		if (*spectator_password->string && Q_stricmp( spectator_password->string, "none" ) )
			need |= 2;

		gi.cvar_set("needpass", va("%d", need));
	}
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules()
{
	if (level.intermissiontime)
		return;

	if (!deathmatch->value)
		return;

	if (timelimit->value)
	{
		if (level.time >= timelimit->value*60)
		{
			safe_bprintf ( PRINT_HIGH, "Timelimit hit.\n" );
			EndDMLevel ();
			return;
		}
	}

	if (fraglimit->value )
	{
		for ( int i = 0 ; i<maxclients->value ; i++)
		{
			const gclient_t *cl = game.clients + i;
			if (!g_edicts[i+1].get()->inuse)
			{
				continue;
			}

			if (cl->resp.score >= fraglimit->value)
			{
				safe_bprintf ( PRINT_HIGH, "Fraglimit hit.\n" );
				EndDMLevel ();
				return;
			}
		}
	}
}

/*
=============
ExitLevel
=============
*/
void ExitLevel ()
{
	std::string command = "gamemap \"" + std::string( level.changemap ) + "\"\n";
	gi.AddCommandString( command.data() );

	level.changemap = nullptr;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames ();

	// clear some things before going to next level
	for ( int i = 0; i < static_cast< int >( maxclients->value ); i++ )
	{
		edict_t *ent = g_edicts[ 1 + i ].get();
		if ( !ent->inuse )
		{
			continue;
		}

		if ( ent->health > ent->client->pers.max_health )
		{
			ent->health = ent->client->pers.max_health;
		}
	}

	// mxd added
	gibsthisframe = 0;
	lastgibframe = 0;
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
void G_RunFrame ()
{
	// Knightmare- dm pause
	if (paused && deathmatch->value)
		return;

	if (level.freeze)
	{
		level.freezeframes++;
		if (level.freezeframes >= sk_stasis_time->value*10)
			level.freeze = false;
	}
	else
	{
		level.framenum++;
	}

	level.time = level.framenum*FRAMETIME;

	// choose a client for monsters to target this frame
	AI_SetSightClient ();

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	if ( use_techs->value || (ctf->value && !((int)dmflags->value & DF_CTF_NO_TECH)) )
		CheckNumTechs ();

	//
	// treat each object in turn
	// even the world gets a chance to think
	//

	for ( auto &ent : g_edicts )
	{
		if ( !ent->inuse )
		{
			continue;
		}

		level.current_entity = ent.get();

		VectorCopy( ent->s.origin, ent->s.old_origin );

		// if the ground entity moved, make sure we are still on it
		if ( ent->groundentity && ent->groundentity->linkcount != ent->groundentity_linkcount )
		{
			ent->groundentity = nullptr;
			if ( !( ent->flags & ( FL_SWIM | FL_FLY ) ) && ent->svflags & SVF_MONSTER )
			{
				M_CheckGround( ent.get() );
			}
		}

		if ( ent->client != nullptr )
		{
			ClientBeginServerFrame( ent.get() );
			// ACEBOT_ADD
			if ( !ent->is_bot )// Bots need G_RunEntity called
			{
				continue;
			}
			// ACEBOT_END
		}

		G_RunEntity( ent.get() );
	}

	// see if it is time to end a deathmatch
	CheckDMRules ();

	// see if needpass needs updated
	CheckNeedPass ();

	// build the playerstate_t structures for all players
	ClientEndServerFrames ();

}

