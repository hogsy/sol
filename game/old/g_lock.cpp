/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2000-2002 Mr. Hyde and Mad Dog

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

#include "g_local.h"

#define KEY_MESSAGE_SIZE	10

//=====================================================

void SP_target_lock_digit (edict_t *self)
{
	self->class_id = ENTITY_TARGET_LOCK_DIGIT;

	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);
	self->solid = SOLID_BSP;
	self->s.frame = 12;
	gi.linkentity (self);
}


void target_lock_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *e;
	int		n;
	char    current[16];

	memset(current,0,16);

	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;
		n = e->count - 1;
		current[n] = '0' + e->s.frame;
	}
	if (strcmp(current, self->key_message)==0)
	{
		const std::string copy_message = self->message;
		self->message = "";
		G_UseTargets(self,activator);
		self->message = copy_message;
	}
	else
	{
		if (!self->message.empty()) safe_centerprintf( activator, "%s", self->message.c_str() );
		if (self->pathtarget) {
			e = G_Find(nullptr,FOFS(targetname),self->pathtarget);
			if (e) e->use(e,other,activator);
		}
		else {
			BeepBeep(activator);
		}
	}
}

void lock_digit_increment (edict_t *digit, edict_t *activator)
{
	if (digit->s.frame == 9)
		digit->s.frame = 0;
	else
		digit->s.frame++;
}

void lock_initialize (edict_t *lock)
{
	edict_t *e;
	int		n, l;
	int		numdigits;
	char	c;
	size_t	msgSize;

	if (lock->spawnflags & 1 && strlen(game.lock_code) != 0)
	{
		// Knightmare- dynamically allocate this to be safe
		// This will temporarily leak the already allocated key_message
		// buffer, but that will be freed on the next level change.
	//	strncpy(lock->key_message, game.lock_code);
		msgSize = KEY_MESSAGE_SIZE;
		lock->key_message = static_cast< char * >( gi.TagMalloc( msgSize, TAG_LEVEL ) );
		Q_strncpyz (lock->key_message, msgSize, game.lock_code);
		return;
	}
	// Maximum of 8 digits in combination
	l = min((int)strlen(lock->key_message),8);
	numdigits = 0;
	for (e = lock->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;
		numdigits = max(numdigits,e->count);
		n = e->count - 1;
		if (n > l)
		{
			e->s.frame = 12;
			continue;
		}

		c = lock->key_message[n];
		if (c >= '0' && c <= '9')
			e->s.frame = c - '0';
		else
			e->s.frame = 0;
	}
	// Now generate a random number as the lock combination
	srand(time(NULL));
	n = random();
	n = random();
	n = random();
	for (n=0; n<numdigits; n++)
		lock->key_message[n] = '0' + (int)(random() * 9.99);
	lock->key_message[numdigits] = '\0';

//	strncpy(game.lock_code, lock->key_message);
	Q_strncpyz(game.lock_code, sizeof(game.lock_code), lock->key_message);
	game.lock_revealed = 0;
}

void SP_target_lock (edict_t *self)
{
	if (game.lock_hud && (self->spawnflags & 2))
	{
		gi.dprintf("Map contains multiple target_locks and HUD is set.\n");
		G_FreeEdict(self);
		return;
	}

	self->class_id = ENTITY_TARGET_LOCK;

	if (self->spawnflags & 2) game.lock_hud = true;
	if (!self->key_message) {
		// Knightmare- dynamically allocate this to be safe
	//	self->key_message = "000000000";
		self->key_message = static_cast<char*>( gi.TagMalloc(KEY_MESSAGE_SIZE, TAG_LEVEL) );
		Q_strncpyz (self->key_message, KEY_MESSAGE_SIZE, "000000000");
	}
	self->use = target_lock_use;
	self->think = lock_initialize;
	self->nextthink = level.time + 1.0;
	gi.linkentity(self);
}

