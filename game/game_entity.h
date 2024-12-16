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

class Entity : public edict_t
{
public:
	Entity()
	{
		gravity      = 1.0f;
		s.number     = static_cast< int >( g_edicts.size() );
		org_movetype = -1;
		inuse        = true;
	}

	~Entity() override
	{
		gi.unlinkentity( this );

		edict_t   *ed;
		const auto i = std::find_if( g_edicts.begin(), g_edicts.end(),
		                             [ &ed ]( const std::unique_ptr< edict_s > &ptr )
		                             {
			                             return ptr.get() == ed;
		                             } );
		if ( i != g_edicts.end() )
		{
			g_edicts.erase( i );
		}
	}

	virtual const char *GetClassName() { return "Entity"; }

	virtual void Spawn() {}
};

#define IMPLEMENT_SUPER( PARENT ) typedef PARENT Super;
#define IMPLEMENT_ENTITY( BASE, PARENT )                  \
	IMPLEMENT_SUPER( PARENT )                             \
public:                                                   \
	const char *GetClassName() override { return #BASE; } \
                                                          \
private:
