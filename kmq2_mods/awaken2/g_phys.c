// g_phys.c

#include "g_local.h"

/*
pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects 

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.
*/


/*
============
SV_TestEntityPosition

============
*/
edict_t	*SV_TestEntityPosition(edict_t *ent)
{
	trace_t	trace;
	int		mask;

	
	if (ent->clipmask)
		mask = ent->clipmask;
	else
		mask = MASK_SOLID;
	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin, ent, mask);

	if (trace.startsolid)
		return g_edicts;
		
	return NULL;
}


/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity(edict_t *ent)
{
//DH++
//	Correct method used.

	if (VectorLength(ent->velocity) > sv_maxvelocity->value)
	{
		VectorNormalize (ent->velocity);
		VectorScale (ent->velocity, sv_maxvelocity->value, ent->velocity);
	}
//DH--
}

/*
=============
SV_RunThink

Runs thinking code for this frame if necessary
=============
*/
qboolean SV_RunThink(edict_t *ent)
{
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0.0)
		return true;

	if (thinktime > level.time + 0.001)
		return true;

	ent->nextthink = 0.0;
	if (!ent->think)
	{
//CW++
//		Print out extra debugging info.

		gi.dprintf("\n------ B0RKAGE REPORT ------\n");
		gi.dprintf("NULL ent->think\n");
		if (ent->classname)
			gi.dprintf("ent->classname = %s\n", ent->classname);
		if (ent->movetype)
			gi.dprintf("ent->movetype = %d\n", ent->movetype);
		if (ent->solid)
			gi.dprintf("ent->solid = %d\n", ent->solid);
		if (ent->model)
			gi.dprintf("ent->model = %s\n", ent->model);
		if (ent->client)
			gi.dprintf("ent->client->name = %s\n", ent->client->pers.netname);
		gi.dprintf("ent->burning = %s\n", (ent->burning)?"TRUE":"FALSE");
		gi.dprintf("ent->disintegrated = %s\n", (ent->disintegrated)?"TRUE":"FALSE");

		if (ent->owner && ent->owner->client)
			gi.dprintf("owner->name = %s\n", ent->owner->client->pers.netname);
		if (ent->owner && ent->owner->classname)
			gi.dprintf("owner->classname = %s\n", ent->owner->classname);

		if (ent->enemy)
		{
			if (ent->enemy->client)
				gi.dprintf("enemy->name = %s\n", ent->enemy->client->pers.netname);
			if (ent->enemy->classname)
				gi.dprintf("enemy->classname = %s\n", ent->enemy->classname);
			gi.dprintf("enemy->burning = %s\n", (ent->enemy->burning)?"TRUE":"FALSE");
			gi.dprintf("enemy->disintegrated = %s\n", (ent->enemy->disintegrated)?"TRUE":"FALSE");
		}

		if (ent->oldenemy)
		{
			if (ent->oldenemy->client)
				gi.dprintf("oldenemy->name = %s\n", ent->oldenemy->client->pers.netname);
			if (ent->oldenemy->classname)
				gi.dprintf("oldenemy->classname = %s\n", ent->oldenemy->classname);
			gi.dprintf("oldenemy->burning = %s\n", (ent->oldenemy->burning)?"TRUE":"FALSE");
			gi.dprintf("oldenemy->disintegrated = %s\n", (ent->oldenemy->disintegrated)?"TRUE":"FALSE");
		}

		gi.dprintf("----------------------------\n\n");
		gi.error("SV_RunThink()");
//CW--
	}

	ent->think(ent);

	return false;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact(edict_t *e1, trace_t *trace)
{
	edict_t	*e2;

	e2 = trace->ent;

	if (e1->touch && (e1->solid != SOLID_NOT))
		e1->touch(e1, e2, &trace->plane, trace->surface);
	
	if (e2->touch && (e2->solid != SOLID_NOT))
		e2->touch(e2, e1, NULL, NULL);
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

int ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i;
	int		blocked;
	
	blocked = 0;
	if (normal[2] > 0.0)
		blocked |= 1;		// floor
	if (!normal[2])
		blocked |= 2;		// step
	
	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if ((out[i] > -STOP_EPSILON) && (out[i] < STOP_EPSILON))
			out[i] = 0.0;
	}
	
	return blocked;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
============
*/
#define	MAX_CLIP_PLANES	5

