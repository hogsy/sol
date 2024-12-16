// SPDX-License-Identifier: GPL-2.0-or-later
#include "../g_local.h"

#include "entity_manager.h"
#include "entity.h"

std::map< std::string, EntityManager::constructor > EntityManager::classRegistry __attribute__( ( init_priority( 1000 ) ) );

bool EntityManager::ParseSpawnVariables( char *buf, SpawnVariables &variables )
{
	while ( true )
	{
		char *token = COM_Parse( &buf );
		if ( *token == '}' )
		{
			break;
		}

		if ( buf == nullptr )
		{
			gi.dprintf( "EOF without closing brace!\n" );
			return false;
		}

		std::string key = token;

		token = COM_Parse( &buf );
		if ( buf == nullptr )
		{
			gi.dprintf( "EOF without closing brace!\n" );
			return false;
		}

		if ( *token == '}' )
		{
			gi.dprintf( "Closing brace without data!\n" );
			return false;
		}

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if ( key[ 0 ] == '_' )
		{
			continue;
		}

		variables[ key ] = { key, token };
	}

	return !variables.empty();
}

Entity *EntityManager::CreateEntity( edict_t *edict, const std::string &classname )
{
	const auto spawn = classRegistry.find( classname );
	if ( spawn == classRegistry.end() )
	{
		gi.dprintf( "Unknown entity class \"%s\"!\n", classname.c_str() );
		return nullptr;
	}

	return spawn->second( edict );
}

void EntityManager::SpawnEntity( edict_t *edict, const std::string &classname, const SpawnVariables &variables )
{
	Entity *entity = CreateEntity( edict, classname );
	if ( entity == nullptr )
	{
		return;
	}

	entity->Spawn( variables );
}
