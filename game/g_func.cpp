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

// Lazarus: universal change to all sound-playing pushers. If s.sound is 0, then sound_end is NOT
//          played. Only way s.sound might be 0 is if pusher was blocked in physics code, since
//          all enitities that have a sound_end also have a sound_middle.

void bob_init (edict_t *self);

#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3
#define STATE_LOWEST		4	// Knightmare added

#define PLAT_LOW_TRIGGER	1
// Knightmare added
#define PLAT_TOGGLE			2
#define PLAT_TOP			4
#define PLAT_TRIGGER_TOP	8
#define PLAT_TRIGGER_BOTTOM	16
#define PLAT_BOX_LIFT		32
// end Knightmare

#define DOOR_START_OPEN		1
#define DOOR_REVERSE		2
#define DOOR_CRUSHER		4
#define DOOR_NOMONSTER		8
#define DOOR_TOGGLE			32
#define DOOR_X_AXIS			64
#define DOOR_Y_AXIS			128

#define SF_DOOR_DESTROY     2048

void Move_Done (edict_t *ent);
void plat_go_down (edict_t *ent);
void plat_go_up (edict_t *ent);
void plat2_go_down (edict_t *ent);
void plat2_go_up (edict_t *ent);
void train_children_think (edict_t *self);
void train_blocked (edict_t *self, edict_t *other);

/*
==============
set_child_movement

Sets movement and angles of child entities for a func_train
or any other entity that supports movewith.
==============
*/
void set_child_movement (edict_t *self)
{
	edict_t *e;
	edict_t	*prev;
	vec3_t	forward, right, up;
	vec3_t	angles, amove;
	vec3_t	offset;
	vec3_t	delta_angles;
	qboolean	is_monster;

	if (!self->inuse)
		return;
	if (self->classname.empty())
		return;
	if (!self->movewith_next)
		return;

	e = self->movewith_next;
	prev = self;
	while (e != nullptr )
//	for (e = self->movewith_next; e != NULL; e = e->movewith_next)
	{
restart:
		if (!e->inuse) break;
	//	if (!e->inuse) continue;

		// Get change in parent's angles from when child was attached, this tells us how far we need to rotate
		VectorSubtract (self->s.angles, e->parent_attach_angles, delta_angles);
		AngleVectors (delta_angles, forward, right, up);
		VectorNegate (right, right);

		// remove gibbed monsters from the chain
		if (e->svflags & SVF_MONSTER)
		{
			if (e->health <= e->gib_health)
			{
				prev->movewith_next = e->movewith_next;
				e = e->movewith_next;
				if (e)
					goto restart;
				else
					break;
			}
			is_monster = true;
		}
		else
			is_monster = false;

		// For all but func_button and func_door, move origin and match velocities
	//	if (Q_stricmp(e->classname, "func_door") && Q_stricmp(e->classname, "func_button"))
		if ( strcmp(e->classname.c_str(), "func_door") && strcmp(e->classname.c_str(), "func_button")
		//	&& strcmp(e->classname, "func_door_secret") && strcmp(e->classname, "func_door_secret2")
			&& (e->class_id != ENTITY_FUNC_DOOR_SECRET) && strcmp(e->classname.c_str(), "func_door_secret2")
			&& strcmp(e->classname.c_str(), "func_plat") && strcmp(e->classname.c_str(), "func_plat2")
			&& strcmp(e->classname.c_str(), "func_water") )
		{
			VectorMA (self->s.origin, e->movewith_offset[0], forward, e->s.origin);
			VectorMA (e->s.origin,    e->movewith_offset[1], right,   e->s.origin);
			VectorMA (e->s.origin,    e->movewith_offset[2], up,      e->s.origin);
			VectorCopy (self->velocity, e->velocity);
		}

		// If parent is spinning, add appropriate velocities
		VectorSubtract(e->s.origin, self->s.origin, offset);
		if (self->avelocity[PITCH] != 0) {
			e->velocity[2] -= offset[0] * self->avelocity[PITCH] * M_PI / 180;
			e->velocity[0] += offset[2] * self->avelocity[PITCH] * M_PI / 180;
		}
		if (self->avelocity[YAW] != 0) {
			e->velocity[0] -= offset[1] * self->avelocity[YAW] * M_PI / 180.;
			e->velocity[1] += offset[0] * self->avelocity[YAW] * M_PI / 180.;
		}
		if (self->avelocity[ROLL] != 0) {
			e->velocity[1] -= offset[2] * self->avelocity[ROLL] * M_PI / 180;
			e->velocity[2] += offset[1] * self->avelocity[ROLL] * M_PI / 180;
		}

		VectorScale(self->avelocity, FRAMETIME, amove);
		if (self->turn_rider)
		{
			// Match angular velocities
			if (!Q_stricmp( e->classname.c_str(), "func_rotating" ) )
			{
				float cy = std::cos( ( e->s.angles[ 1 ] - delta_angles[ 1 ] ) * M_PI / 180.0f );
				float sy = std::sin( ( e->s.angles[ 1 ] - delta_angles[ 1 ] ) * M_PI / 180.0f );
				if (e->movedir[0] > 0) {
					e->s.angles[1] = delta_angles[1];
				}
				else if (e->movedir[1] > 0) {
					e->s.angles[1] += amove[1];
					e->s.angles[2] =  delta_angles[2]*cy;
					e->s.angles[0] = -delta_angles[2]*sy;
				}
				else if (e->movedir[2] > 0) {
					e->s.angles[1] = delta_angles[0]*-sy;
				}
			}
			else if ( !is_monster )
			{	// Not a monster/actor. We want monsters/actors to be able to turn on
				// their own.
				if ( !e->do_not_rotate )
				{
					if (!Q_stricmp( e->classname.c_str(), "turret_breach" ) || !Q_stricmp( e->classname.c_str(), "turret_base" ) )
					{
						VectorCopy (self->avelocity, e->avelocity);
					}
					else if (!Q_stricmp( e->classname.c_str(), "func_door_rotating" ) )
					{
						VectorCopy (self->avelocity,e->avelocity);
					//	VectorCopy (delta_angles, e->pos1);
						VectorAdd (e->child_attach_angles, delta_angles, e->pos1);
						VectorMA (e->pos1, e->moveinfo.distance, e->movedir, e->pos2);
						if (e->moveinfo.state == STATE_TOP)
							VectorCopy (e->pos2, e->s.angles);
						else
							VectorCopy (e->pos1, e->s.angles);
						VectorCopy (e->s.origin, e->moveinfo.start_origin);
						VectorCopy (e->pos1,     e->moveinfo.start_angles);
						VectorCopy (e->s.origin, e->moveinfo.end_origin);
						VectorCopy (e->pos2,     e->moveinfo.end_angles);
					}
					else if (e->solid == SOLID_BSP)
					{
						// Brush models always start out with angles=0,0,0 (after G_SetMoveDir).
						// Use more accuracy here.
						VectorCopy (self->avelocity, e->avelocity);
					//	VectorCopy (delta_angles, e->s.angles);
						VectorAdd (e->child_attach_angles, delta_angles, e->s.angles);
					}
					else if (e->movetype == MOVETYPE_NONE)
					{
						VectorCopy (self->avelocity, e->avelocity);
					//	VectorCopy (delta_angles, e->s.angles);
						VectorAdd (e->child_attach_angles, delta_angles, e->s.angles);
					}
					else
					{
						// For point entities, best we can do is apply a delta to
						// the angles. This may result in foulups if anything gets blocked
					//	VectorAdd(e->s.angles, amove, e->s.angles);
						VectorAdd (e->child_attach_angles, delta_angles, e->s.angles);
					}
				}
			}
			// Knightmare- monsters that need special rotation
			// manipulation would be handled here
		/*	else if (is_monster)
			{
			}*/
		}

		// Special cases:
		// Func_door/func_button and trigger fields
	//	if ( (!Q_stricmp(e->classname, "func_door"  )) || (!Q_stricmp(e->classname, "func_button")) )
			if ( !strcmp(e->classname.c_str(), "func_door") || !strcmp(e->classname.c_str(), "func_button")
			//	|| !strcmp(e->classname, "func_door_secret") || !strcmp(e->classname, "func_door_secret2")
				|| (e->class_id == ENTITY_FUNC_DOOR_SECRET) || !strcmp(e->classname.c_str(), "func_door_secret2")
				|| !strcmp(e->classname.c_str(), "func_plat") || !strcmp(e->classname.c_str(), "func_plat2")
				|| !strcmp(e->classname.c_str(), "func_water") )
		{
			VectorAdd (e->s.angles, e->org_angles, angles);
			G_SetMovedir (angles, e->movedir);

			// Knightmare- these entities need special calculations
		//	if (!strcmp(e->classname, "func_door_secret"))
			if (e->class_id == ENTITY_FUNC_DOOR_SECRET)
			{
				vec3_t	eforward, eright, eup;

				AngleVectors (e->s.angles, eforward, eright, eup);
				VectorMA (self->s.origin, e->movewith_offset[0], forward, e->pos0);
				VectorMA (e->pos0, e->movewith_offset[1], right, e->pos0);
				VectorMA (e->pos0,  e->movewith_offset[2], up, e->pos0);
				if (e->spawnflags & 4) // SECRET_1ST_DOWN
					VectorMA (e->pos0, -1 * e->width, eup, e->pos1);
				else
					VectorMA (e->pos0, e->side * e->width, eright, e->pos1);
				VectorMA (e->pos1, e->length, eforward, e->pos2);
			}
			else if (!strcmp(e->classname.c_str(), "func_door_secret2"))
			{
				vec3_t	eforward, eright, eup;

				AngleVectors (e->s.angles, eforward, eright, eup);
				VectorMA (self->s.origin, e->movewith_offset[0], forward, e->pos0);
				VectorMA (e->pos0, e->movewith_offset[1], right, e->pos0);
				VectorMA (e->pos0,  e->movewith_offset[2], up, e->pos0);
				if (e->spawnflags & 64) // SECDR2_MOVE_FORWARD
					VectorScale (eforward, e->length, eforward);
				else
					VectorScale (eforward, e->length * -1 , eforward);
				if (e->spawnflags & 32) // SECDR2_MOVE_RIGHT
					VectorScale (eright, e->width, eright);
				else
					VectorScale (eright, e->width * -1, eright);
				if (e->spawnflags & 4) // SECDR2_1ST_DOWN
				{
					VectorAdd (e->pos0, eforward, e->pos1);
					VectorAdd (e->pos1, eright, e->pos2);
				}
				else
				{
					VectorAdd (e->pos0, eright, e->pos1);
					VectorAdd (e->pos1, eforward, e->pos2);
				}
			}
			else if (!strcmp(e->classname.c_str(), "func_plat") || !strcmp(e->classname.c_str(), "func_plat2"))
			{
				VectorMA (self->s.origin, e->movewith_offset[0], forward, e->pos1);
				VectorMA (e->pos1, e->movewith_offset[1], right, e->pos1);
				VectorMA (e->pos1,  e->movewith_offset[2], up, e->pos1);
				VectorCopy (e->pos1, e->pos2);
				e->pos2[2] -= e->moveinfo.distance;
				VectorCopy (e->pos1, e->moveinfo.start_origin);
				VectorCopy (e->s.angles, e->moveinfo.start_angles);
				VectorCopy (e->pos2, e->moveinfo.end_origin);
				VectorCopy (e->s.angles, e->moveinfo.end_angles);
			}
			else // func_button or func_door or func_water
			{
				VectorMA (self->s.origin, e->movewith_offset[0], forward, e->pos1);
				VectorMA (e->pos1,        e->movewith_offset[1], right,   e->pos1);
				VectorMA (e->pos1,        e->movewith_offset[2], up,      e->pos1);
				VectorMA (e->pos1, e->moveinfo.distance, e->movedir, e->pos2);
				VectorCopy (e->pos1,     e->moveinfo.start_origin);
				VectorCopy (e->s.angles, e->moveinfo.start_angles);
				VectorCopy (e->pos2,     e->moveinfo.end_origin);
				VectorCopy (e->s.angles, e->moveinfo.end_angles);
			}

			if (e->moveinfo.state == STATE_LOWEST ||
				e->moveinfo.state == STATE_BOTTOM || e->moveinfo.state == STATE_TOP)
			{
				// Velocities for door/button movement are handled in normal
				// movement routines
				VectorCopy (self->velocity, e->velocity);
				// Sanity insurance:
				if (!strcmp(e->classname.c_str(), "func_plat") || !strcmp(e->classname.c_str(), "func_plat2"))
				{	// top and bottom states are reversed for plats
					if (e->moveinfo.state == STATE_BOTTOM)
						VectorCopy (e->pos2, e->s.origin);
					else if (e->moveinfo.state == STATE_TOP)
						VectorCopy (e->pos1, e->s.origin);
				}
				else
				{	// STATE_LOWEST is for func_door_secret in its starting postion
					if (e->moveinfo.state == STATE_LOWEST)
						VectorCopy (e->pos0, e->s.origin);
					else if (e->moveinfo.state == STATE_BOTTOM)
						VectorCopy (e->pos1, e->s.origin);
					else if (e->moveinfo.state == STATE_TOP)
						VectorCopy (e->pos2, e->s.origin);
				}
			}
		}

		if (amove[YAW])
		{
			// Cross fingers here... move bounding boxes of doors and buttons
		//	if ( (!Q_stricmp(e->classname, "func_door"  )) || (!Q_stricmp(e->classname, "func_button")) ||
		//		(e->solid == SOLID_TRIGGER) )
			if ( !strcmp(e->classname.c_str(), "func_door") || !strcmp(e->classname.c_str(), "func_button")
				|| (e->class_id == ENTITY_FUNC_DOOR_SECRET) || !strcmp(e->classname.c_str(), "func_door_secret2")
				|| !strcmp(e->classname.c_str(), "func_plat") || !strcmp(e->classname.c_str(), "func_plat2")
				|| !strcmp(e->classname.c_str(), "func_water") || (e->solid == SOLID_TRIGGER) )
			{
				float		ca, sa, yaw;
				vec3_t		p00, p01, p10, p11;

				// Adjust bounding box for yaw
				yaw = e->s.angles[YAW] * M_PI / 180.;
				ca  = cos(yaw);
				sa  = sin(yaw);
				p00[0] = e->org_mins[0]*ca - e->org_mins[1]*sa;
				p00[1] = e->org_mins[1]*ca + e->org_mins[0]*sa;
				p01[0] = e->org_mins[0]*ca - e->org_maxs[1]*sa;
				p01[1] = e->org_maxs[1]*ca + e->org_mins[0]*sa;
				p10[0] = e->org_maxs[0]*ca - e->org_mins[1]*sa;
				p10[1] = e->org_mins[1]*ca + e->org_maxs[0]*sa;
				p11[0] = e->org_maxs[0]*ca - e->org_maxs[1]*sa;
				p11[1] = e->org_maxs[1]*ca + e->org_maxs[0]*sa;
				e->mins[0] = p00[0];
				e->mins[0] = min(e->mins[0],p01[0]);
				e->mins[0] = min(e->mins[0],p10[0]);
				e->mins[0] = min(e->mins[0],p11[0]);
				e->mins[1] = p00[1];
				e->mins[1] = min(e->mins[1],p01[1]);
				e->mins[1] = min(e->mins[1],p10[1]);
				e->mins[1] = min(e->mins[1],p11[1]);
				e->maxs[0] = p00[0];
				e->maxs[0] = max(e->maxs[0],p01[0]);
				e->maxs[0] = max(e->maxs[0],p10[0]);
				e->maxs[0] = max(e->maxs[0],p11[0]);
				e->maxs[1] = p00[1];
				e->maxs[1] = max(e->maxs[1],p01[1]);
				e->maxs[1] = max(e->maxs[1],p10[1]);
				e->maxs[1] = max(e->maxs[1],p11[1]);
			}
		}

		e->s.event = self->s.event;
		gi.linkentity(e);
		prev = e;
		e = e->movewith_next;
	}
}

void movewith_update (edict_t *self)
{
	if (self->moveinfo.state == STATE_BOTTOM) {
		VectorCopy (self->s.origin, self->pos1);
		VectorMA (self->pos1, self->moveinfo.distance, self->movedir, self->pos2);
	}
	else if (self->moveinfo.state == STATE_TOP) {
		VectorCopy (self->s.origin, self->pos2);
		VectorMA( self->pos2, -self->moveinfo.distance, self->movedir, self->pos1);
	}
	VectorCopy (self->pos1, self->moveinfo.start_origin);
	VectorCopy (self->pos2, self->moveinfo.end_origin);
}

void spline_calc (edict_t *train, vec3_t p1, vec3_t p2, vec3_t a1, vec3_t a2, float m, vec3_t p, vec3_t a)
{
	/* p1, p2  =  origins of path_* ents
	   a1, a2  =  angles from path_* ents
	   m       =  decimal position along curve */

	vec3_t v1, v2; // direction vectors
	vec3_t c1, c2; // control-point coords
	vec3_t d, v;   // temps
	float  s;      // vector scale
	// these greatly simplify/speed up equations
	// (make sure m is already assigned a value)
	float n     = 1.0 - m;
	float m2    = m * m;
	float m3    = m2 * m;
	float n2    = n * n;
	float n3    = n2 * n;
	float mn2_3 = m * n2 * 3;
	float m2n_3 = m2 * n * 3;
	float mn_2  = m * n * 2;

	// Beziers need two control-points to define the shape of the curve.
	// These can be created from the available data.  They are offset a
	// specific distance from the endpoints (path_*s), in the direction
	// of the endpoints' angle vectors (the 2nd control-point is offset in
	// the opposite direction).  The distance used is a fraction of the total
	// distance between the endpoints, ensuring it's scaled proportionally.
	// The factor of 0.4 is simply based on experimentation, as a value that
	// yields nice even curves.

	AngleVectors(a1, v1, nullptr, nullptr );
	AngleVectors(a2, v2, nullptr, nullptr );

	VectorSubtract(p2, p1, d);
	s = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) * 0.4;

	VectorMA(p1,  s, v1, c1);
	VectorMA(p2, -s, v2, c2);

	// cubic interpolation of the four points
	// gives the position along the curve
	p[0] = n3 * p1[0] + mn2_3 * c1[0] + m2n_3 * c2[0] + m3 * p2[0];
	p[1] = n3 * p1[1] + mn2_3 * c1[1] + m2n_3 * c2[1] + m3 * p2[1];
	p[2] = n3 * p1[2] + mn2_3 * c1[2] + m2n_3 * c2[2] + m3 * p2[2];

	// should be optional:
	// first derivative of bezier formula provides direction vector
	// along the curve (equation simplified in terms of m & n)
	v[0] = (n2 * p1[0] - (n2 - mn_2) * c1[0] - (mn_2 - m2) * c2[0] - m2 * p2[0]) / -s;
	v[1] = (n2 * p1[1] - (n2 - mn_2) * c1[1] - (mn_2 - m2) * c2[1] - m2 * p2[1]) / -s;
	v[2] = (n2 * p1[2] - (n2 - mn_2) * c1[2] - (mn_2 - m2) * c2[2] - m2 * p2[2]) / -s;
	vectoangles2 (v, a);
	if (train->roll_speed > 0)	// Knightmare added
		a[ROLL] = a1[ROLL] + m*(a2[ROLL] - a1[ROLL]);
}

void train_wait (edict_t *self);
void train_spline (edict_t *self)
{
	edict_t	*train;
	vec3_t	p;
	vec3_t	a;

	train = self->enemy;
	if (!train || !train->inuse)
		return;
	// Knightmare- check for killtargeted path_corners
	if ((!train->from) || (!train->from->inuse) || (!train->to) || (!train->to->inuse))
		return;
	// Knightmare- check for removed spline flag- get da hell outta here
	if ( !(train->spawnflags & TRAIN_SPLINE) )
	{
		self->think = train_children_think;
		return;
	}

	if ( (train->from != train->to) && !train->moveinfo.is_blocked && (train->spawnflags & TRAIN_START_ON))
	{
		if (train->moveinfo.ratio >= 1.0f) // Knightmare- don't keep moving at end of curve
		{
			VectorClear(self->avelocity);
			VectorClear(self->velocity);
			self->nextthink = level.time + FRAMETIME;
			return;
		}

		spline_calc (train, train->from->s.origin, train->to->s.origin,
		                    train->from->s.angles, train->to->s.angles,
		 				    train->moveinfo.ratio, p, a);
		if ( !(train->spawnflags & TRAIN_ORIGIN) ) // Knightmare- func_train_origin support
			VectorSubtract(p,train->mins,p);
		VectorSubtract(p,train->s.origin,train->velocity);
		VectorScale(train->velocity,10,train->velocity);
		VectorSubtract(a,train->s.angles,train->avelocity);
		VectorScale(train->avelocity,10,train->avelocity);
		if (train->pitch_speed < 0)
			train->avelocity[PITCH] = 0;
		if (train->yaw_speed < 0)
			train->avelocity[YAW] = 0;
		gi.linkentity(train);
		train->moveinfo.ratio += train->moveinfo.speed * FRAMETIME / train->moveinfo.distance;

#ifndef POSTTHINK_CHILD_MOVEMENT
		if (train->movewith_next && (train->movewith_next->movewith_ent == train))
			set_child_movement (train);
#endif	// POSTTHINK_CHILD_MOVEMENT

		if (train->moveinfo.ratio >= 1.0f)
		{
			train->moveinfo.endfunc = nullptr;
			train->think = train_wait;
			train->nextthink = level.time + FRAMETIME;
		}
	}
	self->nextthink = level.time + FRAMETIME;
}

void check_reverse_rotation (edict_t *self, vec3_t point)
{
	vec3_t	vec;
	vec3_t	vnext;
	vec_t	rotation;
	vec_t	cross;

	if ( !(self->flags & FL_REVERSIBLE) )
		return;

	VectorSubtract (point, self->s.origin, vec);
	VectorCopy (self->move_origin, vnext);
	VectorNormalize (vec);
	VectorNormalize (vnext);

	// Knightmare- use all 3 movedir axis if axis is set manually via move_angles
	if (VectorLength(self->move_angles) > 0.0f)
	{
		vec3_t	rotation_c, cross_c;
		int		i;

		rotation_c[ROLL] = self->moveinfo.distance * self->movedir[ROLL];
		cross_c[ROLL] = vec[1]*vnext[2] - vec[2]*vnext[1];

		rotation_c[PITCH] = self->moveinfo.distance * self->movedir[PITCH];
		cross_c[PITCH] = vec[2]*vnext[0] - vec[0]*vnext[2];

		rotation_c[YAW] = self->moveinfo.distance * self->movedir[YAW];
		cross_c[YAW] = vec[0]*vnext[1] - vec[1]*vnext[0];

		// Use largest of rotation values
		rotation = cross = 0;
		for (i = 0; i < 3; i++)
		{
			if ( fabs(rotation_c[i]) > fabs(rotation) ) {
				rotation = rotation_c[i];
				cross = cross_c[i];
			}
		}
	}
	else
	{
		if (self->spawnflags & DOOR_X_AXIS)
		{
			rotation = self->moveinfo.distance * self->movedir[ROLL];
			cross = vec[1]*vnext[2] - vec[2]*vnext[1];
		}
		else if (self->spawnflags & DOOR_Y_AXIS)
		{
			rotation = self->moveinfo.distance * self->movedir[PITCH];
			cross = vec[2]*vnext[0] - vec[0]*vnext[2];
		}
		else
		{
			rotation = self->moveinfo.distance * self->movedir[YAW];
			cross = vec[0]*vnext[1] - vec[1]*vnext[0];
		}
	}
	// end Knightmare

	if ((self->spawnflags & 1) && (DotProduct(vec, vnext) < 0))
		cross = -cross;

	if ( (cross < 0) && (rotation > 0) )
	{
		VectorNegate (self->movedir, self->movedir);
		VectorMA (self->pos1, self->moveinfo.distance, self->movedir, self->pos2);
		VectorCopy (self->pos2, self->moveinfo.end_angles);
	}
	else if ((cross > 0) && (rotation < 0))
	{
		VectorNegate (self->movedir, self->movedir);
		VectorMA (self->pos1, self->moveinfo.distance, self->movedir, self->pos2);
		VectorCopy (self->pos2, self->moveinfo.end_angles);
	}

}

