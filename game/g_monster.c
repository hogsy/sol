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

void InitiallyDead (edict_t *self);

// Lazarus: If worldspawn CORPSE_SINK effects flag is set,
//          monsters/actors fade out and sink into the floor
//          30 seconds after death

#define SINKAMT			1
void FadeSink (edict_t *ent)
{
	ent->count++;
	ent->s.origin[2]-=SINKAMT;
	ent->think=FadeSink;
	if (ent->count==5)
	{
		ent->s.renderfx &= ~RF_TRANSLUCENT;
		ent->s.effects |= EF_SPHERETRANS;
	}
	if (ent->count == 10)
		ent->think = G_FreeEdict;
	ent->nextthink = level.time+FRAMETIME;
}
void FadeDieSink (edict_t *ent)
{
	ent->takedamage = DAMAGE_NO;	// can't gib 'em once they start sinking
	ent->s.effects &= ~EF_FLIES;
	ent->s.sound = 0;
	ent->s.origin[2] -= SINKAMT;
	ent->s.renderfx = RF_TRANSLUCENT;
	ent->think = FadeSink;
	ent->nextthink = level.time + FRAMETIME;
	ent->count = 0;
}


// Lazarus: M_SetDeath is used to restore the death movement,
//          bounding box, and a few other parameters for dead
//          monsters that change levels with a trigger_transition

qboolean M_SetDeath (edict_t *self, mmove_t **deathmoves)
{
	mmove_t	*move = NULL;
	mmove_t *dmove;

	if (self->health > 0)
		return false;

	while (*deathmoves && !move)
	{
		dmove = *deathmoves;
		if ( (self->s.frame >= dmove->firstframe) &&
			(self->s.frame <= dmove->lastframe)     )
			move = dmove;
		else
			deathmoves++;
	}
	if (move)
	{
		self->monsterinfo.currentmove = move;
		if (self->monsterinfo.currentmove->endfunc)
			self->monsterinfo.currentmove->endfunc(self);
		self->s.frame = move->lastframe;
		self->s.skinnum |= 1;
		return true;
	}
	return false;
}

//
// monster weapons
//

