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

class PlayerStart : public Entity
{
	IMPLEMENT_ENTITY( PlayerStart, Entity )

public:
	PlayerStart() = default;

	void Spawn() override
	{
		Super::Spawn();

		if ( classname == "info_player_deathmatch" )
		{
			class_id = ENTITY_INFO_PLAYER_DEATHMATCH;
		}
		else if ( classname == "info_player_coop" )
		{
			class_id = ENTITY_INFO_PLAYER_COOP;
		}
		else
		{
			class_id = ENTITY_INFO_PLAYER_START;
		}
	}
};

REGISTER_ENTITY_CLASS( info_player_start, PlayerStart )
REGISTER_ENTITY_CLASS( info_player_deathmatch, PlayerStart )
REGISTER_ENTITY_CLASS( info_player_coop, PlayerStart )