/*
=========================================================

  PLATS

  movement options:

  linear
  smooth start, hard stop
  smooth start, smooth stop

  start
  end
  acceleration
  speed
  deceleration
  begin sound
  end sound
  target fired when reaching end
  wait at end

  object characteristics that use move segments
  ---------------------------------------------
  movetype_push, or movetype_stop
  action when touched
  action when blocked
  action when used
	disabled?
  auto trigger spawning


=========================================================
*/

//
// Support routines for movement (changes in origin using velocity)
//

//
// Knightmare- smart functions for child movement
// These constantly check the updated destination point
//

/*void Move_AdjustForParentVelocity (edict_t *ent)
{
	vec3_t	org_offset;

	if (!ent || !ent->movewith_ent)
		return;

	VectorAdd (ent->movewith_ent->velocity, ent->velocity, ent->velocity);

	// If parent is spinning, add appropriate velocities
	VectorSubtract(ent->s.origin, ent->movewith_ent->s.origin, org_offset);
	if (ent->movewith_ent->avelocity[PITCH] != 0)
	{
		ent->velocity[2] -= org_offset[0] * ent->movewith_ent->avelocity[PITCH] * M_PI / 180;
		ent->velocity[0] += org_offset[2] * ent->movewith_ent->avelocity[PITCH] * M_PI / 180;
	}
	if (ent->movewith_ent->avelocity[YAW] != 0)
	{
		ent->velocity[0] -= org_offset[1] * ent->movewith_ent->avelocity[YAW] * M_PI / 180.;
		ent->velocity[1] += org_offset[0] * ent->movewith_ent->avelocity[YAW] * M_PI / 180.;
	}
	if (ent->movewith_ent->avelocity[ROLL] != 0)
	{
		ent->velocity[1] -= org_offset[2] * ent->movewith_ent->avelocity[ROLL] * M_PI / 180;
		ent->velocity[2] += org_offset[1] * ent->movewith_ent->avelocity[ROLL] * M_PI / 180;
	}
}*/

void Move_pos0_Final (edict_t *ent)
{
	if (!ent->movewith_ent)
		return;

//	gi.dprintf ("Move_pos0_Final\n");

	VectorSubtract (ent->pos0, ent->s.origin, ent->moveinfo.dir);
	ent->moveinfo.remaining_distance = VectorNormalize (ent->moveinfo.dir);
	if (ent->moveinfo.remaining_distance == 0)
	{
		Move_Done (ent);
		return;
	}
	VectorScale (ent->moveinfo.dir, ent->moveinfo.remaining_distance / FRAMETIME, ent->velocity);
	VectorAdd (ent->movewith_ent->velocity, ent->velocity, ent->velocity);
//	Move_AdjustForParentVelocity (ent);

	ent->think = Move_Done;
	ent->nextthink = level.time + FRAMETIME;
}

void Move_pos0_Think (edict_t *ent)
{
	if (!ent->movewith_ent)
		return;

//	gi.dprintf ("Move_pos0_Think\n");

	VectorSubtract (ent->pos0, ent->s.origin, ent->moveinfo.dir);
	ent->moveinfo.remaining_distance = VectorNormalize (ent->moveinfo.dir);
	if ((ent->moveinfo.speed * FRAMETIME) >= ent->moveinfo.remaining_distance)
	{
		Move_pos0_Final (ent);
		return;
	}
	VectorScale (ent->moveinfo.dir, ent->moveinfo.speed, ent->velocity);
	VectorAdd (ent->movewith_ent->velocity, ent->velocity, ent->velocity);
//	Move_AdjustForParentVelocity (ent);

	ent->nextthink = level.time + FRAMETIME;
	ent->think = Move_pos0_Think;
}

void Move_pos1_Final (edict_t *ent)
{
	if (!ent->movewith_ent)
		return;

//	gi.dprintf ("Move_pos1_Final\n");

	VectorSubtract (ent->pos1, ent->s.origin, ent->moveinfo.dir);
	ent->moveinfo.remaining_distance = VectorNormalize (ent->moveinfo.dir);
	if (ent->moveinfo.remaining_distance == 0)
	{
		Move_Done (ent);
		return;
	}
	VectorScale (ent->moveinfo.dir, ent->moveinfo.remaining_distance / FRAMETIME, ent->velocity);
	VectorAdd (ent->movewith_ent->velocity, ent->velocity, ent->velocity);
//	Move_AdjustForParentVelocity (ent);

	ent->think = Move_Done;
	ent->nextthink = level.time + FRAMETIME;
}

void Move_pos1_Think (edict_t *ent)
{
	if (!ent->movewith_ent)
		return;

//	gi.dprintf ("Move_pos1_Think\n");

	VectorSubtract (ent->pos1, ent->s.origin, ent->moveinfo.dir);
	ent->moveinfo.remaining_distance = VectorNormalize (ent->moveinfo.dir);
	if ((ent->moveinfo.speed * FRAMETIME) >= ent->moveinfo.remaining_distance)
	{
		Move_pos1_Final (ent);
		return;
	}
	VectorScale (ent->moveinfo.dir, ent->moveinfo.speed, ent->velocity);
	VectorAdd (ent->movewith_ent->velocity, ent->velocity, ent->velocity);
//	Move_AdjustForParentVelocity (ent);

	ent->nextthink = level.time + FRAMETIME;
	ent->think = Move_pos1_Think;
}

void Move_pos2_Final (edict_t *ent)
{
	if (!ent->movewith_ent)
		return;

//	gi.dprintf ("Move_pos2_Final\n");

	VectorSubtract (ent->pos2, ent->s.origin, ent->moveinfo.dir);
	ent->moveinfo.remaining_distance = VectorNormalize (ent->moveinfo.dir);
	if (ent->moveinfo.remaining_distance == 0)
	{
		Move_Done (ent);
		return;
	}
	VectorScale (ent->moveinfo.dir, ent->moveinfo.remaining_distance / FRAMETIME, ent->velocity);
	VectorAdd (ent->movewith_ent->velocity, ent->velocity, ent->velocity);
//	Move_AdjustForParentVelocity (ent);

	ent->think = Move_Done;
	ent->nextthink = level.time + FRAMETIME;
}

void Move_pos2_Think (edict_t *ent)
{
	if (!ent->movewith_ent)
		return;

//	gi.dprintf ("Move_pos2_Think\n");

	VectorSubtract (ent->pos2, ent->s.origin, ent->moveinfo.dir);
	ent->moveinfo.remaining_distance = VectorNormalize (ent->moveinfo.dir);
	if ((ent->moveinfo.speed * FRAMETIME) >= ent->moveinfo.remaining_distance)
	{
		Move_pos2_Final (ent);
		return;
	}
	VectorScale (ent->moveinfo.dir, ent->moveinfo.speed, ent->velocity);
	VectorAdd (ent->movewith_ent->velocity, ent->velocity, ent->velocity);
//	Move_AdjustForParentVelocity (ent);

	ent->nextthink = level.time + FRAMETIME;
	ent->think = Move_pos2_Think;
}

void Move_Done (edict_t *ent)
{
	VectorClear (ent->velocity);
	if (ent->movewith && ent->movewith_ent) {
		VectorCopy (ent->movewith_ent->velocity, ent->velocity);
	}
	if (ent->moveinfo.endfunc)
		ent->moveinfo.endfunc (ent);

#ifndef POSTTHINK_CHILD_MOVEMENT
	if (ent->movewith_next && (ent->movewith_next->movewith_ent == ent))
		set_child_movement (ent);
#endif	// POSTTHINK_CHILD_MOVEMENT
}

void Move_Final (edict_t *ent)
{
	if (ent->moveinfo.remaining_distance == 0 || ent->smooth_movement)
	{
		Move_Done (ent);
		return;
	}

	// velocity = direction * distance * 10
	// so we always reach the goal in 0.1 seconds
	VectorScale (ent->moveinfo.dir, ent->moveinfo.remaining_distance / FRAMETIME, ent->velocity);
	if (ent->movewith)
		VectorAdd(ent->movewith_ent->velocity,ent->velocity,ent->velocity);

	ent->think = Move_Done;
	ent->nextthink = level.time + FRAMETIME;

#ifndef POSTTHINK_CHILD_MOVEMENT
	if (ent->movewith_next && (ent->movewith_next->movewith_ent == ent))
		set_child_movement (ent);
#endif	// POSTTHINK_CHILD_MOVEMENT
}

void Move_Begin (edict_t *ent)
{
	float	frames;

	// if we're within one timestamp of reaching the goal, go to Move_Final
	if ((ent->moveinfo.speed * FRAMETIME) >= ent->moveinfo.remaining_distance)
	{
		Move_Final (ent);
		return;
	}
	VectorScale (ent->moveinfo.dir, ent->moveinfo.speed, ent->velocity);

	// set object's velocity to direction times speed
	if (ent->movewith && ent->movewith_ent)
	{
		VectorAdd (ent->movewith_ent->velocity, ent->velocity, ent->velocity);
		ent->moveinfo.remaining_distance -= ent->moveinfo.speed * FRAMETIME;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = Move_Begin;
	}
	else
	{
		// if func_train is moving toward a moving path_corner
		if (!strcmp(ent->classname.c_str(), "func_train") && ent->target_ent->movewith)
		{
			vec3_t		dest;
			if (ent->spawnflags & TRAIN_ORIGIN)	// Knightmare- func_train_origin support
				VectorCopy (ent->target_ent->s.origin, dest);
			else
				VectorSubtract (ent->target_ent->s.origin, ent->mins, dest);
			VectorCopy (ent->s.origin, ent->moveinfo.start_origin);
			VectorCopy (dest, ent->moveinfo.end_origin);
			if (ent->spawnflags & TRAIN_ROTATE && !(ent->target_ent->spawnflags & 2))
			{
				vec3_t	v, angles;
				if (ent->spawnflags & TRAIN_ORIGIN) {	// Knightmare- func_train_origin support
					VectorSubtract(ent->target_ent->s.origin, ent->s.origin, v);
				}
				else {
					VectorAdd(ent->s.origin,ent->mins,v);
					VectorSubtract(ent->target_ent->s.origin,v,v);
				}
				vectoangles2 (v, angles);
				ent->ideal_yaw = angles[YAW];
				ent->ideal_pitch = angles[PITCH];
				if (ent->ideal_pitch < 0) ent->ideal_pitch += 360;
				VectorClear (ent->movedir);
				ent->movedir[1] = 1.0;
			}
			VectorSubtract (dest, ent->s.origin, ent->moveinfo.dir);
			ent->moveinfo.remaining_distance = VectorNormalize (ent->moveinfo.dir);
			VectorScale (ent->moveinfo.dir, ent->moveinfo.speed, ent->velocity);
			ent->nextthink = level.time + FRAMETIME;
			ent->think = Move_Begin;
		}
		else
		{
			frames = floor((ent->moveinfo.remaining_distance / ent->moveinfo.speed) / FRAMETIME);
			ent->moveinfo.remaining_distance -= frames * ent->moveinfo.speed * FRAMETIME;
			ent->nextthink = level.time + (frames * FRAMETIME);
			ent->think = Move_Final;
		}
	}

#ifndef POSTTHINK_CHILD_MOVEMENT
	if (ent->movewith_next && (ent->movewith_next->movewith_ent == ent))
		set_child_movement (ent);
#endif	// POSTTHINK_CHILD_MOVEMENT
}

void Think_AccelMove (edict_t *ent);

void Move_Calc (edict_t *ent, vec3_t dest, void(*func)(edict_t*))
{
	VectorClear (ent->velocity);
	if (ent->movewith && ent->movewith_ent)
	{
		VectorCopy (ent->movewith_ent->velocity, ent->velocity);
	}
	VectorSubtract (dest, ent->s.origin, ent->moveinfo.dir);
	ent->moveinfo.remaining_distance = VectorNormalize (ent->moveinfo.dir);
	ent->moveinfo.endfunc = func;

	// Knightmare- use smart functions for child movement
	if (ent->movewith && ent->movewith_ent)
	{
		if (VectorCompare(dest, ent->pos0))
			Move_pos0_Think (ent);
		else if (VectorCompare(dest, ent->pos1))
			Move_pos1_Think (ent);
		else if (VectorCompare(dest, ent->pos2))
			Move_pos2_Think (ent);
		else
			Move_Begin (ent);
	}
	else
	// end Knightmare
	if (ent->moveinfo.speed == ent->moveinfo.accel && ent->moveinfo.speed == ent->moveinfo.decel)
	{
		if (level.current_entity == ((ent->flags & FL_TEAMSLAVE) ? ent->teammaster : ent))
		{
			Move_Begin (ent);
		}
		else if (ent->movewith) {
			Move_Begin (ent);
		}
		else // wait 0.1 second to start moving
		{
			ent->nextthink = level.time + FRAMETIME;
			ent->think = Move_Begin;
		}
	}
	else
	{
		// accelerative
		ent->moveinfo.current_speed = 0;
		ent->think = Think_AccelMove;
		ent->nextthink = level.time + FRAMETIME;
	}
}


//
// Support routines for angular movement (changes in angle using avelocity)
//

void AngleMove_Done (edict_t *ent)
{
	VectorClear (ent->avelocity);
	if (ent->moveinfo.endfunc)
		ent->moveinfo.endfunc (ent);
}

void AngleMove_Final (edict_t *ent)
{
	vec3_t	move;

	if (ent->moveinfo.state == STATE_UP)
		VectorSubtract (ent->moveinfo.end_angles, ent->s.angles, move);
	else
		VectorSubtract (ent->moveinfo.start_angles, ent->s.angles, move);

	if (VectorCompare (move, vec3_origin))
	{
		AngleMove_Done (ent);
		return;
	}

	VectorScale (move, 1.0/FRAMETIME, ent->avelocity);
	ent->think = AngleMove_Done;
	ent->nextthink = level.time + FRAMETIME;
}

void AngleMove_Begin (edict_t *ent)
{
	vec3_t	destdelta;
	float	len;
	float	traveltime;
	float	frames;

	// set destdelta to the vector needed to move
	if (ent->moveinfo.state == STATE_UP)
		VectorSubtract (ent->moveinfo.end_angles, ent->s.angles, destdelta);
	else
		VectorSubtract (ent->moveinfo.start_angles, ent->s.angles, destdelta);

	// calculate length of vector
	len = VectorLength (destdelta);

	// divide by speed to get time to reach dest
	traveltime = len / ent->moveinfo.speed;

	if (traveltime < FRAMETIME)
	{
		AngleMove_Final (ent);
		return;
	}

	frames = floor(traveltime / FRAMETIME);

	// scale the destdelta vector by the time spent traveling to get velocity
	VectorScale (destdelta, 1.0 / traveltime, ent->avelocity);
	// set nextthink to trigger a think when dest is reached
	ent->nextthink = level.time + frames * FRAMETIME;
	ent->think = AngleMove_Final;
}

void AngleMove_Calc (edict_t *ent, void(*func)(edict_t*))
{
	VectorClear (ent->avelocity);
	ent->moveinfo.endfunc = func;
	if (level.current_entity == ((ent->flags & FL_TEAMSLAVE) ? ent->teammaster : ent))
	{
		AngleMove_Begin (ent);
	}
	else
	{
		ent->nextthink = level.time + FRAMETIME;
		ent->think = AngleMove_Begin;
	}
}


/*
==============
Think_AccelMove

The team has completed a frame of movement, so
change the speed for the next frame
==============
*/
#define AccelerationDistance(target, rate)	(target * ((target / rate) + 1) / 2)

void plat_CalcAcceleratedMove(moveinfo_t *moveinfo)
{
	float	accel_dist;
	float	decel_dist;

	moveinfo->move_speed = moveinfo->speed;

	if (moveinfo->remaining_distance < moveinfo->accel)
	{
		moveinfo->current_speed = moveinfo->remaining_distance;
		return;
	}

	accel_dist = AccelerationDistance (moveinfo->speed, moveinfo->accel);
	decel_dist = AccelerationDistance (moveinfo->speed, moveinfo->decel);

	if ((moveinfo->remaining_distance - accel_dist - decel_dist) < 0)
	{
		float	f;

		f = (moveinfo->accel + moveinfo->decel) / (moveinfo->accel * moveinfo->decel);
		moveinfo->move_speed = (-2 + sqrt(4 - 4 * f * (-2 * moveinfo->remaining_distance))) / (2 * f);
		decel_dist = AccelerationDistance (moveinfo->move_speed, moveinfo->decel);
	}

	moveinfo->decel_distance = decel_dist;
};

void plat_Accelerate (moveinfo_t *moveinfo)
{
	// are we decelerating?
	if (moveinfo->remaining_distance <= moveinfo->decel_distance)
	{
		if (moveinfo->remaining_distance < moveinfo->decel_distance)
		{
			if (moveinfo->next_speed)
			{
				moveinfo->current_speed = moveinfo->next_speed;
				moveinfo->next_speed = 0;
				return;
			}
			if (moveinfo->current_speed > moveinfo->decel)
				moveinfo->current_speed -= moveinfo->decel;
		}
		return;
	}

	// are we at full speed and need to start decelerating during this move?
	if (moveinfo->current_speed == moveinfo->move_speed)
		if ((moveinfo->remaining_distance - moveinfo->current_speed) < moveinfo->decel_distance)
		{
			float	p1_distance;
			float	p2_distance;
			float	distance;

			p1_distance = moveinfo->remaining_distance - moveinfo->decel_distance;
			p2_distance = moveinfo->move_speed * (1.0 - (p1_distance / moveinfo->move_speed));
			distance = p1_distance + p2_distance;
			moveinfo->current_speed = moveinfo->move_speed;
			moveinfo->next_speed = moveinfo->move_speed - moveinfo->decel * (p2_distance / distance);
			return;
		}

	// are we accelerating?
	if (moveinfo->current_speed < moveinfo->speed)
	{
		float	old_speed;
		float	p1_distance;
		float	p1_speed;
		float	p2_distance;
		float	distance;

		old_speed = moveinfo->current_speed;

		// figure simple acceleration up to move_speed
		moveinfo->current_speed += moveinfo->accel;
		if (moveinfo->current_speed > moveinfo->speed)
			moveinfo->current_speed = moveinfo->speed;

		// are we accelerating throughout this entire move?
		if ((moveinfo->remaining_distance - moveinfo->current_speed) >= moveinfo->decel_distance)
			return;

		// during this move we will accelrate from current_speed to move_speed
		// and cross over the decel_distance; figure the average speed for the
		// entire move
		p1_distance = moveinfo->remaining_distance - moveinfo->decel_distance;
		p1_speed = (old_speed + moveinfo->move_speed) / 2.0;
		p2_distance = moveinfo->move_speed * (1.0 - (p1_distance / p1_speed));
		distance = p1_distance + p2_distance;
		moveinfo->current_speed = (p1_speed * (p1_distance / distance)) + (moveinfo->move_speed * (p2_distance / distance));
		moveinfo->next_speed = moveinfo->move_speed - moveinfo->decel * (p2_distance / distance);
		return;
	}

	// we are at constant velocity (move_speed)
	return;
};

void Think_AccelMove (edict_t *ent)
{
	ent->moveinfo.remaining_distance -= ent->moveinfo.current_speed;

	// Knightmare- fix plats with high accel and decel getting stuck just before top and bottom
//	if (ent->moveinfo.current_speed == 0)		// starting or blocked
		plat_CalcAcceleratedMove(&ent->moveinfo);

	plat_Accelerate (&ent->moveinfo);

	// will the entire move complete on next frame?
	if (ent->moveinfo.remaining_distance <= ent->moveinfo.current_speed)
	{
		Move_Final (ent);
		return;
	}

	VectorScale (ent->moveinfo.dir, ent->moveinfo.current_speed*10, ent->velocity);
	if (ent->movewith)
		VectorAdd(ent->movewith_ent->velocity,ent->velocity,ent->velocity);
	ent->nextthink = level.time + FRAMETIME;
	ent->think = Think_AccelMove;

#ifndef POSTTHINK_CHILD_MOVEMENT
	if (ent->movewith_next && (ent->movewith_next->movewith_ent == ent))
		set_child_movement (ent);
#endif	// POSTTHINK_CHILD_MOVEMENT
}

void plat_hit_top (edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->s.sound && ent->moveinfo.sound_end)
			gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_end, 1, ent->attenuation, 0); // was ATTN_STATIC
	//	ent->s.sound = 0;
	}
	ent->s.sound = 0;	// Knightmare- make sure this is always set to 0, lead mover or not!

	ent->moveinfo.state = STATE_TOP;

	ent->think = plat_go_down;
	ent->nextthink = level.time + 3;
}

void plat_hit_bottom (edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->s.sound && ent->moveinfo.sound_end)
			gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_end, 1, ent->attenuation, 0); // was ATTN_STATIC
	//	ent->s.sound = 0;
	}
	ent->s.sound = 0;	// Knightmare- make sure this is always set to 0, lead mover or not!

	ent->moveinfo.state = STATE_BOTTOM;
}

void plat_go_down (edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
			gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_start, 1, ent->attenuation, 0); // was ATTN_STATIC
		ent->s.sound = ent->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		ent->s.loop_attenuation = ent->attenuation;
#endif
	}
	ent->moveinfo.state = STATE_DOWN;
	Move_Calc (ent, ent->moveinfo.end_origin, plat_hit_bottom);
}

void plat_go_up (edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
			gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_start, 1, ent->attenuation, 0); // was ATTN_STATIC
		ent->s.sound = ent->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		ent->s.loop_attenuation = ent->attenuation;
#endif
	}
	ent->moveinfo.state = STATE_UP;
	Move_Calc (ent, ent->moveinfo.start_origin, plat_hit_top);
}

void plat_blocked (edict_t *self, edict_t *other)
{
	if (!(other->svflags & SVF_MONSTER) && (!other->client) )
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other)
		{
			// Lazarus: Some of our ents don't have origin near the model
			vec3_t save;
			VectorCopy (other->s.origin,save);
			VectorMA (other->absmin, 0.5, other->size, other->s.origin);
			BecomeExplosion1 (other);
		}
		return;
	}

	// Knightmare- if it's dead, gib it
	if (other->health < 1) {
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 100, 0, MOD_CRUSH);
	}
	else {
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
	}
	// end Knightmare

	if (self->moveinfo.state == STATE_UP)
		plat_go_down (self);
	else if (self->moveinfo.state == STATE_DOWN)
		plat_go_up (self);
}


void Use_Plat (edict_t *ent, edict_t *other, edict_t *activator)
{
	if (ent->think)
		return;		// already down
	plat_go_down (ent);
}


void Touch_Plat_Center (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;

	if (other->health <= 0)
		return;

	ent = ent->enemy;	// now point at the plat, not the trigger
	if (ent->moveinfo.state == STATE_BOTTOM)
		plat_go_up (ent);
	else if (ent->moveinfo.state == STATE_TOP)
		ent->nextthink = level.time + 1;	// the player is still on the plat, so delay going down
}