//FIXME monsters should call these with a totally accurate direction
// and we can mess it up based on skill.  Spread should be for normal
// and we can tighten or loosen based on skill.  We could muck with
// the damages too, but I'm not sure that's such a good idea.
void monster_fire_bullet (edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype)
{
	fire_bullet (self, start, dir, damage, kick, hspread, vspread, MOD_UNKNOWN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
#ifdef KMQUAKE2_ENGINE_MOD
	if (flashtype >= MZ2_SEND_SHORT) {
		gi.WriteByte (MZ2_SEND_SHORT);
		gi.WriteShort (flashtype);
		gi.WriteByte (0);
	}
	else
#endif
		gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int flashtype)
{
	fire_shotgun (self, start, aimdir, damage, kick, hspread, vspread, count, MOD_UNKNOWN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
#ifdef KMQUAKE2_ENGINE_MOD
	if (flashtype >= MZ2_SEND_SHORT) {
		gi.WriteByte (MZ2_SEND_SHORT);
		gi.WriteShort (flashtype);
		gi.WriteByte (0);
	}
	else
#endif
		gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect, int color)
{
	fire_blaster (self, start, dir, damage, speed, effect, false, color);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
#ifdef KMQUAKE2_ENGINE_MOD
	if (flashtype >= MZ2_SEND_SHORT) {
		gi.WriteByte (MZ2_SEND_SHORT);
		gi.WriteShort (flashtype);
		gi.WriteByte (color - 1);	// orange = 0, green = 1, blue = 2, red = 3
	}
	else
#endif
		gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}	

void monster_fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int flashtype, qboolean contact)
{
	fire_grenade (self, start, aimdir, damage, speed, 2.5, damage+40, contact);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
#ifdef KMQUAKE2_ENGINE_MOD
	if (flashtype >= MZ2_SEND_SHORT) {
		gi.WriteByte (MZ2_SEND_SHORT);
		gi.WriteShort (flashtype);
		gi.WriteByte (0);
	}
	else
#endif
		gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, edict_t *homing_target)
{
	fire_rocket (self, start, dir, damage, speed, damage+20, damage, homing_target);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
#ifdef KMQUAKE2_ENGINE_MOD
	if (flashtype >= MZ2_SEND_SHORT) {
		gi.WriteByte (MZ2_SEND_SHORT);
		gi.WriteShort (flashtype);
		gi.WriteByte (0);
	}
	else
#endif
		gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}	

void monster_fire_railgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int flashtype)
{
	fire_rail (self, start, aimdir, damage, kick, false, 0, 0, 0);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
#ifdef KMQUAKE2_ENGINE_MOD
	if (flashtype >= MZ2_SEND_SHORT) {
		gi.WriteByte (MZ2_SEND_SHORT);
		gi.WriteShort (flashtype);
		gi.WriteByte (0);
	}
	else
#endif
		gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_bfg (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int kick, float damage_radius, int flashtype)
{
	fire_bfg (self, start, aimdir, damage, speed, damage_radius);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
#ifdef KMQUAKE2_ENGINE_MOD
	if (flashtype >= MZ2_SEND_SHORT) {
		gi.WriteByte (MZ2_SEND_SHORT);
		gi.WriteShort (flashtype);
		gi.WriteByte (0);
	}
	else
#endif
		gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}



//
// Monster utility functions
//

void M_FliesOff (edict_t *self)
{
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
}

void M_FliesOn (edict_t *self)
{
	if (self->waterlevel)
		return;
	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex ("infantry/inflies1.wav");
	self->think = M_FliesOff;
	self->nextthink = level.time + 60;
}

void M_FlyCheck (edict_t *self)
{
	// Knightmare- keep running lava check
	self->postthink = deadmonster_think;

	if (self->monsterinfo.flies > 1.0)
	{
		// should ALREADY have flies
		self->think = M_FliesOff;
		self->nextthink = level.time + 60;
		return;
	}

	if (self->waterlevel)
		return;

	if (random() > self->monsterinfo.flies)
		return;

	if (world->effects & FX_WORLDSPAWN_CORPSEFADE)
		return;

	self->think = M_FliesOn;
	self->nextthink = level.time + 5 + 10 * random();
}

void AttackFinished (edict_t *self, float time)
{
	self->monsterinfo.attack_finished = level.time + time;
}


void M_CheckGround (edict_t *ent)
{
	vec3_t		point;
	trace_t		trace;

	if (level.time < ent->gravity_debounce_time)
		return;

	if (ent->flags & (FL_SWIM|FL_FLY))
		return;

	if (ent->velocity[2] > 100)
	{
		ent->groundentity = NULL;
		return;
	}

// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - 0.25;

	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
	if ( trace.plane.normal[2] < 0.7 && !trace.startsolid)
	{
		ent->groundentity = NULL;
		return;
	}

	// Lazarus: The following 2 lines were in the original code and commented out
	//          by id. However, the effect of this is that a player walking over
	//          a dead monster who is laying on a brush model will cause the 
	//          dead monster to drop through the brush model. This change *may*
	//          have other consequences, though, so watch out for this.

	ent->groundentity = trace.ent;
	ent->groundentity_linkcount = trace.ent->linkcount;
//	if (!trace.startsolid && !trace.allsolid)
//		VectorCopy (trace.endpos, ent->s.origin);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, ent->s.origin);
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->linkcount;
		// mxd fix - setting ent->velocity[2] makes ents on fast ascending plats (speed >= 200) skip ahead
		ent->velocity[2] = 0;	// Lazarus: what if the groundentity is moving?
	//	ent->velocity[2] = trace.ent->velocity[2];
	}
}


void M_CatagorizePosition (edict_t *ent)
{
	vec3_t		point;
	int			cont;

//
// get waterlevel
//
// Lazarus... more broken code because of origin being screwed up
//	point[0] = ent->s.origin[0];
//	point[1] = ent->s.origin[1];
//	point[2] = ent->s.origin[2] + ent->mins[2] + 1;	
	point[0] = (ent->absmax[0] + ent->absmin[0])/2;
	point[1] = (ent->absmax[1] + ent->absmin[1])/2;
	point[2] = ent->absmin[2] + 2;

	cont = gi.pointcontents (point);

	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = 0;
		ent->watertype = 0;
		return;
	}

	ent->watertype = cont;
	ent->waterlevel = 1;
	point[2] += 26;
	cont = gi.pointcontents (point);
	if (!(cont & MASK_WATER))
		return;

	ent->waterlevel = 2;
	point[2] += 22;
	cont = gi.pointcontents (point);
	if (cont & MASK_WATER)
		ent->waterlevel = 3;
}


