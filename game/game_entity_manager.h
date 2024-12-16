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

#pragma once

#include <map>

class Entity;
class EntityManager
{
protected:
	typedef std::unique_ptr< Entity > ( *EntityConstructor )();
	static std::map< std::string, EntityConstructor > entityClassRegistry;

public:
	class EntityClassRegistration
	{
	public:
		EntityClassRegistration( const std::string &name, EntityConstructor constructor );
		~EntityClassRegistration();

	private:
		std::string name;
	};

	struct SpawnVariable
	{
		std::string key;
		std::string value;
	};
	typedef std::map< std::string, SpawnVariable > SpawnVariableRegistry;

	static EntityManager *GetInstance()
	{
		static EntityManager instance;
		return &instance;
	}

	EntityManager();
	~EntityManager();

	static edict_s *CreateEntity( const std::string &classname );
};

#define REGISTER_ENTITY_CLASS( NAME, CLASS )                                                 \
	static std::unique_ptr< Entity > NAME##_make() { return std::make_unique< CLASS >(); }   \
	static EntityManager::EntityClassRegistration __attribute__( ( init_priority( 2000 ) ) ) \
	_register_entity_##NAME##_name( #NAME, NAME##_make );
