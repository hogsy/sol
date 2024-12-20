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
//
// mappack stuff by mr. ed, modified extensively for Tremor by dwh
//

//  Spawns a user defined model, you can specify whether its solid, if so how big the box is, and apply nearly
//  any effect to the entity.
//
//	PLAYER		set this if you want to use a player model
//
//	Note : These flags override any frame settings you may have enterered
//	ANIM01		cycle between frames 0 and 1 at 2 hz
//	ANIM23		cycle between frames 2 and 3 at 2 hz
//	ANIM_ALL		cycle through all frames at 2hz
//	ANIM_ALLFAST	cycle through all frames at 10hz
//
//	START_OFF		Start inactive, when triggered display the model
//	TOGGLE		Start active, when triggered become inactive
//	NO_MODEL		Don't use a model. Usefull for placeing particle effects and
//				dynamic lights on their own
//
//	"delay"      = Combine with TOGGLE spawnflag to start off
//	"usermodel"  = The model to load (models/ is already coded)
//	"startframe" = The starting frame : default 0
//	"framenumbers" = The number of frames you want to display after startframe
//	"spritetype"   = Set this to 1 to make sprites render as angled and vertical
//	"solidstate" = 1 : SOLID_NOT - not solid at all
//			       2 : SOLID_BBOX - solid and affected by gravity
//			       3 : NO DROP - solid but not affected by gravity
//                 4 : SOLID_NOT, but affect by gravity
//
//	NOTE : if you want the model to be solid then you must enter vector values into the following fields :
//	"bleft" = the point that is at the bottom left of the models bounding box in a model editor
//	"tright" = the point that is at the top left of the models bounding box in a model editor
//

#define	TOGGLE		    2
#define	PLAYER_MODEL	8
#define	NO_MODEL		16
#define ANIM_ONCE		32

void model_spawn_use (edict_t *self, edict_t *other, edict_t *activator);
void modelspawn_think (edict_t *self)
{
	self->s.frame++;
	if (self->s.frame >= self->framenumbers)
	{
		self->s.frame = self->startframe;
		if (self->spawnflags & ANIM_ONCE)
		{
			model_spawn_use (self, world, world);
			return;
		}
	}
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

void model_spawn_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->delay) // we started off
	{
		self->svflags &= ~SVF_NOCLIENT;
		self->delay = 0;
		if (self->framenumbers > 1)
		{
			self->think = modelspawn_think;
			self->nextthink = level.time + FRAMETIME;
		}
		self->s.sound = self->noise_index;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}
	else             // we started active
	{
		self->svflags |= SVF_NOCLIENT;
		self->delay = 1;
		self->use = model_spawn_use;
		self->think = NULL;
		self->nextthink = 0;
		self->s.sound = 0;
	}
}

void model_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t	*e, *next;

	e = self->movewith_next;
	while (e)
	{
		next = e->movewith_next;
		if (e->solid == SOLID_NOT) {
			e->nextthink = 0;
			G_FreeEdict(e);
		}
		else
			BecomeExplosion1 (e);
		e = next;
	}

	BecomeExplosion1(self);
}

#define ANIM_MASK (EF_ANIM01|EF_ANIM23|EF_ANIM_ALL|EF_ANIM_ALLFAST)