//void plat_spawn_inside_trigger (edict_t *ent)
edict_t *plat_spawn_inside_trigger (edict_t *ent)
{
	edict_t	*trigger;
	vec3_t	tmin, tmax;

//
// middle trigger
//
	trigger = G_Spawn();
	trigger->touch = Touch_Plat_Center;
	trigger->movetype = MOVETYPE_NONE;
	trigger->solid = SOLID_TRIGGER;
	trigger->enemy = ent;

	tmin[0] = ent->mins[0] + 25;
	tmin[1] = ent->mins[1] + 25;
	tmin[2] = ent->mins[2];

	tmax[0] = ent->maxs[0] - 25;
	tmax[1] = ent->maxs[1] - 25;
	tmax[2] = ent->maxs[2] + 8;

	// Knightmare- for PLAT_LOW_TRIGGER, we need to add st.lip, not subtract it!
/*	tmin[2] = tmax[2] - (ent->pos1[2] - ent->pos2[2] + st.lip);

	if (ent->spawnflags & PLAT_LOW_TRIGGER)
		tmax[2] = tmin[2] + 8; */

	if (ent->spawnflags & PLAT_LOW_TRIGGER) {
		tmin[2] = tmax[2] - (ent->pos1[2] - ent->pos2[2]) + st.lip;
		tmax[2] = tmin[2] + 8;
	}
	else
		tmin[2] = tmax[2] - (ent->pos1[2] - ent->pos2[2] + st.lip);

	if (tmax[0] - tmin[0] <= 0)
	{
		tmin[0] = (ent->mins[0] + ent->maxs[0]) *0.5;
		tmax[0] = tmin[0] + 1;
	}
	if (tmax[1] - tmin[1] <= 0)
	{
		tmin[1] = (ent->mins[1] + ent->maxs[1]) *0.5;
		tmax[1] = tmin[1] + 1;
	}

	VectorCopy (tmin, trigger->mins);
	VectorCopy (tmax, trigger->maxs);

	// Lazarus:
	trigger->movewith = ent->movewith;

	gi.linkentity (trigger);

	return trigger;	// Knightmare added
}


/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is trigger, when it will lower and become a normal plat.

"speed"	overrides default 200
"accel" overrides default 50
"decel" overrides default 50
"lip"	overrides default 8 unit lip

If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determoveinfoned by the model's height.

"sounds"
0,1		default
2-99	custom sound- e.g. plats/pt02_strt.wav, plats/pt02_mid.wav, plats/pt02_end.wav
*/
void SP_func_plat (edict_t *ent)
{
	ent->class_id = ENTITY_FUNC_PLAT;

	VectorClear (ent->s.angles);
	ent->solid = SOLID_BSP;
	ent->movetype = MOVETYPE_PUSH;

	gi.setmodel (ent, ent->model);

	ent->blocked = plat_blocked;

	if (!ent->speed)
		ent->speed = 20;
	else
		ent->speed *= 0.1;

	if (!ent->accel)
		ent->accel = 5;
	else
		ent->accel *= 0.1;

	if (!ent->decel)
		ent->decel = 5;
	else
		ent->decel *= 0.1;

	if (!ent->dmg)
		ent->dmg = 2;

	if (!st.lip)
		st.lip = 8;

	// pos1 is the top position, pos2 is the bottom
	VectorCopy (ent->s.origin, ent->pos1);
	VectorCopy (ent->s.origin, ent->pos2);
	if (st.height)
		ent->pos2[2] -= st.height;
	else
		ent->pos2[2] -= (ent->maxs[2] - ent->mins[2]) - st.lip;

#ifdef POSTTHINK_CHILD_MOVEMENT
	ent->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	ent->use = Use_Plat;

	plat_spawn_inside_trigger (ent);	// the "start moving" trigger

	if (ent->targetname)
	{
		ent->moveinfo.state = STATE_UP;
	}
	else
	{
		VectorCopy (ent->pos2, ent->s.origin);
		gi.linkentity (ent);
		ent->moveinfo.state = STATE_BOTTOM;
	}

	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	VectorCopy (ent->pos1, ent->moveinfo.start_origin);
	VectorCopy (ent->s.angles, ent->moveinfo.start_angles);
	VectorCopy (ent->pos2, ent->moveinfo.end_origin);
	VectorCopy (ent->s.angles, ent->moveinfo.end_angles);
	ent->moveinfo.distance = ent->pos1[2] - ent->pos2[2];	// Knightmare- store distance

	if ( (level.maptype == MAPTYPE_CUSTOM) && (ent->sounds > 1) && (ent->sounds < 100) ) // custom sounds
	{
		ent->moveinfo.sound_start = gi.soundindex (va("plats/pt%02i_strt.wav", ent->sounds));
		ent->moveinfo.sound_middle = gi.soundindex (va("plats/pt%02i_mid.wav", ent->sounds));
		ent->moveinfo.sound_end = gi.soundindex (va("plats/pt%02i_end.wav", ent->sounds));
	}
	else
	{
		ent->moveinfo.sound_start = gi.soundindex ("plats/pt1_strt.wav");
		ent->moveinfo.sound_middle = gi.soundindex ("plats/pt1_mid.wav");
		ent->moveinfo.sound_end = gi.soundindex ("plats/pt1_end.wav");
	}

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;
}

//====================================================================
// Plat2
//====================================================================
#define P2F_CALLED		1
#define P2F_MOVING		2
#define P2F_WAITING		4

void plat2_hit_top (edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->s.sound && ent->moveinfo.sound_end)
			gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_end, 1, ent->attenuation, 0); // was ATTN_STATIC
	//	ent->s.sound = 0;
	}
	ent->s.sound = 0;	// Knightmare- make sure this is always set to 0, lead mover or not!
	ent->moveinfo.state = STATE_TOP;

	if (ent->plat2flags & P2F_CALLED)
	{
		ent->plat2flags = P2F_WAITING;
		if ( !(ent->spawnflags & PLAT_TOGGLE) )
		{
			ent->think = plat2_go_down;
			ent->nextthink = level.time + 5.0;
		}
		if ( (int)deathmatch->value )
			ent->last_move_time = level.time - 1.0;
		else
			ent->last_move_time = level.time - 2.0;
	}
	else if ( !(ent->spawnflags & PLAT_TOP) && !(ent->spawnflags & PLAT_TOGGLE) )
	{
		ent->plat2flags = 0;
		ent->think = plat2_go_down;
		ent->nextthink = level.time + 2.0;
		ent->last_move_time = level.time;
	}
	else
	{
		ent->plat2flags = 0;
		ent->last_move_time = level.time;
	}

	G_UseTargets (ent, ent);
}

void plat2_hit_bottom (edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->s.sound && ent->moveinfo.sound_end)
			gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_end, 1, ent->attenuation, 0); // was ATTN_STATIC
	//	ent->s.sound = 0;
	}
	ent->s.sound = 0;	// Knightmare- make sure this is always set to 0, lead mover or not!
	ent->moveinfo.state = STATE_BOTTOM;

	if (ent->plat2flags & P2F_CALLED)
	{
		ent->plat2flags = P2F_WAITING;
		if ( !(ent->spawnflags & PLAT_TOGGLE) )
		{
			ent->think = plat2_go_up;
			ent->nextthink = level.time + 5.0;
		}
		if ( (int)deathmatch->value )
			ent->last_move_time = level.time - 1.0;
		else
			ent->last_move_time = level.time - 2.0;
	}
	else if ( (ent->spawnflags & PLAT_TOP) && !(ent->spawnflags & PLAT_TOGGLE) )
	{
		ent->plat2flags = 0;
		ent->think = plat2_go_up;
		ent->nextthink = level.time + 2.0;
		ent->last_move_time = level.time;
	}
	else
	{
		ent->plat2flags = 0;
		ent->last_move_time = level.time;
	}

//	plat2_remove_badarea (ent);

	G_UseTargets (ent, ent);
}

void plat2_go_down (edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
			gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_start, 1, ent->attenuation, 0); // was ATTN_STATIC
		ent->s.sound = ent->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		ent->s.loop_attenuation = ent->attenuation;
#endif
	}
	ent->moveinfo.state = STATE_DOWN;
	ent->plat2flags |= P2F_MOVING;

	Move_Calc (ent, ent->moveinfo.end_origin, plat2_hit_bottom);
}

void plat2_go_up (edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
			gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_start, 1, ent->attenuation, 0); // was ATTN_STATIC
		ent->s.sound = ent->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		ent->s.loop_attenuation = ent->attenuation;
#endif
	}
	ent->moveinfo.state = STATE_UP;
	ent->plat2flags |= P2F_MOVING;

//	plat2_create_badarea (ent);

	Move_Calc (ent, ent->moveinfo.start_origin, plat2_hit_top);
}

void plat2_dostuff (edict_t *ent, edict_t *other)
{
	int		newState;
	float	platDelay;
	vec3_t	triggerCenter;
	edict_t	*triggerEnt, *platEnt;

	if (!ent->enemy || !other)	// sanity check
		return;

	// ent is the trigger, plat itself is ent->enemy
	triggerEnt = ent;
	platEnt = ent->enemy;

	// don't respond if currently moving
	if (platEnt->plat2flags & P2F_MOVING)
		return;

	// don't respond for 2 seconds after last activation
	if ((platEnt->last_move_time + 2.0f) > level.time)
		return;

	VectorAdd (triggerEnt->absmin, triggerEnt->absmax, triggerCenter);
	VectorScale (triggerCenter, 0.5f, triggerCenter);

	if (platEnt->moveinfo.state == STATE_TOP)
	{
		newState = STATE_TOP;
		if (platEnt->spawnflags & PLAT_BOX_LIFT) {
			if (triggerCenter[2] > other->s.origin[2])
				newState = STATE_BOTTOM;
		}
		else {
			if (triggerEnt->absmax[2] > triggerCenter[2])
				newState = STATE_BOTTOM;
		}
	}
	else {
		newState = STATE_BOTTOM;
		if (other->s.origin[2] > triggerCenter[2])
			newState = STATE_TOP;
	}

	platEnt->plat2flags = P2F_MOVING;

	platDelay = ((int)deathmatch->value != 0) ? 0.3f : 0.5f;

	if (platEnt->moveinfo.state != newState) {
		platEnt->plat2flags |= P2F_CALLED;
		platDelay = 0.1f;
	}

	platEnt->last_move_time = level.time;

	if (platEnt->moveinfo.state == STATE_BOTTOM) {
		platEnt->think = plat2_go_up;
	}
	else {
		platEnt->think = plat2_go_down;
	}
	platEnt->nextthink = level.time + platDelay;
}


void plat2_touch_center (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!ent || !other)
		return;

	// don't respond to dead player / monsters
	if (other->health <= 0)
		return;

	// don't respond to entities that aren't players or monsters
	if ( (!(other->svflags & SVF_MONSTER)) && (!other->client) )
		return;

	plat2_dostuff (ent, other);
}

void plat2_blocked (edict_t *self, edict_t *other)
{
	if (!(other->svflags & SVF_MONSTER) && (!other->client) )
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other)
		{
			// Lazarus: Some of our ents don't have origin near the model
			vec3_t save;
			VectorCopy (other->s.origin,save);
			VectorMA (other->absmin, 0.5, other->size, other->s.origin);
			BecomeExplosion1 (other);
		}
		return;
	}

	if (other->health < 1) {	// if it's dead, gib it
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 100, 0, MOD_CRUSH);
	}
	else {
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
	}

	if (self->moveinfo.state == STATE_UP)
		plat2_go_down (self);
	else if (self->moveinfo.state == STATE_DOWN)
		plat2_go_up (self);
}

void plat2_use (edict_t *ent, edict_t *other, edict_t *activator)
{
	edict_t		*triggerEnt;
	int			i;

	if (ent->moveinfo.state > STATE_BOTTOM)
		return;
	if ((ent->last_move_time + 2.0f) > level.time)
		return;

	for (i = 1, triggerEnt = g_edicts + 1; i < globals.num_edicts; i++, triggerEnt++)
	{
		if (!triggerEnt->inuse)
			continue;
		if (triggerEnt->touch == plat2_touch_center)
		{
			if (triggerEnt->enemy == ent)
			{
				plat2_dostuff (triggerEnt, activator);
				return;
			}
		}
	}
}

//edict_t *plat2_spawn_inside_trigger (edict_t *ent)
//{
//}

void plat2_activate (edict_t *ent, edict_t *other, edict_t *activator)
{
	edict_t		*triggerEnt;

	ent->use = plat2_use;

	triggerEnt = plat_spawn_inside_trigger (ent);

	triggerEnt->maxs[0] += 10.0f;
	triggerEnt->maxs[1] += 10.0f;
	triggerEnt->mins[0] -= 10.0f;
	triggerEnt->mins[1] -= 10.0f;

	gi.linkentity (triggerEnt);

	triggerEnt->touch = plat2_touch_center;

	plat2_go_down (ent);
}

void SP_func_plat2 (edict_t *ent)
{
	edict_t		*triggerEnt;

	ent->class_id = ENTITY_FUNC_PLAT2;

	VectorClear (ent->s.angles);
	ent->solid = SOLID_BSP;
	ent->movetype = MOVETYPE_PUSH;

	gi.setmodel (ent, ent->model);

	ent->blocked = plat2_blocked;

	if (!ent->speed)
		ent->speed = 20;
	else
		ent->speed *= 0.1;

	if (!ent->accel)
		ent->accel = 5;
	else
		ent->accel *= 0.1;

	if (!ent->decel)
		ent->decel = 5;
	else
		ent->decel *= 0.1;

	if (!ent->dmg)
		ent->dmg = 2;

	if ((int)deathmatch->value) {
		ent->speed *= 2.0f;
		ent->accel *= 2.0f;
		ent->decel *= 2.0f;
	}

	if (!st.lip)
		st.lip = 8;

	// pos1 is the top position, pos2 is the bottom
	VectorCopy (ent->s.origin, ent->pos1);
	VectorCopy (ent->s.origin, ent->pos2);

	if (st.height)
		ent->pos2[2] -= (st.height - st.lip);
	else
		ent->pos2[2] -= (ent->maxs[2] - ent->mins[2]) - st.lip;

	ent->moveinfo.state = STATE_TOP;

	if (ent->targetname)
	{
		ent->use = plat2_activate;
	}
	else
	{
		ent->use = plat2_use;

		triggerEnt = plat_spawn_inside_trigger (ent);

		triggerEnt->maxs[0] += 10.0f;
		triggerEnt->maxs[1] += 10.0f;
		triggerEnt->mins[0] -= 10.0f;
		triggerEnt->mins[1] -= 10.0f;

		gi.linkentity (triggerEnt);

		triggerEnt->touch = plat2_touch_center;

		if ( !(ent->spawnflags & PLAT_TOP) ) {
			VectorCopy (ent->pos2, ent->s.origin);
			ent->moveinfo.state = STATE_BOTTOM;
		}
	}

#ifdef POSTTHINK_CHILD_MOVEMENT
	ent->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	gi.linkentity (ent);

	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	VectorCopy (ent->pos1, ent->moveinfo.start_origin);
	VectorCopy (ent->s.angles, ent->moveinfo.start_angles);
	VectorCopy (ent->pos2, ent->moveinfo.end_origin);
	VectorCopy (ent->s.angles, ent->moveinfo.end_angles);
	ent->moveinfo.distance = ent->pos1[2] - ent->pos2[2];	// Knightmare- store distance

	if ( (level.maptype == MAPTYPE_CUSTOM) && (ent->sounds > 1) && (ent->sounds < 100) ) // custom sounds
	{
		ent->moveinfo.sound_start = gi.soundindex (va("plats/pt%02i_strt.wav", ent->sounds));
		ent->moveinfo.sound_middle = gi.soundindex (va("plats/pt%02i_mid.wav", ent->sounds));
		ent->moveinfo.sound_end = gi.soundindex (va("plats/pt%02i_end.wav", ent->sounds));
	}
	else
	{
		ent->moveinfo.sound_start = gi.soundindex ("plats/pt1_strt.wav");
		ent->moveinfo.sound_middle = gi.soundindex ("plats/pt1_mid.wav");
		ent->moveinfo.sound_end = gi.soundindex ("plats/pt1_end.wav");
	}

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;
}

//====================================================================

/*QUAKED func_rotating (0 .5 .8) ? START_ON REVERSE X_AXIS Y_AXIS TOUCH_PAIN STOP ANIMATED ANIMATED_FAST
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"speed" determines how fast it moves; default value is 100.
"dmg"	damage to inflict when blocked (2 default)

REVERSE will cause the it to rotate in the opposite direction.
STOP mean it will stop moving instead of pushing entities
*/
#define ROTATING_ACCEL	0x2000
void rotating_accel (edict_t *self)
{
	float	current_speed;

	current_speed = VectorLength (self->avelocity);
	if (current_speed >= (self->speed - self->accel))		// done
	{
		VectorScale (self->movedir, self->speed, self->avelocity);
		G_UseTargets (self, self);
	}
	else
	{
		current_speed += self->accel;
		VectorScale (self->movedir, current_speed, self->avelocity);
		self->think = rotating_accel;
		self->nextthink = level.time + FRAMETIME;
	}
}

void rotating_decel (edict_t *self)
{
	float	current_speed;

	current_speed = VectorLength (self->avelocity);
	if (current_speed <= self->decel)		// done
	{
		VectorClear (self->avelocity);
		G_UseTargets (self, self);
		self->touch = nullptr;
	}
	else
	{
		current_speed -= self->decel;
		VectorScale (self->movedir, current_speed, self->avelocity);
		self->think = rotating_decel;
		self->nextthink = level.time + FRAMETIME;
	}
}

void rotating_blocked (edict_t *self, edict_t *other)
{
/*	// Lazarus: This was added to do damage to pickup items, but may break
	// existing maps.

	if (!(other->svflags & SVF_MONSTER) && (!other->client) ) {
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other) {
			// Lazarus: Some of our ents don't have origin near the model
			vec3_t save;
			VectorCopy (other->s.origin,save);
			VectorMA (other->absmin, 0.5, other->size, other->s.origin);
			BecomeExplosion1 (other);
		}
		return;
	} */
	//if (other->groundentity != self)
	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void rotating_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->avelocity[0] || self->avelocity[1] || self->avelocity[2])
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void rotating_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!VectorCompare (self->avelocity, vec3_origin))
	{
		self->s.sound = 0;
		if (self->spawnflags & ROTATING_ACCEL)	// decelerate
			rotating_decel (self);
		else {
			VectorClear (self->avelocity);
			G_UseTargets (self, self);
			self->touch = nullptr;
		}
	}
	else
	{
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
		if (self->spawnflags & ROTATING_ACCEL)	// accelerate
			rotating_accel (self);
		else {
			VectorScale (self->movedir, self->speed, self->avelocity);
			G_UseTargets (self, self);
		}
		if (self->spawnflags & 16)
			self->touch = rotating_touch;
	}
}

void SP_func_rotating (edict_t *ent)
{
	ent->class_id = ENTITY_FUNC_ROTATING;

	ent->solid = SOLID_BSP;
	if (ent->spawnflags & 32)
		ent->movetype = MOVETYPE_STOP;
	else
		ent->movetype = MOVETYPE_PUSH;

	// set the axis of rotation
	// Knightmare- use move_angles for manual setting of axis
	if (VectorLength(ent->move_angles) > 0.0f)	{
		VectorCopy (ent->move_angles, ent->movedir);
		VectorNormalize (ent->movedir);
	}
	else
	{
		VectorClear(ent->movedir);
		if (ent->spawnflags & 4)
			ent->movedir[2] = 1.0;
		else if (ent->spawnflags & 8)
			ent->movedir[0] = 1.0;
		else // Z_AXIS
			ent->movedir[1] = 1.0;
	}
	// end Knightmare

	// check for reverse rotation
	if (ent->spawnflags & 2)
		VectorNegate (ent->movedir, ent->movedir);

	if (!ent->speed)
		ent->speed = 100;

	if (!ent->dmg)
		ent->dmg = 2;

	// Lazarus: Why was this removed? Dunno, but we're gonna use
	//          our own sound anyway
	//ent->moveinfo.sound_middle = "doors/hydro1.wav";
	if (st.noise)
		ent->moveinfo.sound_middle = gi.soundindex  (st.noise);

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;

#ifdef POSTTHINK_CHILD_MOVEMENT
	ent->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	ent->use = rotating_use;
	if (ent->dmg)
		ent->blocked = rotating_blocked;

	if (ent->spawnflags & 1)
		ent->use (ent, nullptr, nullptr );

	if (ent->spawnflags & 64)
		ent->s.effects |= EF_ANIM_ALL;
	if (ent->spawnflags & 128)
		ent->s.effects |= EF_ANIM_ALLFAST;

	if (ent->spawnflags & ROTATING_ACCEL) {	// accel/decel
		if (ent->accel == 0) ent->accel = 1;
		else ent->accel = min(ent->accel, ent->speed);
		if (ent->decel == 0) ent->decel = 1;
		else ent->decel = min(ent->decel, ent->speed);
	}

	gi.setmodel (ent, ent->model);
	gi.linkentity (ent);
}

/* Lazarus: SP_func_rotating_dh is identical to func_rotating, but allows you
            to build the model in a separate location (as with func_train, to
            get the lighting the way you'd like) then move at runtime to the
            origin of the "pathtarget". */

void func_rotating_dh_init (edict_t *ent) {
	edict_t		*new_origin;

	new_origin = G_Find ( nullptr, FOFS(targetname), ent->pathtarget);
	if (new_origin)
		VectorCopy (new_origin->s.origin,ent->s.origin);
	SP_func_rotating (ent);
}

void SP_func_rotating_dh (edict_t *ent)
{
	if ( !ent->pathtarget ) {
		SP_func_rotating(ent);
		return;
	}

	// Wait a few frames so that we're sure pathtarget has been parsed.
	ent->think = func_rotating_dh_init;
	ent->nextthink = level.time + 2*FRAMETIME;
	gi.linkentity(ent);
}

/*
======================================================================

BUTTONS

======================================================================
*/

/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"sounds"
0)	default
1)	silent
2-99 custom sound- e.g. switches/butn02.wav
*/

void button_done (edict_t *self)
{
	self->moveinfo.state = STATE_BOTTOM;
	self->s.effects &= ~EF_ANIM23;
	self->s.effects |= EF_ANIM01;
}

void button_return (edict_t *self)
{
	if (self->movewith)
		movewith_update(self);

	self->moveinfo.state = STATE_DOWN;

	Move_Calc (self, self->moveinfo.start_origin, button_done);

	self->s.frame = 0;

	if (self->health)
		self->takedamage = DAMAGE_YES;
}

void button_wait (edict_t *self)
{
	self->moveinfo.state = STATE_TOP;
	self->s.effects &= ~EF_ANIM01;
	self->s.effects |= EF_ANIM23;

	G_UseTargets (self, self->activator);
	self->s.frame = 1;
	if (self->moveinfo.wait >= 0)
	{
		self->nextthink = level.time + self->moveinfo.wait;
		self->think = button_return;
	}
}

void button_fire (edict_t *self)
{
	if (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)
		return;

	if (self->movewith)
		movewith_update(self);
	self->moveinfo.state = STATE_UP;
	if (self->moveinfo.sound_start && !(self->flags & FL_TEAMSLAVE))
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	Move_Calc (self, self->moveinfo.end_origin, button_wait);
}

void button_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;
	button_fire (self);
}

void button_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// Lazarus: allow robot actors to touch buttons
	if (!other->client && !(other->flags & FL_ROBOT))
		return;

	if (other->health <= 0)
		return;

	if ((self->spawnflags & 1) && (LookingAt(other,0, nullptr, nullptr ) != self))
		return;

	self->activator = other;
	button_fire (self);
}

void button_killed (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->activator = attacker;
	self->health = self->max_health;
	self->takedamage = DAMAGE_NO;
	button_fire (self);
}