int SV_FlyMove(edict_t *ent, float time, int mask)
{
	edict_t	*hit;
	trace_t	trace;
	vec3_t	planes[MAX_CLIP_PLANES];
	vec3_t	primal_velocity;
	vec3_t	original_velocity;
	vec3_t	new_velocity;
	vec3_t	dir;
	vec3_t	end;
	float	time_left;
	float	d;
	int		numplanes;
	int		bumpcount;
	int		numbumps;
	int		blocked;
	int		i;
	int		j;
	
	numbumps = 4;
	
	blocked = 0;
	VectorCopy(ent->velocity, original_velocity);
	VectorCopy(ent->velocity, primal_velocity);
	numplanes = 0;
	time_left = time;
	ent->groundentity = NULL;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		for (i = 0; i < 3; i++)
			end[i] = ent->s.origin[i] + (time_left * ent->velocity[i]);

		trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, mask);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy(vec3_origin, ent->velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy(trace.endpos, ent->s.origin);
			VectorCopy(ent->velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		hit = trace.ent;

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (hit->solid == SOLID_BSP)
			{
				ent->groundentity = hit;
				ent->groundentity_linkcount = hit->linkcount;
			}
		}
		if (!trace.plane.normal[2])
			blocked |= 2;		// step


//		Run the impact function.

		SV_Impact(ent, &trace);
		if (!ent->inuse)
			break;		// removed by the impact function

		
		time_left -= time_left * trace.fraction;
		
		// clipped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy(vec3_origin, ent->velocity);
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;


//		Modify original_velocity so it parallels all of the clip planes.

		for (i = 0; i < numplanes; i++)
		{
			ClipVelocity(original_velocity, planes[i], new_velocity, 1);
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (DotProduct(new_velocity, planes[j]) < 0.0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}
		
		if (i != numplanes)
		{	// go along this plane
			VectorCopy(new_velocity, ent->velocity);
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
				VectorCopy(vec3_origin, ent->velocity);
				return 7;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->velocity);
			VectorScale(dir, d, ent->velocity);
		}

// If original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners.

		if (DotProduct(ent->velocity, primal_velocity) <= 0.0)
		{
			VectorCopy(vec3_origin, ent->velocity);
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_AddGravity

============
*/
void SV_AddGravity(edict_t *ent)
{
	ent->velocity[2] -= ent->gravity * sv_gravity->value * FRAMETIME;
}

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entity's velocity at all.
============
*/
trace_t SV_PushEntity(edict_t *ent, vec3_t push)
{
	trace_t	trace;
	vec3_t	start;
	vec3_t	end;
	int		mask;

//CW++
	qboolean ent_c4trap = false;
//CW--

	VectorCopy(ent->s.origin, start);
	VectorAdd(start, push, end);

retry:
	if (ent->clipmask)
		mask = ent->clipmask;
	else
		mask = MASK_SOLID;

	trace = gi.trace(start, ent->mins, ent->maxs, end, ent, mask);
	
//CW++
//	We don't want weapon projectiles or C4/Traps to interfere with each other.

	if (ent->die && ((ent->die == C4_DieFromDamage) || (ent->die == Trap_DieFromDamage)))
		ent_c4trap = true;

	if (trace.ent->wep_proj && (ent->wep_proj || ent_c4trap))
	{
		VectorCopy(end, ent->s.origin);
		gi.linkentity(ent);
		return trace;
	}

//	Otherwise, do that funky pushing thang.

	else
	{
//CW--
		VectorCopy(trace.endpos, ent->s.origin);
		gi.linkentity(ent);
	}

	if (trace.fraction != 1.0)
	{
		SV_Impact(ent, &trace);

		// if the pushed entity went away and the pusher is still there
		if (!trace.ent->inuse && ent->inuse)
		{
			// move the pusher back and try again
			VectorCopy(start, ent->s.origin);
			gi.linkentity(ent);
			goto retry;
		}
	}

	if (ent->inuse)
		G_TouchTriggers(ent);

	return trace;
}					


typedef struct
{
	edict_t	*ent;
	vec3_t	origin;
	vec3_t	angles;
	float	deltayaw;
} pushed_t;

pushed_t	pushed[MAX_EDICTS];
pushed_t	*pushed_p;
edict_t		*obstacle;

// Knightmare- added from Lazarus
void MoveRiders (edict_t *platform, edict_t *ignore, vec3_t move, vec3_t amove, qboolean turn)
{
	int	i;
	edict_t	*rider;

	for (i=1, rider=g_edicts+i; i<=globals.num_edicts; i++, rider++)
	{
		if ((rider->groundentity == platform) && (rider != ignore)) {
			VectorAdd(rider->s.origin,move,rider->s.origin);
			if (turn && (amove[YAW] != 0.)) {
				if (!rider->client)
					rider->s.angles[YAW] += amove[YAW];
				else
				{
					rider->s.angles[YAW] += amove[YAW];
					rider->client->ps.pmove.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
					rider->client->ps.pmove.pm_type = PM_FREEZE;
					rider->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
				}
			}
			gi.linkentity(rider);
			if (SV_TestEntityPosition(rider)) {
				// Move is blocked. Since this is for riders, not pushees,
				// it should be ok to just back the move for this rider off
				VectorSubtract(rider->s.origin,move,rider->s.origin);
				if (turn && (amove[YAW] != 0.)) {
					rider->s.angles[YAW] -= amove[YAW];
					if (rider->client)
					{
						rider->client->ps.pmove.delta_angles[YAW] -= ANGLE2SHORT(amove[YAW]);
						rider->client->ps.viewangles[YAW] -= amove[YAW];
					}
				}
				gi.linkentity(rider);
			} else {
				// move this rider's riders
				MoveRiders (rider,ignore,move,amove,turn);
			}
		}
	}
}

/*
============
RealBoundingBox

Returns the actual bounding box of a bmodel. This is a big improvement over
what q2 normally does with rotating bmodels - q2 sets absmin, absmax to a cube
that will completely contain the bmodel at *any* rotation on *any* axis, whether
the bmodel can actually rotate to that angle or not. This leads to a lot of
false block tests in SV_Push if another bmodel is in the vicinity.
============
*/

void RealBoundingBox(edict_t *ent, vec3_t mins, vec3_t maxs)
{
	vec3_t	forward, left, up, f1, l1, u1;
	vec3_t	p[8];
	int		i, j, k, j2, k4;

	for (k=0; k<2; k++)
	{
		k4 = k*4;
		if (k)
			p[k4][2] = ent->maxs[2];
		else
			p[k4][2] = ent->mins[2];
		p[k4+1][2] = p[k4][2];
		p[k4+2][2] = p[k4][2];
		p[k4+3][2] = p[k4][2];
		for (j=0; j<2; j++)
		{
			j2 = j*2;
			if (j)
				p[j2+k4][1] = ent->maxs[1];
			else
				p[j2+k4][1] = ent->mins[1];
			p[j2+k4+1][1] = p[j2+k4][1];
			for (i=0; i<2; i++)
			{
				if (i)
					p[i+j2+k4][0] = ent->maxs[0];
				else
					p[i+j2+k4][0] = ent->mins[0];
			}
		}
	}
	AngleVectors(ent->s.angles,forward,left,up);
	for (i=0; i<8; i++)
	{
		VectorScale(forward,p[i][0],f1);
		VectorScale(left,-p[i][1],l1);
		VectorScale(up,p[i][2],u1);
		VectorAdd(ent->s.origin,f1,p[i]);
		VectorAdd(p[i],l1,p[i]);
		VectorAdd(p[i],u1,p[i]);
	}
	VectorCopy(p[0],mins);
	VectorCopy(p[0],maxs);
	for (i=1; i<8; i++)
	{
		mins[0] = min(mins[0],p[i][0]);
		mins[1] = min(mins[1],p[i][1]);
		mins[2] = min(mins[2],p[i][2]);
		maxs[0] = max(maxs[0],p[i][0]);
		maxs[1] = max(maxs[1],p[i][1]);
		maxs[2] = max(maxs[2],p[i][2]);
	}
}
// end Knightmare

/*
============
SV_Push

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
============
*/
qboolean SV_Push (edict_t *pusher, vec3_t move, vec3_t amove)
{
	edict_t		*check;
	edict_t		*block;
	pushed_t	*p;
	vec3_t		mins;
	vec3_t		maxs;
	vec3_t		org;
	vec3_t		org2;
//	vec3_t		move2;
	vec3_t		forward;
	vec3_t		right;
	vec3_t		up;
	int			i, e;
	// Knightmare added
	qboolean	turn;
	vec3_t		move2={0,0,0};
	vec3_t		move3={0,0,0};
	vec3_t		org_check, realmins, realmaxs;
	trace_t		tr;

//	Clamp the move to 1/8 units, so the position will be accurate for client side prediction.

	for (i = 0; i < 3; i++)
	{
		float	temp;
		temp = move[i] * 8.0;
		if (temp > 0.0)
			temp += 0.5;
		else
			temp -= 0.5;
		move[i] = 0.125 * (int)temp;
	}

//	Find the bounding box.

	for (i = 0; i < 3; i++)
	{
		mins[i] = pusher->absmin[i] + move[i];
		maxs[i] = pusher->absmax[i] + move[i];
	}

	// Knightmare added
	// Lazarus: temp turn indicates whether riders
	//          should rotate with the pusher
	if (pusher->turn_rider || turn_rider->value)	// Knightmare- changed this from AND to OR
		turn = true;
	else
		turn = false;
	// end Knightmare


//	We need this for pushing things later.

	VectorSubtract(vec3_origin, amove, org);
	AngleVectors(org, forward, right, up);

//	Save the pusher's original position.

	pushed_p->ent = pusher;
	VectorCopy(pusher->s.origin, pushed_p->origin);
	VectorCopy(pusher->s.angles, pushed_p->angles);
	if (pusher->client)
		pushed_p->deltayaw = pusher->client->ps.pmove.delta_angles[YAW];
	pushed_p++;

//	Move the pusher to its final position.

	VectorAdd(pusher->s.origin, move, pusher->s.origin);
	VectorAdd(pusher->s.angles, amove, pusher->s.angles);
	gi.linkentity(pusher);

	// Knightmare added
	// Lazarus: Standard Q2 takes a horrible shortcut
	//          with rotating brush models, setting
	//          absmin and absmax to a cube that would
	//          contain the brush model if it could
	//          rotate around ANY axis. The result is
	//          a lot of false hits on intersections,
	//          particularly when you have multiple
	//          rotating brush models in the same area.
	//          RealBoundingBox gives us the actual
	//          bounding box at the current angles.
	RealBoundingBox(pusher,realmins,realmaxs);

//	See if any solid entities are inside the final position.

	check = g_edicts + 1;
	for (e = 1; e < globals.num_edicts; e++, check++)
	{
		if (!check->inuse)
			continue;
		// Knightmare added
		if (check == pusher->owner)	// Lazarus: owner can't block us
			continue;

//Maj++
		if (!check->solid)
			continue;

		if (check->classname[0] == 'R')
		{
			if (check->item != item_navi2)
				continue;
		}
//Maj--

		if ((check->movetype == MOVETYPE_PUSH) || (check->movetype == MOVETYPE_STOP) || (check->movetype == MOVETYPE_NONE) || (check->movetype == MOVETYPE_NOCLIP))
			continue;

		if (!check->area.prev)
			continue;		// not linked in anywhere

//		If the entity is standing on the pusher, it will definitely be moved.

		if (check->groundentity != pusher)
		{
			// see if the ent needs to be tested
		/*	if ((check->absmin[0] >= maxs[0]) || (check->absmin[1] >= maxs[1]) || (check->absmin[2] >= maxs[2]) || 
				(check->absmax[0] <= mins[0]) || (check->absmax[1] <= mins[1]) || (check->absmax[2] <= mins[2]))
				continue;*/

			// Knightmare- use realmins & realmaxs
			if ( check->absmin[0] >= realmaxs[0]
			|| check->absmin[1] >= realmaxs[1]
			|| check->absmin[2] >= realmaxs[2]
			|| check->absmax[0] <= realmins[0]
			|| check->absmax[1] <= realmins[1]
			|| check->absmax[2] <= realmins[2] )
				continue;
			// end Knightmare

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}

		if ((pusher->movetype == MOVETYPE_PUSH) || (check->groundentity == pusher))
		{
			// move this entity
			pushed_p->ent = check;
			VectorCopy(check->s.origin, pushed_p->origin);
			VectorCopy(check->s.angles, pushed_p->angles);
			pushed_p++;

			// try moving the contacted entity 
			VectorAdd(check->s.origin, move, check->s.origin);

			// Knightmare added
			// Lazarus: if turn_rider is set, do it. We don't do this by default
			//          'cause it can be a fairly drastic change in gameplay
			if (turn && (check->groundentity == pusher)) {
				if(!check->client)
				{
					check->s.angles[YAW] += amove[YAW];
				}
				else
				{
					if(amove[YAW] != 0.)
					{
						check->client->ps.pmove.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
						check->client->ps.viewangles[YAW] += amove[YAW];

						// PM_FREEZE makes the turn smooth, even though it will
						// be turned off by ClientThink in the very next video frame
						check->client->ps.pmove.pm_type = PM_FREEZE;
						// PMF_NO_PREDICTION overrides .exe's client physics, which
						// really doesn't like for us to change player angles. Note
						// that this isn't strictly necessary, since Lazarus 1.7 and
						// later automatically turn prediction off (in ClientThink) when 
						// player is riding a MOVETYPE_PUSH
						check->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
					}
					if(amove[PITCH] != 0.)
					{
						float	delta_yaw;
						float	pitch = amove[PITCH];

						delta_yaw = check->s.angles[YAW] - pusher->s.angles[YAW];
						delta_yaw *= M_PI / 180.;
						pitch *= cos(delta_yaw);
						check->client->ps.pmove.delta_angles[PITCH] += ANGLE2SHORT(pitch);
						check->client->ps.viewangles[PITCH] += pitch;
						check->client->ps.pmove.pm_type = PM_FREEZE;
						check->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
					}
				}
			}

			// Lazarus: This is where we attempt to move check due to a rotation, WITHOUT embedding
			//          check in pusher (or anything else)
			if((amove[PITCH] != 0) || (amove[YAW] != 0) || (amove[ROLL] != 0))
			{
				// Argh! - always need to do this, except for pendulums
			//	if (pusher->movetype != MOVETYPE_PENDULUM)
				{
					// figure movement due to the pusher's amove
					VectorAdd(check->s.origin, check->origin_offset, org_check);
					VectorSubtract (org_check, pusher->s.origin, org);
					org2[0] = DotProduct (org, forward);
					org2[1] = -DotProduct (org, right);
					org2[2] = DotProduct (org, up);
					VectorSubtract (org2, org, move2);
					VectorAdd (check->s.origin, move2, check->s.origin);
				}

				// Argh! - on top of a rotating pusher (moved the groundentity check here)
				if (check->groundentity == pusher)
				{
					if ((amove[PITCH] != 0) || (amove[ROLL] != 0))
					{
						VectorCopy(check->s.origin,org);
						org[2] += 2*check->mins[2];
						//tr = gi.trace(check->s.origin,vec3_origin,vec3_origin,org,check,MASK_SOLID);
						//if(!tr.startsolid && tr.fraction < 1)
						//	check->s.origin[2] = tr.endpos[2] - check->mins[2]
						//		+ fabs(tr.plane.normal[0])*check->size[0]/2
						//		+ fabs(tr.plane.normal[1])*check->size[1]/2;

						// Argh! - this should fix collision problem with simple
						//         rotating pushers, trains still seem okay too but
						//         I haven't tested them thoroughly
						tr = gi.trace(check->s.origin, check->mins, check->maxs, org, check, MASK_SOLID);
						if(!tr.startsolid && tr.fraction < 1)
							check->s.origin[2] = tr.endpos[2];
					}
				}
			}
			// end Knightmare

		/*
			if (check->client)
			{	// FIXME: doesn't rotate monsters?
				check->client->ps.pmove.delta_angles[YAW] += amove[YAW];
			}

			// figure movement due to the pusher's amove
			VectorSubtract(check->s.origin, pusher->s.origin, org);
			org2[0] = DotProduct(org, forward);
			org2[1] = -DotProduct(org, right);
			org2[2] = DotProduct(org, up);
			VectorSubtract(org2, org, move2);
			VectorAdd(check->s.origin, move2, check->s.origin);
		*/

			// may have pushed them off an edge
			if (check->groundentity != pusher)
				check->groundentity = NULL;

			block = SV_TestEntityPosition(check);
			if (!block)
			{	// pushed ok
				gi.linkentity(check);
				// Knightmare added
				// Lazarus: Move check riders, and riders of riders, and... well, you get the pic
				VectorAdd(move,move2,move3);
				MoveRiders (check,NULL,move3,amove,turn);
				// end Knightmare
				// impact?
				continue;
			}

			// if it is ok to leave in the old position, do it
			// this is only relevent for riding entities, not pushed
			// FIXME: this doesn't acount for rotation
			VectorSubtract(check->s.origin, move, check->s.origin);
			// Knightmare added
			VectorSubtract (check->s.origin, move2, check->s.origin);	
			if (turn)
			{
				// Argh! - angle
				check->s.angles[YAW] -= amove[YAW];
				if (check->client)
				{
					check->client->ps.pmove.delta_angles[YAW] -= ANGLE2SHORT(amove[YAW]);
					check->client->ps.viewangles[YAW] -= amove[YAW];
				}
			}
			// end Knightmare

			block = SV_TestEntityPosition(check);
			if (!block)
			{
				pushed_p--;
				continue;
			}
			if (check->svflags & SVF_GIB) // Knightmare- gibs don't block
			{
				G_FreeEdict(check);
				pushed_p--;
				continue;
			}
		}
		
		// save off the obstacle so we can call the block function
		obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for (p = pushed_p - 1; p >= pushed; p--)
		{
			VectorCopy(p->origin, p->ent->s.origin);
			VectorCopy(p->angles, p->ent->s.angles);
			if (p->ent->client)
				p->ent->client->ps.pmove.delta_angles[YAW] = p->deltayaw;

			gi.linkentity(p->ent);
		}
		return false;
	}

	// see if anything we moved has touched a trigger
	//FIXME: is there a better way to handle this?
	for (p = pushed_p - 1; p >= pushed; p--)
		G_TouchTriggers(p->ent);

	return true;
}

/*
================
SV_Physics_Pusher

Bmodel objects don't interact with each other, but
push all box objects
================
*/
void SV_Physics_Pusher(edict_t *ent)
{
	edict_t	*part;
	edict_t	*mv;
	vec3_t	move;
	vec3_t	amove;

	// if not a team captain, so movement will be handled elsewhere
	if (ent->flags & FL_TEAMSLAVE)
		return;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out

	pushed_p = pushed;
	for (part = ent; part; part = part->teamchain)
	{
		if (part->velocity[0] || part->velocity[1] || part->velocity[2] || part->avelocity[0] || part->avelocity[1] || part->avelocity[2])
		{	// object is moving
			VectorScale(part->velocity, FRAMETIME, move);
			VectorScale(part->avelocity, FRAMETIME, amove);

			if (!SV_Push(part, move, amove))
				break;	// move was blocked

			// Knightmare added
			if (part->moveinfo.is_blocked) {
				part->moveinfo.is_blocked = false;
				if (part->moveinfo.sound_middle)
					part->s.sound = part->moveinfo.sound_middle;
			}
			// end Knightmare
		}
	}
	if (pushed_p > &pushed[MAX_EDICTS])
		gi.error("pushed_p > &pushed[MAX_EDICTS], memory corrupted");								//CW

	if (part)
	{
		// the move failed, bump all nextthink times and back out moves
		for (mv = ent; mv; mv = mv->teamchain)
		{
			if (mv->nextthink > 0)
				mv->nextthink += FRAMETIME;
		}

		// if the pusher has a "blocked" function, call it
		// otherwise, just stay in place until the obstacle is gone
		if (part->blocked)
		{
			part->blocked(part, obstacle);
			part->moveinfo.is_blocked = true;	// Knightmare added
		}
	}
	else
	{
		// the move succeeded, so call all think functions
		for (part = ent; part; part = part->teamchain)
			SV_RunThink(part);
	}
}

//==================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None(edict_t *ent)
{
//	Regular thinking.

	SV_RunThink(ent);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip(edict_t *ent)
{
//	Regular thinking.

	if (!SV_RunThink(ent))
		return;
	
	VectorMA(ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles);
	VectorMA(ent->s.origin, FRAMETIME, ent->velocity, ent->s.origin);

	gi.linkentity(ent);
}


/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void SV_Physics_Toss(edict_t *ent)
{
	edict_t		*slave;
	trace_t		trace;
	vec3_t		move;
	vec3_t		old_origin;
	float		backoff;
	qboolean	wasinwater;
	qboolean	isinwater;

//	Regular thinking.

	SV_RunThink(ent);

//CW++
	if (!ent->inuse)
		return;
//CW--

//	If not a team captain, so movement will be handled elsewhere.

	if (ent->flags & FL_TEAMSLAVE)
		return;

	if (ent->velocity[2] > 0)
		ent->groundentity = NULL;

//	Check for the groundentity going away.

	if (ent->groundentity)
	{
		if (!ent->groundentity->inuse)
			ent->groundentity = NULL;
	}

//	If onground, return without moving.

	if (ent->groundentity)
		return;

	VectorCopy(ent->s.origin, old_origin);
	SV_CheckVelocity(ent);

//	Add gravity.

	if ((ent->movetype != MOVETYPE_FLY)	&& (ent->movetype != MOVETYPE_FLYMISSILE))
		SV_AddGravity(ent);

//	Move angles.

	VectorMA(ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles);

//	Move origin.

	VectorScale(ent->velocity, FRAMETIME, move);
	trace = SV_PushEntity(ent, move);
	if (!ent->inuse)
		return;

//CW++
//	We don't want weapon projectiles or C4/Traps to interfere with each other.

	if (trace.ent->wep_proj)
	{
		if (ent->wep_proj || (ent->die && ((ent->die == C4_DieFromDamage) || (ent->die == Trap_DieFromDamage))))
			return;
	}
//CW--

	if (trace.fraction < 1.0)
	{
		if (ent->movetype == MOVETYPE_BOUNCE)
			backoff = 1.5;
		else
			backoff = 1.0;

		ClipVelocity(ent->velocity, trace.plane.normal, ent->velocity, backoff);

//		Stop if on ground.

		if (trace.plane.normal[2] > 0.7)
		{
			if ((ent->velocity[2] < 60.0) || (ent->movetype != MOVETYPE_BOUNCE))
			{
				ent->groundentity = trace.ent;
				ent->groundentity_linkcount = trace.ent->linkcount;
				VectorCopy(vec3_origin, ent->velocity);
				VectorCopy(vec3_origin, ent->avelocity);
			}
		}
	}
	
//	Check for water transition.

	wasinwater = ent->watertype & MASK_WATER;
	ent->watertype = gi.pointcontents(ent->s.origin);
	isinwater = ent->watertype & MASK_WATER;

	if (isinwater)
		ent->waterlevel = 1;
	else
		ent->waterlevel = 0;

	if (!wasinwater && isinwater)
		gi.positioned_sound(old_origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, ATTN_NORM, 0);
	else if (wasinwater && !isinwater)
		gi.positioned_sound(ent->s.origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, ATTN_NORM, 0);

//	Move teamslaves.

	for (slave = ent->teamchain; slave; slave = slave->teamchain)
	{
		VectorCopy(ent->s.origin, slave->s.origin);
		gi.linkentity(slave);
	}
}


//CW++
/*
=============
SV_Physics_FlyBounce

Used for flying entities that bounce off (non-client) surfaces 
without a reduction in speed. Not affected by gravity.
=============
*/
void SV_Physics_FlyBounce(edict_t *ent)
{
	trace_t		trace;
	vec3_t		move;
	vec3_t		old_origin;
	qboolean	wasinwater;
	qboolean	isinwater;

//	Regular thinking.

	SV_RunThink(ent);

//CW++
	if (!ent->inuse)
		return;
//CW--

//	Can't be on the ground if we're moving upwards.

	if (ent->velocity[2] > 0)
		ent->groundentity = NULL;

//	Check for the groundentity going away.

	if (ent->groundentity)
	{
		if (!ent->groundentity->inuse)
			ent->groundentity = NULL;
	}

//	Update entity position and velocity.

	VectorCopy(ent->s.origin, old_origin);
	SV_CheckVelocity(ent);
	VectorMA(ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles);

	VectorScale(ent->velocity, FRAMETIME, move);
	trace = SV_PushEntity(ent, move);
	if (!ent->inuse)
		return;
	if (trace.ent->wep_proj && ent->wep_proj)	// don't want weapon projectiles to bounce off each other
		return;

//	Flip velocity and angles.

	if (trace.fraction < 1.0)
	{
		ClipVelocity(ent->velocity, trace.plane.normal, ent->velocity, 2.0);
		vectoangles(ent->velocity, ent->s.angles);
		VectorNormalize2(ent->velocity, ent->movedir);
	}
	
//	Check for a water transition.

	wasinwater = ent->watertype & MASK_WATER;
	ent->watertype = gi.pointcontents(ent->s.origin);
	isinwater = ent->watertype & MASK_WATER;

	if (isinwater)
		ent->waterlevel = 1;
	else
		ent->waterlevel = 0;

	if (!wasinwater && isinwater)
		gi.positioned_sound(old_origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, ATTN_NORM, 0);
	else if (wasinwater && !isinwater)
		gi.positioned_sound(ent->s.origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, ATTN_NORM, 0);
}
//CW--


/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
FIXME: is this true?
=============
*/

//FIXME: hacked in for E3 demo
#define	sv_stopspeed		100
#define sv_friction			6
#define sv_waterfriction	1

void SV_AddRotationalFriction(edict_t *ent)
{
	int		n;
	float	adjustment;

	VectorMA(ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles);
	adjustment = FRAMETIME * sv_stopspeed * sv_friction;
	for (n = 0; n < 3; n++)
	{
		if (ent->avelocity[n] > 0.0)
		{
			ent->avelocity[n] -= adjustment;
			if (ent->avelocity[n] < 0.0)
				ent->avelocity[n] = 0.0;
		}
		else
		{
			ent->avelocity[n] += adjustment;
			if (ent->avelocity[n] > 0.0)
				ent->avelocity[n] = 0.0;
		}
	}
}

void SV_Physics_Step(edict_t *ent)
{
	edict_t		*groundentity;
	float		*vel;
	float		speed;
	float		newspeed;
	float		control;
	float		friction;
	qboolean	wasonground;
	qboolean	hitsound = false;
	int			mask;

	// airborn monsters should always check for ground
	if (!ent->groundentity)
		M_CheckGround(ent);

//Maj++
	// bots must also check ground
	else if (ent->isabot)
		M_CheckGround(ent);
//Maj--

	groundentity = ent->groundentity;

	SV_CheckVelocity(ent);

	if (groundentity)
		wasonground = true;
	else
		wasonground = false;
		
	if (ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2])
		SV_AddRotationalFriction(ent);

	// add gravity except:
	//   flying monsters
	//   swimming monsters who are in the water
	if (!wasonground)
	{
		if (!(ent->flags & FL_FLY))
		{
			if (!((ent->flags & FL_SWIM) && (ent->waterlevel > 2)))
			{
				if (ent->velocity[2] < sv_gravity->value * -0.1)
					hitsound = true;

				if (ent->waterlevel == 0)
					SV_AddGravity(ent);
			}
		}
	}

	// friction for flying monsters that have been given vertical velocity
	if ((ent->flags & FL_FLY) && (ent->velocity[2] != 0))
	{
		speed = fabs(ent->velocity[2]);
		control = (speed < sv_stopspeed) ? sv_stopspeed : speed;
		friction = sv_friction / 3;
		newspeed = speed - (FRAMETIME * control * friction);
		if (newspeed < 0.0)
			newspeed = 0.0;
		newspeed /= speed;
		ent->velocity[2] *= newspeed;
	}

	// friction for flying monsters that have been given vertical velocity
	if ((ent->flags & FL_SWIM) && (ent->velocity[2] != 0))
	{
		speed = fabs(ent->velocity[2]);
		control = (speed < sv_stopspeed) ? sv_stopspeed : speed;
		newspeed = speed - (FRAMETIME * control * sv_waterfriction * ent->waterlevel);
		if (newspeed < 0.0)
			newspeed = 0.0;
		newspeed /= speed;
		ent->velocity[2] *= newspeed;
	}

	if (ent->velocity[2] || ent->velocity[1] || ent->velocity[0])
	{
		// apply friction
		// let dead monsters who aren't completely onground slide
		if ((wasonground) || (ent->flags & (FL_SWIM | FL_FLY)))
			if (!(ent->health <= 0.0 && !M_CheckBottom(ent)))
			{
				vel = ent->velocity;
				speed = sqrt((vel[0] * vel[0]) + (vel[1] * vel[1]));
				if (speed)
				{
					friction = sv_friction;
					control = (speed < sv_stopspeed) ? sv_stopspeed : speed;
					newspeed = speed - (FRAMETIME * control * friction);

					if (newspeed < 0.0)
						newspeed = 0.0;
					newspeed /= speed;

					vel[0] *= newspeed;
					vel[1] *= newspeed;
				}
			}

		if (ent->svflags & SVF_MONSTER)
			mask = MASK_MONSTERSOLID;
		else
			mask = MASK_SOLID;

		SV_FlyMove(ent, FRAMETIME, mask);

		gi.linkentity(ent);
		G_TouchTriggers(ent);

		if (ent->groundentity && !wasonground && hitsound && !ent->isabot)							//CW
			gi.sound(ent, 0, gi.soundindex("world/land.wav"), 1, ATTN_NORM, 0);
	}

//	Regular thinking.

	SV_RunThink(ent);
}

//============================================================================
/*
================
G_RunEntity
================
*/
void G_RunEntity(edict_t *ent)
{
	if (ent->prethink)
		ent->prethink(ent);

	switch ((int)ent->movetype)
	{
		case MOVETYPE_PUSH:
		case MOVETYPE_STOP:
			SV_Physics_Pusher(ent);
			break;

		case MOVETYPE_NONE:
			SV_Physics_None(ent);
			break;

		case MOVETYPE_NOCLIP:
			SV_Physics_Noclip(ent);
			break;

		case MOVETYPE_STEP:
			SV_Physics_Step(ent);
			break;

		case MOVETYPE_TOSS:
		case MOVETYPE_BOUNCE:
		case MOVETYPE_FLY:
		case MOVETYPE_FLYMISSILE:
			SV_Physics_Toss(ent);
			break;

//CW++
		case MOVETYPE_FLYBOUNCE:
			SV_Physics_FlyBounce(ent);
			break;
//CW--

//DH++
		case MOVETYPE_WALK:
			SV_Physics_None(ent);
			break;
//DH--
		default:
			gi.error("SV_Physics: bad movetype %i", (int)ent->movetype);			
	}
}