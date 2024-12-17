// SPDX-License-Identifier: GPL-2.0-or-later
#include "../g_local.h"

#include "entity.h"

Entity::Entity( edict_t *edict ) : edict( edict )
{
}

void Entity::SetModel( const std::string &path ) const
{
	gi.setmodel( edict, path.c_str() );
}

void Entity::SetSolid( const solid_t solid ) const
{
	edict->solid = solid;
}

void Entity::Link() const
{
	gi.linkentity( edict );
}