void SP_func_button (edict_t *ent)
{
	vec3_t	abs_movedir;

	ent->class_id = ENTITY_FUNC_BUTTON;

	G_SetMovedir (ent->s.angles, ent->movedir);

	if (ent->movewith) {
		ent->movetype = MOVETYPE_PUSH;
		ent->blocked = train_blocked;
	}
	else
		ent->movetype = MOVETYPE_STOP;
	ent->solid = SOLID_BSP;
	gi.setmodel (ent, ent->model);

	if ( (level.maptype == MAPTYPE_CUSTOM) && (ent->sounds > 1) && (ent->sounds < 100) ) // custom sounds
		ent->moveinfo.sound_start = gi.soundindex (va("switches/butn%02i.wav", ent->sounds));
	else if (ent->sounds != 1)
		ent->moveinfo.sound_start = gi.soundindex ("switches/butn2.wav");

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;

	if (!ent->speed)
		ent->speed = 40;
	if (!ent->accel)
		ent->accel = ent->speed;
	if (!ent->decel)
		ent->decel = ent->speed;

	if (!ent->wait)
		ent->wait = 3;
	if (!st.lip)
		st.lip = 4;

	VectorCopy (ent->s.origin, ent->pos1);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	ent->moveinfo.distance = abs_movedir[0] * ent->size[0] + abs_movedir[1] * ent->size[1] + abs_movedir[2] * ent->size[2] - st.lip;
	VectorMA (ent->pos1, ent->moveinfo.distance, ent->movedir, ent->pos2);

	ent->use = button_use;
	ent->s.effects |= EF_ANIM01;

	if (ent->health)
	{
		ent->max_health = ent->health;
		ent->die = button_killed;
		ent->takedamage = DAMAGE_YES;
	}
	else if (! ent->targetname)
		ent->touch = button_touch;

#ifdef POSTTHINK_CHILD_MOVEMENT
	ent->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	ent->moveinfo.state = STATE_BOTTOM;

	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	VectorCopy (ent->pos1, ent->moveinfo.start_origin);
	VectorCopy (ent->s.angles, ent->moveinfo.start_angles);
	VectorCopy (ent->pos2, ent->moveinfo.end_origin);
	VectorCopy (ent->s.angles, ent->moveinfo.end_angles);

	gi.linkentity (ent);
}

//====================================================================
//
// SP_func_trainbutton is a button linked to a func_train with the
// "movewith" key. The button itself does NOT move relative to the
// train, but the effect can be simulated with animations and sounds
//
// Spawnflags 1 = can be activated by looking at it and pressing +attack
//            2 = can NOT be activated by touching - must use +attack
//
//====================================================================
void trainbutton_done (edict_t *self)
{
	self->moveinfo.state = STATE_BOTTOM;
	self->s.effects &= ~EF_ANIM23;
	self->s.effects |= EF_ANIM01;
}

void trainbutton_return (edict_t *self)
{
	self->moveinfo.state = STATE_DOWN;
	self->s.frame = 0;
	self->s.effects &= ~EF_ANIM23;
	self->s.effects |= EF_ANIM01;
	if (self->health)
		self->takedamage = DAMAGE_YES;
}

void trainbutton_wait (edict_t *self)
{
	self->moveinfo.state = STATE_TOP;
	self->s.effects &= ~EF_ANIM01;
	self->s.effects |= EF_ANIM23;

	G_UseTargets (self, self->activator);
	self->s.frame = 1;
	if (self->moveinfo.wait >= 0)
	{
		self->nextthink = level.time + self->moveinfo.wait;
		self->think = trainbutton_return;
	}
}

void trainbutton_fire (edict_t *self)
{
	if (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)
		return;
	self->moveinfo.state = STATE_UP;
	if (self->moveinfo.sound_start && !(self->flags & FL_TEAMSLAVE))
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	trainbutton_wait(self);
}

void trainbutton_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;
	trainbutton_fire (self);
}

void trainbutton_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;

	if (other->health <= 0)
		return;

	self->activator = other;
	trainbutton_fire (self);
}

void trainbutton_killed (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->activator = attacker;
	self->health = self->max_health;
	self->takedamage = DAMAGE_NO;
	trainbutton_fire (self);
}

void movewith_init (edict_t *ent)
{
	edict_t *e, *child;

	// Unnamed entities can't be movewith parents
	if (!ent->targetname) return;

	child = G_Find( nullptr, FOFS(movewith), ent->targetname);
	e = ent;
	while (child)
	{
		child->movewith_ent = ent;
		// Copy parent's current angles to the child. They SHOULD be 0,0,0 at this point
		// for all currently supported parents, but ya never know.
		VectorCopy (ent->s.angles, child->parent_attach_angles);
		VectorCopy (child->s.angles, child->child_attach_angles);
		if (child->org_movetype < 0)
			child->org_movetype = child->movetype;
		if (child->movetype != MOVETYPE_NONE)
			child->movetype = MOVETYPE_PUSH;
		VectorCopy (child->mins, child->org_mins);
		VectorCopy (child->maxs, child->org_maxs);
		VectorSubtract (child->s.origin, ent->s.origin, child->movewith_offset);
		e->movewith_next = child;
		e = child;
		child = G_Find(child, FOFS(movewith), ent->targetname);
	}
}

void SP_func_trainbutton (edict_t *ent)
{
	if ( !ent->movewith )
	{
		SP_func_button (ent);
		return;
	}
	ent->class_id = ENTITY_FUNC_TRAINBUTTON;

	G_SetMovedir (ent->s.angles, ent->movedir);
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel (ent, ent->model);

	if ( (ent->sounds > 1) && (ent->sounds < 100) ) // custom sounds
		ent->moveinfo.sound_start = gi.soundindex  (va("switches/butn%02i.wav", ent->sounds));
	else if (ent->sounds != 1)
		ent->moveinfo.sound_start = gi.soundindex ("switches/butn2.wav");

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;

	if (!ent->wait)
		ent->wait = 3;
	ent->use = trainbutton_use;
	ent->s.effects |= EF_ANIM01;
	ent->blocked = train_blocked;
	ent->moveinfo.state = STATE_BOTTOM;
	ent->moveinfo.wait = ent->wait;
	if (!ent->targetname && !(ent->spawnflags & 2))
		ent->touch = trainbutton_touch;
	gi.linkentity (ent);
}
/*
======================================================================

DOORS

  spawn a trigger surrounding the entire team unless it is
  already targeted by another

======================================================================
*/

/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER NOMONSTER ANIMATED TOGGLE ANIMATED_FAST
TOGGLE		wait in both the start and end states for a trigger event.
START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER	monsters will not trigger this door

"message"	is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
0)		default
1)		silent
2-4		not used
5-99	custom sound- e.g. doors/dr05_strt.wav, doors/dr05_mid.wav, doors/dr05_end.wav
*/

void door_use_areaportals (edict_t *self, qboolean open)
{
	edict_t	*t = nullptr;

	if (!self->target)
		return;

	while ((t = G_Find (t, FOFS(targetname), self->target)))
	{
		if (Q_stricmp( t->classname.c_str(), "func_areaportal" ) == 0)
		{
			gi.SetAreaPortalState (t->style, open);
		}
	}
}

void door_go_down (edict_t *self);

void swinging_door_reset (edict_t *self)
{
	self->moveinfo.state = STATE_BOTTOM;
	self->health = self->max_health;
	if (self->die)
		self->takedamage = DAMAGE_YES;
	VectorCopy (self->pos2, self->pos1);
	VectorMA (self->s.angles, self->moveinfo.distance, self->movedir, self->pos2);
	VectorCopy (self->pos1, self->moveinfo.start_angles);
	VectorCopy (self->pos2, self->moveinfo.end_angles);
	vectoangles2 (self->move_origin, self->move_origin);
	VectorMA (self->move_origin, self->moveinfo.distance, self->movedir, self->move_origin);
	AngleVectors (self->move_origin, self->move_origin, nullptr, nullptr );
}

void door_hit_top (edict_t *self)
{
	self->do_not_rotate = false;
	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->s.sound && self->moveinfo.sound_end)
			gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC
	//	self->s.sound = 0;
	}
	self->s.sound = 0;	// Knightmare- make sure this is always set to 0, lead mover or not!

	self->moveinfo.state = STATE_TOP;
	if (self->flags & FL_REVOLVING)
	{
		self->think = swinging_door_reset;
		if (self->moveinfo.wait > 0)
			self->nextthink = level.time + self->moveinfo.wait;
		else
			self->think(self);
		return;
	}
	if (self->spawnflags & DOOR_TOGGLE)
	{
		if (self->flags & FL_BOB)
		{
			self->think = bob_init;
			self->nextthink = level.time + FRAMETIME;
		}
		return;
	}
	if (self->moveinfo.wait >= 0)
	{
		self->think = door_go_down;
		self->nextthink = level.time + self->moveinfo.wait;
	}
	else if (self->flags & FL_BOB)
	{
		self->think = bob_init;
		self->nextthink = level.time + FRAMETIME;
	}
}

void door_hit_bottom (edict_t *self)
{
	self->do_not_rotate = false;
	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->s.sound && self->moveinfo.sound_end)
			gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC
	//	self->s.sound = 0;
	}
	self->s.sound = 0;	// Knightmare- make sure this is always set to 0, lead mover or not!

	self->moveinfo.state = STATE_BOTTOM;
	door_use_areaportals (self, false);

	if (self->flags & FL_BOB)
	{
		self->think = bob_init;
		self->nextthink = level.time + FRAMETIME;
	}
}

void door_go_down (edict_t *self)
{
	self->do_not_rotate = true;
	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_start)
			gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}
	if (self->max_health)
	{
		self->takedamage = DAMAGE_YES;
		self->health = self->max_health;
	}

	if (strcmp(self->classname.c_str(), "func_door") == 0)
	{
		if (self->movewith)
			movewith_update(self);
		self->moveinfo.state = STATE_DOWN;
		Move_Calc (self, self->moveinfo.start_origin, door_hit_bottom);
	}
	else if (strcmp(self->classname.c_str(), "func_door_rotating") == 0) {
		self->moveinfo.state = STATE_DOWN;
		AngleMove_Calc (self, door_hit_bottom);
	}
	else if (strcmp(self->classname.c_str(), "func_door_rot_dh") == 0) {
		self->moveinfo.state = STATE_DOWN;
		AngleMove_Calc (self, door_hit_bottom);
	}
}

void door_go_up (edict_t *self, edict_t *activator)
{
	self->do_not_rotate = true;
	if (self->moveinfo.state == STATE_UP)
		return;		// already going up

	if (self->moveinfo.state == STATE_TOP)
	{	// reset top wait time
		if (self->moveinfo.wait >= 0)
			self->nextthink = level.time + self->moveinfo.wait;
		return;
	}

	if ((self->flags & FL_REVERSIBLE) && activator)
		check_reverse_rotation (self, activator->s.origin);

	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_start)
			gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}
	if (strcmp(self->classname.c_str(), "func_door") == 0)
	{
		if (self->movewith)
			movewith_update(self);
		self->moveinfo.state = STATE_UP;
		Move_Calc (self, self->moveinfo.end_origin, door_hit_top);
	}
	else if (strcmp(self->classname.c_str(), "func_door_rotating") == 0) {
		self->moveinfo.state = STATE_UP;
		AngleMove_Calc (self, door_hit_top);
	}
	else if (strcmp(self->classname.c_str(), "func_door_rot_dh") == 0) {
		self->moveinfo.state = STATE_UP;
		AngleMove_Calc (self, door_hit_top);
	}

	G_UseTargets (self, activator);
	door_use_areaportals (self, true);
}

void door_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*ent;

	if (self->flags & FL_TEAMSLAVE)
		return;

	if (self->spawnflags & DOOR_TOGGLE)
	{
		if (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)
		{
			// trigger all paired doors
			for (ent = self ; ent ; ent = ent->teamchain)
			{
				ent->message = std::string();
				ent->touch = nullptr;
				door_go_down (ent);
			}
			return;
		}
	}

	// trigger all paired doors
	for (ent = self ; ent ; ent = ent->teamchain)
	{
		ent->message = std::string();
		ent->touch = nullptr;
		door_go_up (ent, activator);
	}
};

void Touch_DoorTrigger (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->health <= 0)
		return;

	if (!(other->svflags & SVF_MONSTER) && (!other->client))
		return;

	// Lazarus: allow robots to open NOMONSTER doors
	if ((self->owner->spawnflags & DOOR_NOMONSTER) && (other->svflags & SVF_MONSTER) && !(other->flags & FL_ROBOT))
		return;

	if (level.time < self->touch_debounce_time)
		return;
	self->touch_debounce_time = level.time + 1.0;

	door_use (self->owner, other, other);
}

void Think_CalcMoveSpeed (edict_t *self)
{
	edict_t	*ent;
	float	min;
	float	time;
	float	newspeed;
	float	ratio;
	float	dist;

	if (self->flags & FL_TEAMSLAVE)
		return;		// only the team master does this

	// find the smallest distance any member of the team will be moving
	min = fabs(self->moveinfo.distance);
	for (ent = self->teamchain; ent; ent = ent->teamchain)
	{
		dist = fabs(ent->moveinfo.distance);
		if (dist < min)
			min = dist;
	}

	time = min / self->moveinfo.speed;

	// adjust speeds so they will all complete at the same time
	for (ent = self; ent; ent = ent->teamchain)
	{
		newspeed = fabs(ent->moveinfo.distance) / time;
		ratio = newspeed / ent->moveinfo.speed;
		if (ent->moveinfo.accel == ent->moveinfo.speed)
			ent->moveinfo.accel = newspeed;
		else
			ent->moveinfo.accel *= ratio;
		if (ent->moveinfo.decel == ent->moveinfo.speed)
			ent->moveinfo.decel = newspeed;
		else
			ent->moveinfo.decel *= ratio;
		ent->moveinfo.speed = newspeed;
	}
}

void Think_SpawnDoorTrigger (edict_t *ent)
{
	edict_t		*other;
	vec3_t		mins, maxs;
	int			expand;

	if (ent->flags & FL_TEAMSLAVE)
		return;		// only the team leader spawns a trigger

	VectorCopy (ent->absmin, mins);
	VectorCopy (ent->absmax, maxs);

	for (other = ent->teamchain ; other ; other=other->teamchain)
	{
		AddPointToBounds (other->absmin, mins, maxs);
		AddPointToBounds (other->absmax, mins, maxs);
	}

	if (ent->movewith)
		expand = 16;
	else
		expand = 60;

	mins[0] -= expand;
	mins[1] -= expand;
	maxs[0] += expand;
	maxs[1] += expand;

	other = G_Spawn ();
	VectorCopy (mins, other->mins);
	VectorCopy (maxs, other->maxs);
	other->owner = ent;
	other->solid = SOLID_TRIGGER;
	other->movetype = MOVETYPE_NONE;
	other->touch = Touch_DoorTrigger;

	gi.linkentity (other);

	// Lazarus movewith
	if (ent->movewith)
	{
		other->movewith = ent->movewith;
		VectorCopy (ent->s.origin,other->s.origin);
		VectorSubtract(other->mins,other->s.origin,other->mins);
		VectorSubtract(other->maxs,other->s.origin,other->maxs);
		if (ent->movewith_ent) {
			// Uh-oh... movewith_init was already called.. no harm in calling it again
			movewith_init (ent->movewith_ent);
		}
	}

	if (ent->spawnflags & DOOR_START_OPEN)
		door_use_areaportals (ent, true);

	Think_CalcMoveSpeed (ent);
}

void door_blocked  (edict_t *self, edict_t *other)
{
	edict_t	*ent;

	if (!(other->svflags & SVF_MONSTER) && (!other->client) ) {
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other) {
			// Lazarus: Some of our ents don't have origin near the model
			vec3_t save;
			VectorCopy (other->s.origin,save);
			VectorMA (other->absmin, 0.5, other->size, other->s.origin);
			BecomeExplosion1 (other);
		}
		return;
	}

	//if ( !( (!strcmp(self->classname, "func_door_rotating")||!strcmp(self->classname, "func_door_rot_dh")
	//	&& other->groundentity == self) ) )
	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);

	if (self->spawnflags & DOOR_CRUSHER)
		return;

// if a door has a negative wait, it would never come back if blocked,
// so let it just squash the object to death real fast
	if (self->moveinfo.wait >= 0) {
		if (self->moveinfo.state == STATE_DOWN) {
			for (ent = self->teammaster ; ent ; ent = ent->teamchain)
				door_go_up (ent, ent->activator);
		}
		else {
			for (ent = self->teammaster ; ent ; ent = ent->teamchain)
				door_go_down (ent);
		}
	}
}

void func_explosive_explode (edict_t *self);
void door_destroyed (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->moveinfo.state == STATE_DOWN || self->moveinfo.state == STATE_BOTTOM)
		door_use_areaportals(self,true);
	self->dmg = 0;
	func_explosive_explode(self);
}

void door_killed (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t	*ent;

	for (ent = self->teammaster ; ent ; ent = ent->teamchain)
	{
		ent->health = ent->max_health;
		ent->takedamage = DAMAGE_NO;
	}
	door_use (self->teammaster, attacker, attacker);
}

void door_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// Lazarus: Allows robot usage
	if (!other->client && !(other->flags & FL_ROBOT))
		return;

	if (level.time < self->touch_debounce_time)
		return;
	self->touch_debounce_time = level.time + 5.0;

	if (!self->message.empty()) {
		safe_centerprintf (other, "%s", self->message.c_str());
		gi.sound (other, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}
}

void SP_func_door (edict_t *ent)
{
	vec3_t	abs_movedir;

	ent->class_id = ENTITY_FUNC_DOOR;

	if ( (level.maptype == MAPTYPE_CUSTOM) && (ent->sounds > 4) && (ent->sounds < 100) ) // custom sounds
	{
		ent->moveinfo.sound_start = gi.soundindex (va("doors/dr%02i_strt.wav", ent->sounds));
		ent->moveinfo.sound_middle = gi.soundindex (va("doors/dr%02i_mid.wav", ent->sounds));
		ent->moveinfo.sound_end = gi.soundindex (va("doors/dr%02i_end.wav", ent->sounds));
	}
	else if (ent->sounds != 1)
	{
		ent->moveinfo.sound_start = gi.soundindex ("doors/dr1_strt.wav");
		ent->moveinfo.sound_middle = gi.soundindex ("doors/dr1_mid.wav");
		ent->moveinfo.sound_end = gi.soundindex ("doors/dr1_end.wav");
	}
	else
	{
		ent->moveinfo.sound_start = 0;
		ent->moveinfo.sound_middle = 0;
		ent->moveinfo.sound_end = 0;
	}

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;

	G_SetMovedir (ent->s.angles, ent->movedir);
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel (ent, ent->model);

	ent->blocked = door_blocked;
	ent->use = door_use;

	if (!ent->speed)
		ent->speed = 100;
	if (deathmatch->value)
		ent->speed *= 2;

	if (!ent->accel)
		ent->accel = ent->speed;
	if (!ent->decel)
		ent->decel = ent->speed;

	if (!ent->wait)
		ent->wait = 3;
	if (!st.lip)
		st.lip = 8;
	if (!ent->dmg)
		ent->dmg = 2;

	// calculate second position
	VectorCopy (ent->s.origin, ent->pos1);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	ent->moveinfo.distance = abs_movedir[0] * ent->size[0] + abs_movedir[1] * ent->size[1] + abs_movedir[2] * ent->size[2] - st.lip;
	VectorMA (ent->pos1, ent->moveinfo.distance, ent->movedir, ent->pos2);

	// if it starts open, switch the positions
	if (ent->spawnflags & DOOR_START_OPEN)
	{
		VectorCopy (ent->pos2, ent->s.origin);
		VectorCopy (ent->pos1, ent->pos2);
		VectorCopy (ent->s.origin, ent->pos1);
	}

	ent->moveinfo.state = STATE_BOTTOM;

	if (ent->health)
	{
		ent->takedamage = DAMAGE_YES;

		// Lazarus: negative health means "damage kills me" rather than "damage opens me"
		if (ent->health < 0)
		{
			PrecacheDebris(ent->gib_type);
			ent->die = door_destroyed;
			ent->health = -ent->health;
		}
		else
			ent->die = door_killed;
		ent->max_health = ent->health;
	}
	else if (ent->targetname && !ent->message.empty())
	{
		gi.soundindex ("misc/talk.wav");
		ent->touch = door_touch;
	}

	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	VectorCopy (ent->pos1, ent->moveinfo.start_origin);
	VectorCopy (ent->s.angles, ent->moveinfo.start_angles);
	VectorCopy (ent->pos2, ent->moveinfo.end_origin);
	VectorCopy (ent->s.angles, ent->moveinfo.end_angles);

	if (ent->spawnflags & 16)
		ent->s.effects |= EF_ANIM_ALL;
	if (ent->spawnflags & 64)
		ent->s.effects |= EF_ANIM_ALLFAST;

	// to simplify logic elsewhere, make non-teamed doors into a team of one
	if (!ent->team)
		ent->teammaster = ent;

	gi.linkentity (ent);

#ifdef POSTTHINK_CHILD_MOVEMENT
	ent->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	ent->nextthink = level.time + FRAMETIME;
	if (ent->health || ent->targetname )
		ent->think = Think_CalcMoveSpeed;
	else
		ent->think = Think_SpawnDoorTrigger;
}


/*QUAKED func_door_rotating (0 .5 .8) ? START_OPEN REVERSE CRUSHER NOMONSTER ANIMATED TOGGLE X_AXIS Y_AXIS
TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER	monsters will not trigger this door

You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"distance" is how many degrees the door will be rotated.
"speed" determines how fast the door moves; default value is 100.

REVERSE will cause the door to rotate in the opposite direction.

"message"	is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
0)		default
1)		silent
2-4		not used
5-99	custom sound- e.g. doors/dr05_strt.wav, doors/dr05_mid.wav, doors/dr05_end.wav
*/

