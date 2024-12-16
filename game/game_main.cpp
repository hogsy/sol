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

#include "g_local.h"

game_import_t gi;
game_export_t globals;

std::vector< std::unique_ptr< edict_t > >          g_edicts;
static std::vector< std::unique_ptr< gclient_t > > g_clients;

static void init_game()
{
}

static void shutdown_game()
{
}

static char *parse_entity_vars( char *data, EntityManager::SpawnVariableRegistry &vars )
{
	// go through all the dictionary pairs
	while ( true )
	{
		char keyname[ 256 ];
		// parse key
		char *com_token = COM_Parse( &data );
		if ( com_token[ 0 ] == '}' )
		{
			break;
		}

		if ( !data )
		{
			gi.error( "ED_ParseEntity: EOF without closing brace" );
		}

		Q_strncpyz( keyname, sizeof( keyname ), com_token );

		// parse value
		com_token = COM_Parse( &data );
		if ( !data )
		{
			gi.error( "ED_ParseEntity: EOF without closing brace" );
		}

		if ( com_token[ 0 ] == '}' )
		{
			gi.error( "ED_ParseEntity: closing brace without data" );
		}

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if ( keyname[ 0 ] == '_' )
		{
			continue;
		}

		EntityManager::SpawnVariable var = { keyname, com_token };
		vars.emplace( keyname, var );
	}

	return data;
}

static void spawn_entities( const char *mapname, char *entstring, const char *spawnpoint )
{
	gi.FreeTags( TAG_LEVEL );

	while ( true )
	{
		const char *token = COM_Parse( &entstring );
		if ( entstring == nullptr )
		{
			break;
		}

		if ( *token != '{' )
		{
			gi.error( "ED_LoadFromFile: found %s when expecting {", token );
		}

		EntityManager::SpawnVariableRegistry vars;
		entstring = parse_entity_vars( entstring, vars );

		auto i = vars.find( "classname" );
		if ( i == vars.end() )
		{
			gi.dprintf( "Encountered an entity with no classname!\n" );
			continue;
		}

		edict_s *entity = EntityManager::CreateEntity( i->second.value );
		if ( entity == nullptr )
		{
			continue;
		}

		entity->s.renderfx |= RF_IR_VISIBLE;// ir goggles flag
	}
}

static void write_game( char *filename, qboolean autosave )
{
}

static void read_game( char *filename )
{
}

static void write_level( char *filename )
{
}

static void read_level( char *filename )
{
}

static void client_think( edict_t *ent, usercmd_t *cmd )
{
}

static qboolean client_connect( edict_t *ent, char *userinfo )
{
	return true;
}

static void client_user_info_changed( edict_t *ent, char *userinfo )
{
}

static void client_disconnect( edict_t *ent )
{
}

static void client_begin( edict_t *ent )
{
}

static void client_command( edict_t *ent )
{
}

static void run_frame()
{
}

static void server_command()
{
}

game_export_t *GetGameAPI( const game_import_t *import )
{
	gi = *import;

	globals.apiversion    = GAME_API_VERSION;
	globals.Init          = init_game;
	globals.Shutdown      = shutdown_game;
	globals.SpawnEntities = spawn_entities;

	globals.WriteGame  = write_game;
	globals.ReadGame   = read_game;
	globals.WriteLevel = write_level;
	globals.ReadLevel  = read_level;

	globals.ClientThink           = client_think;
	globals.ClientConnect         = client_connect;
	globals.ClientUserinfoChanged = client_user_info_changed;
	globals.ClientDisconnect      = client_disconnect;
	globals.ClientBegin           = client_begin;
	globals.ClientCommand         = client_command;

	globals.RunFrame = run_frame;

	globals.ServerCommand = server_command;

	return &globals;
}
