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

void Entity::SetOrigin( const vec3_t &origin ) const
{
	VectorCopy( edict->s.origin, edict->s.old_origin );
	VectorCopy( origin, edict->s.origin );
}

void Entity::SetAngles( const vec3_t &angles ) const
{
	VectorCopy( angles, edict->s.angles );
}

void Entity::SetSize( const vec3_t &mins, const vec3_t &maxs ) const
{
	VectorCopy( mins, edict->mins );
	VectorCopy( maxs, edict->maxs );
}

void Entity::Link() const
{
	gi.linkentity( edict );
}

void Entity::Unlink() const
{
	gi.unlinkentity( edict );
}
