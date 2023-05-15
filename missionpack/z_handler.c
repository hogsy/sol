/*
==============================================================================

handler handler

==============================================================================
*/

#include "g_local.h"
#include "z_handler.h"




void handler_standWhatNext (edict_t *self);
void handler_standSitWhatNext (edict_t *self);
void handler_stand (edict_t *self);
void handler_attack (edict_t *self);
void hound_createHound (edict_t *self, float healthPercent);
void handler_ConvertToInfantry (edict_t *self);

void hound_sight (edict_t *self, edict_t *other);
void infantry_sight (edict_t *self, edict_t *other);

static int	sound_attack;
/*
static int	sound_scratch;
static int	sound_sitdown;
static int	sound_standup;
*/

void handler_sight (edict_t *self, edict_t *other)
{
	hound_sight (self, other);
	infantry_sight (self, other);
}

//
// STAND
//

mframe_t handler_frames_stand1 [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL
};
mmove_t handler_stand1 = {FRAME_stand1start, FRAME_stand1end, handler_frames_stand1, handler_standSitWhatNext};


void handler_scratch (edict_t *self)
{
//	gi.sound (self, CHAN_VOICE, sound_scratch, 1, ATTN_NORM, 0);
}


mframe_t handler_frames_stand2 [] =
{
	ai_stand, 0, handler_scratch,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
};
mmove_t handler_stand2 = {FRAME_stand2start, FRAME_stand2end, handler_frames_stand2, handler_standSitWhatNext};



mframe_t handler_frames_stand3 [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
};
mmove_t handler_stand3 = {FRAME_stand3start, FRAME_stand3end, handler_frames_stand3, handler_standWhatNext};


void handler_standup(edict_t *self)
{
//	gi.sound (self, CHAN_VOICE, sound_standup, 1, ATTN_NORM, 0);
}


mframe_t handler_frames_stand4 [] =
{
	ai_stand, 0, handler_standup,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
};
mmove_t handler_stand4 = {FRAME_stand4start, FRAME_stand4end, handler_frames_stand4, handler_standWhatNext};



void handler_sitdown(edict_t *self)
{
//	gi.sound (self, CHAN_VOICE, sound_sitdown, 1, ATTN_NORM, 0);
}


mframe_t handler_frames_stand5 [] =
{
	ai_stand, 0, handler_sitdown,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
};
mmove_t handler_stand5 = {FRAME_stand5start, FRAME_stand5end, handler_frames_stand5, handler_standSitWhatNext};


/*
 00-30  Idle1 (sitting down)
 31-59  Idle2 (pat on head)
 60-89  Idle3 (standing)
 90-100 Stand (standing up from sitting)
101-110 Sit   (sitting down from standing)
111-128 Restrain (handler lets go)
*/
void handler_standWhatNext (edict_t *self)
{
	float r = random();

	if (r < 0.90)
	{
		self->monsterinfo.currentmove = &handler_stand3;
	}
	else 
	{
		self->monsterinfo.currentmove = &handler_stand5;
	}
}


void handler_standSitWhatNext (edict_t *self)
{
	float r = random();

	if (r < 0.70)
	{
		self->monsterinfo.currentmove = &handler_stand1;
	}
	else if (r < 0.85)
	{
		self->monsterinfo.currentmove = &handler_stand2;
	}
	else
	{
		self->monsterinfo.currentmove = &handler_stand4;
	}
}


void handler_stand (edict_t *self)
{
//	float r = random();

	if (self->monsterinfo.currentmove != &handler_stand1 &&
		self->monsterinfo.currentmove != &handler_stand2 &&
		self->monsterinfo.currentmove != &handler_stand3 &&
		self->monsterinfo.currentmove != &handler_stand4 &&
		self->monsterinfo.currentmove != &handler_stand5)
	{
		self->monsterinfo.currentmove = &handler_stand3;
	}
}



//
// PAIN
//

void handler_pain (edict_t *self, edict_t *other, float kick, int damage)
{
//	if (self->health < (self->max_health / 2))
//		self->s.skinnum = 1;
}


//
// ATTACK and MELEE
//

void handler_createHound (edict_t *self)
{
	self->s.modelindex2 = 0;
	hound_createHound(self, (self->health / 175.0));
}


void CheckIdleLoop (edict_t *self)
{
	if (!self->powerarmor_time && self->spawnflags & 8)
	{
		self->powerarmor_time = level.time + (FRAMETIME * random() * 3);
	}

	if (self->powerarmor_time > level.time)
	{
		self->s.frame -= 2;
	}
}