void M_WorldEffects (edict_t *ent)
{
	int		dmg;

	if (ent->health > 0)
	{
		if (!(ent->flags & FL_SWIM))
		{
			if (ent->waterlevel < 3)
			{
				ent->air_finished = level.time + 12;
			}
			else if (ent->air_finished < level.time)
			{	// drown!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
		else
		{
			if (ent->waterlevel > 0)
			{
				ent->air_finished = level.time + 9;
			}
			else if (ent->air_finished < level.time)
			{	// suffocate!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
	}
	
	if (ent->waterlevel == 0)
	{
		if (ent->flags & FL_INWATER)
		{	
			if (ent->watertype & CONTENTS_MUD)
				gi.sound (ent, CHAN_BODY, gi.soundindex("mud/mud_out1.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}
		return;
	}

	if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 0.2;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 10*ent->waterlevel, 0, 0, MOD_LAVA);
		}
	}
	// slime doesn't damage dead monsters
	if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME) && !(ent->svflags & SVF_DEADMONSTER))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 1;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 4*ent->waterlevel, 0, 0, MOD_SLIME);
		}
	}
	
	if ( !(ent->flags & FL_INWATER) )
	{	
		if (!(ent->svflags & SVF_DEADMONSTER))
		{
			if (ent->watertype & CONTENTS_LAVA)
				if (random() <= 0.5)
					gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_SLIME)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_MUD)
				gi.sound (ent, CHAN_BODY, gi.soundindex("mud/mud_in2.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_WATER)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}

		ent->flags |= FL_INWATER;
		ent->old_watertype = ent->watertype;
		ent->damage_debounce_time = 0;
	}
}


void M_droptofloor (edict_t *ent)
{
	vec3_t		end;
	trace_t		trace;

	ent->s.origin[2] += 1;
	VectorCopy (ent->s.origin, end);
	end[2] -= 256;
	
	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1 || trace.allsolid)
		return;

	VectorCopy (trace.endpos, ent->s.origin);

	gi.linkentity (ent);
	M_CheckGround (ent);
	M_CatagorizePosition (ent);
}


void M_SetEffects (edict_t *ent)
{
	ent->s.effects &= ~(EF_COLOR_SHELL|EF_POWERSCREEN);
	ent->s.renderfx &= ~(RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);

	if (ent->monsterinfo.aiflags & AI_RESURRECTING)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	if (ent->health <= 0)
		return;

	if (ent->powerarmor_time > level.time)
	{
		if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SCREEN)
		{
			ent->s.effects |= EF_POWERSCREEN;
		}
		else if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SHIELD)
		{
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}
}


void M_MoveFrame (edict_t *self)
{
	mmove_t	*move;
	int		index;

	// Lazarus: For live monsters weaker than gladiator who aren't already running from
	//          something, evade live grenades on the ground.
	if ( (self->health > 0) && (self->max_health < 400) && !(self->monsterinfo.aiflags & AI_CHASE_THING) && self->monsterinfo.run
		&& !((Q_stricmp(self->classname, "misc_insane") == 0) && (self->moreflags & FL2_CRUCIFIED)) )	// Knightmare- crucified insanes don't evade
		Grenade_Evade (self);

	move = self->monsterinfo.currentmove;
	self->nextthink = level.time + FRAMETIME;

	if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= move->firstframe) && (self->monsterinfo.nextframe <= move->lastframe))
	{
		self->s.frame = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = 0;
	}
	else
	{
		if (self->s.frame == move->lastframe)
		{
			if (move->endfunc)
			{
				move->endfunc (self);

				// regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				// check for death
				if (self->svflags & SVF_DEADMONSTER)
					return;
			}
		}

		if (self->s.frame < move->firstframe || self->s.frame > move->lastframe)
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self->s.frame = move->firstframe;
		}
		else
		{
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			{
				self->s.frame++;
				if (self->s.frame > move->lastframe)
					self->s.frame = move->firstframe;
			}
		}
	}

	index = self->s.frame - move->firstframe;
	if (move->frame[index].aifunc)
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			move->frame[index].aifunc (self, move->frame[index].dist * self->monsterinfo.scale);
		else
			move->frame[index].aifunc (self, 0);

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc (self);
}


