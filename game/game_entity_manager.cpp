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
#include "game_entity_manager.h"
#include "game_entity.h"

std::map< std::string, EntityManager::EntityConstructor > EntityManager::entityClassRegistry __attribute__( ( init_priority( 1000 ) ) );

EntityManager::EntityClassRegistration::EntityClassRegistration( const std::string &name, EntityConstructor constructor ) : name( name )
{
	entityClassRegistry[ name ] = constructor;
}

EntityManager::EntityClassRegistration::~EntityClassRegistration()
{
	entityClassRegistry.erase( name );
}

edict_s *EntityManager::CreateEntity( const std::string &classname )
{
	const auto spawn = entityClassRegistry.find( classname );
	if ( spawn == entityClassRegistry.end() )
	{
		gi.dprintf( "Unknown entity class \"%s\"!\n", classname.c_str() );
		return nullptr;
	}

	g_edicts.emplace_back( spawn->second() );
	return g_edicts.back().get();
}
