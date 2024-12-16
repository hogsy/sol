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

#include "../g_local.h"
#include "../game_entity_manager.h"
#include "../game_entity.h"

class Light : public Entity
{
	IMPLEMENT_ENTITY( Light, Entity )

	static constexpr unsigned int START_OFF = 1 << 0;

	static void DestroyLight( edict_t *self )
	{
		auto *light = dynamic_cast< Light * >( self );
		assert( light != nullptr );
		delete light;
	}

	static void UseLight( edict_t *self, edict_t *, edict_t * )
	{
		auto *light = dynamic_cast< Light * >( self );
		assert( light != nullptr );

		if ( light->spawnflags & START_OFF )
		{
			gi.configstring( CS_LIGHTS + light->style, "m" );
			light->spawnflags &= ~START_OFF;
			return;
		}

		gi.configstring( CS_LIGHTS + light->style, "a" );
		light->spawnflags |= START_OFF;

		light->count--;
		if ( light->count <= 0 )
		{
			light->think = DestroyLight;
			//TODO
			//light->nextthink = level.time + 1;
		}
	}

public:
	Light() = default;

	void Spawn() override
	{
		if ( targetname == nullptr )
		{
			delete this;
			return;
		}

		class_id = ENTITY_LIGHT;

		if ( style >= 32 )
		{
			use = UseLight;

			if ( spawnflags & START_OFF )
			{
				gi.configstring( CS_LIGHTS + style, "a" );
			}
			else
			{
				gi.configstring( CS_LIGHTS + style, "m" );
			}
		}
	}
};

REGISTER_ENTITY_CLASS( light, Light )
