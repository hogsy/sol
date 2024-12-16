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

	virtual const char *GetClassName() = 0;

	virtual void Spawn( const EntityManager::SpawnVariables &variables ) = 0;

private:
	edict_t *edict{};

public:
	[[nodiscard]] edict_t *GetEdict() const
	{
		return edict;
	}
};
