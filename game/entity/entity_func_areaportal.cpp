/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2000-2002 Mr. Hyde and Mad Dog
Copyright (C) 2024 Mark E. Sowden <hogsy@oldtimes-software.com>

This file is part of Lazarus Quake 2 Mod source code.

Lazarus Quake 2 Mod source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Lazarus Quake 2 Mod source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lazarus Quake 2 Mod source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../g_local.h"
#include "../game_entity_manager.h"
#include "../game_entity.h"

class AreaPortal : public Entity
{
	IMPLEMENT_ENTITY( AreaPortal, Entity )

public:
	AreaPortal();

	void Spawn() override
	{
		class_id = ENTITY_FUNC_AREAPORTAL;
		count    = 0;
	}
};

REGISTER_ENTITY_CLASS( func_areaportal, AreaPortal )