void monster_think (edict_t *self)
{
	M_MoveFrame (self);
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);
}

// Knightmare- for dead monsters to check
// if they've fallen into lava, etc.
void deadmonster_think (edict_t *self)
{
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);
}


/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if ( !activator || self->enemy )	// Phatman: Solves a crash condition
		return;
	if (self->health <= 0)
		return;
	if (activator->flags & FL_NOTARGET)
		return;
	if (!(activator->client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
		return;
	if (activator->flags & FL_DISGUISED)
		return;

	// if monster is "used" by player, turn off good guy stuff
	if (activator->client)
	{
		if (UseRegularGoodGuyFlag(self)) {
			self->spawnflags &= ~SF_MONSTER_GOODGUY;
		}
		self->monsterinfo.aiflags &= ~(AI_GOOD_GUY + AI_FOLLOW_LEADER);
		if (self->dmgteam && !Q_stricmp(self->dmgteam,"player"))
			self->dmgteam = NULL;
	}

// delay reaction so if the monster is teleported, its sound is still heard
	self->enemy = activator;
	FoundTarget (self);
}


void monster_start_go (edict_t *self);


void monster_triggered_spawn (edict_t *self)
{
	self->s.origin[2] += 1;
	KillBox (self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12;
	gi.linkentity (self);

	monster_start_go (self);

	if (self->enemy && !(self->spawnflags & SF_MONSTER_SIGHT) && !(self->enemy->flags & FL_NOTARGET))
	{
		if (!(self->enemy->flags & FL_DISGUISED))
			FoundTarget (self);
		else
			self->enemy = NULL;
	}
	else
		self->enemy = NULL;
}

void monster_triggered_spawn_use (edict_t *self, edict_t *other, edict_t *activator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = monster_triggered_spawn;
	self->nextthink = level.time + FRAMETIME;
	// Knightmare- good guy monsters shouldn't have an enemy from this
	if (activator && activator->client && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
		self->enemy = activator;
	// Lazarus: Add 'em up
	// Knightmare- this is annoying, and will likely annoy most Q2 players as well
//	if ( !(self->monsterinfo.aiflags & AI_GOOD_GUY) )
//		level.total_monsters++;
	self->use = monster_use;
}

void monster_triggered_start (edict_t *self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
	self->use = monster_triggered_spawn_use;
	// Lazarus
	self->spawnflags &= ~SF_MONSTER_TRIGGER_SPAWN;
}


/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use (edict_t *self)
{
	edict_t	*player;
	int		i;

	self->flags &= ~(FL_FLY|FL_SWIM);
	self->monsterinfo.aiflags &= AI_GOOD_GUY;

	// Lazarus: If actor/monster is being used as a camera by a player,
	// turn camera off for that player
	for (i = 0, player = g_edicts+1; i<maxclients->value; i++, player++)
	{
		if ( !player->inuse )	// Phatman: Solves a crash condition
			continue;
		if ( player->client && (player->client->spycam == self) )
			camera_off(player);
	}

	if (self->item)
	{
		Drop_Item (self, self->item);
		self->item = NULL;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (!self->target)
		return;

	G_UseTargets (self, self->enemy);
}


//============================================================================

qboolean monster_start (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return false;
	}

	// Lazarus: Already gibbed monsters passed across levels via trigger_transition:
	if ( (self->max_health > 0) && (self->health <= self->gib_health) && !(self->spawnflags & SF_MONSTER_NOGIB) )
	{
		void	SP_gibhead(edict_t *);

		SP_gibhead(self);
		return true;
	}

	// Lazarus: Good guys
	if ( UseRegularGoodGuyFlag(self) && (self->spawnflags & SF_MONSTER_GOODGUY) )
	{
		self->monsterinfo.aiflags |= AI_GOOD_GUY;
		if ( !self->dmgteam ) {
			self->dmgteam = gi.TagMalloc(8*sizeof(char), TAG_LEVEL);
		//	strncpy(self->dmgteam,"player");
			Q_strncpyz(self->dmgteam, 8, "player");
		}
	}

	// Lazarus: Max range for sight/attack
	if (st.distance)
		self->monsterinfo.max_range = max(500,st.distance);
	else
		self->monsterinfo.max_range = 1280;	// Q2 default is 1000. We're mean.

	// Lazarus: We keep SIGHT to mean what old AMBUSH does, and AMBUSH additionally
	//          now means don't play idle sounds
/*	if ((self->spawnflags & MONSTER_SIGHT) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self->spawnflags &= ~MONSTER_SIGHT;
		self->spawnflags |= MONSTER_AMBUSH;
	} */
	if ( (self->spawnflags & SF_MONSTER_AMBUSH) && !(self->monsterinfo.aiflags & AI_GOOD_GUY) )
		self->spawnflags |= SF_MONSTER_SIGHT;

	// Lazarus: Don't add trigger spawned monsters until they are actually spawned
	// Knightmare- this is annoying, and will likely annoy most Q2 players as well
//	if ( !(self->monsterinfo.aiflags & AI_GOOD_GUY) && !(self->spawnflags & SF_MONSTER_TRIGGER_SPAWN) )
	if ( !(self->monsterinfo.aiflags & AI_GOOD_GUY) )
		level.total_monsters++;

	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_MONSTER;
	self->s.renderfx |= RF_FRAMELERP;
	self->air_finished = level.time + 12;
	self->use = monster_use;
	// Lazarus - don't reset max_health unnecessarily
	if (!self->max_health)
		self->max_health = self->health;
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;
	else
		self->s.skinnum &= ~1;
	self->clipmask = MASK_MONSTERSOLID;

	if (self->s.skinnum < 1) // Knightmare added
		self->s.skinnum = 0;
	self->deadflag = DEAD_NO;
	self->svflags &= ~SVF_DEADMONSTER;

	if (self->monsterinfo.flies > 1.0)
	{
		self->s.effects |= EF_FLIES;
		self->s.sound = gi.soundindex ("infantry/inflies1.wav");
	}

	// Lazarus
	if (self->health <=0)
	{
		self->svflags |= SVF_DEADMONSTER;
		self->movetype = MOVETYPE_TOSS;
		self->takedamage = DAMAGE_YES;
		self->monsterinfo.pausetime = 100000000;
		self->monsterinfo.aiflags &= ~AI_RESPAWN_FINDPLAYER;
		if (self->max_health > 0)
		{
			// This must be a dead monster who changed levels
			// via trigger_transition
			self->nextthink = 0;
			self->deadflag = DEAD_DEAD;
		}
		if (self->s.effects & EF_FLIES && self->monsterinfo.flies <= 1.0)
		{
			self->think = M_FliesOff;
			self->nextthink = level.time + 1 + random()*60;
		}
		return true;
	}
	else
	{
		// make sure red shell is turned off in case medic got confused:
		self->monsterinfo.aiflags &= ~AI_RESURRECTING;
		self->svflags &= ~SVF_DEADMONSTER;
		self->takedamage = DAMAGE_AIM;
	}

	if (!self->monsterinfo.checkattack)
		self->monsterinfo.checkattack = M_CheckAttack;
	VectorCopy (self->s.origin, self->s.old_origin);

	if (st.item)
	{
		self->item = FindItemByClassname (st.item);
		if (!self->item)
			gi.dprintf("%s at %s has bad item: %s\n", self->classname, vtos(self->s.origin), st.item);
	}

	// randomize what frame they start on
	// Lazarus: preserve frame if set for monsters changing levels
	if (!self->s.frame)
	{
		if (self->monsterinfo.currentmove)
			self->s.frame = self->monsterinfo.currentmove->firstframe + (rand() % (self->monsterinfo.currentmove->lastframe - self->monsterinfo.currentmove->firstframe + 1));
	}

	return true;
}

void monster_start_go (edict_t *self)
{
	vec3_t	v;

	if (self->health <= 0)
	{
		if (self->max_health <= 0)
			InitiallyDead(self);
		return;
	}

	// Lazarus: move_origin for func_monitor
	if (!VectorLength(self->move_origin))
		VectorSet(self->move_origin,0,0,self->viewheight);

	// check for target to point_combat and change to combattarget
	if (self->target)
	{
		qboolean	notcombat;
		qboolean	fixup;
		edict_t		*target;

		target = NULL;
		notcombat = false;
		fixup = false;
		while ((target = G_Find (target, FOFS(targetname), self->target)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") == 0)
			{
				self->combattarget = self->target;
				fixup = true;
			}
			else
			{
				notcombat = true;
			}
		}
		if (notcombat && self->combattarget)
			gi.dprintf("%s at %s has target with mixed types\n", self->classname, vtos(self->s.origin));
		if (fixup)
			self->target = NULL;
	}

	// validate combattarget
	if (self->combattarget)
	{
		edict_t		*target;

		target = NULL;
		while ((target = G_Find (target, FOFS(targetname), self->combattarget)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") != 0)
			{
				gi.dprintf("%s at (%i %i %i) has a bad combattarget %s : %s at (%i %i %i)\n",
					self->classname, (int)self->s.origin[0], (int)self->s.origin[1], (int)self->s.origin[2],
					self->combattarget, target->classname, (int)target->s.origin[0], (int)target->s.origin[1],
					(int)target->s.origin[2]);
			}
		}
	}

	if (self->target)
	{
		self->goalentity = self->movetarget = G_PickTarget(self->target);
		if (!self->movetarget)
		{
			gi.dprintf ("%s can't find target %s at %s\n", self->classname, self->target, vtos(self->s.origin));
			self->target = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
		else if (strcmp (self->movetarget->classname, "path_corner") == 0)
		{
			// Lazarus: Don't wipe out target for trigger spawned monsters
			//          that aren't triggered yet
			if ( ! (self->spawnflags & SF_MONSTER_TRIGGER_SPAWN) ) {
				VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
				self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
				self->monsterinfo.walk (self);
				self->target = NULL;
			}
		}
		else
		{
			self->goalentity = self->movetarget = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
	}
	else
	{
		self->monsterinfo.pausetime = 100000000;
		self->monsterinfo.stand (self);
	}

	self->think = monster_think;
	self->nextthink = level.time + FRAMETIME;
}


void walkmonster_start_go (edict_t *self)
{
	if (!(self->spawnflags & SF_MONSTER_TRIGGER_SPAWN) && level.time < 1)
	{
		M_droptofloor (self);

		if (self->groundentity)
			if (!M_walkmove (self, 0, 0))
				gi.dprintf ("%s in solid at %s\n", self->classname, vtos(self->s.origin));
	}
	
	if (!self->yaw_speed)
		self->yaw_speed = 20;
	self->viewheight = 25;

	monster_start_go (self);

	if (self->spawnflags & SF_MONSTER_TRIGGER_SPAWN)
		monster_triggered_start (self);

}

void walkmonster_start (edict_t *self)
{
	self->think = walkmonster_start_go;
	monster_start (self);
}


void flymonster_start_go (edict_t *self)
{
	if (!M_walkmove (self, 0, 0))
		gi.dprintf ("%s in solid at %s\n", self->classname, vtos(self->s.origin));

	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 25;

	self->monsterinfo.flies = 0.0;

	monster_start_go (self);

	if (self->spawnflags & SF_MONSTER_TRIGGER_SPAWN)
		monster_triggered_start (self);
}


void flymonster_start (edict_t *self)
{
	self->flags |= FL_FLY;
	self->think = flymonster_start_go;
	monster_start (self);
}


void swimmonster_start_go (edict_t *self)
{
	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 10;

	monster_start_go (self);

	if (self->spawnflags & SF_MONSTER_TRIGGER_SPAWN)
		monster_triggered_start (self);
}

void swimmonster_start (edict_t *self)
{
	self->flags |= FL_SWIM;
	self->think = swimmonster_start_go;
	monster_start (self);
}


//===============================================================
// Following functions unique to Lazarus

void InitiallyDead (edict_t *self)
{
	int	damage;

	if (self->max_health > 0)
		return;

//	gi.dprintf("InitiallyDead on %s at %s\n",self->classname,vtos(self->s.origin));
	
	// initially dead bad guys shouldn't count against totals
	if ((self->max_health <= 0) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		level.total_monsters--;
		if (self->deadflag != DEAD_DEAD)
			level.killed_monsters--;
	}
	if (self->deadflag != DEAD_DEAD)
	{
		damage = 1 - self->health;
		self->health = 1;
		T_Damage (self, world, world, vec3_origin, self->s.origin, vec3_origin, damage, 0, DAMAGE_NO_ARMOR, 0);
		if (self->svflags & SVF_MONSTER)
		{
			self->svflags |= SVF_DEADMONSTER;
			self->think = monster_think;
			self->nextthink = level.time + FRAMETIME;
		}
	}
	gi.linkentity(self);
}


void HintTestNext (edict_t *self, edict_t *hint)
{
	edict_t		*next=NULL;
	edict_t		*e;
	vec3_t		dir;

	self->monsterinfo.aiflags &= ~AI_HINT_TEST;
	if (self->goalentity == hint)
		self->goalentity = NULL;
	if (self->movetarget == hint)
		self->movetarget = NULL;
	if (self->monsterinfo.pathdir == 1)
	{
		if (hint->hint_chain)
			next = hint->hint_chain;
		else
			self->monsterinfo.pathdir = -1;
	}
	if (self->monsterinfo.pathdir == -1)
	{
		e = hint_chain_starts[hint->hint_chain_id];
		while (e)
		{
			if (e->hint_chain == hint)
			{
				next = e;
				break;
			}
			e = e->hint_chain;
		}
	}
	if (!next)
	{
		self->monsterinfo.pathdir = 1;
		next = hint->hint_chain;
	}
	if (next)
	{
		self->hint_chain_id = next->hint_chain_id;
		VectorSubtract (next->s.origin, self->s.origin, dir);
		self->ideal_yaw = vectoyaw(dir);
		self->goalentity = self->movetarget = next;
		self->monsterinfo.pausetime = 0;
		self->monsterinfo.aiflags |= AI_HINT_TEST;
		// run for it
		self->monsterinfo.run (self);
		gi.dprintf("%s (%s): Reached hint_path %s,\nsearching for hint_path %s at %s. %s\n",
			self->classname, (self->targetname ? self->targetname : "<noname>"),
			(hint->targetname ? hint->targetname : "<noname>"),
			(next->targetname ? next->targetname : "<noname>"),
			vtos(next->s.origin),
			(visible(self,next) ? "I see it." : "I don't see it."));
	}
	else
	{
		self->monsterinfo.pausetime = level.time + 100000000;
		self->monsterinfo.stand (self);
		gi.dprintf("%s (%s): Error finding next/previous hint_path from %s at %s.\n",
			self->classname, (self->targetname ? self->targetname : "<noname>"),
			(hint->targetname ? hint->targetname : "<noname>"),
			vtos(hint->s.origin));
	}
}

int HintTestStart (edict_t *self)
{
	edict_t	*e;
	edict_t	*hint=NULL;
	float	dist;
	vec3_t	dir;
	int		i;
	float	bestdistance=99999;

	if (!hint_chains_exist)
		return 0;

	for (i=game.maxclients+1; i<globals.num_edicts; i++)
	{
			e = &g_edicts[i];
			if (!e->inuse)
				continue;
			if (Q_stricmp(e->classname, "hint_path"))
				continue;
			if (!visible(self, e))
				continue;
			if (!canReach(self, e))
				continue;
			VectorSubtract(e->s.origin,self->s.origin,dir);
			dist = VectorLength(dir);
			if (dist < bestdistance)
			{
				hint = e;
				bestdistance = dist;
			}
	}
	if (hint)
	{
		self->hint_chain_id = hint->hint_chain_id;
		if (!self->monsterinfo.pathdir)
			self->monsterinfo.pathdir = 1;
		VectorSubtract(hint->s.origin, self->s.origin, dir);
		self->ideal_yaw = vectoyaw(dir);
		self->enemy = self->oldenemy = NULL;
		self->goalentity = self->movetarget = hint;
		self->monsterinfo.pausetime = 0;
		self->monsterinfo.aiflags |= AI_HINT_TEST;
		// run for it
		self->monsterinfo.run (self);
		return 1;
	}
	else
		return -1;
}
