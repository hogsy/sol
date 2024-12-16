// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <map>
#include <memory>

class Entity;
class EntityManager
{
protected:
	typedef Entity *( *constructor )( edict_t *edict );
	static std::map< std::string, constructor > classRegistry;

public:
	class ClassRegistration
	{
	public:
		ClassRegistration( const std::string &name, constructor constructor ) : name( name )
		{
			classRegistry[ name ] = constructor;
		}
		ClassRegistration() = delete;

	private:
		std::string name;
	};

	struct SpawnVariable
	{
		std::string key;
		std::string value;
	};
	typedef std::map< std::string, SpawnVariable > SpawnVariables;

	static bool ParseSpawnVariables( char *buf, SpawnVariables &variables );

	static EntityManager *GetInstance()
	{
		static EntityManager instance;
		return &instance;
	}

	static Entity *CreateEntity( edict_t *edict, const std::string &classname );
	static void    SpawnEntity( edict_t *edict, const std::string &classname, const SpawnVariables &variables );

private:
	EntityManager()  = default;
	~EntityManager() = default;
};

#define REGISTER_ENTITY_CLASS( NAME, CLASS )                                           \
	static Entity *NAME##_make( edict_t *edict ) { return new CLASS( edict ); }        \
	static EntityManager::ClassRegistration __attribute__( ( init_priority( 2000 ) ) ) \
	_register_entity_##NAME##_name( #NAME, NAME##_make );
