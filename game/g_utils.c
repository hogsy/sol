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

// g_utils.c -- misc utility functions for game module

#include "g_local.h"


void G_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

void G_ProjectSource2 (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t up, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1] + up[0] * distance[2];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1] + up[1] * distance[2];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + up[2] * distance[2];
}

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
edict_t *G_Find (edict_t *from, size_t fieldofs, char *match)	// Knightmare- changed fieldofs from int
{
	char	*s;

	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts] ; from++)
	{
		if (!from->inuse)
			continue;
		s = *(char **) ((byte *)from + fieldofs);
		if (!s)
			continue;
		if (!Q_stricmp (s, match))
			return from;
	}

	return NULL;
}


/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t *findradius (edict_t *from, vec3_t org, float rad)
{
	vec3_t	eorg;
	int		j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
		if (VectorLength(eorg) > rad)
			continue;
		return from;
	}

	return NULL;
}


/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
#define MAXCHOICES	8

edict_t *G_PickTarget (char *targetname)
{
	edict_t	*ent = NULL;
	int		num_choices = 0;
	edict_t	*choice[MAXCHOICES];

	if (!targetname)
	{
		gi.dprintf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while (1)
	{
		ent = G_Find (ent, FOFS(targetname), targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
	{
		gi.dprintf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}



void Think_Delay (edict_t *ent)
{
	G_UseTargets (ent, ent->activator);
	G_FreeEdict (ent);
}

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets (edict_t *ent, edict_t *activator)
{
	edict_t		*t;

	if (!ent)	// sanity check
		return;

//
// check for a delay
//
	if (ent->delay)
	{
	// create a temp object to fire at a later time
		t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay;
		t->activator = activator;
		if (!activator)
			gi.dprintf ("Think_Delay with no activator\n");
		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		t->noise_index = ent->noise_index;
		return;
	}
	
	
//
// print the message
//
	if ( (ent->message) && (activator) && !(activator->svflags & SVF_MONSTER) )
	{
	//	Lazarus - change so that noise_index < 0 means no sound
		safe_centerprintf (activator, "%s", ent->message);
		if (ent->noise_index > 0)
			gi.sound (activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
		else if (ent->noise_index == 0)
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

//
// kill killtargets
//
	if (ent->killtarget)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), ent->killtarget)))
		{
			// Lazarus: remove LIVE killtargeted monsters from total_monsters
			if ((t->svflags & SVF_MONSTER) && (t->deadflag == DEAD_NO))
			{
				if (!t->dmgteam || strcmp(t->dmgteam,"player"))
					if (!(t->monsterinfo.aiflags & AI_GOOD_GUY))
						level.total_monsters--;
			}
			// and decrement secret count if target_secret is removed
			else if (!Q_stricmp(t->classname,"target_secret"))
				level.total_secrets--;
			// same deal with target_goal, but also turn off CD music if applicable
			else if (!Q_stricmp(t->classname,"target_goal"))
			{
				level.total_goals--;
				if (level.found_goals >= level.total_goals)
				gi.configstring (CS_CDTRACK, "0");
			}
			G_FreeEdict (t);
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

//
// fire targets
//
	if (ent->target)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), ent->target)))
		{
			// doors fire area portals in a specific way
			if (!Q_stricmp(t->classname, "func_areaportal") &&
				(!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating") 
				/*DWH*/ || !Q_stricmp(ent->classname,"func_door_rot_dh")))
				continue;

			if (t == ent)
			{
				gi.dprintf ("WARNING: Entity used itself.\n");
			}
			else
			{
				if (t->use)
					t->use (t, ent, activator);
			}
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}
}


/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float	*tv (float x, float y, float z)
{
	static	int		index;
	static	vec3_t	vecs[8];
	float	*v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1)&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}


/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char	*vtos (vec3_t v)
{
	static	int		index;
	static	char	str[8][32];
	char	*s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = (index + 1)&7;

	Com_sprintf (s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);

	return s;
}