void SP_func_door_rotating (edict_t *ent)
{
	ent->class_id = ENTITY_FUNC_DOOR_ROTATING;

	VectorClear (ent->s.angles);

	// set the axis of rotation
	// Knightmare- use move_angles for manual setting of axis
	if (VectorLength(ent->move_angles) > 0.0f)	{
		VectorCopy (ent->move_angles, ent->movedir);
		VectorNormalize (ent->movedir);
	}
	else
	{
		VectorClear(ent->movedir);
		if (ent->spawnflags & DOOR_X_AXIS)
			ent->movedir[2] = 1.0;
		else if (ent->spawnflags & DOOR_Y_AXIS)
			ent->movedir[0] = 1.0;
		else // Z_AXIS
			ent->movedir[1] = 1.0;
	}
	// end Knightmare

	// check for reverse rotation
	if (ent->spawnflags & DOOR_REVERSE)
		VectorNegate (ent->movedir, ent->movedir);

	if (!st.distance)
	{
		gi.dprintf("%s at %s with no distance set\n", ent->classname, vtos(ent->s.origin));
		st.distance = 90;
	}

	VectorCopy (ent->s.angles, ent->pos1);
	VectorMA (ent->s.angles, st.distance, ent->movedir, ent->pos2);
	ent->moveinfo.distance = st.distance;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel (ent, ent->model);

	ent->blocked = door_blocked;
	ent->use = door_use;

	if (!ent->speed)
		ent->speed = 100;
	if (!ent->accel)
		ent->accel = ent->speed;
	if (!ent->decel)
		ent->decel = ent->speed;

	if (!ent->wait && !(ent->flags & FL_REVOLVING))	// OK for revolving doors to have 0 wait
		ent->wait = 3;
	if (!ent->dmg)
		ent->dmg = 2;

	if ( (level.maptype == MAPTYPE_CUSTOM) && (ent->sounds > 4) && (ent->sounds < 100) ) // custom sounds
	{
		ent->moveinfo.sound_start = gi.soundindex (va("doors/dr%02i_strt.wav", ent->sounds));
		ent->moveinfo.sound_middle = gi.soundindex (va("doors/dr%02i_mid.wav", ent->sounds));
		ent->moveinfo.sound_end = gi.soundindex (va("doors/dr%02i_end.wav", ent->sounds));
	}
	else if (ent->sounds != 1)
	{
		ent->moveinfo.sound_start = gi.soundindex ("doors/dr1_strt.wav");
		ent->moveinfo.sound_middle = gi.soundindex ("doors/dr1_mid.wav");
		ent->moveinfo.sound_end = gi.soundindex ("doors/dr1_end.wav");
	}
	else
	{
		ent->moveinfo.sound_start = 0;
		ent->moveinfo.sound_middle = 0;
		ent->moveinfo.sound_end = 0;
	}

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;

	// if it starts open, switch the positions
	if (ent->spawnflags & DOOR_START_OPEN)
	{
		VectorCopy (ent->pos2, ent->s.angles);
		VectorCopy (ent->pos1, ent->pos2);
		VectorCopy (ent->s.angles, ent->pos1);
		VectorNegate (ent->movedir, ent->movedir);
	}

	if (ent->health)
	{
		ent->takedamage = DAMAGE_YES;
		// Lazarus: negative health means "damage kills me" rather than "damage opens me"
		if (ent->health < 0)
		{
			PrecacheDebris(ent->gib_type);
			ent->die = door_destroyed;
			ent->health = -ent->health;
		}
		else
			ent->die = door_killed;
		ent->max_health = ent->health;
	}

	if (ent->targetname && !ent->message.empty())
	{
		gi.soundindex ("misc/talk.wav");
		ent->touch = door_touch;
	}

	ent->moveinfo.state = STATE_BOTTOM;
	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	VectorCopy (ent->s.origin, ent->moveinfo.start_origin);
	VectorCopy (ent->pos1, ent->moveinfo.start_angles);
	VectorCopy (ent->s.origin, ent->moveinfo.end_origin);
	VectorCopy (ent->pos2, ent->moveinfo.end_angles);

	if (ent->spawnflags & 16)
		ent->s.effects |= EF_ANIM_ALL;

	// to simplify logic elsewhere, make non-teamed doors into a team of one
	if (!ent->team)
		ent->teammaster = ent;

#ifdef POSTTHINK_CHILD_MOVEMENT
	ent->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	gi.linkentity (ent);

	ent->nextthink = level.time + FRAMETIME;
	if (ent->health || ent->targetname)
		ent->think = Think_CalcMoveSpeed;
	else
		ent->think = Think_SpawnDoorTrigger;
}

/* Lazarus: SP_func_door_rot_dh is identical to func_door_rotating, but allows you
            to build the model in a separate location (as with func_train, to
            get the lighting the way you'd like) then move at runtime to the
            origin of the "pathtarget". */

void func_door_rot_dh_init (edict_t *ent)
{
	edict_t		*new_origin;

	new_origin = G_Find ( nullptr, FOFS(targetname), ent->pathtarget);
	if (new_origin) {
		VectorCopy (new_origin->s.origin,ent->s.origin);
		VectorCopy (ent->s.origin, ent->moveinfo.start_origin);
		VectorCopy (ent->s.origin, ent->moveinfo.end_origin);
		gi.linkentity(ent);
	}
	ent->nextthink = level.time + FRAMETIME;
	if (ent->health || ent->targetname)
		ent->think = Think_CalcMoveSpeed;
	else
		ent->think = Think_SpawnDoorTrigger;
}

void SP_func_door_rot_dh (edict_t *ent)
{
	SP_func_door_rotating (ent);

	if ( !ent->pathtarget )
		return;

	// Wait a few frames so that we're sure pathtarget has been parsed.
	ent->think = func_door_rot_dh_init;
	ent->nextthink = level.time + 2*FRAMETIME;
	gi.linkentity (ent);
}

/*QUAKED func_water (0 .5 .8) ? START_OPEN MUD
func_water is a moveable water brush.  It must be targeted to operate.  Use a non-water texture at your own risk.

START_OPEN causes the water to move to its destination when spawned and operate in reverse.
MUD        turns the water to mud - difficult to move through, swallows players

"angle"		determines the opening direction (up or down only)
"speed"		movement speed (25 default)
"wait"		wait before returning (-1 default, -1 = TOGGLE)
"lip"		lip remaining at end of move (0 default)
"sounds"	(yes, these need to be changed)
0)	no sound
1)	water
2)	lava
*/

void SP_func_water (edict_t *self)
{
	vec3_t	abs_movedir;

	self->class_id = ENTITY_FUNC_WATER;
	G_SetMovedir (self->s.angles, self->movedir);
	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_BSP;
	gi.setmodel (self, self->model);

	if (self->spawnflags & 2)
	{
		level.mud_puddles++;
		self->svflags |= SVF_MUD;
	}

	switch (self->sounds)
	{
		default:
			break;

		case 1: // water
			self->moveinfo.sound_start = gi.soundindex  ("world/mov_watr.wav");
			self->moveinfo.sound_end = gi.soundindex  ("world/stp_watr.wav");
			break;

		case 2: // lava
			self->moveinfo.sound_start = gi.soundindex  ("world/mov_watr.wav");
			self->moveinfo.sound_end = gi.soundindex  ("world/stp_watr.wav");
			break;
	}

	// calculate second position
	VectorCopy (self->s.origin, self->pos1);
	abs_movedir[0] = fabs(self->movedir[0]);
	abs_movedir[1] = fabs(self->movedir[1]);
	abs_movedir[2] = fabs(self->movedir[2]);
	self->moveinfo.distance = abs_movedir[0] * self->size[0] + abs_movedir[1] * self->size[1] + abs_movedir[2] * self->size[2] - st.lip;
	VectorMA (self->pos1, self->moveinfo.distance, self->movedir, self->pos2);

	// if it starts open, switch the positions
	if (self->spawnflags & DOOR_START_OPEN)
	{
		VectorCopy (self->pos2, self->s.origin);
		VectorCopy (self->pos1, self->pos2);
		VectorCopy (self->s.origin, self->pos1);
	}

	VectorCopy (self->pos1, self->moveinfo.start_origin);
	VectorCopy (self->s.angles, self->moveinfo.start_angles);
	VectorCopy (self->pos2, self->moveinfo.end_origin);
	VectorCopy (self->s.angles, self->moveinfo.end_angles);

	self->moveinfo.state = STATE_BOTTOM;

	if (!self->speed)
		self->speed = 25;
	self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed = self->speed;

	if (!self->wait)
		self->wait = -1;
	self->moveinfo.wait = self->wait;

	self->use = door_use;

	if (self->wait == -1)
		self->spawnflags |= DOOR_TOGGLE;

#ifdef POSTTHINK_CHILD_MOVEMENT
	self->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	self->classname = "func_door";

	gi.linkentity (self);
}


/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
noise	looping sound to play when the train is in motion

*/
// Lazarus: Added health key to func_train
void train_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t	*e, *next;

	if (self->deathtarget)
	{
		self->target = self->deathtarget;
		G_UseTargets (self, attacker);
	}
	e = self->movewith_next;
	while (e)
	{
		next = e->movewith_next;
		e->nextthink = 0;
		if (e->takedamage)
			T_Damage (e, self, self, vec3_origin, e->s.origin, vec3_origin, 100000, 1, DAMAGE_NO_PROTECTION, MOD_CRUSH);
		else if (e->die)
			e->die(e,self,self,100000,e->s.origin);
		else if (e->solid == SOLID_NOT)
			G_FreeEdict(e);
		else
			BecomeExplosion1 (e);
		e = next;
	}
	BecomeExplosion1 (self);
}

void train_next (edict_t *self);

void train_blocked (edict_t *self, edict_t *other)
{
	if (!(other->svflags & SVF_MONSTER) && (!other->client) )
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other)
		{
			// Lazarus: Some of our ents don't have origin near the model
			vec3_t save;
			VectorCopy (other->s.origin,save);
			VectorMA (other->absmin, 0.5, other->size, other->s.origin);
			BecomeExplosion1 (other);
		}
		return;
	}

	if (level.time < self->touch_debounce_time)
		return;

	if (!self->dmg)
		return;
	self->touch_debounce_time = level.time + 0.5;
	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void train_wait (edict_t *self)
{
	if (self->target_ent->pathtarget)
	{
		char	*savetarget;
		edict_t	*ent;

		ent = self->target_ent;
		savetarget = ent->target;
		ent->target = ent->pathtarget;
		G_UseTargets (ent, self->activator);
		ent->target = savetarget;

		// make sure we didn't get killed by a killtarget
		if (!self->inuse)
			return;
	}

	// Lazarus: rotating trains
//	if ( self->target_ent )
	if ( self->target_ent && (level.maptype != MAPTYPE_ROGUE) )	// Knightmare- skip this for Rogue maps
	{
		if (self->target_ent->speed)
		{
			self->speed = self->target_ent->speed;
			self->moveinfo.speed = self->speed;
			self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;
		}
		if (self->spawnflags & TRAIN_ROTATE)
		{
			if (self->target_ent->pitch_speed)
				self->pitch_speed = self->target_ent->pitch_speed;
			if (self->target_ent->yaw_speed)
				self->yaw_speed   = self->target_ent->yaw_speed;
			if (self->target_ent->roll_speed)
				self->roll_speed  = self->target_ent->roll_speed;
		}
		else if (self->spawnflags & TRAIN_ROTATE_CONSTANT)
		{
			if (self->target_ent->pitch_speed)
				self->pitch_speed += self->target_ent->pitch_speed;
			if (self->target_ent->yaw_speed)
				self->yaw_speed   += self->target_ent->yaw_speed;
			if (self->target_ent->roll_speed)
				self->roll_speed  += self->target_ent->roll_speed;
		}
	}

	if (self->moveinfo.wait)
	{
		// Spline trains stop rotating when waiting
		if (self->spawnflags & TRAIN_SPLINE)
		{
			VectorClear (self->avelocity);
			VectorClear (self->velocity);
#ifndef POSTTHINK_CHILD_MOVEMENT
			if (self->movewith_next && (self->movewith_next->movewith_ent == self))
				set_child_movement (self);
#endif	// POSTTHINK_CHILD_MOVEMENT
		}

		if (self->moveinfo.wait > 0)
		{
			// Lazarus: turn off animation for stationary trains
			if (!strcmp(self->classname.c_str(), "func_train"))
				self->s.effects &= ~(EF_ANIM_ALL | EF_ANIM_ALLFAST);
			self->nextthink = level.time + self->moveinfo.wait;
			self->think = train_next;
		}
		else if (self->spawnflags & TRAIN_TOGGLE)  // && wait < 0
		{
			// Knightmare- for Rogue maps, let train_next wait until we get called
			if (level.maptype == MAPTYPE_ROGUE) {
				self->target_ent = nullptr;
			}
			else {
				train_next (self);
			}
			self->spawnflags &= ~TRAIN_START_ON;
			VectorClear (self->velocity);
			if ( !(self->spawnflags & TRAIN_ROTATE_CONSTANT) )
				VectorClear (self->avelocity); // Knightmare added
			// Lazarus: turn off animation for stationary trains
			if (!strcmp(self->classname.c_str(), "func_train"))
				self->s.effects &= ~(EF_ANIM_ALL | EF_ANIM_ALLFAST);
			self->nextthink = 0;
		}

		if (!(self->flags & FL_TEAMSLAVE))
		{
			if (self->s.sound && self->moveinfo.sound_end)
				gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC
			self->s.sound = 0;
		}
	}
	else
	{
		if (self->spawnflags & TRAIN_SPLINE)
			set_child_movement (self);
		train_next (self);
	}

}

// Knightmare- dummy function for Rogue train teams
void train_piece_wait (edict_t *self)
{
}

// Rroff's rotating train stuff, with quite a few changes

void train_yaw (edict_t *self);
void train_spline (edict_t *self);
void train_children_think (edict_t *self)
{
	if (!self || !self->enemy) return;

	if (self->enemy->spawnflags & TRAIN_ROTATE)
	{
		// The the train was changed from TRAIN_ROTATE_CONSTANT to TRAIN_ROTATE
		// by a target_change... get da hell outta here.
		self->think = train_yaw;
		self->think (self);
		return;
	}

#ifndef POSTTHINK_CHILD_MOVEMENT
	if (self->enemy->movewith_next && (self->enemy->movewith_next->movewith_ent == self->enemy))
	{
		set_child_movement (self->enemy);
		self->nextthink = level.time + FRAMETIME;
	}
	else if (level.time < 2)
#endif	// POSTTHINK_CHILD_MOVEMENT
		self->nextthink = level.time + FRAMETIME;
}

void train_yaw (edict_t *self)
{
	float	cur_yaw, idl_yaw, cur_pitch, idl_pitch, cur_roll, idl_roll;
	float	yaw_vel, pitch_vel, roll_vel;
	float	Dist_1, Dist_2, Distance;

	if (!self || !self->enemy || !self->enemy->inuse)
		return;

	if (self->enemy->spawnflags & TRAIN_ROTATE_CONSTANT)
	{
		// The the train was changed from TRAIN_ROTATE to TRAIN_ROTATE_CONSTANT
		// by a target_change... get da hell outta here.
		self->think = train_children_think;
		self->think (self); // crashes here
		return;
	}

	cur_yaw   = self->enemy->s.angles[YAW];
	idl_yaw   = self->enemy->ideal_yaw;
	cur_pitch = self->enemy->s.angles[PITCH];
	idl_pitch = self->enemy->ideal_pitch;
	cur_roll  = self->enemy->s.angles[ROLL];
	idl_roll  = self->enemy->ideal_roll;

//	gi.dprintf("current angles=%g %g %g, ideal angles=%g %g %g\n",
//		cur_pitch,cur_yaw,cur_roll,idl_pitch,idl_yaw,idl_roll);

	yaw_vel   = self->enemy->yaw_speed;
	pitch_vel = self->enemy->pitch_speed;
	roll_vel  = self->enemy->roll_speed;

	if (cur_yaw == idl_yaw)
		self->enemy->avelocity[YAW] = 0;
	if (cur_pitch == idl_pitch)
		self->enemy->avelocity[PITCH] = 0;
	if (cur_roll == idl_roll)
		self->enemy->avelocity[ROLL] = 0;
	if ((cur_yaw == idl_yaw) && (cur_pitch == idl_pitch) && (cur_roll == idl_roll) )
	{
		self->nextthink = level.time + FRAMETIME;

#ifndef POSTTHINK_CHILD_MOVEMENT
		if (self->enemy->movewith_next && (self->enemy->movewith_next->movewith_ent == self->enemy))
			set_child_movement (self->enemy);
#endif	// POSTTHINK_CHILD_MOVEMENT

		return;
	}

	if (cur_yaw != idl_yaw) {
		if (cur_yaw < idl_yaw)
		{
			Dist_1 = (idl_yaw - cur_yaw)*10;
			Dist_2 = ((360 - idl_yaw) + cur_yaw)*10;

			if (Dist_1 < Dist_2)
			{
				Distance = Dist_1;

				if (Distance < yaw_vel)
					yaw_vel = Distance;

				self->enemy->avelocity[YAW] = yaw_vel;
			}
			else
			{
				Distance = Dist_2;

				if (Distance < yaw_vel)
					yaw_vel = Distance;

				self->enemy->avelocity[YAW] = -yaw_vel;
			}
		}
		else
		{
			Dist_1 = (cur_yaw - idl_yaw)*10;
			Dist_2 = ((360 - cur_yaw) + idl_yaw)*10;

			if (Dist_1 < Dist_2)
			{
				Distance = Dist_1;

				if (Distance < yaw_vel)
					yaw_vel = Distance;

				self->enemy->avelocity[YAW] = -yaw_vel;
			}
			else
			{
				Distance = Dist_2;

				if (Distance < yaw_vel)
					yaw_vel = Distance;

				self->enemy->avelocity[YAW] = yaw_vel;
			}
		}

		//	gi.dprintf ("train cy: %g iy: %g ys: %g\n", cur_yaw, idl_yaw, self->enemy->avelocity[1]);

		if (self->enemy->s.angles[YAW] < 0)
			self->enemy->s.angles[YAW] += 360;

		if (self->enemy->s.angles[YAW] >= 360)
			self->enemy->s.angles[YAW] -= 360;
	}

	if (cur_pitch != idl_pitch) {

		if (cur_pitch < idl_pitch)
		{
			Dist_1 = (idl_pitch - cur_pitch)*10;
			Dist_2 = ((360 - idl_pitch) + cur_pitch)*10;

			if (Dist_1 < Dist_2)
			{
				Distance = Dist_1;

				if (Distance < pitch_vel)
					pitch_vel = Distance;

				self->enemy->avelocity[PITCH] = pitch_vel;
			}
			else
			{
				Distance = Dist_2;

				if (Distance < pitch_vel)
					pitch_vel = Distance;

				self->enemy->avelocity[PITCH] = -pitch_vel;
			}
		}
		else
		{
			Dist_1 = (cur_pitch - idl_pitch)*10;
			Dist_2 = ((360 - cur_pitch) + idl_pitch)*10;

			if (Dist_1 < Dist_2)
			{
				Distance = Dist_1;

				if (Distance < pitch_vel)
					pitch_vel = Distance;

				self->enemy->avelocity[PITCH] = -pitch_vel;
			}
			else
			{
				Distance = Dist_2;

				if (Distance < pitch_vel)
					pitch_vel = Distance;

				self->enemy->avelocity[PITCH] = pitch_vel;
			}
		}

		if (self->enemy->s.angles[PITCH] <  0)
			self->enemy->s.angles[PITCH] += 360;

		if (self->enemy->s.angles[PITCH] >= 360)
			self->enemy->s.angles[PITCH] -= 360;
	}

	if (cur_roll != idl_roll) {
		if (cur_roll < idl_roll)
		{
			Dist_1 = (idl_roll - cur_roll)*10;
			Dist_2 = ((360 - idl_roll) + cur_roll)*10;

			if (Dist_1 < Dist_2)
			{
				Distance = Dist_1;

				if (Distance < roll_vel)
					roll_vel = Distance;

				self->enemy->avelocity[ROLL] = roll_vel;
			}
			else
			{
				Distance = Dist_2;

				if (Distance < roll_vel)
					roll_vel = Distance;

				self->enemy->avelocity[ROLL] = -roll_vel;
			}
		}
		else
		{
			Dist_1 = (cur_roll - idl_roll)*10;
			Dist_2 = ((360 - cur_roll) + idl_roll)*10;

			if (Dist_1 < Dist_2)
			{
				Distance = Dist_1;

				if (Distance < roll_vel)
					roll_vel = Distance;

				self->enemy->avelocity[ROLL] = -roll_vel;
			}
			else
			{
				Distance = Dist_2;

				if (Distance < roll_vel)
					roll_vel = Distance;

				self->enemy->avelocity[ROLL] = roll_vel;
			}
		}

		if (self->enemy->s.angles[ROLL] < 0)
			self->enemy->s.angles[ROLL] += 360;

		if (self->enemy->s.angles[ROLL] >= 360)
			self->enemy->s.angles[ROLL] -= 360;
	}

#ifndef POSTTHINK_CHILD_MOVEMENT
	if (self->enemy->movewith_next && (self->enemy->movewith_next->movewith_ent == self->enemy))
		set_child_movement (self->enemy);
#endif	// POSTTHINK_CHILD_MOVEMENT

	self->nextthink = level.time + FRAMETIME;
}

void train_next (edict_t *self)
{
	edict_t		*ent;
	vec3_t		dest;
	qboolean	first;
	vec3_t		daoldorigin;
	vec3_t		angles, v;
	vec3_t		adjusted_pathpoint;
	vec3_t		corner_offset = {1,1,1};

	// Knightmare- func_train_origin support
//	if (self->spawnflags & TRAIN_ORIGIN)
//		gi.dprintf ("train_next: pathing by train origin\n");

	first = true;
again:
	if (!self->target)
	{
//		gi.dprintf ("train_next: no next target\n");
		self->s.sound = 0;
		return;
	}

	ent = G_PickTarget (self->target);
	if (!ent)
	{
		gi.dprintf ("train_next: bad target %s\n", self->target);
		return;
	}

	// Knightmare- calc the real target for the train's mins,
	// since that is 1 unit below the corner of the bmodel in all 3 dimensions
	if (adjust_train_corners->value)
		VectorSubtract(ent->s.origin, corner_offset, adjusted_pathpoint);
	else
		VectorCopy (ent->s.origin, adjusted_pathpoint);

	self->target = ent->target;

	// Rroff: path_corners can control train speed
	// DHW: This shouldn't go here... we're one path_corner behind.
	//      Set speed before train_next is called
	/*if (ent->speed)
		self->speed = ent->speed;
	self->moveinfo.speed = self->speed; */

	// check for a teleport path_corner
	if (ent->spawnflags & 1)
	{
		if (!first)
		{
			gi.dprintf ("connected teleport path_corners, see %s at %s\n", ent->classname, vtos(ent->s.origin));
			return;
		}
		first = false;
		VectorCopy (self->s.origin, daoldorigin); // Knightmare- copy old orgin for reference
		if (self->spawnflags & TRAIN_ORIGIN)	// Knightmare- func_train_origin support
			VectorCopy (ent->s.origin, self->s.origin);
		else
			VectorSubtract (adjusted_pathpoint, self->mins, self->s.origin); // was ent->s.origin
		VectorCopy (self->s.origin, self->s.old_origin);
		self->s.event = EV_OTHER_TELEPORT;
		gi.linkentity (self);

#ifndef POSTTHINK_CHILD_MOVEMENT
		if (self->movewith_next && (self->movewith_next->movewith_ent == self))
			set_child_movement (self);
#else	// POSTTHINK_CHILD_MOVEMENT
		// Knightmare- move movewith children
		if (self->movewith_next && (self->movewith_next->movewith_ent == self))
		{
			edict_t	*e = nullptr;
			vec3_t	dir;

			VectorSubtract (self->s.origin, daoldorigin, dir);

			e = self->movewith_next;
			// cycle through all ents to find ones to move with
			while (e != nullptr )
			{
				if (!e->inuse) break;

				VectorAdd (dir, e->s.origin, e->s.origin);
				VectorCopy (e->s.origin, e->s.old_origin);
				e->s.event = EV_OTHER_TELEPORT;
				gi.linkentity (e);

				e = e->movewith_next;
			}
		}
#endif	// POSTTHINK_CHILD_MOVEMENT

		goto again;
	}

	// Knightmare- handle speed changes for Rogue maps
	if ( (ent->speed) && (level.maptype == MAPTYPE_ROGUE) )
	{
		self->speed = ent->speed;
		self->moveinfo.speed = ent->speed;
		if (ent->accel)
			self->moveinfo.accel = ent->accel;
		else
			self->moveinfo.accel = ent->speed;
		if (ent->decel)
			self->moveinfo.decel = ent->decel;
		else
			self->moveinfo.decel = ent->speed;
		self->moveinfo.current_speed = 0;
	}
	// end Knightmare

	self->moveinfo.wait = ent->wait;
	self->target_ent = ent;

	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_start)
			gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}

	if (self->spawnflags & TRAIN_ORIGIN)	// Knightmare- func_train_origin support
		VectorCopy (ent->s.origin, dest);
	else
		VectorSubtract (adjusted_pathpoint, self->mins, dest); // was ent->s.origin
	self->moveinfo.state = STATE_TOP;
	VectorCopy (self->s.origin, self->moveinfo.start_origin);
	VectorCopy (dest, self->moveinfo.end_origin);

	if (self->spawnflags & TRAIN_SPLINE)
	{
		float	speed;
		int		frames;

		self->from = self->to;
		self->to   = ent;
		self->moveinfo.ratio = 0.0f;

		VectorSubtract(dest,self->s.origin,v);
		self->moveinfo.distance = VectorLength(v);
		frames = (int)(10 * self->moveinfo.distance/self->speed);
		if (frames < 1) frames = 1;
		speed = (10*self->moveinfo.distance)/(float)frames;
		self->moveinfo.speed = speed;
		self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;
#ifndef POSTTHINK_CHILD_MOVEMENT
		if (self->movewith_next && (self->movewith_next->movewith_ent == self))
			set_child_movement (self);
#endif	// POSTTHINK_CHILD_MOVEMENT
	}

	// Rroff rotating
	if ( (self->spawnflags & TRAIN_ROTATE) && !(ent->spawnflags & 2) )
	{
		// Lazarus: No no no :-). This is measuring from the center
		//          of the func_train to the path_corner. Should
		//          be path_corner to path_corner.
		//VectorSubtract (ent->s.origin, self->s.origin, v);
		if (self->spawnflags & TRAIN_ORIGIN) {	// Knightmare- func_train_origin support
			VectorSubtract(ent->s.origin, self->s.origin, v);
		}
		else {
			VectorAdd(self->s.origin,self->mins,v);
			VectorSubtract(adjusted_pathpoint, v, v); // was ent->s.origin
		}
		vectoangles2 (v, angles);
		self->ideal_yaw = angles[YAW];
		self->ideal_pitch = angles[PITCH];
		if (self->ideal_pitch < 0) self->ideal_pitch += 360;
		self->ideal_roll = ent->roll;

		VectorClear (self->movedir);
		self->movedir[1] = 1.0;

	}
	/* Lazarus: We don't want to do this... this would give an
	//          instantaneous change in pitch and roll and look
	//          pretty goofy. Instead we'll set the new ideal_pitch
	//          to the path_corner's angles[PITCH], and move to that
	//          angle at pitch_speed. Roll changes? Can't do it (yet)
	if (ent->count)
		if (ent->count >= 0 && ent->count <= 360)
			self->s.angles[PITCH] = ent->count;

	if (ent->sounds)
		if (ent->sounds >= 0 && ent->sounds <= 360)
			self->s.angles[ROLL] = ent->sounds;
	*/
	// end Rroff

	// Lazarus:
	if (self->spawnflags & TRAIN_ROTATE_CONSTANT)
	{
		self->avelocity[PITCH] = self->pitch_speed;
		self->avelocity[YAW]   = self->yaw_speed;
		self->avelocity[ROLL]  = self->roll_speed;
	}

	Move_Calc (self, dest, train_wait);
	self->spawnflags |= TRAIN_START_ON;

	// Knightmare- special team hack for Rogue maps
	if ( self->team && (level.maptype == MAPTYPE_ROGUE) )
	{
		edict_t	*teamEnt;
		vec3_t	teamDir, teamDest;
		int		num_pieces_found = 0;

		VectorSubtract (dest, self->s.origin, teamDir);
		for (teamEnt = self->teamchain; teamEnt; teamEnt = teamEnt->teamchain)
		{
			VectorAdd (teamDir, teamEnt->s.origin, teamDest);
			VectorCopy (teamEnt->s.origin, teamEnt->moveinfo.start_origin);
			VectorCopy (teamDest, teamEnt->moveinfo.end_origin);

			teamEnt->moveinfo.state = STATE_TOP;
			teamEnt->speed = self->speed;
			teamEnt->moveinfo.speed = self->moveinfo.speed;
			teamEnt->moveinfo.accel = self->moveinfo.accel;
			teamEnt->moveinfo.decel = self->moveinfo.decel;
			teamEnt->movetype = MOVETYPE_PUSH;
			Move_Calc (teamEnt, teamDest, train_piece_wait);
			num_pieces_found++;
		}
	}
	// end Knightmare
}