void CheckForEnemy (edict_t *self)
{
	if(self->enemy && (self->enemy->client || (self->enemy->svflags & SVF_MONSTER)))
	{
		self->powerarmor_time = 0;
		return;
	}

	if(self->powerarmor_time < level.time)
	{
		self->enemy = NULL;
		handler_stand(self);
		return;
	}

	self->s.frame--;
}

void StartCount (edict_t *self)
{
	self->powerarmor_time = level.time + 3;
}

mframe_t handler_frames_attack1 [] =
{
	ai_run, 0, StartCount,
	ai_run, 0, CheckForEnemy,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,

	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, CheckIdleLoop,
	ai_charge, 0, NULL,

	ai_charge, 0,	NULL,
	ai_charge, 0,	handler_createHound,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
};
mmove_t handler_move_attack1 = {FRAME_attack1Start, FRAME_attack1End, handler_frames_attack1, handler_ConvertToInfantry};


void handler_attack (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_attack, 1, ATTN_NORM, 0);

	self->monsterinfo.currentmove = &handler_move_attack1;

	self->powerarmor_time = 0;
}





/*
===
Death Stuff Starts
===
*/

void handler_dead (edict_t *self)
{
	// Lazarus: Stupid... if flies aren't set by M_FlyCheck, monster_think
	//          will cause us to come back here over and over and over
	//          until flies ARE set or monster is gibbed.
	//          This line fixes that:
	self->nextthink = 0;

	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
	M_FlyCheck (self);

	// Lazarus monster fade
	if (world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		self->think = FadeDieSink;
		self->nextthink = level.time+corpse_fadetime->value;
	}
}


void handler_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->health = 1; // can't die while together...
}


/*
===
End Death Stuff
===
*/

void SP_monster_infantry_precache (void);
void SP_monster_hound_precache (void);

void SP_monster_handler_precache (void)
{
	SP_monster_infantry_precache ();
	SP_monster_hound_precache ();

	sound_attack = gi.soundindex("monsters/guard/hhattack.wav");
/*
	sound_scratch = gi.soundindex("monsters/guard/hhscratch.wav");
	sound_sitdown = gi.soundindex("monsters/guard/hhsitdown.wav");
	sound_standup = gi.soundindex("monsters/guard/hhstandup.wav");
*/
}

// Knightmare- added soundcache function
void monster_handler_soundcache (edict_t *self)
{
	SP_monster_handler_precache ();
}


/*QUAKED monster_handler (1 .5 0) (-32 -32 -24) (32 32 32) Ambush Trigger_Spawn Sight
Enforcer with hound.  Releases hound on sight.
model="models/monsters/guard/handler/"
model2="models/monsters/guard/hound/"
*/
void SP_monster_handler (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	SP_monster_handler_precache ();

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/guard/handler/tris.md2");
		PatchMonsterModel("models/monsters/guard/hound/tris.md2");
		self->s.skinnum = self->style * 2;
	}

	self->s.modelindex = gi.modelindex ("models/monsters/guard/handler/tris.md2");
	self->s.modelindex2 = gi.modelindex ("models/monsters/guard/hound/tris.md2");

	/*
	Handler
	X = -36 to 3
	Y = -3  to 27
	Z = -24 to 28

	Hound
	X = -12 to 11
	Y = -30 to 30
	Z = -24  to 8
	*/

	VectorSet (self->mins, -32, -32, -24);
	VectorSet (self->maxs, 32, 32, 32);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	if (!self->health)
		self->health = 175;
	if (!self->gib_health)
		self->gib_health = -50;
	if (!self->mass)
		self->mass = 250;

	self->pain = handler_pain;
	self->die = handler_die;

	self->monsterinfo.stand = handler_stand;
	self->monsterinfo.walk = handler_stand;
	self->monsterinfo.run = handler_attack;
	self->monsterinfo.attack = handler_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = handler_sight;
	self->monsterinfo.idle = NULL;

	// Lazarus
	if (self->powerarmor)
	{
		if (self->powerarmortype == 1)
			self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
		else
			self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = self->powerarmor;
	}

	if (!self->monsterinfo.flies)
		self->monsterinfo.flies = 0.40;

	self->common_name = "Enforcer with HellHound";
	self->class_id = ENTITY_MONSTER_HANDLER;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &handler_stand1;
/*	if (self->health < 0)
	{
		mmove_t	*deathmoves[] = {&infantry_move_death1,
			                     &infantry_move_death2,
								 &infantry_move_death3,
								 NULL};
		M_SetDeath (self,(mmove_t **)&deathmoves);
	}*/
	self->monsterinfo.scale = MODEL_SCALE;

	if (!(self->spawnflags & 16))
	{
		level.total_monsters++;	// add one for the hound which is created later :)
	}

	walkmonster_start (self);
}
