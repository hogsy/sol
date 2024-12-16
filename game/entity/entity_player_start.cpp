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
}

REGISTER_ENTITY_CLASS( info_player_start, PlayerStart )