void train_resume (edict_t *self)
{
	edict_t		*ent;
	vec3_t		dest;
	vec3_t		adjusted_pathpoint;
	vec3_t		corner_offset = {1,1,1};

	ent = self->target_ent;

	// Knightmare- calc the real target for the train's mins,
	// since that is 1 unit below the corner of the bmodel in all 3 dimensions
	if (adjust_train_corners->value)
		VectorSubtract(ent->s.origin, corner_offset, adjusted_pathpoint);
	else
		VectorCopy (ent->s.origin, adjusted_pathpoint);

	if (self->spawnflags & TRAIN_ORIGIN)	// Knightmare- func_train_origin support
		VectorCopy (ent->s.origin, dest);
	else
		VectorSubtract (adjusted_pathpoint, self->mins, dest); //was ent->s.origin
	self->moveinfo.state = STATE_TOP;
	VectorCopy (self->s.origin, self->moveinfo.start_origin);
	VectorCopy (dest, self->moveinfo.end_origin);
	Move_Calc (self, dest, train_wait);
	self->spawnflags |= TRAIN_START_ON;
	// Lazarus:
	if (self->spawnflags & TRAIN_ROTATE_CONSTANT)
	{
		self->avelocity[PITCH] = self->pitch_speed;
		self->avelocity[YAW]   = self->yaw_speed;
		self->avelocity[ROLL]  = self->roll_speed;
	}
}

void func_train_find (edict_t *self)
{
	edict_t		*ent;
	vec3_t		daoldorigin;
	vec3_t		adjusted_pathpoint;
	vec3_t		corner_offset = {1,1,1};

	if (!self->target)
	{
		gi.dprintf ("train_find: no target\n");
		return;
	}
	ent = G_PickTarget (self->target);
	if (!ent)
	{
		gi.dprintf ("train_find: target %s not found\n", self->target);
		return;
	}

	// Lazarus: trains can change speed at path_corners
	if (ent->speed) {
		self->speed = ent->speed;
		self->moveinfo.speed = self->speed;
		self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;
	}
	if (ent->pitch_speed)
		self->pitch_speed = ent->pitch_speed;
	if (ent->yaw_speed)
		self->yaw_speed   = ent->yaw_speed;
	if (ent->roll_speed)
		self->roll_speed  = ent->roll_speed;

	// Lazarus: spline stuff
	self->from = self->to = ent;
	// end spline stuff

	self->target = ent->target;

	// Knightmare- calc the real target for the train's mins,
	// since that is 1 unit below the corner of the bmodel in all 3 dimensions
	if (adjust_train_corners->value)
		VectorSubtract(ent->s.origin, corner_offset, adjusted_pathpoint);
	else
		VectorCopy (ent->s.origin, adjusted_pathpoint);

	if (self->spawnflags & TRAIN_ROTATE) {
		ent->think = train_yaw;
	}
	else if (self->spawnflags & TRAIN_SPLINE) {
		ent->think = train_spline;
	}
	else {
		ent->think = train_children_think;
	}
	ent->enemy = self;
	ent->nextthink = level.time + FRAMETIME;

#ifdef POSTTHINK_CHILD_MOVEMENT
	self->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	VectorCopy (self->s.origin, daoldorigin); // Knightmare- copy old orgin for reference
	if (self->spawnflags & TRAIN_ORIGIN)	// Knightmare- func_train_origin support
		VectorCopy (ent->s.origin, self->s.origin);
	else
		VectorSubtract (adjusted_pathpoint, self->mins, self->s.origin); // was ent->s.origin
	gi.linkentity (self);

	// Knightmare- move movewith pieces to spawning point
	if (self->movewith_next && (self->movewith_next->movewith_ent == self))
	{
		edict_t	*e = nullptr;
		vec3_t	dir;

		VectorSubtract (self->s.origin, daoldorigin, dir);
		for (e = self->movewith_next; e; e = e->movewith_next)
		{
			if (!e->inuse)
				break;
			if (e->classname.empty())
				continue;

			// Knightmare- save distance moved for turret_breach to add to its firing point
			if (!strcmp(e->classname.c_str(), "turret_breach") || !strcmp(e->classname.c_str(), "model_turret"))
				VectorCopy (dir, e->offset);

			VectorAdd (dir, e->s.origin, e->s.origin);
			VectorCopy (e->s.origin, e->s.old_origin);
			// This is now the child's original position
			if ( (e->solid == SOLID_BSP) && strcmp(e->classname.c_str(), "func_rotating") && strcmp(e->classname.c_str(), "func_door_rotating"))
				ReInitialize_Entity (e);
			gi.linkentity (e);
		}
	}

	// Knightmare- no sound or animation if we're not moving yet
	// sound is turned on in train_next
	self->s.sound = 0;
	self->s.effects &= ~(EF_ANIM_ALL | EF_ANIM_ALLFAST);

	// if not triggered, start immediately
	if (!self->targetname)
		self->spawnflags |= TRAIN_START_ON;

	if (self->spawnflags & TRAIN_START_ON)
	{
		// Lazarus: animated trains
		if (!strcmp(self->classname.c_str(), "func_train"))
		{
			if (self->spawnflags & TRAIN_ANIMATE)
				self->s.effects |= EF_ANIM_ALL;
			else if (self->spawnflags & TRAIN_ANIMATE_FAST)
				self->s.effects |= EF_ANIM_ALLFAST;
		}
		self->nextthink = level.time + FRAMETIME;
		self->think = train_next;
		self->activator = self;
	}
}

void train_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;

	if (self->spawnflags & TRAIN_START_ON)
	{
		if (!(self->spawnflags & TRAIN_TOGGLE))
			return;
		self->spawnflags &= ~TRAIN_START_ON;
		VectorClear (self->velocity);
		VectorClear (self->avelocity);
		if (!strcmp(self->classname.c_str(), "func_train"))
			self->s.effects &= ~(EF_ANIM_ALL | EF_ANIM_ALLFAST);
		self->nextthink = 0;
	}
	else
	{
		if (!strcmp(self->classname.c_str(), "func_train"))
		{
			if (self->spawnflags & TRAIN_ANIMATE)
				self->s.effects |= EF_ANIM_ALL;
			else if (self->spawnflags & TRAIN_ANIMATE_FAST)
				self->s.effects |= EF_ANIM_ALLFAST;
		}
		if (self->spawnflags & TRAIN_SPLINE)
		{
			// Back up a step
			self->moveinfo.ratio -= self->moveinfo.speed * FRAMETIME / self->moveinfo.distance;
			if (self->moveinfo.ratio < 0.0f)
				self->moveinfo.ratio = 0.0f;
		}

		if (self->target_ent)
			train_resume(self);
		else
			train_next(self);
	}
}

void SP_func_train (edict_t *self)
{
	self->class_id = ENTITY_FUNC_TRAIN;

	self->movetype = MOVETYPE_PUSH;

	if ( (self->spawnflags & (TRAIN_ROTATE | TRAIN_ROTATE_CONSTANT)) == (TRAIN_ROTATE | TRAIN_ROTATE_CONSTANT))
	{
//		gi.dprintf("%s has ROTATE and ROTATE_CONSTANT flags set.\n",
//			self->classname);
//		G_FreeEdict(self);
//		return;
		self->spawnflags &= ~(TRAIN_ROTATE | TRAIN_ROTATE_CONSTANT);
		self->spawnflags |= TRAIN_SPLINE;
	}

	VectorClear (self->s.angles);
	self->blocked = train_blocked;
	if (self->spawnflags & TRAIN_BLOCK_STOPS)
		self->dmg = 0;
	else
	{
		if (!self->dmg)
			self->dmg = 100;
	}
	self->solid = SOLID_BSP;
	gi.setmodel (self, self->model);

	// usermodel (for alias model vehicles, etc) goes on modelindex2
	if (self->usermodel && strlen(self->usermodel)) {
		char	modelname[256];
		// check for "models/" already in path
		if ( !strncmp(self->usermodel, "models/", 7) )
			snprintf (modelname, sizeof(modelname), "%s", self->usermodel);
		else
			snprintf (modelname, sizeof(modelname), "models/%s", self->usermodel);
		self->s.modelindex2 = gi.modelindex (modelname);
	}

	if (st.noise)
		self->moveinfo.sound_middle = gi.soundindex  (st.noise);

	if (self->attenuation <= 0)
		self->attenuation = ATTN_IDLE;

	if (!self->speed)
		self->speed = 100;

	// Lazarus: Do NOT set default values for rotational speeds - if they're 0, then they're 0.

	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;

	self->use = train_use;

	// Lazarus: damageable
	if (self->health) {
		self->die = train_die;
		self->takedamage = DAMAGE_YES;
	}
	else {
		self->die = nullptr;
		self->takedamage = DAMAGE_NO;
	}

	gi.linkentity (self);
	if (self->target)
	{
		// start trains on the second frame, to make sure their targets have had
		// a chance to spawn
		self->nextthink = level.time + FRAMETIME;
		self->think = func_train_find;
	}
	else
	{
		gi.dprintf ("func_train without a target at %s\n", vtos(self->absmin));
	}

	// Lazarus: TRAIN_SMOOTH forces trains to go directly to Move_Done from
	//       Move_Final rather than slowing down (if necessary) for one
	//       frame.
	if (self->spawnflags & TRAIN_SMOOTH)
		self->smooth_movement = true;
	else
		self->smooth_movement = false;


	// Lazarus: make noise field work w/o origin brush
	// ver. 1.3 change - do this for ALL trains
//	if (st.noise && !VectorLength(self->s.origin) )
	if (st.noise)
	{
		edict_t *speaker;

		self->noise_index		= self->moveinfo.sound_middle;
		self->moveinfo.sound_middle = 0;
		speaker					= G_Spawn();
		speaker->classname		= "moving_speaker";
		speaker->s.sound		= 0;
		speaker->volume			= 1;
		speaker->attenuation	= self->attenuation; // was 3
		speaker->owner			= self;
		speaker->think			= Moving_Speaker_Think;
		speaker->nextthink		= level.time + 2*FRAMETIME;
		speaker->spawnflags		= 7;       // owner must be moving to play
		self->speaker			= speaker;
		if ( VectorLength(self->s.origin) )
			VectorCopy (self->s.origin, speaker->s.origin);
		else {
			VectorAdd (self->absmin, self->absmax, speaker->s.origin);
			VectorScale (speaker->s.origin, 0.5, speaker->s.origin);
		}
		VectorSubtract (speaker->s.origin, self->s.origin, speaker->offset);
	}

}

void SP_func_train_origin (edict_t *self)
{
	self->spawnflags |= TRAIN_ORIGIN;
	self->classname = "func_train";
	SP_func_train (self);
}


/*QUAKED trigger_elevator (0.3 0.1 0.6) (-8 -8 -8) (8 8 8)
*/
void trigger_elevator_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *target;

	if (self->movetarget->nextthink)
	{
//		gi.dprintf("elevator busy\n");
		return;
	}

	if (!other->pathtarget)
	{
		gi.dprintf("elevator used with no pathtarget\n");
		return;
	}

	target = G_PickTarget (other->pathtarget);
	if (!target)
	{
		gi.dprintf("elevator used with bad pathtarget: %s\n", other->pathtarget);
		return;
	}

	self->movetarget->target_ent = target;
	train_resume (self->movetarget);
}

void trigger_elevator_init (edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf("trigger_elevator has no target\n");
		return;
	}
	self->movetarget = G_PickTarget (self->target);
	if (!self->movetarget)
	{
		gi.dprintf("trigger_elevator unable to find target %s\n", self->target);
		return;
	}
	if (strcmp(self->movetarget->classname.c_str(), "func_train") != 0)
	{
		gi.dprintf("trigger_elevator target %s is not a train\n", self->target);
		return;
	}

	self->use = trigger_elevator_use;
	self->svflags = SVF_NOCLIENT;

}

void SP_trigger_elevator (edict_t *self)
{
	self->class_id = ENTITY_TRIGGER_ELEVATOR;

	self->think = trigger_elevator_init;
	self->nextthink = level.time + FRAMETIME;
}


/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0

so, the basic time between firing is a random time between
(wait - random) and (wait + random)

"delay"			delay before first firing when turned on, default is 0

"pausetime"		additional delay used only the very first time
				and only if spawned with START_ON

These can used but not touched.
*/
void func_timer_think (edict_t *self)
{
	G_UseTargets (self, self->activator);
	self->nextthink = level.time + self->wait + crandom() * self->random;
}

void func_timer_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;

	// if on, turn it off
	if (self->nextthink)
	{
		self->count--;
		if (!self->count) {
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
		} else {
			self->nextthink = 0;
		}
		return;
	}

	// turn it on
	if (self->delay)
		self->nextthink = level.time + self->delay;
	else
		func_timer_think (self);
}

void SP_func_timer (edict_t *self)
{
	self->class_id = ENTITY_FUNC_TIMER;

	if (!self->wait)
		self->wait = 1.0;

	self->use = func_timer_use;
	self->think = func_timer_think;

	if (self->random >= self->wait)
	{
		self->random = self->wait - FRAMETIME;
		gi.dprintf("func_timer at %s has random >= wait\n", vtos(self->s.origin));
	}

	if (self->spawnflags & 1)
	{
		self->nextthink = level.time + 1.0 + st.pausetime + self->delay + self->wait + crandom() * self->random;
		self->activator = self;
	}

	self->svflags = SVF_NOCLIENT;
}


/*QUAKED func_conveyor (0 .5 .8) ? START_ON TOGGLE
Conveyors are stationary brushes that move what's on them.
The brush should be have a surface with at least one current content enabled.
speed	default 100
*/

void func_conveyor_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & 1)
	{
		self->spawnflags &= ~1;
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	else
	{
		self->spawnflags |= 1;
		self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		KillBox (self);
	}
	gi.linkentity(self);

	if (!(self->spawnflags & 2))
		self->use = nullptr;
}

void SP_func_conveyor (edict_t *self)
{
	self->class_id = ENTITY_FUNC_CONVEYOR;

	if (!self->speed)
		self->speed = 102;

	self->use = func_conveyor_use;
	gi.setmodel (self, self->model);

	// Lazarus changes:
	self->movetype = MOVETYPE_CONVEYOR;
	G_SetMovedir(self->s.angles,self->movedir);
	if (self->spawnflags & 1)
		self->solid = SOLID_BSP;
	else {
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}

	gi.linkentity (self);
}


/*QUAKED func_door_secret (0 .5 .8) ? always_shoot 1st_left 1st_down
A secret door.  Slide back and then to the side.

open_once		doors never closes
1st_left		1st move is left of arrow
1st_down		1st move is down from arrow
always_shoot	door is shootebale even if targeted

"angle"		determines the direction
"dmg"		damage to inflic when blocked (default 2)
"wait"		how long to hold in the open position (default 5, -1 means hold)
"sounds"
0)		default
1)		silent
2-4		not used
5-99	custom sound- e.g. doors/dr05_strt.wav, doors/dr05_mid.wav, doors/dr05_end.wav
*/

#define SECRET_ALWAYS_SHOOT	1
#define SECRET_1ST_LEFT		2
#define SECRET_1ST_DOWN		4

void door_secret_move1 (edict_t *self);
void door_secret_move2 (edict_t *self);
void door_secret_move3 (edict_t *self);
void door_secret_move4 (edict_t *self);
void door_secret_move5 (edict_t *self);
void door_secret_move6 (edict_t *self);
void door_secret_done (edict_t *self);

void door_secret_use (edict_t *self, edict_t *other, edict_t *activator)
{
	// make sure we're not already moving
//	if ( !VectorCompare(self->s.origin, vec3_origin) )
	if ( (self->moveinfo.state != STATE_LOWEST) && (self->moveinfo.state != STATE_TOP) )
		return;

	// added sound
	if (self->moveinfo.sound_start)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	if (self->moveinfo.sound_middle) {
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}
//	Move_Calc (self, self->pos1, door_secret_move1);
//	door_use_areaportals (self, true);
	if (self->moveinfo.state == STATE_LOWEST) {
		self->moveinfo.state = STATE_DOWN;
		Move_Calc (self, self->pos1, door_secret_move1);
		door_use_areaportals (self, true);
	}
	else { // Knightmare added
		self->moveinfo.state = STATE_UP;
		Move_Calc (self, self->pos1, door_secret_move5);
	}
}

void door_secret_move1 (edict_t *self)
{
	self->nextthink = level.time + 1.0;
	self->think = door_secret_move2;
	self->moveinfo.state = STATE_BOTTOM;	// Knightmare added
	// added sound
	self->s.sound = 0;
	if (self->moveinfo.sound_end)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC
}

void door_secret_move2 (edict_t *self)
{
    // added sound
	if (self->moveinfo.sound_start)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	if (self->moveinfo.sound_middle) {
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}
	self->moveinfo.state = STATE_UP;	// Knightmare added
	Move_Calc (self, self->pos2, door_secret_move3);
}

void door_secret_move3 (edict_t *self)
{
	self->moveinfo.state = STATE_TOP;	// Knightmare added
	// added sound
	self->s.sound = 0;
	if (self->moveinfo.sound_end)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC

	if (self->wait == -1)
		return;
	self->nextthink = level.time + self->wait;
	self->think = door_secret_move4;
}

void door_secret_move4 (edict_t *self)
{
	// added sound
	if (self->moveinfo.sound_start)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	if (self->moveinfo.sound_middle) {
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}
	self->moveinfo.state = STATE_UP;	// Knightmare added
	Move_Calc (self, self->pos1, door_secret_move5);
}

void door_secret_move5 (edict_t *self)
{
	self->nextthink = level.time + 1.0;
	self->think = door_secret_move6;
	self->moveinfo.state = STATE_BOTTOM;	// Knightmare added
	// added sound
	self->s.sound = 0;
	if (self->moveinfo.sound_end)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC
}

void door_secret_move6 (edict_t *self)
{
    // added sound
	if (self->moveinfo.sound_start)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	if (self->moveinfo.sound_middle) {
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}

	self->moveinfo.state = STATE_DOWN;	// Knightmare added
//	Move_Calc (self, vec3_origin, door_secret_done);
	Move_Calc (self, self->pos0, door_secret_done);
}

void door_secret_done (edict_t *self)
{
	if (!(self->targetname) || (self->spawnflags & SECRET_ALWAYS_SHOOT))
	{
		// Knightmare- restore user-set health here
		// now that the correct die function is set
	//	self->health = 0;
		self->health = self->max_health;
		self->takedamage = DAMAGE_YES;
	}
	self->moveinfo.state = STATE_LOWEST;	// Knightmare added
	// added sound
	self->s.sound = 0;
	if (self->moveinfo.sound_end)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC

	door_use_areaportals (self, false);
}

void door_secret_blocked  (edict_t *self, edict_t *other)
{
	if (!(other->svflags & SVF_MONSTER) && (!other->client) )
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other)
		{
			// Lazarus: Some of our ents don't have origin near the model
			vec3_t save;
			VectorCopy (other->s.origin,save);
			VectorMA (other->absmin, 0.5, other->size, other->s.origin);
			BecomeExplosion1 (other);
		}
		return;
	}

	if (level.time < self->touch_debounce_time)
		return;
	self->touch_debounce_time = level.time + 0.5;

	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void door_secret_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->takedamage = DAMAGE_NO;
	door_secret_use (self, attacker, attacker);
}

