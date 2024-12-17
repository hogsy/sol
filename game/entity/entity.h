// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "entity_manager.h"

#define IMPLEMENT_SUPER( PARENT ) typedef PARENT Super;
#define IMPLEMENT_ENTITY( BASE, PARENT )                  \
	IMPLEMENT_SUPER( PARENT )                             \
public:                                                   \
	const char *GetClassName() override { return #BASE; } \
                                                          \
private:

class Entity
{
public:
	explicit Entity( edict_t *edict );
	virtual ~Entity() = default;

	/**
	 * Returns the internal classname (not to be confused with edict classname!)
	 */
	virtual const char *GetClassName() = 0;

	virtual void Spawn( const EntityManager::SpawnVariables &variables ) = 0;

	void SetModel( const std::string &path ) const;
	void SetSolid( solid_t solid ) const;

	void SetOrigin( const vec3_t &origin ) const;
	void SetAngles( const vec3_t &angles ) const;

	void SetSize( const vec3_t &mins, const vec3_t &maxs ) const;

	void Link() const;
	void Unlink() const;

protected:
	edict_t *edict{};

public:
	[[nodiscard]] edict_t *GetEdict() const
	{
		return edict;
	}
};