//
// target_lock_code reveals the lock combination to the 
// target_lock specified in it's target field, or in global
// game data for crosslevel locks
//
void lock_code_use (edict_t *self, edict_t *other, edict_t *activator)
{
	int		i, L;
	char	message[64];
	if (self->spawnflags & 1)
	{
		if (!strlen(game.lock_code))
		{
			gi.dprintf("Lock has not been properly initialized.\n");
			return;
		}
		snprintf (message, sizeof(message), "Lock combination is %s",game.lock_code);
		Do_Text_Display(activator, 0, message);
		L = (int)strlen(game.lock_code);
		for (i=0; i<L; i++)
			game.lock_revealed |= 1<<i;
	}
	else
	{
		edict_t *lock;
		lock = G_Find(NULL,FOFS(targetname),self->target);
		if (!lock)
			gi.dprintf("Target of target_lock_code does not exist\n");
		else
		{
			snprintf (message, sizeof(message), "Lock combination is %s",game.lock_code);
			Do_Text_Display(activator, 0, message);
			L = min(8, (int)strlen(lock->key_message));
			for (i=0; i<L; i++)
				game.lock_revealed |= 1<<i;
		}
	}
}

void SP_target_lock_code (edict_t *self)
{
	self->class_id = ENTITY_TARGET_LOCK_CODE;

	if (!self->target && !(self->spawnflags & 1))
	{
		gi.dprintf("non-crosslevel target_lock_code w/o target\n");
		G_FreeEdict(self);
	}
	self->use = lock_code_use;
}

void lock_clue_use (edict_t *self, edict_t *other, edict_t *activator)
{
	int		i, L;
	if (self->spawnflags & 1)
	{
		if (!strlen(game.lock_code))
		{
			gi.dprintf("Lock has not been properly initialized.\n");
			return;
		}
		L = (int)strlen(game.lock_code);
		for (i=0; i<L; i++)
			if (self->message[i] != '?') game.lock_revealed |= 1<<i;
	}
	else
	{
		edict_t *lock;
		lock = G_Find(NULL,FOFS(targetname),self->target);
		if (!lock)
			gi.dprintf("Target of target_lock_clue does not exist\n");
		else
		{
			L = min(8, (int)strlen(lock->key_message));
			for (i=0; i<L; i++)
				if (self->message[i] != '?') game.lock_revealed |= 1<<i;
		}
	}
}

void lock_clue_think(edict_t *self)
{
	int		n;
	int		unrevealed_count;
	edict_t *e;

	if (!self->team)
		return;

	unrevealed_count = 0;
	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;
		n = e->count - 1;
		if (game.lock_revealed & 1<<n)
			e->s.frame = game.lock_code[n] - '0';
		else
		{
			e->s.frame++; // spin unknown digits
			if (e->s.frame > 9) e->s.frame = 0;
			unrevealed_count++;
		}
	}
	if (unrevealed_count)
	{
		self->nextthink = level.time + FRAMETIME;
		gi.linkentity(self);
	}
}
void lock_clue_initialize(edict_t *self)
{
	// Randomize the revealed digits so spinning display doesn't
	// show the same numbers across the board.

	edict_t *e;
	if (self->team)
	{
		for (e = self->teammaster; e; e = e->teamchain)
		{
			if (!e->count)
				continue;
			e->s.frame = (int)(random() * 9.99);
		}
	}
	self->think = lock_clue_think;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}
void SP_target_lock_clue (edict_t *self)
{
	self->class_id = ENTITY_TARGET_LOCK_CLUE;

	if (!self->target && !(self->spawnflags & 1))
	{
		gi.dprintf("non-crosslevel target_lock_clue w/o target\n");
		G_FreeEdict(self);
	}
	self->use = lock_clue_use;
	self->think = lock_clue_initialize;
	self->nextthink = level.time + 2*FRAMETIME;
	gi.linkentity(self);
}
