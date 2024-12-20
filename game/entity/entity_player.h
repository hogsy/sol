// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "entity.h"

class Player : public Entity
{
	IMPLEMENT_ENTITY( Player, Entity )

public:
	explicit Player( edict_t *edict ) : Entity( edict ) {}
	~Player() override = default;

	void Spawn( const EntityManager::SpawnVariables &variables ) override;

	void OnDisconnect() const;

private:
	void UpdateWeapon();
	void UpdateView();
	void UpdateStats();

public:
	void OnEndServerFrame();

	[[nodiscard]] gclient_t      *GetClient() const { return edict->client; }
	[[nodiscard]] player_state_t *GetPlayerState() const { return &edict->client->ps; }

private:
	void SelectSpawnPoint() const;

	static constexpr vec3_t PLAYER_MINS = { -16.0f, -16.0f, -24.0f };
	static constexpr vec3_t PLAYER_MAXS = { 16.0f, 16.0f, 32.0f };

	static constexpr float VIEW_HEIGHT     = 22.0f;
	static constexpr float VIEW_MIN_HEIGHT = 5.0f;
	static constexpr float VIEW_STIFFNESS  = 0.5f;
	static constexpr float VIEW_DAMPENING  = 0.8f;

	vec3_t oldViewAngles{};

	float bobCycle{};
	float springHeight{}, springVelocity{};
};