void SP_model_spawn (edict_t *ent)
{
	char	modelname[256];

	// paranoia checks
	if ((!ent->usermodel) && !(ent->spawnflags & NO_MODEL) && !(ent->spawnflags & PLAYER_MODEL))
	{
		gi.dprintf("%s without a model at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	ent->class_id = ENTITY_MODEL_SPAWN;

	switch (ent->solidstate)
	{
		case 1 : ent->solid = SOLID_NOT; ent->movetype = MOVETYPE_NONE; break;
		case 2 : ent->solid = SOLID_BBOX; ent->movetype = MOVETYPE_TOSS; break;
		case 3 : ent->solid = SOLID_BBOX; ent->movetype = MOVETYPE_NONE; break;
		case 4 : ent->solid = SOLID_NOT; ent->movetype = MOVETYPE_TOSS; break;
		default: ent->solid = SOLID_NOT; ent->movetype = MOVETYPE_NONE; break;
	}
	if (ent->solid != SOLID_NOT ) {
		if (ent->health > 0) {
			ent->die = model_die;
			ent->takedamage = DAMAGE_YES;
		}
	}

	switch (ent->style)
	{
		case 1 : ent->s.effects |= EF_ANIM01; break;
		case 2 : ent->s.effects |= EF_ANIM23; break;
		case 3 : ent->s.effects |= EF_ANIM_ALL; break;
		case 4 : ent->s.effects |= EF_ANIM_ALLFAST; break;
	}

	// DWH: Rather than use one value (renderfx) we use the
	//      actual values for effects and renderfx. All may
	//      be combined.
	ent->s.effects  |= ent->effects;
	ent->s.renderfx |= ent->renderfx;

	if (ent->startframe < 0)
		ent->startframe = 0;
	if (!ent->framenumbers)
		ent->framenumbers = 1;
	// Change framenumbers to last frame to play
	ent->framenumbers += ent->startframe;

	if (ent->bleft)
	{
		VectorCopy (ent->bleft, ent->mins);
	}
	else
	{
		if (ent->solid != SOLID_NOT)
		{
			gi.dprintf("%s solid with no bleft vector at %s\n", ent->classname, vtos(ent->s.origin));
			ent->solid = SOLID_NOT;
		}
	}

	if (ent->tright)
	{
		VectorCopy (ent->tright, ent->maxs);
	}
	else
	{
		if (ent->solid != SOLID_NOT)
		{
			gi.dprintf("%s solid with no tright vector at %s\n", ent->classname, vtos(ent->s.origin));
			ent->solid = SOLID_NOT;
		}
	}

//	if (ent->movewith && (ent->solid == SOLID_BBOX))
	if (ent->movewith)
		ent->movetype = MOVETYPE_PUSH;

	if (ent->solid != SOLID_NOT)
		ent->clipmask = CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER;

	if (ent->spawnflags & NO_MODEL)
	{	// For rendering effects to work, we MUST use a model
		ent->s.modelindex = gi.modelindex ("sprites/point.sp2");
		ent->movetype = MOVETYPE_NOCLIP;
	}
	else
	{
		if (ent->spawnflags & PLAYER_MODEL) {
			if (!ent->usermodel || !strlen(ent->usermodel))
				ent->s.modelindex = MAX_MODELS-1;
			else
			{
				if (strstr(ent->usermodel,"tris.md2"))
					snprintf (modelname, sizeof(modelname), "players/%s", ent->usermodel);
				else
					snprintf (modelname, sizeof(modelname), "players/%s/tris.md2", ent->usermodel);
				ent->s.modelindex = gi.modelindex(modelname);
			}
		}
		else
		{
			if (strstr(ent->usermodel, ".sp2")) {
				// Knightmare- check for "sprites/" already in path
				if ( !strncmp(ent->usermodel, "sprites/", 8) )
					snprintf (modelname, sizeof(modelname), "%s", ent->usermodel);
				else
					snprintf (modelname, sizeof(modelname), "sprites/%s", ent->usermodel);
			}
			else {
				// Knightmare- check for "models/" already in path
				if ( !strncmp(ent->usermodel, "models/", 7) )
					snprintf (modelname, sizeof(modelname), "%s", ent->usermodel);
				else
					snprintf (modelname, sizeof(modelname), "models/%s", ent->usermodel);
			}
			ent->s.modelindex = gi.modelindex (modelname);
		}
		ent->s.frame = ent->startframe;
	}

	// Knightmare- support for angled sprites
	if (st.spritetype == 1)
		ent->s.renderfx |= RF_SPRITE_ORIENTED;

	if (st.noise)
		ent->noise_index = gi.soundindex (st.noise);
	ent->s.sound = ent->noise_index;
#ifdef KMQUAKE2_ENGINE_MOD
	ent->s.loop_attenuation = ent->attenuation;
#endif

	if (ent->skinnum)		// Knightmare- selectable skin
		ent->s.skinnum = ent->skinnum;

	if (ent->spawnflags & ANIM_ONCE) {
		ent->spawnflags |= TOGGLE;
	}

	if (ent->spawnflags & TOGGLE)
	{	// Knightmare- allow starting off (but not for model_train)
		if ( (strcmp(ent->classname.c_str(), "model_train") != 0) && (ent->delay != 0) ) {
			ent->delay = 1;
			ent->svflags |= SVF_NOCLIENT;
		}
		else {
			ent->delay = 0;
		}
		ent->use = model_spawn_use;
	}

	if ( !(ent->s.effects & ANIM_MASK) && (ent->framenumbers > 1) )
	{
		ent->think = modelspawn_think;
		ent->nextthink = level.time + 2*FRAMETIME;
	}
	gi.linkentity (ent);
}