vec3_t VEC_UP		= {0, -1, 0};
vec3_t MOVEDIR_UP	= {0, 0, 1};
vec3_t VEC_DOWN		= {0, -2, 0};
vec3_t MOVEDIR_DOWN	= {0, 0, -1};

void G_SetMovedir (vec3_t angles, vec3_t movedir)
{
	if (VectorCompare (angles, VEC_UP))
	{
		VectorCopy (MOVEDIR_UP, movedir);
	}
	else if (VectorCompare (angles, VEC_DOWN))
	{
		VectorCopy (MOVEDIR_DOWN, movedir);
	}
	else
	{
		AngleVectors (angles, movedir, NULL, NULL);
	}

	VectorClear (angles);
}


float vectoyaw (vec3_t vec)
{
	float	yaw;
	
	if (vec[PITCH] == 0) {
		if (vec[YAW] == 0)
			yaw = 0;
		else if (vec[YAW] > 0)
			yaw = 90;
		else
			yaw = 270;
	} else {
		yaw = (int) (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}

float vectoyaw2 (vec3_t vec)
{
	float	yaw;
	
	if (vec[PITCH] == 0) {
		if (vec[YAW] == 0)
			yaw = 0;
		else if (vec[YAW] > 0)
			yaw = 90;
		else
			yaw = 270;
	} else {
		yaw = (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}


void vectoangles (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;
	
	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		if (value1[0])
			yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

void vectoangles2 (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;
	
	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		if (value1[0])
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

char *G_CopyString (char *in)
{
	char	*out;
	size_t	outSize;
	
	outSize = strlen(in)+1;
	out = gi.TagMalloc (outSize, TAG_LEVEL);
//	strncpy (out, outSize, in);
	Q_strncpyz (out, outSize, in);
	return out;
}

// Knightmare added
mmove_t *G_NewCustomAnim (void)
{
	int	idx = 0;

	if (level.num_custom_anims < MAX_CUSTOM_ANIMS) {
		idx = level.num_custom_anims;
		level.num_custom_anims++;
		return &g_custom_anims[idx];
	}
	else {
	//	gi.dprintf ("G_NewCustomAnimIndex: no more custom anims available!\n");
		return NULL;
	}
}
// end Knightmare

void G_InitEdict (edict_t *e)
{
	e->inuse = true;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_edicts;
	e->org_movetype = -1;
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_Spawn (void)
{
	int			i;
	edict_t		*e;

	e = &g_edicts[(int)maxclients->value+1];
	for ( i=maxclients->value+1 ; i<globals.num_edicts ; i++, e++)
	{
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inuse && ( e->freetime < 2 || level.time - e->freetime > 0.5 ) )
		{
			G_InitEdict (e);
			return e;
		}
	}

	if (i == game.maxentities)
		gi.error ("ED_Alloc: no free edicts");

	globals.num_edicts++;

	if (developer->value && readout->value)
		gi.dprintf("num_edicts = %d\n",globals.num_edicts);

	G_InitEdict (e);
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void G_FreeEdict (edict_t *ed)
{
	// Lazarus - if part of a movewith chain, remove from
	// the chain and repair broken links
	if (ed->movewith)
	{
		edict_t	*e;
		edict_t	*parent = NULL;
		int		i;

		for (i=1; i<globals.num_edicts && !parent; i++) {
			e = g_edicts + i;
			if (e->movewith_next == ed) parent = e;
		}
		if (parent) parent->movewith_next = ed->movewith_next;
	}

	if (ed->speaker) // recursively remove train's speaker entity
		G_FreeEdict (ed->speaker);

	gi.unlinkentity (ed);		// unlink from world

	// Lazarus: In SP we no longer reserve slots for bodyque's
	if (deathmatch->value || coop->value)
	{
		if ((ed - g_edicts) <= (maxclients->value + BODY_QUEUE_SIZE))
		{
		//	gi.dprintf("tried to free special edict\n");
			return;
		}
	}
	else {
		if ((ed - g_edicts) <= maxclients->value)
		{
			return;
		}
	}

	// Lazarus: actor muzzle flash
	if (ed->flash)
	{
		memset (ed->flash, 0, sizeof(*ed));
		ed->flash->classname = "freed";
		ed->flash->freetime  = level.time;
		ed->flash->inuse     = false;
	}

	// Lazarus: reflections
	if (!(ed->flags & FL_REFLECT))
		DeleteReflection(ed,-1);

	memset (ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
}

/*
============
G_TouchTriggers

============
*/
void G_TouchTriggers (edict_t *ent)
{
	int				i, num;
	static edict_t	*touch[MAX_EDICTS];	// Knightmare- made static due to stack size
	edict_t			*hit;

	// Lazarus: nothing touches anything if game is frozen
	if (level.freeze)
		return;

	// dead things don't activate triggers!
	if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))
		return;

	num = gi.BoxEdicts (ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;
		if (ent->client && ent->client->spycam && !(hit->svflags & SVF_TRIGGER_CAMOWNER))
			continue;
		hit->touch (hit, ent, NULL, NULL);
	}
}

/*
============
G_TouchSolids

Call after linking a new trigger in during gameplay
to force all entities it covers to immediately touch it
============
*/
void G_TouchSolids (edict_t *ent)
{
	int				i, num;
	static edict_t	*touch[MAX_EDICTS];	// Knightmare- made static due to stack size
	edict_t			*hit;

	num = gi.BoxEdicts (ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (ent->touch)
			ent->touch (hit, ent, NULL, NULL);
		if (!ent->inuse)
			break;
	}
}




/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
qboolean KillBox (edict_t *ent)
{
	trace_t		tr;

	while (1)
	{
		tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_PLAYERSOLID);
		if (!tr.ent)
			break;
		// nail it
		T_Damage (tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// if we didn't kill it, fail
		if (tr.ent->solid)
			return false;
	}

	return true;		// all clear
}

void AnglesNormalize(vec3_t vec)
{
	while (vec[0] > 180)
		vec[0] -= 360;
	while (vec[0] < -180)
		vec[0] += 360;
	while (vec[1] > 360)
		vec[1] -= 360;
	while (vec[1] < 0)
		vec[1] += 360;
}

float SnapToEights(float x)
{
	x *= 8.0;
	if (x > 0.0)
		x += 0.5;
	else
		x -= 0.5;
	return 0.125 * (int)x;
}


/* Lazarus - added functions */

void stuffcmd (edict_t *pent, char *pszCommand)
{
	gi.WriteByte(svc_stufftext);
	gi.WriteString(pszCommand);
	gi.unicast(pent, true);
}

qboolean point_infront (edict_t *self, vec3_t point)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;
	
	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorSubtract (point, self->s.origin, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);
	
	if (dot > 0.3)
		return true;
	return false;
}

float AtLeast (float x, float dx)
{
	float xx;

	xx = (float)(floor(x/dx - 0.5)+1.)*dx;
	if (xx < x) xx += dx;
	return xx;
}

edict_t	*LookingAt (edict_t *ent, int filter, vec3_t endpos, float *range)
{
	edict_t			*who;
	static edict_t	*trigger[MAX_EDICTS];	// Knightmare- made static due to stack size
	edict_t			*ignore;
	trace_t			tr;
	vec_t			r;
	vec3_t			end, forward, start;
	vec3_t			dir, entp, mins, maxs;
	int				i, num;

	if (!ent->client)
	{
		if (endpos) VectorClear (endpos);
		if (range) *range = 0;
		return NULL;
	}
	VectorClear(end);
	if (ent->client->chasetoggle)
	{
		AngleVectors (ent->client->v_angle, forward, NULL, NULL);
		VectorCopy (ent->client->chasecam->s.origin, start);
		ignore = ent->client->chasecam;
	}
	else if (ent->client->spycam)
	{
		AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
		VectorCopy (ent->s.origin, start);
		ignore = ent->client->spycam;
	}
	else
	{
		AngleVectors (ent->client->v_angle, forward, NULL, NULL);
		VectorCopy (ent->s.origin, start);
		start[2] += ent->viewheight;
		ignore = ent;
	}

	VectorMA(start, WORLD_SIZE, forward, end);	// was 8192
	
	/* First check for looking directly at a pickup item */
	VectorSet (mins, MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);	// was -4096, -4096, -4096
	VectorSet (maxs, MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);	// was 4096, 4096, 4096
	num = gi.BoxEdicts (mins, maxs, trigger, MAX_EDICTS, AREA_TRIGGERS);
	for (i=0 ; i<num ; i++)
	{
		who = trigger[i];
		if (!who->inuse)
			continue;
		if (!who->item)
			continue;
		if (!visible(ent,who))
			continue;
		if (!infront(ent,who))
			continue;
		VectorSubtract(who->s.origin,start,dir);
		r = VectorLength(dir);
		VectorMA (start, r, forward, entp);
		if (entp[0] < who->s.origin[0] - 17) continue;
		if (entp[1] < who->s.origin[1] - 17) continue;
		if (entp[2] < who->s.origin[2] - 17) continue;
		if (entp[0] > who->s.origin[0] + 17) continue;
		if (entp[1] > who->s.origin[1] + 17) continue;
		if (entp[2] > who->s.origin[2] + 17) continue;
		if (endpos)
			VectorCopy (who->s.origin, endpos);
		if (range)
			*range = r;
		return who;
	}

	tr = gi.trace (start, NULL, NULL, end, ignore, MASK_SHOT);
	if (tr.fraction == 1.0)
	{
		// too far away
		gi.sound (ent, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}
	if (!tr.ent)
	{
		// no hit
		gi.sound (ent, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}
	if (!tr.ent->classname)
	{
		// should never happen
		gi.sound (ent, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}

	if ((strstr(tr.ent->classname,"func_") != NULL) && (filter & LOOKAT_NOBRUSHMODELS))
	{
		// don't hit on brush models
		gi.sound (ent, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}
	if ((Q_stricmp(tr.ent->classname,"worldspawn") == 0) && (filter & LOOKAT_NOWORLD))
	{
		// world brush
		gi.sound (ent, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}
	if (endpos) {
		endpos[0] = tr.endpos[0];
		endpos[1] = tr.endpos[1];
		endpos[2] = tr.endpos[2];
	}
	if (range) {
		VectorSubtract (tr.endpos, start, start);
		*range = VectorLength(start);
	}
	return tr.ent;
}

char *GameDir (void)
{
#ifdef KMQUAKE2_ENGINE_MOD
	return gi.GameDir();
#else	// KMQUAKE2_ENGINE_MOD
	static char	buffer[MAX_OSPATH];
	cvar_t		*basedir, *gamedir;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);
	if ( strlen(gamedir->string) > 0 )
		Com_sprintf (buffer, sizeof(buffer), "%s/%s", basedir->string, gamedir->string);
	else
		Com_sprintf (buffer, sizeof(buffer), "%s/baseq2", basedir->string);

	return buffer;
#endif	// KMQUAKE2_ENGINE_MOD
}

char *SavegameDir (void)
{
#ifdef KMQUAKE2_ENGINE_MOD
	return gi.SaveGameDir();
#else	// KMQUAKE2_ENGINE_MOD
	static char	buffer[MAX_OSPATH];
	cvar_t	*basedir, *gamedir, *savegamepath;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);
	savegamepath = gi.cvar("savegamepath", "", 0);
	// use Unofficial Q2 Patch's savegamepath cvar if set
	if ( strlen(savegamepath->string) > 0 ) {
		Q_strncpyz (buffer, sizeof(buffer), savegamepath->string);
	}
	else {
		if ( strlen(gamedir->string) > 0 )
			Com_sprintf (buffer, sizeof(buffer), "%s/%s", basedir->string, gamedir->string);
		else
			Com_sprintf (buffer, sizeof(buffer), "%s/baseq2", basedir->string);
	}

	return buffer;
#endif	// KMQUAKE2_ENGINE_MOD
}

void GameDirRelativePath (const char *filename, char *output, size_t outputSize)
{
#ifdef KMQUAKE2_ENGINE_MOD
	Com_sprintf(output, outputSize, "%s/%s", gi.GameDir(), filename);
#else	// KMQUAKE2_ENGINE_MOD
	cvar_t	*basedir, *gamedir;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);
	if ( strlen(gamedir->string) > 0 )
		Com_sprintf (output, outputSize, "%s/%s/%s", basedir->string, gamedir->string, filename);
	else
		Com_sprintf (output, outputSize, "%s/baseq2/%s", basedir->string, filename);
#endif	// KMQUAKE2_ENGINE_MOD
}

void SavegameDirRelativePath (const char *filename, char *output, size_t outputSize)
{
#ifdef KMQUAKE2_ENGINE_MOD
	Com_sprintf(output, outputSize, "%s/%s", gi.SaveGameDir(), filename);
#else	// KMQUAKE2_ENGINE_MOD
	cvar_t	*basedir, *gamedir, *savegamepath;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);
	savegamepath = gi.cvar("savegamepath", "", 0);
	// use Unofficial Q2 Patch's savegamepath cvar if set
	if ( strlen(savegamepath->string) > 0 ) {
		Com_sprintf (output, outputSize, "%s/%s", savegamepath->string, filename);
	}
	else {
		if ( strlen(gamedir->string) > 0 )
			Com_sprintf (output, outputSize, "%s/%s/%s", basedir->string, gamedir->string, filename);
		else
			Com_sprintf (output, outputSize, "%s/baseq2/%s", basedir->string, filename);
	}
#endif	// KMQUAKE2_ENGINE_MOD
}

void CreatePath (const char *path)
{
#ifdef KMQUAKE2_ENGINE_MOD
	gi.CreatePath (path);
#else	// KMQUAKE2_ENGINE_MOD
	char	tmpBuf[MAX_OSPATH];
	char	*ofs;

	if (strstr(path, "..") || strstr(path, "::") || strstr(path, "\\\\") || strstr(path, "//"))
	{
		gi.dprintf("WARNING: refusing to create relative path '%s'\n", path);
		return;
	}
	Q_strncpyz (tmpBuf, sizeof(tmpBuf), path);

	for (ofs = tmpBuf+1; *ofs; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{	// create the directory
			*ofs = 0;
			_mkdir (tmpBuf);
			*ofs = '/';
		}
	}
#endif	// KMQUAKE2_ENGINE_MOD
}

qboolean LocalFileExists (const char *path)
{
	char	realPath[MAX_OSPATH];
	FILE	*f;

	SavegameDirRelativePath (path, realPath, sizeof(realPath));
	f = fopen (realPath, "rb");
	if (f) {
		fclose (f);
		return true;
	}
	return false;
}


// Knightmare added
/*
====================
AnyPlayerSpawned

Checks if any player has spawned.
Original code by Phatman.
====================
*/
qboolean AnyPlayerSpawned (void)
{
	int		i;

	for (i = 0; i < game.maxclients; i++) {
		if ( g_edicts[i + 1].inuse && g_edicts[i + 1].linkcount )
			return true;
	}

	return false;
}


/*
====================
AllPlayersSpawned

Checks if all players have spawned.
Original code by Phatman.
====================
*/
qboolean AllPlayersSpawned (void)
{
	int		i;

	for (i = 0; i < game.maxclients; i++) {
		if ( g_edicts[i + 1].inuse && !g_edicts[i + 1].linkcount )
			return false;
	}

	return true;
}


/*
====================
AllPlayersLinkcountCmp

Checks if all players linkcount matches value.
Returns true if any value matches.
cmp_linkcount is the value to compare against.
====================
*/
qboolean AllPlayersLinkcountCmp (int cmp_linkcount)
{
	int			i;
	qboolean	matched = false;

	for (i = 0; i < game.maxclients; i++) {
		if ( g_edicts[i + 1].inuse && (g_edicts[i + 1].linkcount == cmp_linkcount) )
			matched = true;
	}

	return matched;
}
// end Knightmare


/* Lazarus: G_UseTarget is similar to G_UseTargets, but only triggers
            a single target rather than all entities matching target
			criteria. It *does*, however, kill all killtargets */

void Think_Delay_Single (edict_t *ent)
{
	G_UseTarget (ent, ent->activator, ent->target_ent);
	G_FreeEdict (ent);
}

void G_UseTarget (edict_t *ent, edict_t *activator, edict_t *target)
{
	edict_t		*t;

//
// check for a delay
//
	if (ent->delay)
	{
	// create a temp object to fire at a later time
		t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay_Single;
		t->activator = activator;
		t->target_ent = target;
		if (!activator)
			gi.dprintf ("Think_Delay_Single with no activator\n");
		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		t->noise_index = ent->noise_index;
		return;
	}
	
	
//
// print the message
//
	if ((ent->message) && !(activator->svflags & SVF_MONSTER))
	{
		safe_centerprintf (activator, "%s", ent->message);
		if (ent->noise_index > 0)
			gi.sound (activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
		else if (ent->noise_index == 0)
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

//
// kill killtargets
//
	if (ent->killtarget)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), ent->killtarget)))
		{
			// Lazarus: remove killtargeted monsters from total_monsters
			if (t->svflags & SVF_MONSTER) {
				if (!t->dmgteam || strcmp(t->dmgteam,"player"))
					if (!(t->monsterinfo.aiflags & AI_GOOD_GUY))
						level.total_monsters--;
			}
			G_FreeEdict (t);
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

//
// fire target
//
	if (target)
	{
		// doors fire area portals in a specific way
		if (!Q_stricmp(target->classname, "func_areaportal") &&
			(!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating") 
			|| !Q_stricmp(ent->classname,"func_door_rot_dh")))
			return;

		if (target == ent)
		{
			gi.dprintf ("WARNING: Entity used itself.\n");
		}
		else
		{
			if (target->use)
				target->use (target, ent, activator);
		}
		if (!ent->inuse)
		{
			gi.dprintf("entity was removed while using target\n");
			return;
		}
	}
}


// Knightmare added
/*
====================
IsIdMap

Checks if the current map is a stock id map,
this is used for certain hacks.
====================
*/
qboolean IsIdMap (void)
{
	if (Q_stricmp(level.mapname, "base1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "base2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "base3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "biggun") == 0)
		return true;
	if (Q_stricmp(level.mapname, "boss1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "boss2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "bunk1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "city1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "city2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "city3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "command") == 0)
		return true;
	if (Q_stricmp(level.mapname, "cool1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "fact1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "fact2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "fact3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "hangar1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "hangar2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "jail1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "jail2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "jail3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "jail4") == 0)
		return true;
	if (Q_stricmp(level.mapname, "jail5") == 0)
		return true;
	if (Q_stricmp(level.mapname, "lab") == 0)
		return true;
	if (Q_stricmp(level.mapname, "mine1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "mine2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "mine3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "mine4") == 0)
		return true;
	if (Q_stricmp(level.mapname, "mintro") == 0)
		return true;
	if (Q_stricmp(level.mapname, "power1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "power2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "security") == 0)
		return true;
	if (Q_stricmp(level.mapname, "space") == 0)
		return true;
	if (Q_stricmp(level.mapname, "strike") == 0)
		return true;
	if (Q_stricmp(level.mapname, "train") == 0)
		return true;
	if (Q_stricmp(level.mapname, "ware1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "ware2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "waste1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "waste2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "waste3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "q2dm1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "q2dm2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "q2dm3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "q2dm4") == 0)
		return true;
	if (Q_stricmp(level.mapname, "q2dm5") == 0)
		return true;
	if (Q_stricmp(level.mapname, "q2dm6") == 0)
		return true;
	if (Q_stricmp(level.mapname, "q2dm7") == 0)
		return true;
	if (Q_stricmp(level.mapname, "q2dm8") == 0)
		return true;
	if (Q_stricmp(level.mapname, "base64") == 0)
		return true;
	if (Q_stricmp(level.mapname, "city64") == 0)
		return true;
	if (Q_stricmp(level.mapname, "sewer64") == 0)
		return true;

	return false;
}


// Knightmare added
/*
====================
IsXatrixMap

Checks if the current map is from the Xatrix mission pack.
This is used for certain hacks.
====================
*/
qboolean IsXatrixMap (void)
{
	if (Q_stricmp(level.mapname, "badlands") == 0)
		return true;
	if (Q_stricmp(level.mapname, "industry") == 0)
		return true;
	if (Q_stricmp(level.mapname, "outbase") == 0)
		return true;
	if (Q_stricmp(level.mapname, "refinery") == 0)
		return true;
	if (Q_stricmp(level.mapname, "w_treat") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xcompnd1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xcompnd2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xhangar1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xhangar2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xintell") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xmoon1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xmoon2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xreactor") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xsewer1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xsewer2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xship") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xswamp") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xware") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xdm1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xdm2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xdm3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xdm4") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xdm5") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xdm6") == 0)
		return true;
	if (Q_stricmp(level.mapname, "xdm7") == 0)
		return true;

	return false;
}


// Knightmare added
/*
====================
IsRogueMap

Checks if the current map is from the Rogue mission pack.
This is used for certain hacks.
====================
*/
qboolean IsRogueMap (void)
{
	if (Q_stricmp(level.mapname, "rammo1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rammo2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rbase1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rbase2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rboss") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rhangar1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rhangar2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rlava1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rlava2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rmine1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rmine2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rsewer1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rsewer2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rware1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rware2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm4") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm5") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm6") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm7") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm8") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm9") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm10") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm11") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm12") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm13") == 0)
		return true;
	if (Q_stricmp(level.mapname, "rdm14") == 0)
		return true;

	return false;
}


// Knightmare added
/*
====================
IsZaeroMap

Checks if the current map is from the Zaero mission pack.
This is used for certain hacks.
====================
*/
qboolean IsZaeroMap (void)
{
	if (Q_stricmp(level.mapname, "zbase1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zbase2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zboss") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zdef1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zdef2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zdef3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zdef4") == 0)
		return true;
	if (Q_stricmp(level.mapname, "ztomb1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "ztomb2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "ztomb3") == 0)
		return true;
	if (Q_stricmp(level.mapname, "ztomb4") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zwaste1") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zwaste2") == 0)
		return true;
	if (Q_stricmp(level.mapname, "zwaste3") == 0)
		return true;
	return false;
}


void my_bprintf (int printlevel, char *fmt, ...)
{
	int i;
	char	bigbuffer[0x10000];
	int		len;
	va_list	argptr;
	edict_t	*cl_ent;

	va_start (argptr, fmt);
	len = Q_vsnprintf (bigbuffer, sizeof(bigbuffer), fmt, argptr);
	va_end (argptr);

	if (dedicated->value)
		safe_cprintf(NULL, printlevel, bigbuffer);

	for (i=0 ; i<maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;

		safe_cprintf(cl_ent, printlevel, bigbuffer);
	}
}

// Knightmare added
/*
====================
UseRegularGoodGuyFlag

Checks the classname to see if a monster should use
the standard goodguy flag (e.g. not a gekk, stalker, turret, or fixbot).
====================
*/
qboolean UseRegularGoodGuyFlag (edict_t *monster)
{
	// check for bad entity reference
	if (!monster || !monster->inuse || !monster->classname)
		return false;

	if ( /*strcmp(monster->classname, "monster_gekk")
		&& strcmp(monster->classname, "monster_stalker")
		&& strcmp(monster->classname, "monster_turret")
		&& strcmp(monster->classname, "monster_fixbot")
		&& strcmp(monster->classname, "monster_handler")
		&&*/ strcmp(monster->classname, "misc_insane") )
		return true;

	return false;
}