void SP_func_door_secret (edict_t *ent)
{
	vec3_t	forward, right, up;
//	float	side;
//	float	width;
//	float	length;

	ent->class_id = ENTITY_FUNC_DOOR_SECRET;

	if ( (level.maptype == MAPTYPE_CUSTOM) && (ent->sounds > 4) && (ent->sounds < 100) ) // custom sounds
	{
		ent->moveinfo.sound_start = gi.soundindex (va("doors/dr%02i_strt.wav", ent->sounds));
		ent->moveinfo.sound_middle = gi.soundindex (va("doors/dr%02i_mid.wav", ent->sounds));
		ent->moveinfo.sound_end = gi.soundindex (va("doors/dr%02i_end.wav", ent->sounds));
	}
	else if (ent->sounds != 1)
	{
		ent->moveinfo.sound_start = gi.soundindex ("doors/dr1_strt.wav");
		ent->moveinfo.sound_middle = gi.soundindex ("doors/dr1_mid.wav");
		ent->moveinfo.sound_end = gi.soundindex ("doors/dr1_end.wav");
	}
	else
	{
		ent->moveinfo.sound_start = 0;
		ent->moveinfo.sound_middle = 0;
		ent->moveinfo.sound_end = 0;
	}

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel (ent, ent->model);

	ent->blocked = door_secret_blocked;
	ent->use = door_secret_use;

	if (!(ent->targetname) || (ent->spawnflags & SECRET_ALWAYS_SHOOT))
	{
		// Knightmare- we can allow user-set health here
		// now that the correct die function is set
	//	ent->health = 0;
		if (!ent->health)
			ent->health = 1;
		ent->max_health = ent->health;
		ent->takedamage = DAMAGE_YES;
		ent->die = door_secret_die;
	}

	if (!ent->dmg)
		ent->dmg = 2;

	if (!ent->wait)
		ent->wait = 5;

	ent->moveinfo.accel =
	ent->moveinfo.decel =
	ent->moveinfo.speed = 50;
	ent->moveinfo.state = STATE_LOWEST;	// Knightmare added

	// calculate positions
	VectorCopy (ent->s.origin, ent->pos0);
	AngleVectors (ent->s.angles, forward, right, up);
//	VectorClear (ent->s.angles);
	VectorCopy (ent->s.angles, ent->move_angles);	// Knightmare- backup angles to move_angles
	G_SetMovedir (ent->s.angles, ent->movedir);		// Knightmare- don't just clear angles, set movedir as well
//	ent->side = 1.0 - (ent->spawnflags & SECRET_1ST_LEFT);

	if (ent->spawnflags & SECRET_1ST_LEFT)
		ent->side = -1.0;
	else
		ent->side = 1.0;

	if (!ent->width)
	{
		if (ent->spawnflags & SECRET_1ST_DOWN)
			ent->width = fabs(DotProduct(up, ent->size));
		else
			ent->width = fabs(DotProduct(right, ent->size));
	}
	if (!ent->length)
		ent->length = fabs(DotProduct(forward, ent->size));
	if (ent->spawnflags & SECRET_1ST_DOWN)
		VectorMA (ent->s.origin, -1 * ent->width, up, ent->pos1);
	else
		VectorMA (ent->s.origin, ent->side * ent->width, right, ent->pos1);
	VectorMA (ent->pos1, ent->length, forward, ent->pos2);

	if (ent->health)
	{
		ent->takedamage = DAMAGE_YES;
	//	ent->die = door_killed;
		ent->die = door_secret_die;	// Knightmare- this had the wrong die function set!
		ent->max_health = ent->health;
	}
	else if (ent->targetname && !ent->message.empty())
	{
		gi.soundindex ("misc/talk.wav");
		ent->touch = door_touch;
	}

	ent->classname = "func_door";

#ifdef POSTTHINK_CHILD_MOVEMENT
	ent->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	gi.linkentity (ent);
}

/*QUAKED func_door_secret2 (0 .5 .8) ? open_once 1st_left 1st_down no_shoot always_shoot slide_right slide_forward
Improved secret door. Slides back, then to the left. Angle determines direction.

Spawnflags:
open_once		Stay open, never close
1st_left		1st move is left/right of arrow
1st_down		1st move is forwards/backwards
no_shoot		not implemented yet
always_shoot	even if targeted, keep shootable
slide_right		the sideways move will be to right of arrow
slide_forward	the to/fro move will be forward

Values:
"angle" 	determines the direction
"wait"		# of seconds before coming back (default 5)
"dmg"		damage to inflict when blocked (default 2)
"message"	to be printed when activated
"width"		overrides how far the door pops out
"length"	overrides how far the door slides
"sounds"
0)		default
1)		silent
2-4		not used
5-99	custom sound- e.g. doors/dr05_strt.wav, doors/dr05_mid.wav, doors/dr05_end.wav
*/

#define SECDR2_OPEN_ONCE		1		// door stays open until triggered again
#define SECDR2_1ST_LEFT			2		// 1st move is left of arrow
#define SECDR2_1ST_DOWN			4		// 1st move is down from arrow
#define SECDR2_NO_SHOOT			8		// door is only opened by triggering
#define SECDR2_ALWAYS_SHOOT		16		// door can be shot even if targeted
#define SECDR2_MOVE_RIGHT		32		// slide right instead of left
#define SECDR2_MOVE_FORWARD		64		// slides forward instead of back

void door_secret2_move1 (edict_t *self);
void door_secret2_move2 (edict_t *self);
void door_secret2_move3 (edict_t *self);
void door_secret2_move4 (edict_t *self);
void door_secret2_move5 (edict_t *self);
void door_secret2_move6 (edict_t *self);
void door_secret2_done (edict_t *self);

void door_secret2_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*door;

	// This entity supports teamed movement, so this function is only for the team master
	if (self->flags & FL_TEAMSLAVE)
		return;

	// make sure we're not already moving
	if ( (self->moveinfo.state != STATE_LOWEST) && (self->moveinfo.state != STATE_TOP) )
		return;

	// move all team members
	for (door = self; door; door = door->teamchain)
	{
		// added sound
		if (door->moveinfo.sound_start)
			gi.sound (door, CHAN_NO_PHS_ADD+CHAN_VOICE, door->moveinfo.sound_start, 1, door->attenuation, 0); // was ATTN_STATIC
		if (door->moveinfo.sound_middle) {
			door->s.sound = door->moveinfo.sound_middle;
	#ifdef KMQUAKE2_ENGINE_MOD
			door->s.loop_attenuation = door->attenuation;
	#endif
		}
		if (door->moveinfo.state == STATE_LOWEST) {
			door->moveinfo.state = STATE_DOWN;
			Move_Calc (door, door->pos1, door_secret2_move1);
			door_use_areaportals (door, true);
		}
		else {
			door->moveinfo.state = STATE_UP;
			Move_Calc (door, door->pos1, door_secret2_move5);
		}
	}
}

void door_secret2_killed (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
//	self->health = self->max_health;
	self->takedamage = DAMAGE_NO;

	// if a team slave, call this function for the team master
	if ( (self->flags & FL_TEAMSLAVE) && (self->teammaster) && (self->teammaster->takedamage != DAMAGE_NO) ) {
		door_secret2_killed (self->teammaster, inflictor, attacker, damage, point);
	}
	else {
		door_secret2_use (self, inflictor, attacker);
	}
}

void door_secret2_move1 (edict_t *self)
{
	self->nextthink = level.time + 1.0;
	self->think = door_secret2_move2;
	self->moveinfo.state = STATE_BOTTOM;
	// added sound
	self->s.sound = 0;
	if (self->moveinfo.sound_end)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC
}

void door_secret2_move2 (edict_t *self)
{
    // added sound
	if (self->moveinfo.sound_start)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	if (self->moveinfo.sound_middle) {
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}
	self->moveinfo.state = STATE_UP;
	Move_Calc (self, self->pos2, door_secret2_move3);
}

void door_secret2_move3 (edict_t *self)
{
	self->moveinfo.state = STATE_TOP;
	// added sound
	self->s.sound = 0;
	if (self->moveinfo.sound_end)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC

	// don't set returning move if open_once flag is set
	if ( !(self->spawnflags & SECDR2_OPEN_ONCE) ) {
		self->nextthink = level.time + self->wait;
		self->think = door_secret2_move4;
	}
}

void door_secret2_move4 (edict_t *self)
{
	// added sound
	if (self->moveinfo.sound_start)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	if (self->moveinfo.sound_middle) {
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	}
	self->moveinfo.state = STATE_UP;
	Move_Calc (self, self->pos1, door_secret2_move5);
}

void door_secret2_move5 (edict_t *self)
{
	self->nextthink = level.time + 1.0;
	self->think = door_secret2_move6;
	self->moveinfo.state = STATE_BOTTOM;
	// added sound
	self->s.sound = 0;
	if (self->moveinfo.sound_end)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC
}

void door_secret2_move6 (edict_t *self)
{
    // added sound
	if (self->moveinfo.sound_start)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_STATIC
	if (self->moveinfo.sound_middle)
		self->s.sound = self->moveinfo.sound_middle;
#ifdef KMQUAKE2_ENGINE_MOD
		self->s.loop_attenuation = self->attenuation;
#endif
	self->moveinfo.state = STATE_DOWN;
	Move_Calc (self, self->pos0, door_secret2_done);
}

void door_secret2_done (edict_t *self)
{
	if ( !(self->targetname) || (self->spawnflags & SECDR2_ALWAYS_SHOOT) )
	{
		self->health = 1;
		self->takedamage = DAMAGE_YES;
		self->die = door_secret2_killed;
	}
	self->moveinfo.state = STATE_LOWEST;
	// added sound
	self->s.sound = 0;
	if (self->moveinfo.sound_end)
		gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_STATIC

	door_use_areaportals (self, false);
}

void door_secret2_blocked (edict_t *self, edict_t *other)
{
	// This entity supports teamed movement, so this function is only for the team master
	if (self->flags & FL_TEAMSLAVE)
		return;

	// Limit damage frequency to 2x per second
	if (level.time < self->touch_debounce_time)
		return;
	self->touch_debounce_time = level.time + 0.5;

	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 0, 0, MOD_CRUSH);
}

void door_secret2_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// Lazarus: allow robot usage
	if ( !other->client && !(other->flags & FL_ROBOT) )

	// Can't touch if they're dead
	if (other->health <= 0)
		return;

	if (self->touch_debounce_time > level.time)
		return;
	self->touch_debounce_time = level.time + 2.0f;

	if (!self->message.empty()) {
		safe_centerprintf (other, "%s", self->message.c_str());
		gi.sound (other, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}
}

void SP_func_door_secret2 (edict_t *ent)
{
	vec3_t	forward, right, up;

	ent->class_id = ENTITY_FUNC_DOOR_SECRET2;

	if ( (level.maptype == MAPTYPE_CUSTOM) && (ent->sounds > 4) && (ent->sounds < 100) ) // custom sounds
	{
		ent->moveinfo.sound_start = gi.soundindex (va("doors/dr%02i_strt.wav", ent->sounds));
		ent->moveinfo.sound_middle = gi.soundindex (va("doors/dr%02i_mid.wav", ent->sounds));
		ent->moveinfo.sound_end = gi.soundindex (va("doors/dr%02i_end.wav", ent->sounds));
	}
	else if (ent->sounds != 1)
	{
		ent->moveinfo.sound_start = gi.soundindex ("doors/dr1_strt.wav");
		ent->moveinfo.sound_middle = gi.soundindex ("doors/dr1_mid.wav");
		ent->moveinfo.sound_end = gi.soundindex ("doors/dr1_end.wav");
	}
	else
	{
		ent->moveinfo.sound_start = 0;
		ent->moveinfo.sound_middle = 0;
		ent->moveinfo.sound_end = 0;
	}

	if (ent->attenuation <= 0)
		ent->attenuation = ATTN_STATIC;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel (ent, ent->model);

	ent->blocked = door_secret2_blocked;
	ent->use = door_secret2_use;

	if (!ent->dmg)
		ent->dmg = 2;

	if (!ent->wait)
		ent->wait = 5;

	ent->moveinfo.accel =
	ent->moveinfo.decel =
	ent->moveinfo.speed = 50;
	ent->moveinfo.state = STATE_LOWEST;		// Knightmare added

	// calculate positions
	VectorCopy (ent->s.origin, ent->pos0);
	AngleVectors (ent->s.angles, forward, right, up);
	VectorCopy (ent->s.angles, ent->move_angles);
	G_SetMovedir (ent->s.angles, ent->movedir);

	if ( (ent->move_angles[1] == 90) || (ent->move_angles[1] == 270) )
	{
		if (!ent->width)
			ent->width = ent->size[0] - 2;
		if (!ent->length)
			ent->length = ent->size[1] - 2;
	}
	else if ( (ent->move_angles[1] == 0) || (ent->move_angles[1] == 180) )
	{
		if (!ent->width)
			ent->width = ent->size[1] - 2;
		if (!ent->length)
			ent->length = ent->size[0] - 2;
	}
	else {
		gi.dprintf ("func_door_secret2 yaw angle not set to 0, 90, 180, or 270!\n");
	}

	if (ent->spawnflags & SECDR2_MOVE_FORWARD)
		VectorScale (forward, ent->length, forward);
	else
		VectorScale (forward, -1.0f * ent->length, forward);

	if (ent->spawnflags & SECDR2_MOVE_RIGHT)
		VectorScale (right, ent->width, right);
	else
		VectorScale (right, -1.0f * ent->width, right);

	if (ent->spawnflags & SECDR2_1ST_DOWN) {
		VectorAdd (ent->s.origin, forward, ent->pos1);
		VectorAdd (ent->pos1, right, ent->pos2);
	}
	else {
		VectorAdd (ent->s.origin, right, ent->pos1);
		VectorAdd (ent->pos1, forward, ent->pos2);
	}

	if (!(ent->targetname) || (ent->spawnflags & SECDR2_ALWAYS_SHOOT))
	{
		ent->health = 1;
		ent->max_health = ent->health;
		ent->takedamage = DAMAGE_YES;
		ent->die = door_secret2_killed;
	}

	if (!ent->message.empty())
	{
		gi.soundindex ("misc/talk.wav");
		ent->touch = door_touch;
	}

#ifdef POSTTHINK_CHILD_MOVEMENT
	ent->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	gi.linkentity (ent);
}

//===================================================================

/*QUAKED func_killbox (1 0 0) ?
Kills everything inside when fired, irrespective of protection.
*/
void use_killbox (edict_t *self, edict_t *other, edict_t *activator)
{
	KillBox (self);

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void SP_func_killbox (edict_t *ent)
{
	ent->class_id = ENTITY_FUNC_KILLBOX;

	gi.setmodel (ent, ent->model);
	ent->use = use_killbox;
	ent->svflags = SVF_NOCLIENT;
}

//===================================================================
// LAZARUS additions
//===================================================================
//
// func_pushable
// Moveable crate. Similar to misc_explobox, but:
// 1) Uses a brush model rather than a .md2 model. Model can be ANY
//    shape, but the func_pushable uses the bounding box to determine
//    whether it is being touched or not, so cubes are generally best.
// 2) Can be pushed off a ledge and damaged by falling
// 3) Default dmg = 0 (no fireball) and health = 0 (indestructible)
// 4) Plays a sound when moving
//
// targetname - If triggered, pushable object self-destructs, throwing
//              debris chunks and (if dmg>0) exploding
// health     - Damage sustained by object before it "dies". On death,
//              object throws debris and (if dmg>0) explodes. func_pushable
//              can be damaged by shooting (inapplicable in Tremor) or
//              by falling damage or by being crushed by a func_door,
//              func_train, etc. Set health=0 for a func_pushable that
//              cannot be damaged by falling or weapon fire. Set health=-1
//              for func_pushable that will block MOVETYPE_PUSH entities
//              (func_door, etc.)
// dmg        - Radius damage due to object explosion. Set to 0 for
//              no fireball
// mass       - Weight of the object. Heavier objects are harder to push.
//              Default = 400.
// sounds     - Sound played when moved.
//              0 = none
//              1 = tank/thud.wav
//              2 = weapons/rg_hum.wav
//              3 = weapons/rockfly.wav
//
// SF=1       - Trigger spawn. Func_pushable is invisible and non-solid until triggered.
//    2       - No knockback. Not moved by weapon fire (invulnerable func_pushables aren't
//              affected by weapon fire in either case)
//
//=============
// box_movestep
//
// Similar to SV_movestep in monster code, but handles falling
// objects, and doesn't balk at water/lava/slime
//=============
//
qboolean box_movestep (edict_t *ent, vec3_t move, qboolean relink)
{
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
	float		stepsize;

	vec3_t		maxs, mins, origin;

// try the move
	VectorAdd (ent->s.origin, ent->origin_offset, origin);
	VectorCopy (origin, oldorg);
	VectorAdd (origin, move, neworg);
	VectorCopy (ent->size, maxs);
	VectorScale (maxs, 0.5, maxs);
	VectorNegate (maxs, mins);


// push down from a step height above the wished position
	stepsize = 1;

	neworg[2] += stepsize;
	VectorCopy (neworg, end);
	end[2] -= stepsize*2;

	trace = gi.trace (neworg, mins, maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= stepsize;
		trace = gi.trace (neworg, mins, maxs, end, ent, MASK_MONSTERSOLID);
		if (trace.allsolid || trace.startsolid)
			return false;
	}

	if (trace.fraction == 1)
	{
	// if box had the ground pulled out, go ahead and fall
		VectorAdd (ent->s.origin, move, ent->s.origin);
		if (relink)
		{
			gi.linkentity (ent);
			G_TouchTriggers (ent);
		}
		ent->groundentity = nullptr;
		return true;
	}

	VectorCopy (trace.endpos, origin);
	VectorSubtract (origin, ent->origin_offset, ent->s.origin);

	ent->groundentity = trace.ent;
	ent->groundentity_linkcount = trace.ent->linkcount;

// the move is ok
	if (relink)
	{
		gi.linkentity (ent);
		G_TouchTriggers (ent);
	}
	return true;
}

void box_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deathtarget)
	{
		self->target = self->deathtarget;
		if (self->activator)
			G_UseTargets (self, self->activator);
		else
			G_UseTargets (self, attacker);
		self->target = nullptr;
	}
	func_explosive_die (self, inflictor, attacker, damage, point);
}

void box_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;
	box_die(self, self, other, self->health, vec3_origin);
}

//
//===============
//box_walkmove
// similar to M_walkmove, but:
// 1) works with entities not on the ground
// 2) calls moving box-specific box_movestep, which allows
//    boxes to fall
//===============
//

qboolean box_walkmove (edict_t *ent, float yaw, float dist)
{
	vec3_t	move;

	if (!ent->groundentity && !(ent->flags & (FL_FLY|FL_SWIM)))
		return false;

	yaw = yaw*M_PI*2 / 360;

	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	return box_movestep(ent, move, true);
}

void box_water_friction(edict_t *ent)
{
	int i;
	float		speed, newspeed, control;

	if (!(ent->flags & FL_SWIM)) return;	// should not be here
	if (ent->waterlevel==0) return;			// likewise
	if (ent->crane_control) return;			// currently under control of a crane
	if ((ent->velocity[0]==0) && (ent->velocity[1]==0))
	{
		ent->nextthink = 0;
		return;
	}
	for (i=0; i<2; i++)
	{
		if (ent->velocity[i] != 0)
		{
			speed = fabs(ent->velocity[i]);
			control = speed < 100 ? 100 : speed;
			newspeed = speed - (FRAMETIME * control * ent->waterlevel);
			if (newspeed < 0)
				newspeed = 0;
			newspeed /= speed;
			ent->velocity[i] *= newspeed;
		}
	}
	ent->nextthink = level.time + FRAMETIME;
	gi.linkentity(ent);
}

edict_t *CrateOnTop (edict_t *from, edict_t *ent);
void box_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	float	e;   // coefficient of restitution
	float   m;
	float	ratio;
	float	v11, v12, v21, v22;
	int     axis;
	vec3_t	v1, v2, v;
	vec3_t	origin;
	edict_t *bottom, *top;

	// if other is another func_pushable, AND self is in
	// water, move in proportion to relative masses
	if (other->movetype == MOVETYPE_PUSHABLE)
	{
		int		damage;
		float	delta;
		float	vslide;
		vec3_t	dir, impact_v;

		// Check for impact damage first
		if (self->health > 0)
		{
			VectorSubtract(other->velocity,self->velocity,impact_v);
			delta = VectorLength(impact_v);
			delta = delta*delta*0.0001;
			if (delta > 30)
			{
				damage = (float)(other->mass)/(float)(self->mass) * ( (delta-30)/2 );
				if (damage > 0)
				{
					VectorSubtract(self->s.origin,other->s.origin,dir);
					VectorNormalize(dir);
					T_Damage (self, other, other, dir, self->s.origin, vec3_origin, damage, 0, 0, MOD_FALLING);
					if (self->health <= 0) return;
				}
			}
		}
		if (self->waterlevel==0) return;

		// 06/03/00 change: If either func_pushable is currently being moved
		// by crane, bail out.
		if (self->crane_control) return;
		if (other->crane_control) return;

		// Since func_pushables have a bounding box, impact will ALWAYS be on one of the
		// planes of the bounding box. The "plane" argument isn't always used, but since
		// all entities involved use a parallelepiped bounding box we can rely on offsets
		// to centers to figure out which side the impact is on.
		VectorAdd (self->absmax,self->absmin,v1);
		VectorScale(v1,0.5,v1);
		VectorAdd (other->absmax,other->absmin,v2);
		VectorScale(v2,0.5,v2);
		VectorSubtract(v1,v2,v);
		VectorNormalize(v);
		axis = 0;
		if (fabs(v[1]) > fabs(v[axis])) axis = 1;
		if (fabs(v[2]) > fabs(v[axis])) axis = 2;

		e = 0.5; // coefficient of restitution
		m = (float)(other->mass)/(float)(self->mass);
		v11 = self->velocity[axis];
		v21 = other->velocity[axis];
		v22 = ( e*(v11-v21) + v11 + m*v21 ) / (1.0 + m);
		v12 = v22 + e*(v21-v11);
		self->velocity[axis] = v12;
		other->velocity[axis] = v22;

		// Assuming frictionless surfaces, momentum of crate is conserved in
		// other two directions (so velocity doesn't change)... BUT we want
		// to get the bottom crate out from underneath the other one,
		// so we're gonna be a little "creative"
		if (axis == 2)
		{
			if (v[2] > 0)
			{
				bottom = other;
				top = self;
				VectorNegate(v,v);
			}
			else {
				bottom = self;
				top = other;
			}
			v[2] = 0;
			VectorNormalize(v);
			if (!VectorLength(v)) {
				v[0] = crandom();
				v[1] = sqrt(1.0 - v[0]*v[0]);
			}
			vslide = 10;
			if (fabs(bottom->velocity[0]) < 50)
				bottom->velocity[0] += v[0] * vslide;
			if (fabs(bottom->velocity[1]) < 50)
				bottom->velocity[1] += v[1] * vslide;
			top->velocity[0] = -bottom->velocity[0]/2;
			top->velocity[1] = -bottom->velocity[1]/2;
			other->think     = box_water_friction;
			other->nextthink = level.time + 0.2;
			gi.linkentity(other);
			self->think      = box_water_friction;
			self->nextthink  = level.time + 0.2;
		}
		else {
			// Frictionless horizontal motion for 1 second
			self->think = box_water_friction;
			self->nextthink = level.time + 1.0;
		}

		// Override oldvelocity
		VectorCopy (self->velocity,self->oldvelocity);
		gi.linkentity(self);
		return;
	}
	// if other is a monster or a player and box is on other's head and moving down,
	// do impact damage
	VectorAdd(self->s.origin,self->origin_offset,origin);
	if ( other->client || (other->svflags & SVF_MONSTER) )
	{
		VectorAdd (self->absmax,self->absmin,v1);
		VectorScale(v1,0.5,v1);
		VectorSubtract(v1,other->s.origin,v);
		VectorNormalize(v);
		axis = 0;
		if (fabs(v[1]) > fabs(v[axis])) axis = 1;
		if (fabs(v[2]) > fabs(v[axis])) axis = 2;
		if (axis == 2 && v[axis] > 0)
		{
			v11 = VectorLength(self->velocity);
			VectorCopy (self->velocity,v);
			VectorNormalize(v);
			if (!other->groundentity) {
				other->velocity[2] = self->velocity[2];
				gi.linkentity(other);
			}
			else if ((v11 > 0) && (v[2] < -0.7))
			{
				int		damage;
				float	delta;
				vec3_t	dir, impact_v;
				VectorSubtract(other->velocity,self->velocity,impact_v);
				delta = VectorLength(impact_v);
				delta = delta*delta*0.001;
				damage = (float)(self->mass)/(float)(other->mass) * delta;
				// always do some minimum amount of damage, or we get boxes on
				// heads, which looks awfully damn odd
				damage = max(2,damage);
				VectorSubtract(self->s.origin,other->s.origin,dir);
				VectorNormalize(dir);
				T_Damage (other, self, self, dir, other->s.origin, vec3_origin, damage, 0, 0, MOD_CRUSH);
				self->bounce_me = 1;
				return;
			}
		}
		else if ( (other->groundentity == self) && (self->velocity[2] > 0)) {
			self->bounce_me = 2;
			other->velocity[2] = self->velocity[2];
			gi.linkentity(other);
			return;
		}
	}

	// if not a player return
	if (!other->client) return;

	// if func_pushable is on ground and has a mass > 1000, go away
	if (self->groundentity && self->mass > 1000) return;

	// if player not pressing use key, do nothing
	if (!other->client->use)
	{
		if (self->activator == other) self->activator = nullptr;
		return;
	}

	// if player in contact with this func_pushable is already pushing
	// something else, return
	if ((other->client->push != nullptr ) && (other->client->push != self))
	{
		if (self->activator == other) self->activator = nullptr;
		return;
	}

	// if another player got here first, he maintains control
	if (self->activator)
	{
		if (self->activator->client)
		{
			if (self->activator != other)
			{
				return;
			}
		}
	}

	VectorAdd (self->absmax,self->absmin,v1);
	VectorScale(v1,0.5,v1);

	// if func_pushable isn't in front of pusher, do nothing
	if (!point_infront(other,v1))
	{
		if (self->activator == other) self->activator = nullptr;
		return;
	}

	// if player isn't on solid ground AND object isn't in water,
	// OR if player is standing on this box, do nothing
	if ( ((!other->groundentity) && (self->waterlevel==0)) || (other->groundentity == self) ) {
		if (self->activator == other) self->activator = nullptr;
		return;
	}

	// if this box has another box stacked on top, balk
	if ( CrateOnTop ( nullptr, self) )
	{
		if (self->activator == other) self->activator = nullptr;
		return;
	}

	// Give object a little nudge to give us some clearance
	VectorSubtract (v1, other->s.origin, v);
	box_walkmove (self, vectoyaw(v), 4);
	// Now get the offset from the player to the object,
	//   and preserve that offset in ClientThink by shifting
	//   object as needed.
	VectorSubtract (other->s.origin, self->s.origin, self->offset);
	self->offset[2] = 0;
	self->activator = other;
	other->client->push = self;
	ratio = (float)other->mass / (float)self->mass;
	other->client->maxvelocity = 20. * ratio;
	// Knightmare- don't autoswitch if client-side chasecam is on
