// SPDX-License-Identifier: GPL-2.0-or-later
#include "../g_local.h"

#include "entity_manager.h"
#include "entity.h"

class PlayerStart : public Entity
{
	IMPLEMENT_ENTITY( PlayerStart, Entity )

public:
	explicit PlayerStart( edict_t *edict ) : Entity( edict ) {}
	~PlayerStart() override = default;

	void Spawn( const EntityManager::SpawnVariables &variables ) override;
};

void PlayerStart::Spawn( const EntityManager::SpawnVariables &variables )
{
	const std::string &classname = variables.at( "classname" ).value;
	if ( classname == "info_player_coop" )
	{
		if ( coop->value == 0.0f )
		{
			G_FreeEdict( edict );
			return;
		}

		edict->class_id = ENTITY_INFO_PLAYER_COOP;
	}
	else if ( classname == "info_player_deathmatch" )
	{
		if ( deathmatch->value == 0.0f )
		{
			G_FreeEdict( edict );
			return;
		}

		edict->class_id = ENTITY_INFO_PLAYER_DEATHMATCH;
	}
	else if ( classname == "info_player_intermission" )
	{
		edict->class_id = ENTITY_INFO_PLAYER_INTERMISSION;
	}
	else
	{
		edict->class_id = ENTITY_INFO_PLAYER_START;
	}
}

REGISTER_ENTITY_CLASS( info_player_start, PlayerStart )
REGISTER_ENTITY_CLASS( info_player_coop, PlayerStart )
REGISTER_ENTITY_CLASS( info_player_deathmatch, PlayerStart )
REGISTER_ENTITY_CLASS( info_player_intermission, PlayerStart )
