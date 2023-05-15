// g_utils_q1.c

#include "g_local.h"

float PointDist (vec3_t x, vec3_t y)
{
	vec3_t len;
	float dist;

	VectorSubtract (x, y, len);
	dist = VectorLength(len);
	return dist;
}


qboolean IsQ1Chthon (edict_t *self)
{
	if ( !self || !self->classname || (self->classname[0] == 0) )
		return false;

	if ( (strcmp(self->classname, "q1_monster_chton") == 0) || (strcmp(self->classname, "monster_q1_chton") == 0) )
		return true;

	return false;
}


void Q1TeleportSounds (edict_t *ent)
{
	float sound = random();

	if (sound < 0.2)
		gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, gi.soundindex("q1world/teleport/r_tele1.wav"), 1, ATTN_NORM, 0);
	else if (sound < 0.4)
		gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, gi.soundindex("q1world/teleport/r_tele2.wav"), 1, ATTN_NORM, 0);
	else if (sound < 0.6)
		gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, gi.soundindex("q1world/teleport/r_tele3.wav"), 1, ATTN_NORM, 0);
	else if (sound < 0.8)
		gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, gi.soundindex("q1world/teleport/r_tele4.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound (ent, CHAN_NO_PHS_ADD+CHAN_VOICE, gi.soundindex("q1world/teleport/r_tele5.wav"), 1, ATTN_NORM, 0);
}