#ifdef KMQUAKE2_ENGINE_MOD
	if (tpp_auto->value && (!cl_thirdperson->value || deathmatch->value || coop->value)
		&& !other->client->chasetoggle)
#else
	if (tpp_auto->value && !other->client->chasetoggle)
#endif
		Cmd_Chasecam_Toggle(other);
}

void func_pushable_spawn (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid     = SOLID_BSP;
	self->movetype  = MOVETYPE_PUSHABLE;
	self->svflags  &= ~SVF_NOCLIENT;
	self->use       = box_use;
	self->clipmask  = MASK_PLAYERSOLID|MASK_MONSTERSOLID;
	self->touch     = box_touch;
}

void SP_func_pushable (edict_t *self)
{
//	vec3_t border = {2,2,2};
	vec3_t border = {1,1,1};

	PrecacheDebris(self->gib_type);

	self->class_id = ENTITY_FUNC_PUSHABLE;

	gi.setmodel (self, self->model);

	// usermodel (for alias model pushables) goes on modelindex2
	if (self->usermodel && strlen(self->usermodel)) {
		char	modelname[256];
		// check for "models/" already in path
		if ( !strncmp(self->usermodel, "models/", 7) )
			snprintf (modelname, sizeof(modelname), "%s", self->usermodel);
		else
			snprintf (modelname, sizeof(modelname), "models/%s", self->usermodel);
		self->s.modelindex2 = gi.modelindex (modelname);
	}

	/* Game places a 2 unit border around brush model absmin and absmax */
	VectorAdd(self->mins,border,self->mins);
	VectorSubtract(self->maxs,border,self->maxs);
	VectorAdd(self->absmin,border,self->absmin);
	VectorSubtract(self->absmax,border,self->absmax);

	if (!self->mass)
		self->mass = 400;

	if (st.item) // Knightmare- item support
	{
		self->item = FindItemByClassname (st.item);
		if (!self->item)
			gi.dprintf("%s at %s has bad item: %s\n", self->classname, vtos(self->s.origin), st.item);
	}

	self->flags = FL_SWIM;

	if (self->health > 0)
	{
		self->die = box_die;
		self->takedamage = DAMAGE_YES;
	}
	else
	{
		self->die = nullptr;
		self->takedamage = DAMAGE_NO;
	}

	if (self->spawnflags & 2)
	{
		// trigger spawn
		self->solid    = SOLID_NOT;
		self->movetype = MOVETYPE_NONE;
		self->use      = func_pushable_spawn;
		self->svflags |= SVF_NOCLIENT;
	}
	else {
		self->solid     = SOLID_BSP;
		self->movetype  = MOVETYPE_PUSHABLE;
		self->use       = box_use;
		self->clipmask  = MASK_PLAYERSOLID|MASK_MONSTERSOLID;
		self->touch     = box_touch;
		self->think     = M_droptofloor;
		self->nextthink = level.time + 2 * FRAMETIME;
	}

	if (self->spawnflags & 4)
		self->flags |= FL_NO_KNOCKBACK;

	switch (self->sounds)
	{
	case 1:
		self->noise_index = gi.soundindex ("tank/thud.wav");
		break;
	case 2:
		self->noise_index = gi.soundindex ("weapons/rg_hum.wav");
		break;
	case 3:
		self->noise_index = gi.soundindex ("weapons/rockfly.wav");
		break;
	}

	if (self->sounds && !VectorLength(self->s.origin) )
	{
		edict_t *speaker;

		speaker = G_Spawn();
		speaker->classname   = "moving_speaker";
		speaker->s.sound     = 0;
		speaker->volume      = 1;
		speaker->attenuation = ATTN_STATIC; // was 1
		speaker->owner       = self;
		speaker->think       = Moving_Speaker_Think;
		speaker->nextthink   = level.time + 2*FRAMETIME;
		speaker->spawnflags  = 11;       // owner must be moving and on ground to play
		self->speaker        = speaker;
		VectorAdd(self->absmin,self->absmax,speaker->s.origin);
		VectorScale(speaker->s.origin,0.5,speaker->s.origin);
		VectorSubtract(speaker->s.origin,self->s.origin,speaker->offset);
	}

	// Move up 1 unit so that func_pushable sitting directly on a func_train
	// won't block it's movement (since train has a 1 unit border)
	self->s.origin[2] += 1;
	gi.linkentity (self);
}

//
// Bobbing water - identical to func_water, but bobs up and down with
// amplitude specified by "bob" value and duration of one cycle specified by
// "duration"
//
void bob_think (edict_t *self)
{
	float	delta;
	int 	time = self->duration*10;
	float	t1, t0, z0, z1;

	t0 = self->bobframe%time;
	t1 = (self->bobframe+1)%time;

	z0 = sin(2*M_PI*t0/time);
	z1 = sin(2*M_PI*t1/time);
	delta = self->bob/2 * (z1-z0);
	self->velocity[2] = delta/FRAMETIME;
	self->nextthink = level.time + FRAMETIME;
	self->bobframe  = (self->bobframe+1)%time;
	gi.linkentity(self);
}

void bob_init (edict_t *self)
{
	self->bobframe  = 0;
	self->think     = bob_think;
	self->nextthink = level.time + FRAMETIME;
}

void SP_func_bobbingwater (edict_t *self)
{
	vec3_t	abs_movedir;

	self->class_id = ENTITY_FUNC_BOBBINGWATER;

	G_SetMovedir (self->s.angles, self->movedir);
	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_BSP;
	gi.setmodel (self, self->model);

	if (self->spawnflags & 2)
	{
		level.mud_puddles++;
		self->svflags |= SVF_MUD;
	}

	switch (self->sounds)
	{
		default:
			break;

		case 1: // water
			self->moveinfo.sound_start = gi.soundindex  ("world/mov_watr.wav");
			self->moveinfo.sound_end = gi.soundindex  ("world/stp_watr.wav");
			break;

		case 2: // lava
			self->moveinfo.sound_start = gi.soundindex  ("world/mov_watr.wav");
			self->moveinfo.sound_end = gi.soundindex  ("world/stp_watr.wav");
			break;
	}

	// calculate second position
	VectorCopy (self->s.origin, self->pos1);
	abs_movedir[0] = fabs(self->movedir[0]);
	abs_movedir[1] = fabs(self->movedir[1]);
	abs_movedir[2] = fabs(self->movedir[2]);
	self->moveinfo.distance = abs_movedir[0] * self->size[0] + abs_movedir[1] * self->size[1] + abs_movedir[2] * self->size[2] - st.lip;
	VectorMA (self->pos1, self->moveinfo.distance, self->movedir, self->pos2);

	// if it starts open, switch the positions
	if (self->spawnflags & DOOR_START_OPEN)
	{
		VectorCopy (self->pos2, self->s.origin);
		VectorCopy (self->pos1, self->pos2);
		VectorCopy (self->s.origin, self->pos1);
	}

	VectorCopy (self->pos1, self->moveinfo.start_origin);
	VectorCopy (self->s.angles, self->moveinfo.start_angles);
	VectorCopy (self->pos2, self->moveinfo.end_origin);
	VectorCopy (self->s.angles, self->moveinfo.end_angles);

	self->moveinfo.state = STATE_BOTTOM;

	if (!self->speed)
		self->speed = 25;
	self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed = self->speed;

	if (!self->wait)
		self->wait = -1;
	self->moveinfo.wait = self->wait;

	self->use = door_use;

	if (self->wait == -1)
		self->spawnflags |= DOOR_TOGGLE;

	self->classname = "func_door";
	self->flags |= FL_BOB;

	if (!self->bob) self->bob = 16;
	if (!self->duration) self->duration = 8;

#ifdef POSTTHINK_CHILD_MOVEMENT
	self->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	self->think     = bob_init;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity (self);
}

//
// func_pivot - works like a see-saw
//
void pivot_blocked (edict_t *self, edict_t *other)
{
	VectorCopy (vec3_origin, self->avelocity);
	gi.linkentity(self);
}

void pivot_stop (edict_t *ent)
{
	VectorClear (ent->avelocity);
	gi.linkentity(ent);
}

void pivot_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	float	time;
	vec3_t	offset;
	vec3_t	avelocity;
	vec3_t	delta;

	if (!other->mass) return;               // other is weightless
	if (!other->groundentity) return;       // other is in air
	if (other->groundentity != ent) return; // other is not standing on ent

	VectorSubtract (ent->s.origin, other->s.origin, offset);
	offset[2] = 0.; // z offset is irrelevant
	VectorCopy (ent->avelocity, avelocity);
	if (ent->spawnflags & 1)
	{
		avelocity[PITCH] = -other->mass*offset[0]/400;
	//	if (avelocity[PITCH] = ent->avelocity[PITCH]) return;
		if (offset[0] > 0)
			ent->move_angles[PITCH] = ent->pos2[PITCH];
		else
			ent->move_angles[PITCH] = ent->pos1[PITCH];
		VectorSubtract (ent->move_angles, ent->s.angles, delta);
		time = delta[PITCH]/avelocity[PITCH];
	}
	else
	{
		avelocity[ROLL]  = other->mass*offset[1]/400;
	//	if (avelocity[ROLL] = ent->avelocity[ROLL]) return;
		if (offset[1] > 0)
			ent->move_angles[ROLL] = ent->pos1[ROLL];
		else
			ent->move_angles[ROLL] = ent->pos2[ROLL];
		VectorSubtract (ent->move_angles, ent->s.angles, delta);
		time = delta[ROLL] / avelocity[ROLL];
	}
	gi.dprintf("time=%f, v=%f %f %f\n", time, avelocity[0], avelocity[1], avelocity[2]);
	if (time > 0)
	{
		VectorCopy (avelocity, ent->avelocity);
		ent->think = pivot_stop;
		ent->nextthink = level.time + time;
		gi.linkentity(ent);
	}
	else
	{
		VectorClear (ent->avelocity);
		ent->nextthink = 0;
	}
}
void pivot_init (edict_t *ent)
{
	float	zmin;
	trace_t	tr;
	vec3_t	start, end;

	VectorClear (ent->pos1);
	VectorClear (ent->pos2);
	if (ent->spawnflags & 1)
	{
		end[0] = start[0] = ent->absmin[0];
		end[1] = start[1] = (ent->absmin[1] + ent->absmax[1]) / 2;
		start[2] = ent->absmin[2];
		end[2]   = ent->absmin[2] - (ent->s.origin[0] - ent->absmin[0]);
		tr = gi.trace(start, nullptr, nullptr, end, ent, MASK_SOLID);
		if (tr.fraction < 1.0)
			zmin = tr.endpos[2];
		else
			zmin = end[2];
		ent->pos2[PITCH] = asin((ent->absmin[2] - zmin) / (ent->s.origin[0] - ent->absmin[0]));

		end[0] = start[0] = ent->absmax[0];
		end[2] = ent->absmin[2] - (ent->absmax[0]-ent->s.origin[0]);
		tr=gi.trace(start, nullptr, nullptr, end,ent,MASK_SOLID);
		if (tr.fraction < 1.0)
			zmin = tr.endpos[2];
		else
			zmin = end[2];
		ent->pos1[PITCH] = asin((ent->absmin[2] - zmin) / (ent->absmax[0] - ent->s.origin[0]));

		ent->pos1[PITCH] *= 180/M_PI;
		ent->pos2[PITCH] *= -180/M_PI;
	}
	else
	{
		end[0] = start[0] = (ent->absmin[0] + ent->absmax[0]) / 2;
		end[1] = start[1] = ent->absmin[1];
		start[2] = ent->absmin[2];
		end[2]   = ent->absmin[2] - (ent->s.origin[1] - ent->absmin[1]);
		tr = gi.trace(start, nullptr, nullptr, end, ent, MASK_SOLID);
		if (tr.fraction < 1.0)
			zmin = tr.endpos[2];
		else
			zmin = end[2];
		ent->pos1[ROLL] = asin((ent->absmin[2] - zmin) / (ent->s.origin[1] - ent->absmin[1]));

		end[1] = start[1] = ent->absmax[1];
		end[2] = ent->absmin[2] - (ent->absmax[1] - ent->s.origin[1]);
		tr = gi.trace(start, nullptr, nullptr, end, ent, MASK_SOLID);
		if (tr.fraction < 1.0)
			zmin = tr.endpos[2];
		else
			zmin = end[2];
		ent->pos2[ROLL] = asin((ent->absmin[2] - zmin) / (ent->absmax[1] - ent->s.origin[1]));

		ent->pos1[ROLL] *= 180/M_PI;
		ent->pos2[ROLL] *= -180/M_PI;
	}

	VectorClear (ent->move_angles);
	gi.linkentity(ent);
}

void SP_func_pivot (edict_t *ent)
{
	ent->class_id = ENTITY_FUNC_PIVOT;

	ent->solid = SOLID_BSP;
	ent->movetype = MOVETYPE_PUSH;

	if (!ent->speed)
		ent->speed = 100;
	if (!ent->dmg)
		ent->dmg = 2;

	ent->touch = pivot_touch;
	ent->blocked = pivot_blocked;
	ent->gravity = 0;
	ent->think = pivot_init;
	ent->nextthink = level.time + FRAMETIME;
	gi.setmodel (ent, ent->model);
	gi.linkentity (ent);
}

// ==================================================

#define FWALL_START_ON		1

void force_wall_think(edict_t *self)
{
	if (!self->wait)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_FORCEWALL);
		gi.WritePosition (self->pos1);
		gi.WritePosition (self->pos2);
		gi.WriteByte  (self->style);
		gi.multicast (self->offset, MULTICAST_PVS);
	}

	self->think = force_wall_think;
	self->nextthink = level.time + FRAMETIME;
}

void func_force_wall_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->takedamage)
		return;
	if (self->timestamp > level.time)
		return;
	self->timestamp = level.time + FRAMETIME;
	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_SPLASH);
}

void force_wall_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self->wait)
	{
		self->wait = 1;
		self->think = nullptr;
		self->nextthink = 0;
		self->solid = SOLID_NOT;
		self->touch = nullptr;
		gi.linkentity( self );

		self->count--;
		if (!self->count) {
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
		}

	}
	else
	{
		self->wait = 0;
		self->think = force_wall_think;
		self->nextthink = level.time + 0.1;
		self->solid = SOLID_BSP;
		if (self->dmg)
			self->touch = func_force_wall_touch;
		KillBox(self);		// Is this appropriate?
		gi.linkentity (self);
	}
}

/*QUAKED func_force_wall (1 0 1) ? start_on
A vertical particle force wall. Turns on and solid when triggered.
If someone is in the force wall when it turns on, they're telefragged.

start_on - forcewall begins activated. triggering will turn it off.
style - color of particles to use.
	208: green, 240: red, 241: blue, 224: orange
*/
void SP_func_force_wall (edict_t *ent)
{
	ent->class_id = ENTITY_FUNC_FORCE_WALL;

	gi.setmodel (ent, ent->model);

	ent->offset[0] = (ent->absmax[0] + ent->absmin[0]) / 2;
	ent->offset[1] = (ent->absmax[1] + ent->absmin[1]) / 2;
	ent->offset[2] = (ent->absmax[2] + ent->absmin[2]) / 2;

	ent->pos1[2] = ent->absmax[2];
	ent->pos2[2] = ent->absmax[2];
	if (ent->size[0] > ent->size[1])
	{
		ent->pos1[0] = ent->absmin[0];
		ent->pos2[0] = ent->absmax[0];
		ent->pos1[1] = ent->offset[1];
		ent->pos2[1] = ent->offset[1];
	}
	else
	{
		ent->pos1[0] = ent->offset[0];
		ent->pos2[0] = ent->offset[0];
		ent->pos1[1] = ent->absmin[1];
		ent->pos2[1] = ent->absmax[1];
	}

	if (!ent->style)
		ent->style = 208;

	ent->movetype = MOVETYPE_NONE;

	if (ent->spawnflags & FWALL_START_ON)
	{
		ent->solid = SOLID_BSP;
		if (ent->dmg)
			ent->touch = func_force_wall_touch;
		ent->think = force_wall_think;
		ent->nextthink = level.time + FRAMETIME;
		ent->wait = 0;
	}
	else {
		ent->wait = 1;
		ent->solid = SOLID_NOT;
	}

	ent->use = force_wall_use;

	ent->svflags = SVF_NOCLIENT;

	gi.linkentity(ent);
}

//
// func_door_swinging is identical to func_door_rotating, but will always open AWAY
// from whoever opens it. If opened by a trigger the normal func_door_rotating rotation
// is used.
//
void swinging_door_killed (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t	*ent;
	edict_t	*master;

	for (ent = self->teammaster ; ent ; ent = ent->teamchain)
	{
		ent->health = ent->max_health;
		ent->takedamage = DAMAGE_NO;
	}
	master = self->teammaster;

	if (master->spawnflags & DOOR_TOGGLE)
	{
		if (master->moveinfo.state == STATE_UP || master->moveinfo.state == STATE_TOP)
		{
			// trigger all paired doors
			for (ent = master ; ent ; ent = ent->teamchain)
			{
				ent->message = std::string();
				ent->touch = nullptr;
				door_go_down (ent);
			}
			return;
		}
	}

	// trigger all paired doors
	for (ent = master ; ent ; ent = ent->teamchain)
	{
		ent->message = std::string();
		ent->touch = nullptr;
		ent->do_not_rotate = true;
		if (ent->moveinfo.state == STATE_UP)
			continue;		// already going up
		if (ent->moveinfo.state == STATE_TOP)
		{	// reset top wait time
			if (ent->moveinfo.wait >= 0)
				ent->nextthink = level.time + ent->moveinfo.wait;
			return;
		}
		check_reverse_rotation (ent, point);
		if (!(ent->flags & FL_TEAMSLAVE))
		{
			if (ent->moveinfo.sound_start)
				gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, ent->moveinfo.sound_start, 1, ent->attenuation, 0); // was ATTN_STATIC
			ent->s.sound = ent->moveinfo.sound_middle;
	#ifdef KMQUAKE2_ENGINE_MOD
			ent->s.loop_attenuation = ent->attenuation;
	#endif
		}
		ent->moveinfo.state = STATE_UP;
		AngleMove_Calc (ent, door_hit_top);
		G_UseTargets (ent, attacker);
		door_use_areaportals (ent, true);
	}
}


void func_door_swinging_init (edict_t *self)
{
	edict_t		*new_origin;
	edict_t		*follow;

	follow = G_Find( nullptr, FOFS(targetname), self->followtarget);
	if (!follow)
	{
		gi.dprintf("func_door_swinging at %s, followtarget not found\n", vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	VectorSubtract (follow->s.origin, self->s.origin, self->move_origin);
	VectorNormalize (self->move_origin);
	G_FreeEdict(follow);
	if (self->pathtarget)
	{
		new_origin = G_Find ( nullptr, FOFS(targetname), self->pathtarget);
		if (new_origin)
		{
			VectorCopy (new_origin->s.origin,self->s.origin);
			VectorCopy (self->s.origin, self->moveinfo.start_origin);
			VectorCopy (self->s.origin, self->moveinfo.end_origin);
			gi.linkentity(self);
		}
	}
	self->nextthink = level.time + FRAMETIME;
	if (self->health || self->targetname)
		self->think = Think_CalcMoveSpeed;
	else
		self->think = Think_SpawnDoorTrigger;
}


void SP_func_door_swinging (edict_t *self)
{
	int	pivot;

	self->class_id = ENTITY_FUNC_DOOR_SWINGING;

	pivot = self->spawnflags & 1;	// 1 means "start open" for normal doors, so turn it
	self->spawnflags &= ~1;			// off temporarily until normal door initialization
									// is done

	if (self->spawnflags & DOOR_REVERSE)
	{
		self->spawnflags &= ~DOOR_REVERSE;
		self->flags |= FL_REVOLVING;
	}
	if ( !self->followtarget )
	{
		gi.dprintf ("func_door_swinging with no followtarget at %s\n",vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}
	SP_func_door_rotating (self);
	self->spawnflags |= pivot;
	if ( pivot && (self->health > 0) )
		self->die = swinging_door_killed;

#ifdef POSTTHINK_CHILD_MOVEMENT
	self->postthink = set_child_movement; // Knightmare- supports movewith
#endif	// POSTTHINK_CHILD_MOVEMENT

	self->flags |= FL_REVERSIBLE;
//	strncpy(self->classname,"func_door_rotating");
	self->classname = "func_door_rotating";

	// Wait a few frames so that we're sure pathtarget has been parsed.
	self->think = func_door_swinging_init;
	self->nextthink = level.time + 2*FRAMETIME;
	gi.linkentity (self);
}
