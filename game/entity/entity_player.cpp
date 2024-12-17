// SPDX-License-Identifier: GPL-2.0-or-later
#include "../g_local.h"

#include "entity_player.h"

void Player::Spawn( const EntityManager::SpawnVariables &variables )
{
	// a little strange for this one, but the variables will always be empty for a player
	assert( edict->client != nullptr );

	edict->groundentity = nullptr;
	edict->takedamage   = DAMAGE_AIM;
	edict->movetype     = MOVETYPE_WALK;
	edict->viewheight   = PLAYER_VIEW_HEIGHT;
	edict->inuse        = true;
	edict->classname    = "player";
	edict->mass         = 200;
	edict->solid        = SOLID_BBOX;
	edict->deadflag     = DEAD_NO;
	edict->air_finished = level.time + 12;
	edict->clipmask     = MASK_PLAYERSOLID;
	edict->model        = "players/male/tris.md2";
	edict->pain         = player_pain;
	edict->die          = player_die;
	edict->waterlevel   = 0;
	edict->watertype    = 0;
	edict->flags &= ~FL_NO_KNOCKBACK;
	edict->svflags &= ~SVF_DEADMONSTER;
	edict->svflags &= ~SVF_NOCLIENT;
	edict->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	edict->client->spycam    = nullptr;
	edict->client->camplayer = nullptr;

	SetSize( PLAYER_MINS, PLAYER_MAXS );

	SelectSpawnPoint();

	VectorClear( edict->velocity );
}

void Player::OnDisconnect() const
{
	gclient_t *client = GetClient();
	if ( client == nullptr )
	{
		return;
	}

	safe_bprintf( PRINT_HIGH, "%s disconnected\n", client->pers.netname );

	// send effect
	gi.WriteByte( svc_muzzleflash );
	gi.WriteShort( edict - g_edicts );
	gi.WriteByte( MZ_LOGOUT );
	gi.multicast( edict->s.origin, MULTICAST_PVS );

	SetSolid( SOLID_NOT );

	edict->s.modelindex           = 0;
	edict->inuse                  = false;
	edict->classname              = "disconnected";
	edict->client->pers.connected = false;

	Unlink();
}

void Player::SelectSpawnPoint() const
{
	edict_t *spot = G_Find( nullptr, FOFS( classname ), "info_player_start" );
	if ( spot == nullptr )
	{
		gi.dprintf( "Failed to find spawn point for player!\n" );
		return;
	}

	if ( spot->health > 0 )
	{
		edict->health = spot->health;
	}

	SetOrigin( { spot->s.origin[ 0 ], spot->s.origin[ 1 ], spot->s.origin[ 2 ] + 1.0f } );
	SetAngles( spot->s.angles );

	if ( deathmatch->value == 0.0f && coop->value == 0.0f )
	{
		spot->count--;
		if ( spot->count <= 0 )
		{
			spot->think     = G_FreeEdict;
			spot->nextthink = level.time + 1.0f;
		}
	}
}

REGISTER_ENTITY_CLASS( player, Player )
