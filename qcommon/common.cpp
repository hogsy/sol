/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.

This file is part of Quake 2 source code.

Quake 2 source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake 2 source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake 2 source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// common.c -- misc functions used in client and server

#include "qcommon.h"
#include <setjmp.h>

#ifdef _WIN32
#include "../win32/winquake.h"
#endif

#define	MAXPRINTMSG	8192 // was 4096, fix for nVidia 191.xx crash

#define MAX_NUM_ARGVS	50


int		com_argc;
char	*com_argv[MAX_NUM_ARGVS+1];

int		realtime;

jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame


FILE	*log_stats_file;

cvar_t	*host_speeds;
cvar_t	*log_stats;
cvar_t	*developer;
cvar_t	*timescale;
cvar_t	*fixedtime;
cvar_t	*logfile_active;	// 1 = buffer log, 2 = flush after each print
cvar_t	*showtrace;
cvar_t	*con_show_description;	// Knightmare added
cvar_t	*dedicated;

// Knightmare- for the game DLL to tell what engine it's running under
cvar_t *sv_engine;
cvar_t *sv_engine_version;

FILE	*logfile;

int			server_state;

// host_speeds times
int		time_before_game;
int		time_after_game;
int		time_before_ref;
int		time_after_ref;

qboolean LegacyProtocol (void);

/*
============================================================================

CLIENT / SERVER interactions

============================================================================
*/

static int	rd_target;
static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush)(int target, char *buffer);

void Com_BeginRedirect (int target, char *buffer, int buffersize, void (*flush)(int, char*))
{
	if (!target || !buffer || !buffersize || !flush)
		return;
	rd_target = target;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	rd_flush(rd_target, rd_buffer);

	rd_target = 0;
	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.
=============
*/
void Com_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr, fmt);
//	vsprintf (msg, fmt, argptr);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);	// fix for nVidia 191.xx crash
	va_end (argptr);

	if (rd_target)
	{
		if ((strlen (msg) + strlen(rd_buffer)) > (rd_buffersize - 1))
		{
			rd_flush(rd_target, rd_buffer);
			*rd_buffer = 0;
		}
	//	strncat (rd_buffer, msg);
		Q_strncatz (rd_buffer, rd_buffersize, msg);
		return;
	}

	Con_Print (msg);
		
	// also echo to debugging console
	if (msg[strlen(msg)-1] != '\r') // skip overwrittten outputs
		Sys_ConsoleOutput (msg);

	// logfile
//	if (logfile_active && logfile_active->value)
	if (logfile_active && logfile_active->integer)
	{
		char	name[MAX_OSPATH];
		
		if (!logfile)
		{
			snprintf (name, sizeof(name), "%s/qconsole.log", FS_Savegamedir ());	// was FS_Gamedir()
		//	if (logfile_active->value > 2)
			if (logfile_active->integer > 2)
				logfile = fopen (name, "a");
			else
				logfile = fopen (name, "w");
		}
		if (logfile)
			fprintf (logfile, "%s", msg);
	//	if (logfile_active->value > 1)
		if (logfile_active->integer > 1)
			fflush (logfile);		// force it to save every time
	}
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void Com_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
//	if (!developer || !developer->value)
	if (!developer || !developer->integer)
		return;			// don't confuse non-developers with techie stuff...

	va_start (argptr, fmt);
//	vsprintf (msg, fmt, argptr);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);	// fix for nVidia 191.xx crash
	va_end (argptr);
	
	Com_Printf ("%s", msg);
}


/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Error (int code, const char *fmt, ...)
{
	va_list		argptr;
	static char		msg[MAXPRINTMSG];
	static	qboolean	recursive;

	if (recursive)
		Sys_Error ("recursive error after: %s", msg);
	recursive = true;

	va_start (argptr, fmt);
//	vsprintf (msg, fmt, argptr);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);	// fix for nVidia 191.xx crash
	va_end (argptr);
	
	if (code == ERR_DISCONNECT)
	{
		CL_Drop ();
		recursive = false;
		longjmp (abortframe, -1);
	}
	else if (code == ERR_DROP)
	{
		Com_Printf (S_COLOR_RED"********************\n"
					S_COLOR_RED"ERROR: %s\n"
					S_COLOR_RED"********************\n", msg);
		SV_Shutdown (va("Server crashed: %s\n", msg), false);
		CL_Drop ();
		recursive = false;
		longjmp (abortframe, -1);
	}
	else
	{
		SV_Shutdown (va("Server fatal crashed: %s\n", msg), false);
		CL_Shutdown ();
	}

	if (logfile)
	{
		fclose (logfile);
		logfile = NULL;
	}

	Sys_Error ("%s", msg);
}


/*
=============
Com_Quit

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit (void)
{
	SV_Shutdown ("Server quit\n", false);
	CL_Shutdown ();

	if (logfile)
	{
		fclose (logfile);
		logfile = NULL;
	}

	Sys_Quit ();
}


/*
==================
Com_ServerState
==================
*/
int Com_ServerState (void)
{
	return server_state;
}

/*
==================
Com_SetServerState
==================
*/
void Com_SetServerState (int state)
{
	server_state = state;
}


/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

#define BIT_23	0x00800000
#define UPRBITS	0xFF000000

vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
#include "../client/anorms.h"






};

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte	*buf;
	
#ifdef PARANOID
	if (c < -128 || c > 127)
		Com_Error (ERR_FATAL, "MSG_WriteChar: range error");
#endif

	buf = static_cast< byte * >( SZ_GetSpace( sb, 1 ) );
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte	*buf;
	
#ifdef PARANOID
	if (c < 0 || c > 255)
		Com_Error (ERR_FATAL, "MSG_WriteByte: range error");
#endif

	buf = static_cast< byte * >( SZ_GetSpace( sb, 1 ) );
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte	*buf;
	
#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Com_Error (ERR_FATAL, "MSG_WriteShort: range error");
#endif

	buf = static_cast< byte * >( SZ_GetSpace( sb, 2 ) );
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte *buf = static_cast< byte * >( SZ_GetSpace( sb, 4 ) );
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float	f;
		int	l;
	} dat;
	
	
	dat.f = f;
	dat.l = LittleLong (dat.l);
	
	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteVec2( sizebuf_t *msg, const vec2_t src )
{
	MSG_WriteFloat( msg, src[ 0 ] );
	MSG_WriteFloat( msg, src[ 1 ] );
}

void MSG_WriteVec3( sizebuf_t *msg, const vec3_t src )
{
	MSG_WriteFloat( msg, src[ 0 ] );
	MSG_WriteFloat( msg, src[ 1 ] );
	MSG_WriteFloat( msg, src[ 2 ] );
}

void MSG_WriteVec4( sizebuf_t *msg, const vec4_t src )
{
	MSG_WriteFloat( msg, src[ 0 ] );
	MSG_WriteFloat( msg, src[ 1 ] );
	MSG_WriteFloat( msg, src[ 2 ] );
	MSG_WriteFloat( msg, src[ 3 ] );
}

// Knightmare added- for sending floats as nearest-integer shorts by rounding to nearest int first
void MSG_WriteFloatAsShort (sizebuf_t *sb, float f)
{
	// DG: float -> int always rounds down (3.99999f => 3), let's round up if > x.5
	// by adding almost 0.5 (analog for negative numbers) => x.5 becomes x, x.51 becomes (x+1)
	float	rounding = 0.4999f;
	short	asShort;

	if (f < 0.0f)
		rounding = -rounding;
	asShort = f + rounding;

	MSG_WriteShort (sb, asShort);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)
		SZ_Write (sb, (void*)"", 1);
	else
		SZ_Write (sb, s, (int)strlen(s)+1);
}


// Knightmare- 24-bit player coordinate transmission code
// Player movement coords are already in 1/8 precision integer form
void MSG_WritePMCoord24 (sizebuf_t *sb, int in)
{
	byte trans1;
	unsigned short trans2;

	trans1 = in >> 16;	// bits 16-23
	trans2 = in;		// bits 0-15

	MSG_WriteByte (sb, trans1);
	MSG_WriteShort (sb, trans2);
}

void MSG_WritePMCoord16 (sizebuf_t *sb, int in)
{
	MSG_WriteShort (sb, in);
}

#ifdef LARGE_MAP_SIZE

void MSG_WritePMCoord (sizebuf_t *sb, int in)
{
	MSG_WritePMCoord24 (sb, in);
}

#else // LARGE_MAP_SIZE

void MSG_WritePMCoord (sizebuf_t *sb, int in)
{
	MSG_WritePMCoord16 (sb, in);
}

#endif // LARGE_MAP_SIZE


int MSG_ReadPMCoord24 (sizebuf_t *msg_read)
{
	int tmp;
	byte trans1;
	unsigned short trans2;

	trans1 = MSG_ReadByte(msg_read);
	trans2 = MSG_ReadShort(msg_read);

	tmp = trans1 << 16;	// bits 16-23
	tmp += trans2;		// bits 0-15

	// Sign bit 23 means it's negative, so fill upper
	// 8 bits with 1s for 2's complement negative.
	if (tmp & BIT_23)	
		tmp |= UPRBITS;

	return tmp;
}

int MSG_ReadPMCoord16 (sizebuf_t *msg_read)
{
	return MSG_ReadShort (&net_message);
}

#ifdef LARGE_MAP_SIZE

int MSG_ReadPMCoord (sizebuf_t *msg_read)
{
	if ( LegacyProtocol() )
		return MSG_ReadPMCoord16 (msg_read);
	else
		return MSG_ReadPMCoord24 (msg_read);
}

#else // LARGE_MAP_SIZE

int MSG_ReadPMCoord (sizebuf_t *msg_read)
{
	return MSG_ReadPMCoord16 (msg_read);
}

#endif // LARGE_MAP_SIZE

void MSG_WriteCoord24 (sizebuf_t *sb, float f)
{
	int tmp;
	byte trans1;
	unsigned short trans2;

	tmp = f*8;			// 1/8 granulation, leaves bounds of +/-1M in signed 24-bit form
	trans1 = tmp >> 16;	// bits 16-23
	trans2 = tmp;		// bits 0-15

	// Don't mess with sign bits on this end to allow overflow (map wrap-around).

	MSG_WriteByte (sb, trans1);
	MSG_WriteShort (sb, trans2);
}

void MSG_WriteCoord16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, (int)(f*8));
}

#ifdef LARGE_MAP_SIZE

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	MSG_WriteCoord24 (sb, f);
}

#else // LARGE_MAP_SIZE

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	MSG_WriteCoord16 (sb, f);
}

#endif // LARGE_MAP_SIZE

void MSG_WritePos24 (sizebuf_t *sb, vec3_t pos)
{
	 MSG_WriteCoord24 (sb, pos[0]);
	 MSG_WriteCoord24 (sb, pos[1]);
	 MSG_WriteCoord24 (sb, pos[2]);
}

void MSG_WritePos16 (sizebuf_t *sb, vec3_t pos)
{
	 MSG_WriteCoord16 (sb, pos[0]);
	 MSG_WriteCoord16 (sb, pos[1]);
	 MSG_WriteCoord16 (sb, pos[2]);
}

#ifdef LARGE_MAP_SIZE

void MSG_WritePos (sizebuf_t *sb, vec3_t pos)
{
	MSG_WritePos24 (sb, pos);
}

#else // LARGE_MAP_SIZE

void MSG_WritePos (sizebuf_t *sb, vec3_t pos)
{
	MSG_WritePos16 (sb, pos);
}

#endif // LARGE_MAP_SIZE


void MSG_WriteAngle8 (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, (int)(f*256/360) & 255);
}

void MSG_WriteAngle16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, ANGLE2SHORT(f));
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteAngle8 (sb, f);
}


void MSG_WriteDeltaUsercmd (sizebuf_t *buf, usercmd_t *from, usercmd_t *cmd)
{
	int		bits;

//
// send the movement message
//
	bits = 0;
	if (cmd->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

    MSG_WriteByte (buf, bits);

	if (bits & CM_ANGLE1)
		MSG_WriteShort (buf, cmd->angles[0]);
	if (bits & CM_ANGLE2)
		MSG_WriteShort (buf, cmd->angles[1]);
	if (bits & CM_ANGLE3)
		MSG_WriteShort (buf, cmd->angles[2]);
	
	if (bits & CM_FORWARD)
		MSG_WriteShort (buf, cmd->forwardmove);
	if (bits & CM_SIDE)
	  	MSG_WriteShort (buf, cmd->sidemove);
	if (bits & CM_UP)
		MSG_WriteShort (buf, cmd->upmove);

 	if (bits & CM_BUTTONS)
	  	MSG_WriteByte (buf, cmd->buttons);
 	if (bits & CM_IMPULSE)
	    MSG_WriteByte (buf, cmd->impulse);

    MSG_WriteByte (buf, cmd->msec);
	MSG_WriteByte (buf, cmd->lightlevel);
}


void MSG_WriteDir (sizebuf_t *sb, vec3_t dir)
{
	int		i, best;
	float	d, bestd;
	
	if (!dir)
	{
		MSG_WriteByte (sb, 0);
		return;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	MSG_WriteByte (sb, best);
}


void MSG_ReadDir (sizebuf_t *sb, vec3_t dir)
{
	int		b;

	b = MSG_ReadByte (sb);
	if (b >= NUMVERTEXNORMALS)
		Com_Error (ERR_DROP, "MSF_ReadDir: out of range");
	VectorCopy (bytedirs[b], dir);
}


/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void MSG_WriteDeltaEntity (centity_state_t *from, centity_state_t *to, sizebuf_t *msg, qboolean force, qboolean newentity)
{
	unsigned int	bits, bits2;

	if (!to->number)
		Com_Error (ERR_FATAL, "Unset entity number");
	if (to->number >= MAX_EDICTS)
		Com_Error (ERR_FATAL, "Entity number >= MAX_EDICTS");

// send an update
	bits = bits2 = 0;

	if (to->number >= 256)
		bits |= U_NUMBER16;		// number8 is implicit otherwise

	if (to->origin[0] != from->origin[0])
		bits |= U_ORIGIN1;
	if (to->origin[1] != from->origin[1])
		bits |= U_ORIGIN2;
	if (to->origin[2] != from->origin[2])
		bits |= U_ORIGIN3;

	if ( to->angles[0] != from->angles[0] )
		bits |= U_ANGLE1;		
	if ( to->angles[1] != from->angles[1] )
		bits |= U_ANGLE2;
	if ( to->angles[2] != from->angles[2] )
		bits |= U_ANGLE3;
		
	if ( to->skinnum != from->skinnum )
	{
		if ((unsigned)to->skinnum < 256)
			bits |= U_SKIN8;
		else if ((unsigned)to->skinnum < 0x10000)
			bits |= U_SKIN16;
		else
			bits |= (U_SKIN8|U_SKIN16);
	}
		
	if ( to->frame != from->frame )
	{
		if (to->frame < 256)
			bits |= U_FRAME8;
		else
			bits |= U_FRAME16;
	}

	if ( to->effects != from->effects )
	{
		if (to->effects < 256)
			bits |= U_EFFECTS8;
		else if (to->effects < 0x8000)
			bits |= U_EFFECTS16;
		else
			bits |= U_EFFECTS8|U_EFFECTS16;
	}
	
	if ( to->renderfx != from->renderfx )
	{
		if (to->renderfx < 256)
			bits |= U_RENDERFX8;
		else if (to->renderfx < 0x8000)
			bits |= U_RENDERFX16;
		else
			bits |= U_RENDERFX8|U_RENDERFX16;
	}

	// Knightmare- added alpha
#ifdef NEW_ENTITY_STATE_MEMBERS
	// cap new value to correct range
	if (to->alpha < 0.0)
		to->alpha = 0.0;
	if (to->alpha > 1.0)
		to->alpha = 1.0;
	// Since the floating point value is never quite the same,
	// compare the new and the old as what they will be sent as
	if ((int)(to->alpha*255) != (int)(from->alpha*255))
		bits |= U_ALPHA;
#endif
	
	if ( to->solid != from->solid )
		bits |= U_SOLID;

	if ( (to->iflags & (IF_REAL_BBOX|IF_REAL_BBOX_16|IF_REAL_BBOX_8)) &&
		( !VectorCompare(to->mins, from->mins) || !VectorCompare(to->maxs, from->maxs) ) )
	{
		if (to->iflags & IF_REAL_BBOX)
			bits |= U_MINSMAXS_8|U_MINSMAXS_16;
		else if (to->iflags & IF_REAL_BBOX_16)
			bits |= U_MINSMAXS_16;
		else if (to->iflags & IF_REAL_BBOX_8)
			bits |= U_MINSMAXS_8;
	}

	// event is not delta compressed, just 0 compressed
	if ( to->event  )
		bits |= U_EVENT;
	
	if ( to->modelindex != from->modelindex )
		bits |= U_MODEL;
	if ( to->modelindex2 != from->modelindex2 )
		bits |= U_MODEL2;
	if ( to->modelindex3 != from->modelindex3 )
		bits |= U_MODEL3;
	if ( to->modelindex4 != from->modelindex4 )
		bits |= U_MODEL4;

#ifdef NEW_ENTITY_STATE_MEMBERS
	// Knightmare- extra model indices	
	if ( to->modelindex5 != from->modelindex5 )
		bits2 |= U2_MODEL5;
	if ( to->modelindex6 != from->modelindex6 )
		bits2 |= U2_MODEL6;
#endif

	if ( to->sound != from->sound )
		bits |= U_SOUND;

#ifdef NEW_ENTITY_STATE_MEMBERS
	if (to->loop_attenuation != from->loop_attenuation)
		bits |= U_ATTENUAT;
#endif

	if (newentity || (to->renderfx & RF_BEAM))
		bits |= U_OLDORIGIN;

	// alpha was here

	//
	// write the message
	//
	if (!bits && !bits2 && !force)
		return;		// nothing to send!

	//----------

	// Knightmare- handle 2nd dword of bits
	if (bits2 != 0)
	{
		bits |= U_MOREBITS4 | U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
		if (bits2 & 0xff000000)
			bits2 |= U2_MOREBITS7 | U2_MOREBITS6 | U2_MOREBITS5;
		else if (bits2 & 0x00ff0000)
			bits2 |= U2_MOREBITS6 | U2_MOREBITS5;
		else if (bits2 & 0x0000ff00)
			bits2 |= U2_MOREBITS5;
	}
	else
	{
		if (bits & 0xff000000)
			bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
		else if (bits & 0x00ff0000)
			bits |= U_MOREBITS2 | U_MOREBITS1;
		else if (bits & 0x0000ff00)
			bits |= U_MOREBITS1;
	}

	// send first dword of bits
	MSG_WriteByte (msg,	bits&255 );
	if (bits & 0xff000000)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
		MSG_WriteByte (msg,	(bits>>16)&255 );
		MSG_WriteByte (msg,	(bits>>24)&255 );
	}
	else if (bits & 0x00ff0000)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
		MSG_WriteByte (msg,	(bits>>16)&255 );
	}
	else if (bits & 0x0000ff00)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
	}

	if (bits2 != 0)
	{
		// send second dword of bits
		MSG_WriteByte (msg,	bits2 & 255);
		if (bits2 & 0xff000000)
		{
			MSG_WriteByte (msg,	(bits2 >> 8) & 255);
			MSG_WriteByte (msg,	(bits2 >> 16) & 255);
			MSG_WriteByte (msg,	(bits2 >> 24) & 255);
		}
		else if (bits2 & 0x00ff0000)
		{
			MSG_WriteByte (msg,	(bits2 >> 8) & 255);
			MSG_WriteByte (msg,	(bits2 >> 16) & 255);
		}
		else if (bits2 & 0x0000ff00)
		{
			MSG_WriteByte (msg,	(bits2 >> 8) & 255);
		}
	}

	//----------

	if (bits & U_NUMBER16)
		MSG_WriteShort (msg, to->number);
	else
		MSG_WriteByte (msg,	to->number);

	// Knightmare- changed these to shorts
	if (bits & U_MODEL)
		MSG_WriteShort (msg, to->modelindex);
	if (bits & U_MODEL2)
		MSG_WriteShort (msg, to->modelindex2);
	if (bits & U_MODEL3)
		MSG_WriteShort (msg, to->modelindex3);
	if (bits & U_MODEL4)
		MSG_WriteShort (msg, to->modelindex4);

#ifdef NEW_ENTITY_STATE_MEMBERS
	// Knightmare- extra model indices	
	if (bits2 & U2_MODEL5)
		MSG_WriteShort (msg, to->modelindex5);
	if (bits2 & U2_MODEL6)
		MSG_WriteShort (msg, to->modelindex6);
#endif

	if (bits & U_FRAME8)
		MSG_WriteByte (msg, to->frame);
	if (bits & U_FRAME16)
		MSG_WriteShort (msg, to->frame);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))		// used for laser colors
		MSG_WriteLong (msg, to->skinnum);
	else if (bits & U_SKIN8)
		MSG_WriteByte (msg, to->skinnum);
	else if (bits & U_SKIN16)
		MSG_WriteShort (msg, to->skinnum);


	if ( (bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16) )
		MSG_WriteLong (msg, to->effects);
	else if (bits & U_EFFECTS8)
		MSG_WriteByte (msg, to->effects);
	else if (bits & U_EFFECTS16)
		MSG_WriteShort (msg, to->effects);

	if ( (bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16) )
		MSG_WriteLong (msg, to->renderfx);
	else if (bits & U_RENDERFX8)
		MSG_WriteByte (msg, to->renderfx);
	else if (bits & U_RENDERFX16)
		MSG_WriteShort (msg, to->renderfx);

	// Knightmare- added alpha
#ifdef NEW_ENTITY_STATE_MEMBERS
	if (bits & U_ALPHA)
	{
	//	Com_Printf ("Sending alpha of %.2f for entity %i\n", to->alpha, to->number);
		MSG_WriteByte (msg, (byte)(to->alpha*255));
	}
#endif

	if (bits & U_ORIGIN1)
		MSG_WriteCoord (msg, to->origin[0]);		
	if (bits & U_ORIGIN2)
		MSG_WriteCoord (msg, to->origin[1]);
	if (bits & U_ORIGIN3)
		MSG_WriteCoord (msg, to->origin[2]);

	// Knightmare- switched to 16-bit angles
	if (bits & U_ANGLE1)
		MSG_WriteAngle16 (msg, to->angles[0]);
	if (bits & U_ANGLE2)
		MSG_WriteAngle16 (msg, to->angles[1]);
	if (bits & U_ANGLE3)
		MSG_WriteAngle16 (msg, to->angles[2]);

	if (bits & U_OLDORIGIN)
	{
		MSG_WriteCoord (msg, to->old_origin[0]);
		MSG_WriteCoord (msg, to->old_origin[1]);
		MSG_WriteCoord (msg, to->old_origin[2]);
	}

	// alpha was here

	// Knightmare- changed this to short
	if (bits & U_SOUND)
		MSG_WriteShort (msg, to->sound);

#ifdef NEW_ENTITY_STATE_MEMBERS
	if (bits & U_ATTENUAT)
		MSG_WriteByte (msg, (int)(min(max(to->loop_attenuation, 0.0f), 4.0f)*64.0));
#endif

	if (bits & U_EVENT)
		MSG_WriteByte (msg, to->event);
	if (bits & U_SOLID)
		MSG_WriteShort (msg, to->solid);

	if ( (bits & (U_MINSMAXS_8|U_MINSMAXS_16)) == (U_MINSMAXS_8|U_MINSMAXS_16) ) {
		MSG_WritePos (msg, to->mins);
		MSG_WritePos (msg, to->maxs);
	/*	Com_Printf ("Sending world-coord bbox for entity %i (%.2f %.2f %.2f)->(%.2f %.2f %.2f)\n",
					to->number, to->mins[0], to->mins[1], to->mins[2], to->maxs[0], to->maxs[1], to->maxs[2]); */
	}
	else if (bits & U_MINSMAXS_8) {
		MSG_WriteBBox8 (msg, to->mins, to->maxs);
	/*	Com_Printf ("Sending 8-bit bbox for entity %i (%.2f %.2f %.2f)->(%.2f %.2f %.2f)\n",
					to->number, to->mins[0], to->mins[1], to->mins[2], to->maxs[0], to->maxs[1], to->maxs[2]); */
	}
	else if (bits & U_MINSMAXS_16) {
		MSG_WriteBBox16 (msg, to->mins, to->maxs);
	/*	Com_Printf ("Sending 16-bit bbox for entity %i (%.2f %.2f %.2f)->(%.2f %.2f %.2f)\n",
					to->number, to->mins[0], to->mins[1], to->mins[2], to->maxs[0], to->maxs[1], to->maxs[2]); */
	}
}

//============================================================

//
// reading functions
//

void MSG_BeginReading (sizebuf_t *msg)
{
	msg->readcount = 0;
}

// returns -1 if no more characters are available
int MSG_ReadChar (sizebuf_t *msg_read)
{
	int	c;
	
	if (msg_read->readcount+1 > msg_read->cursize)
		c = -1;
	else
		c = (signed char)msg_read->data[msg_read->readcount];
	msg_read->readcount++;
	
	return c;
}

int MSG_ReadByte (sizebuf_t *msg_read)
{
	int	c;
	
	if (msg_read->readcount+1 > msg_read->cursize)
		c = -1;
	else
		c = (unsigned char)msg_read->data[msg_read->readcount];
	msg_read->readcount++;
	
	return c;
}

int MSG_ReadShort (sizebuf_t *msg_read)
{
	int	c;
	
	if (msg_read->readcount+2 > msg_read->cursize)
		c = -1;
	else		
		c = (short)(msg_read->data[msg_read->readcount]
		+ (msg_read->data[msg_read->readcount+1]<<8));
	
	msg_read->readcount += 2;
	
	return c;
}

int MSG_ReadLong (sizebuf_t *msg_read)
{
	int	c;
	
	if (msg_read->readcount+4 > msg_read->cursize)
		c = -1;
	else
		c = msg_read->data[msg_read->readcount]
		+ (msg_read->data[msg_read->readcount+1]<<8)
		+ (msg_read->data[msg_read->readcount+2]<<16)
		+ (msg_read->data[msg_read->readcount+3]<<24);
	
	msg_read->readcount += 4;
	
	return c;
}

float MSG_ReadFloat (sizebuf_t *msg_read)
{
	union
	{
		byte	b[4];
		float	f;
		int	l;
	} dat;
	
	if (msg_read->readcount+4 > msg_read->cursize)
		dat.f = -1;
	else
	{
		dat.b[0] =	msg_read->data[msg_read->readcount];
		dat.b[1] =	msg_read->data[msg_read->readcount+1];
		dat.b[2] =	msg_read->data[msg_read->readcount+2];
		dat.b[3] =	msg_read->data[msg_read->readcount+3];
	}
	msg_read->readcount += 4;
	
	dat.l = LittleLong (dat.l);

	return dat.f;	
}

void MSG_ReadVec2( sizebuf_t *msg, vec2_t dst )
{
	dst[ 0 ] = MSG_ReadFloat( msg );
	dst[ 1 ] = MSG_ReadFloat( msg );
}

void MSG_ReadVec3( sizebuf_t *msg, vec3_t dst )
{
	dst[ 0 ] = MSG_ReadFloat( msg );
	dst[ 1 ] = MSG_ReadFloat( msg );
	dst[ 2 ] = MSG_ReadFloat( msg );
}

void MSG_ReadVec4( sizebuf_t *msg, vec4_t dst )
{
	dst[ 0 ] = MSG_ReadFloat( msg );
	dst[ 1 ] = MSG_ReadFloat( msg );
	dst[ 2 ] = MSG_ReadFloat( msg );
	dst[ 3 ] = MSG_ReadFloat( msg );
}

// Knightmare added- for reading floats sent as shorts
float MSG_ReadFloatAsShort (sizebuf_t *msg_read)
{
	int n = MSG_ReadShort (msg_read);
	return (float)n;
}

char *MSG_ReadString (sizebuf_t *msg_read)
{
	static char	string[MSG_STRING_SIZE];	// 2048
	int		l,c;
	
	l = 0;
	do
	{
		c = MSG_ReadChar (msg_read);
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);
	
	string[l] = 0;
	
	return string;
}

char *MSG_ReadStringLine (sizebuf_t *msg_read)
{
	static char	string[MSG_STRING_SIZE];	// 2048
	int		l,c;
	
	l = 0;
	do
	{
		c = MSG_ReadChar (msg_read);
		if (c == -1 || c == 0 || c == '\n')
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);
	
	string[l] = 0;
	
	return string;
}

float MSG_ReadCoord24 (sizebuf_t *msg_read)
{
	int tmp;
	byte trans1;
	unsigned short trans2;

	trans1 = MSG_ReadByte (msg_read);
	trans2 = MSG_ReadShort (msg_read);

	tmp = trans1 << 16;	// bits 16-23
	tmp += trans2;		// bits 0-15

	// Sign bit 23 means it's negative, so fill upper
	// 8 bits with 1s for 2's complement negative.
	if (tmp & BIT_23)	
		tmp |= UPRBITS;

	return tmp * (1.0/8);	// restore 1/8 granulation
}

float MSG_ReadCoord16 (sizebuf_t *msg_read)
{
	return MSG_ReadShort(msg_read) * (1.0/8);
}

#ifdef LARGE_MAP_SIZE

float MSG_ReadCoord (sizebuf_t *msg_read)
{
	if ( LegacyProtocol() )
		return MSG_ReadCoord16 (msg_read);
	else
		return MSG_ReadCoord24 (msg_read);
}

#else // LARGE_MAP_SIZE

float MSG_ReadCoord (sizebuf_t *msg_read)
{
	return MSG_ReadCoord16 (msg_read);
}

#endif // LARGE_MAP_SIZE

void MSG_ReadPos24 (sizebuf_t *msg_read, vec3_t pos)
{
	pos[0] = MSG_ReadCoord24 (msg_read);
	pos[1] = MSG_ReadCoord24 (msg_read);
	pos[2] = MSG_ReadCoord24 (msg_read);
}

void MSG_ReadPos16 (sizebuf_t *msg_read, vec3_t pos)
{
	pos[0] = MSG_ReadCoord16 (msg_read);
	pos[1] = MSG_ReadCoord16 (msg_read);
	pos[2] = MSG_ReadCoord16 (msg_read);
}

#ifdef LARGE_MAP_SIZE

void MSG_ReadPos (sizebuf_t *msg_read, vec3_t pos)
{
	if ( LegacyProtocol() )
		MSG_ReadPos16 (msg_read, pos);
	else
		MSG_ReadPos24 (msg_read, pos);
}

#else // LARGE_MAP_SIZE

void MSG_ReadPos (sizebuf_t *msg_read, vec3_t pos)
{
	MSG_ReadPos16 (msg_read, pos);
}

#endif // LARGE_MAP_SIZE


float MSG_ReadAngle8 (sizebuf_t *msg_read)
{
	return MSG_ReadChar(msg_read) * (360.0/256);
}

float MSG_ReadAngle16 (sizebuf_t *msg_read)
{
	return SHORT2ANGLE(MSG_ReadShort(msg_read));
}

void MSG_ReadDeltaUsercmd (sizebuf_t *msg_read, usercmd_t *from, usercmd_t *move)
{
	int bits;

	memcpy (move, from, sizeof(*move));

	bits = MSG_ReadByte (msg_read);
		
// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = MSG_ReadShort (msg_read);
	if (bits & CM_ANGLE2)
		move->angles[1] = MSG_ReadShort (msg_read);
	if (bits & CM_ANGLE3)
		move->angles[2] = MSG_ReadShort (msg_read);
		
// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = MSG_ReadShort (msg_read);
	if (bits & CM_SIDE)
		move->sidemove = MSG_ReadShort (msg_read);
	if (bits & CM_UP)
		move->upmove = MSG_ReadShort (msg_read);
	
// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadByte (msg_read);

	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte (msg_read);

// read time to run command
	move->msec = MSG_ReadByte (msg_read);

// read the light level
	move->lightlevel = MSG_ReadByte (msg_read);
}


void MSG_ReadData (sizebuf_t *msg_read, void *data, int len)
{
	int		i;

	for (i=0 ; i<len ; i++)
		((byte *)data)[i] = MSG_ReadByte (msg_read);
}

//===========================================================================

int MSG_PackSolid16 (vec3_t bmins, vec3_t bmaxs)
{
	int 	i, j, k, packed;
	
	// assume that x/y are equal and symetric
	i = bmaxs[0] / 8;
	i = min(max(i, 1), 31);

	// z is not symetric
	j = (-bmins[2]) / 8;
	j = min(max(j, 1), 31);

	// and z maxs can be negative...
	k = (bmaxs[2] + 32) / 8;
	k = min(max(k, 1), 63);

	packed = (k<<10) | (j<<5) | i;
	
	return packed;
}

void MSG_UnpackSolid16 (int packed, vec3_t bmins, vec3_t bmaxs)
{
	int		x, zd, zu;
	
	x = 8 *(packed & 31);
	zd = 8 * ((packed >> 5) & 31);
	zu = 8 * ((packed >> 10) & 63) - 32;

	VectorSet (bmins, -x, -x, -zd);
	VectorSet (bmaxs, x, x, zu);
}

void MSG_WriteBBox8 (sizebuf_t *sb, vec3_t bmins, vec3_t bmaxs)
{
	MSG_WriteByte (sb, (int)(-bmins[0]) & 255);
	MSG_WriteByte (sb, (int)(-bmins[1]) & 255);
	MSG_WriteByte (sb, (int)(-bmins[2]) & 255);
	MSG_WriteByte (sb, (int)bmaxs[0] & 255);
	MSG_WriteByte (sb, (int)bmaxs[1] & 255);
	MSG_WriteByte (sb, (int)(bmaxs[2] + 32.0f) & 255);	// z maxs can go to -32
}

void MSG_WriteBBox16 (sizebuf_t *sb, vec3_t bmins, vec3_t bmaxs)
{
	MSG_WriteFloatAsShort (sb, bmins[0]);
	MSG_WriteFloatAsShort (sb, bmins[1]);
	MSG_WriteFloatAsShort (sb, bmins[2]);
	MSG_WriteFloatAsShort (sb, bmaxs[0]);
	MSG_WriteFloatAsShort (sb, bmaxs[1]);
	MSG_WriteFloatAsShort (sb, bmaxs[2]);
}

void MSG_ReadBBox8 (sizebuf_t *msg_read, vec3_t bmins, vec3_t bmaxs)
{
	bmins[0] = -1.0f * (float)MSG_ReadByte (msg_read);
	bmins[1] = -1.0f * (float)MSG_ReadByte (msg_read);
	bmins[2] = -1.0f * (float)MSG_ReadByte (msg_read);
	bmaxs[0] = (float)MSG_ReadByte (msg_read);
	bmaxs[1] = (float)MSG_ReadByte (msg_read);
	bmaxs[2] = (float)MSG_ReadByte (msg_read) - 32.0f;
}

void MSG_ReadBBox16 (sizebuf_t *msg_read, vec3_t bmins, vec3_t bmaxs)
{
	bmins[0] = MSG_ReadFloatAsShort (msg_read);
	bmins[1] = MSG_ReadFloatAsShort (msg_read);
	bmins[2] = MSG_ReadFloatAsShort (msg_read);
	bmaxs[0] = MSG_ReadFloatAsShort (msg_read);
	bmaxs[1] = MSG_ReadFloatAsShort (msg_read);
	bmaxs[2] = MSG_ReadFloatAsShort (msg_read);
}

//===========================================================================

void SZ_Init (sizebuf_t *buf, byte *data, int length)
{
	memset (buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
	buf->overflowed = false;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Com_Error (ERR_FATAL, "SZ_GetSpace: overflow without allowoverflow set");
		
		if (length > buf->maxsize)
			Com_Error (ERR_FATAL, "SZ_GetSpace: %i is > full buffer size", length);
			
		Com_Printf ("SZ_GetSpace: overflow\n");
		SZ_Clear (buf); 
		buf->overflowed = true;
	}

	void *data = buf->data + buf->cursize;
	buf->cursize += length;
	
	return data;
}

void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	memcpy (SZ_GetSpace(buf,length),data,length);		
}

void SZ_Print (sizebuf_t *buf, const char *data)
{
	int len = ( int ) strlen( data ) + 1;

	if (buf->cursize)
	{
		if (buf->data[buf->cursize-1])
			memcpy ((byte *)SZ_GetSpace(buf, len),data,len); // no trailing 0
		else
			memcpy ((byte *)SZ_GetSpace(buf, len-1)-1,data,len); // write over trailing 0
	}
	else
		memcpy ((byte *)SZ_GetSpace(buf, len),data,len);
}

//============================================================================

/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm ( const char *parm)
{
	int		i;
	
	for (i=1 ; i<com_argc ; i++)
	{
		if (!strcmp (parm,com_argv[i]))
			return i;
	}
		
	return 0;
}

int COM_Argc ()
{
	return com_argc;
}

char *COM_Argv (int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
		return "";
	return com_argv[arg];
}

void COM_ClearArgv (int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
		return;
	com_argv[arg] = "";
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, char **argv)
{
	int		i;

	if (argc > MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "argc > MAX_NUM_ARGVS");
	com_argc = argc;
	for (i=0 ; i<argc ; i++)
	{
		if (!argv[i] || strlen(argv[i]) >= MAX_TOKEN_CHARS )
			com_argv[i] = "";
		else
			com_argv[i] = argv[i];
	}
}

/*
================
COM_AddParm

Adds the given string at the end of the current argument list
================
*/
void COM_AddParm (char *parm)
{
	if (com_argc == MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "COM_AddParm: MAX_NUM)ARGS");
	com_argv[com_argc++] = parm;
}


/// just for debugging
int	memsearch ( const byte *start, int count, int search)
{
	int		i;
	
	for (i=0 ; i<count ; i++)
		if (start[i] == search)
			return i;
	return -1;
}


char *CopyString (const char *in)
{
	size_t outSize = strlen( in ) + 1;
	char  *out     = static_cast< char * >( Z_Malloc( outSize ) );
//	strncpy (out, in);
	Q_strncpyz (out, outSize, in);
	return out;
}


void Info_Print ( const char *s)
{
	char	key[512];

	if (*s == '\\')
		s++;
	while (*s)
	{
		char value[ 512 ];
		char *o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		int l = o - key;
		if (l < 20)
		{
			memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s", key);

		if (!*s)
		{
			Com_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Com_Printf ("%s\n", value);
	}
}

/*
==============================================================================

						ZONE MEMORY ALLOCATION

just cleared malloc with counters now...

==============================================================================
*/

#define	Z_MAGIC		0x1d1d


typedef struct zhead_s
{
	struct zhead_s	*prev, *next;
	short	magic;
	short	tag;			// for group free
	size_t		size;
} zhead_t;

zhead_t		z_chain;
int			z_count;
size_t		z_bytes;

/*
========================
Z_Free
========================
*/
void Z_Free (void *ptr)
{
	zhead_t *z = ( ( zhead_t * ) ptr ) - 1;

	if (z->magic != Z_MAGIC)
		Com_Error (ERR_FATAL, "Z_Free: bad magic");

	z->prev->next = z->next;
	z->next->prev = z->prev;

	z_count--;
	z_bytes -= z->size;
	free (z);
}


/*
========================
Z_Stats_f
========================
*/
void Z_Stats_f ()
{
	Com_Printf ("%i bytes in %i blocks\n", z_bytes, z_count);
}

/*
========================
Z_FreeTags
========================
*/
void Z_FreeTags (int tag)
{
	zhead_t	*z, *next;

	for (z=z_chain.next ; z != &z_chain ; z=next)
	{
		next = z->next;
		if (z->tag == tag)
			Z_Free ((void *)(z+1));
	}
}

/*
========================
Z_TagMalloc
========================
*/
void *Z_TagMalloc (size_t size, int tag)
{
	size = size + sizeof(zhead_t);
	zhead_t *z = static_cast< zhead_t * >( malloc( size ) );
	if (!z)
		Com_Error (ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes",size);
	memset (z, 0, size);
	z_count++;
	z_bytes += size;
	z->magic = Z_MAGIC;
	z->tag = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return (void *)(z+1);
}

/*
========================
Z_Malloc
========================
*/
void *Z_Malloc (size_t size)
{
	return Z_TagMalloc (size, 0);
}

//============================================================================

/*
====================
COM_BlockSequenceCheckByte

For proxy protecting

// THIS IS MASSIVELY BROKEN!  CHALLENGE MAY BE NEGATIVE
// DON'T USE THIS FUNCTION!!!!!

====================
*/
byte COM_BlockSequenceCheckByte (byte *base, int length, int sequence, int challenge)
{
	Sys_Error("COM_BlockSequenceCheckByte called\n");

#if 0
	int		checksum;
	byte	buf[68];
	byte	*p;
	float temp;
	byte c;

	temp = bytedirs[(sequence/3) % NUMVERTEXNORMALS][sequence % 3];
	temp = LittleFloat(temp);
	p = ((byte *)&temp);

	if (length > 60)
		length = 60;
	memcpy (buf, base, length);

	buf[length] = (sequence & 0xff) ^ p[0];
	buf[length+1] = p[1];
	buf[length+2] = ((sequence>>8) & 0xff) ^ p[2];
	buf[length+3] = p[3];

	temp = bytedirs[((sequence+challenge)/3) % NUMVERTEXNORMALS][(sequence+challenge) % 3];
	temp = LittleFloat(temp);
	p = ((byte *)&temp);

	buf[length+4] = (sequence & 0xff) ^ p[3];
	buf[length+5] = (challenge & 0xff) ^ p[2];
	buf[length+6] = ((sequence>>8) & 0xff) ^ p[1];
	buf[length+7] = ((challenge >> 7) & 0xff) ^ p[0];

	length += 8;

	checksum = LittleLong(Com_BlockChecksum (buf, length));

	checksum &= 0xff;

	return checksum;
#endif
	return 0;
}

static byte chktbl[1024] = {
0x84, 0x47, 0x51, 0xc1, 0x93, 0x22, 0x21, 0x24, 0x2f, 0x66, 0x60, 0x4d, 0xb0, 0x7c, 0xda,
0x88, 0x54, 0x15, 0x2b, 0xc6, 0x6c, 0x89, 0xc5, 0x9d, 0x48, 0xee, 0xe6, 0x8a, 0xb5, 0xf4,
0xcb, 0xfb, 0xf1, 0x0c, 0x2e, 0xa0, 0xd7, 0xc9, 0x1f, 0xd6, 0x06, 0x9a, 0x09, 0x41, 0x54,
0x67, 0x46, 0xc7, 0x74, 0xe3, 0xc8, 0xb6, 0x5d, 0xa6, 0x36, 0xc4, 0xab, 0x2c, 0x7e, 0x85,
0xa8, 0xa4, 0xa6, 0x4d, 0x96, 0x19, 0x19, 0x9a, 0xcc, 0xd8, 0xac, 0x39, 0x5e, 0x3c, 0xf2,
0xf5, 0x5a, 0x72, 0xe5, 0xa9, 0xd1, 0xb3, 0x23, 0x82, 0x6f, 0x29, 0xcb, 0xd1, 0xcc, 0x71,
0xfb, 0xea, 0x92, 0xeb, 0x1c, 0xca, 0x4c, 0x70, 0xfe, 0x4d, 0xc9, 0x67, 0x43, 0x47, 0x94,
0xb9, 0x47, 0xbc, 0x3f, 0x01, 0xab, 0x7b, 0xa6, 0xe2, 0x76, 0xef, 0x5a, 0x7a, 0x29, 0x0b,
0x51, 0x54, 0x67, 0xd8, 0x1c, 0x14, 0x3e, 0x29, 0xec, 0xe9, 0x2d, 0x48, 0x67, 0xff, 0xed,
0x54, 0x4f, 0x48, 0xc0, 0xaa, 0x61, 0xf7, 0x78, 0x12, 0x03, 0x7a, 0x9e, 0x8b, 0xcf, 0x83,
0x7b, 0xae, 0xca, 0x7b, 0xd9, 0xe9, 0x53, 0x2a, 0xeb, 0xd2, 0xd8, 0xcd, 0xa3, 0x10, 0x25,
0x78, 0x5a, 0xb5, 0x23, 0x06, 0x93, 0xb7, 0x84, 0xd2, 0xbd, 0x96, 0x75, 0xa5, 0x5e, 0xcf,
0x4e, 0xe9, 0x50, 0xa1, 0xe6, 0x9d, 0xb1, 0xe3, 0x85, 0x66, 0x28, 0x4e, 0x43, 0xdc, 0x6e,
0xbb, 0x33, 0x9e, 0xf3, 0x0d, 0x00, 0xc1, 0xcf, 0x67, 0x34, 0x06, 0x7c, 0x71, 0xe3, 0x63,
0xb7, 0xb7, 0xdf, 0x92, 0xc4, 0xc2, 0x25, 0x5c, 0xff, 0xc3, 0x6e, 0xfc, 0xaa, 0x1e, 0x2a,
0x48, 0x11, 0x1c, 0x36, 0x68, 0x78, 0x86, 0x79, 0x30, 0xc3, 0xd6, 0xde, 0xbc, 0x3a, 0x2a,
0x6d, 0x1e, 0x46, 0xdd, 0xe0, 0x80, 0x1e, 0x44, 0x3b, 0x6f, 0xaf, 0x31, 0xda, 0xa2, 0xbd,
0x77, 0x06, 0x56, 0xc0, 0xb7, 0x92, 0x4b, 0x37, 0xc0, 0xfc, 0xc2, 0xd5, 0xfb, 0xa8, 0xda,
0xf5, 0x57, 0xa8, 0x18, 0xc0, 0xdf, 0xe7, 0xaa, 0x2a, 0xe0, 0x7c, 0x6f, 0x77, 0xb1, 0x26,
0xba, 0xf9, 0x2e, 0x1d, 0x16, 0xcb, 0xb8, 0xa2, 0x44, 0xd5, 0x2f, 0x1a, 0x79, 0x74, 0x87,
0x4b, 0x00, 0xc9, 0x4a, 0x3a, 0x65, 0x8f, 0xe6, 0x5d, 0xe5, 0x0a, 0x77, 0xd8, 0x1a, 0x14,
0x41, 0x75, 0xb1, 0xe2, 0x50, 0x2c, 0x93, 0x38, 0x2b, 0x6d, 0xf3, 0xf6, 0xdb, 0x1f, 0xcd,
0xff, 0x14, 0x70, 0xe7, 0x16, 0xe8, 0x3d, 0xf0, 0xe3, 0xbc, 0x5e, 0xb6, 0x3f, 0xcc, 0x81,
0x24, 0x67, 0xf3, 0x97, 0x3b, 0xfe, 0x3a, 0x96, 0x85, 0xdf, 0xe4, 0x6e, 0x3c, 0x85, 0x05,
0x0e, 0xa3, 0x2b, 0x07, 0xc8, 0xbf, 0xe5, 0x13, 0x82, 0x62, 0x08, 0x61, 0x69, 0x4b, 0x47,
0x62, 0x73, 0x44, 0x64, 0x8e, 0xe2, 0x91, 0xa6, 0x9a, 0xb7, 0xe9, 0x04, 0xb6, 0x54, 0x0c,
0xc5, 0xa9, 0x47, 0xa6, 0xc9, 0x08, 0xfe, 0x4e, 0xa6, 0xcc, 0x8a, 0x5b, 0x90, 0x6f, 0x2b,
0x3f, 0xb6, 0x0a, 0x96, 0xc0, 0x78, 0x58, 0x3c, 0x76, 0x6d, 0x94, 0x1a, 0xe4, 0x4e, 0xb8,
0x38, 0xbb, 0xf5, 0xeb, 0x29, 0xd8, 0xb0, 0xf3, 0x15, 0x1e, 0x99, 0x96, 0x3c, 0x5d, 0x63,
0xd5, 0xb1, 0xad, 0x52, 0xb8, 0x55, 0x70, 0x75, 0x3e, 0x1a, 0xd5, 0xda, 0xf6, 0x7a, 0x48,
0x7d, 0x44, 0x41, 0xf9, 0x11, 0xce, 0xd7, 0xca, 0xa5, 0x3d, 0x7a, 0x79, 0x7e, 0x7d, 0x25,
0x1b, 0x77, 0xbc, 0xf7, 0xc7, 0x0f, 0x84, 0x95, 0x10, 0x92, 0x67, 0x15, 0x11, 0x5a, 0x5e,
0x41, 0x66, 0x0f, 0x38, 0x03, 0xb2, 0xf1, 0x5d, 0xf8, 0xab, 0xc0, 0x02, 0x76, 0x84, 0x28,
0xf4, 0x9d, 0x56, 0x46, 0x60, 0x20, 0xdb, 0x68, 0xa7, 0xbb, 0xee, 0xac, 0x15, 0x01, 0x2f,
0x20, 0x09, 0xdb, 0xc0, 0x16, 0xa1, 0x89, 0xf9, 0x94, 0x59, 0x00, 0xc1, 0x76, 0xbf, 0xc1,
0x4d, 0x5d, 0x2d, 0xa9, 0x85, 0x2c, 0xd6, 0xd3, 0x14, 0xcc, 0x02, 0xc3, 0xc2, 0xfa, 0x6b,
0xb7, 0xa6, 0xef, 0xdd, 0x12, 0x26, 0xa4, 0x63, 0xe3, 0x62, 0xbd, 0x56, 0x8a, 0x52, 0x2b,
0xb9, 0xdf, 0x09, 0xbc, 0x0e, 0x97, 0xa9, 0xb0, 0x82, 0x46, 0x08, 0xd5, 0x1a, 0x8e, 0x1b,
0xa7, 0x90, 0x98, 0xb9, 0xbb, 0x3c, 0x17, 0x9a, 0xf2, 0x82, 0xba, 0x64, 0x0a, 0x7f, 0xca,
0x5a, 0x8c, 0x7c, 0xd3, 0x79, 0x09, 0x5b, 0x26, 0xbb, 0xbd, 0x25, 0xdf, 0x3d, 0x6f, 0x9a,
0x8f, 0xee, 0x21, 0x66, 0xb0, 0x8d, 0x84, 0x4c, 0x91, 0x45, 0xd4, 0x77, 0x4f, 0xb3, 0x8c,
0xbc, 0xa8, 0x99, 0xaa, 0x19, 0x53, 0x7c, 0x02, 0x87, 0xbb, 0x0b, 0x7c, 0x1a, 0x2d, 0xdf,
0x48, 0x44, 0x06, 0xd6, 0x7d, 0x0c, 0x2d, 0x35, 0x76, 0xae, 0xc4, 0x5f, 0x71, 0x85, 0x97,
0xc4, 0x3d, 0xef, 0x52, 0xbe, 0x00, 0xe4, 0xcd, 0x49, 0xd1, 0xd1, 0x1c, 0x3c, 0xd0, 0x1c,
0x42, 0xaf, 0xd4, 0xbd, 0x58, 0x34, 0x07, 0x32, 0xee, 0xb9, 0xb5, 0xea, 0xff, 0xd7, 0x8c,
0x0d, 0x2e, 0x2f, 0xaf, 0x87, 0xbb, 0xe6, 0x52, 0x71, 0x22, 0xf5, 0x25, 0x17, 0xa1, 0x82,
0x04, 0xc2, 0x4a, 0xbd, 0x57, 0xc6, 0xab, 0xc8, 0x35, 0x0c, 0x3c, 0xd9, 0xc2, 0x43, 0xdb,
0x27, 0x92, 0xcf, 0xb8, 0x25, 0x60, 0xfa, 0x21, 0x3b, 0x04, 0x52, 0xc8, 0x96, 0xba, 0x74,
0xe3, 0x67, 0x3e, 0x8e, 0x8d, 0x61, 0x90, 0x92, 0x59, 0xb6, 0x1a, 0x1c, 0x5e, 0x21, 0xc1,
0x65, 0xe5, 0xa6, 0x34, 0x05, 0x6f, 0xc5, 0x60, 0xb1, 0x83, 0xc1, 0xd5, 0xd5, 0xed, 0xd9,
0xc7, 0x11, 0x7b, 0x49, 0x7a, 0xf9, 0xf9, 0x84, 0x47, 0x9b, 0xe2, 0xa5, 0x82, 0xe0, 0xc2,
0x88, 0xd0, 0xb2, 0x58, 0x88, 0x7f, 0x45, 0x09, 0x67, 0x74, 0x61, 0xbf, 0xe6, 0x40, 0xe2,
0x9d, 0xc2, 0x47, 0x05, 0x89, 0xed, 0xcb, 0xbb, 0xb7, 0x27, 0xe7, 0xdc, 0x7a, 0xfd, 0xbf,
0xa8, 0xd0, 0xaa, 0x10, 0x39, 0x3c, 0x20, 0xf0, 0xd3, 0x6e, 0xb1, 0x72, 0xf8, 0xe6, 0x0f,
0xef, 0x37, 0xe5, 0x09, 0x33, 0x5a, 0x83, 0x43, 0x80, 0x4f, 0x65, 0x2f, 0x7c, 0x8c, 0x6a,
0xa0, 0x82, 0x0c, 0xd4, 0xd4, 0xfa, 0x81, 0x60, 0x3d, 0xdf, 0x06, 0xf1, 0x5f, 0x08, 0x0d,
0x6d, 0x43, 0xf2, 0xe3, 0x11, 0x7d, 0x80, 0x32, 0xc5, 0xfb, 0xc5, 0xd9, 0x27, 0xec, 0xc6,
0x4e, 0x65, 0x27, 0x76, 0x87, 0xa6, 0xee, 0xee, 0xd7, 0x8b, 0xd1, 0xa0, 0x5c, 0xb0, 0x42,
0x13, 0x0e, 0x95, 0x4a, 0xf2, 0x06, 0xc6, 0x43, 0x33, 0xf4, 0xc7, 0xf8, 0xe7, 0x1f, 0xdd,
0xe4, 0x46, 0x4a, 0x70, 0x39, 0x6c, 0xd0, 0xed, 0xca, 0xbe, 0x60, 0x3b, 0xd1, 0x7b, 0x57,
0x48, 0xe5, 0x3a, 0x79, 0xc1, 0x69, 0x33, 0x53, 0x1b, 0x80, 0xb8, 0x91, 0x7d, 0xb4, 0xf6,
0x17, 0x1a, 0x1d, 0x5a, 0x32, 0xd6, 0xcc, 0x71, 0x29, 0x3f, 0x28, 0xbb, 0xf3, 0x5e, 0x71,
0xb8, 0x43, 0xaf, 0xf8, 0xb9, 0x64, 0xef, 0xc4, 0xa5, 0x6c, 0x08, 0x53, 0xc7, 0x00, 0x10,
0x39, 0x4f, 0xdd, 0xe4, 0xb6, 0x19, 0x27, 0xfb, 0xb8, 0xf5, 0x32, 0x73, 0xe5, 0xcb, 0x32
};

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
byte COM_BlockSequenceCRCByte ( const byte *base, int length, int sequence)
{
	int		n;
	byte	*p;
	int		x;
	byte chkb[60 + 4];
	unsigned short crc;


	if (sequence < 0)
		Sys_Error("sequence < 0, this shouldn't happen\n");

	p = chktbl + (sequence % (sizeof(chktbl) - 4));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = CRC_Block(chkb, length);

	for (x=0, n=0; n<length; n++)
		x += chkb[n];

	crc = (crc ^ x) & 0xff;

	return crc;
}

//========================================================

float	frand()
{
	return (rand()&32767)* (1.0/32767);
}

float	crand()
{
	return (rand()&32767)* (2.0/32767) - 1;
}

void Key_Init ();
void SCR_EndLoadingPlaque ();

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
void Com_Error_f ()
{
	Com_Error (ERR_FATAL, "%s", Cmd_Argv(1));
}


/*
=================
Qcommon_Init
=================
*/
void Qcommon_Init (int argc, char **argv)
{
	char	*s;

	if (setjmp (abortframe) )
		Sys_Error ("Error during initialization");

	z_chain.next = z_chain.prev = &z_chain;

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	COM_InitArgv (argc, argv);

	Swap_Init ();
	Cbuf_Init ();

	Cmd_Init ();
	Cvar_Init ();

	Key_Init ();

	// we need to add the early commands twice, because
	// a basedir or cddir needs to be set before execing
	// config files, but we want other parms to override
	// the settings of the config files
	Cbuf_AddEarlyCommands (false);
	Cbuf_Execute ();

	FS_InitFilesystem ();

//	Cbuf_AddText ("exec default.cfg\n");
//	Cbuf_AddText ("exec kmq2config.cfg\n");
	// Knightmare- use encapsulated function
	FS_ExecConfigs (false);

	Cbuf_AddEarlyCommands (true);
	Cbuf_Execute ();

	//
	// init commands and vars
	//
    Cmd_AddCommand ( "z_stats", Z_Stats_f );
    Cmd_AddCommand ( "error", Com_Error_f );

	host_speeds = Cvar_Get ( "host_speeds", "0", 0 );
	Cvar_SetDescription ( "host_speeds", "Enables output of per-frame time elapsed in ms for server/game/client/renderer." );
	log_stats = Cvar_Get ( "log_stats", "0", 0 );
	Cvar_SetDescription ( "log_stats", "Enables output of entities, dlights, particles, and frame time for each frame to stats.log." );
	developer = Cvar_Get ( "developer", "0", 0 );
	Cvar_SetDescription ( "developer", "Enables developer messages in console.  Values > 1 will enable more verbose outputs, such as for image loading." );

	timescale = Cvar_Get ( "timescale", "1", CVAR_CHEAT );
	Cvar_SetDescription ( "timescale", "Timescaling feature.  Values lower than 1 slow down the game, while higher values speed it up.  This is considered a cheat in multiplayer." );
	fixedtime = Cvar_Get ( "fixedtime", "0", CVAR_CHEAT );
	Cvar_SetDescription ( "fixedtime", "Fixed frametime feature.  Non-zero values set time in ms for each common frame.  This is considered a cheat in multiplayer." );

	logfile_active = Cvar_Get ( "logfile", "0", 0 );
	Cvar_SetDescription ( "logfile", "Enables logging of console to qconsole.log.  Values > 1 cause writes on every console ouput." );
	showtrace = Cvar_Get ( "showtrace", "0", 0 );
	Cvar_SetDescription ( "showtrace", "Toggles output of per-frame trace operation counts to console." );
	con_show_description = Cvar_Get ( "con_show_description", "1", CVAR_ARCHIVE );	// Knightmare added
	Cvar_SetDescription ( "con_show_description", "Toggles output of descriptions for cvars.  This cvar will always show its description." );

#ifdef DEDICATED_ONLY
	dedicated = Cvar_Get ("dedicated", "1", CVAR_NOSET);
#else
	dedicated = Cvar_Get ( "dedicated", "0", CVAR_NOSET );
#endif
	Cvar_SetDescription ( "dedicated", "Toggles dedicated server.  Can only be set from the command line or the start server menu." );

	// Knightmare- for the game DLL to tell what engine it's running under
	sv_engine = Cvar_Get ( "sv_engine", "KMQuake2", CVAR_SERVERINFO | CVAR_NOSET | CVAR_LATCH );
	Cvar_SetDescription ( "sv_engine", "Identifies the server engine." );
	sv_engine_version = Cvar_Get ( "sv_engine_version", va( "%4.2f", VERSION ), CVAR_SERVERINFO | CVAR_NOSET | CVAR_LATCH );
	Cvar_SetDescription ( "sv_engine_version", "Identifies the server engine version." );
	// end Knightmare
	
	s = va("KMQ2 %4.2fu%d %s %s %s %s", VERSION, VERSION_UPDATE, CPUSTRING, OS_STRING, COMPILETYPE_STRING, __DATE__);
	Cvar_Get ( "version", s, CVAR_SERVERINFO | CVAR_NOSET );

//	if (dedicated->value)
	if (dedicated->integer)
		Cmd_AddCommand ( "quit", Com_Quit );

	dedicated->modified = false;	// make sure this starts false

	Sys_Init ();

	NET_Init ();
	Netchan_Init ();

	SV_Init ();
	CL_Init ();

#ifdef _WIN32  // Knightmare- remove startup logo, code from TomazQuake
	if (!dedicated->integer)
		Sys_ShowConsole (false);
#endif
/*
#if defined(__APPLE__) || (MACOSX)
	// Hide console
	if (!dedicated->integer)
		Sys_ShowConsole (false);
#endif // defined(__APPLE__) || (MACOSX)
*/
	// add + commands from command line
	if (!Cbuf_AddLateCommands ())
	{	// if the user didn't give any commands, run default action
		if (!dedicated->integer)
			Cbuf_AddText ("d1\n");
		else
			Cbuf_AddText ("dedicated_start\n");
		Cbuf_Execute ();
	}
	else
	{	// the user asked for something explicit
		// so drop the loading plaque
		SCR_EndLoadingPlaque ();
	}

	Com_Printf ("====== KMQuake2 Initialized ======\n\n");
	
	// testing crap
	/*{
		vec3_t	in[3], out[3], angles, axis[3];

		VectorSet (in[0], 1280, 0, 0);
		VectorSet (in[1], 0, 640, 0);
		VectorSet (in[2], 0, 0, -640);
		VectorSet (angles, -104, 108, 60);
		AnglesToAxis (angles, axis);
		VectorRotate (in[0], axis, out[0]);
		VectorRotate (in[1], axis, out[1]);
		VectorRotate (in[2], axis, out[2]);
		Com_Printf ("Target vector: %f %f %f\nRight vector: %f %f %f\nUp vector: %f %f %f\n",
			out[0][0], out[0][1], out[0][2],
			out[1][0], out[1][1], out[1][2],
			out[2][0], out[2][1], out[2][2]);
	}*/
}

/*
=================
Qcommon_Frame
=================
*/
void Qcommon_Frame (int msec)
{
	char	*s;
	int		time_before, time_between, time_after;

	if (setjmp (abortframe) )
		return;			// an ERR_DROP was thrown

	if ( log_stats->modified )
	{
		log_stats->modified = false;
		if ( log_stats->integer )
		{
			char	name[MAX_OSPATH];

			if ( log_stats_file )
			{
				fclose( log_stats_file );
				log_stats_file = 0;
			}
			// Knightmare- write stats.log in fs_savegamedir instead of game root
		//	log_stats_file = fopen( "stats.log", "w" );
			snprintf (name, sizeof(name), "%s/stats.log", FS_Savegamedir());
			log_stats_file = fopen( name, "w" );
			if ( log_stats_file )
				fprintf( log_stats_file, "entities,dlights,parts,frame time\n" );
		}
		else
		{
			if ( log_stats_file )
			{
				fclose( log_stats_file );
				log_stats_file = 0;
			}
		}
	}

//	if (fixedtime->value)
//		msec = fixedtime->value;
	if (fixedtime->integer)
		msec = abs(fixedtime->integer);
	else if (timescale->value)
	{
		msec *= timescale->value;
		if (msec < 1)
			msec = 1;
	}

//	if (showtrace->value)
	if (showtrace->integer)
	{
		extern	int c_traces, c_brush_traces;
		extern	int	c_pointcontents;

		Com_Printf ("%4i traces  %4i points\n", c_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_pointcontents = 0;
	}

	do
	{
		s = Sys_ConsoleInput ();
		if (s)
			Cbuf_AddText (va("%s\n",s));
	} while (s);

	Cbuf_Execute ();

//	if (host_speeds->value)
	if (host_speeds->integer)
		time_before = Sys_Milliseconds ();

	SV_Frame (msec);

	// switch to/from dedicated here
	if (dedicated->modified)
	{
	//	Com_Printf ("dedicated is %f\n", dedicated->value);
		dedicated->modified = false;
		if (!dedicated->integer)
		{
			// remove server quit command, to be replaced with client quit command
			Cmd_RemoveCommand ( "quit" );

			CL_Init ();
			Sys_ShowConsole (false);
		}
		else
		{
			CL_Shutdown ();

			// the above function call removes client quit command, replace it with server quit command
			Cmd_AddCommand ( "quit", Com_Quit );

			Sys_ShowConsole (true);
		}
	}

//	if (host_speeds->value)
	if (host_speeds->integer)
		time_between = Sys_Milliseconds ();		

	CL_Frame (msec);

//	if (host_speeds->value)
	if (host_speeds->integer)
		time_after = Sys_Milliseconds ();		


//	if (host_speeds->value)
	if (host_speeds->integer)
	{
		int			all, sv, gm, cl, rf;

		all = time_after - time_before;
		sv = time_between - time_before;
		cl = time_after - time_between;
		gm = time_after_game - time_before_game;
		rf = time_after_ref - time_before_ref;
		sv -= gm;
		cl -= rf;
		Com_Printf ("all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n",
			all, sv, gm, cl, rf);
	}	
}

/*
=================
Qcommon_Shutdown
=================
*/
void Qcommon_Shutdown (void)
{	
	FS_Shutdown ();
}

//============================================================================

/*
==================
Com_ParseSteamLibraryFolders
==================
*/
char *Com_ParseSteamLibraryFolders (const char *fileContents, size_t contentsLen, const char *relPath, const char *appID)
{
	int			libraryNum = 0;
	char		*s = NULL, *token = NULL;
	char		*key = NULL, *value = NULL;
	char		keySave[128];
	static char	libraryPath[MAX_OSPATH];
	static char	gamePath[MAX_OSPATH];
	qboolean	foundPath = false;

	if ( !fileContents || (fileContents[0] == 0) || !relPath || (relPath[0] == 0) || !appID || (appID[0] == 0) )
		return NULL;

	gamePath[0] = 0;
	s = (char *)fileContents;

	if (strcmp(COM_ParseExt(&s, true), "libraryfolders") != 0)
		return NULL;
	if (strcmp(COM_ParseExt(&s, true), "{") != 0)
		return NULL;

	while (s < (fileContents + contentsLen))
	{
		// get library number, check if at end of file
		token = COM_ParseExt(&s, true);
		if ( !token || (token[0] == 0) || !strcmp(token, "}") )
			break;
		libraryNum = atoi(token);

		// get opening brace for this library section
		token = COM_ParseExt(&s, true);
		if ( !token || (token[0] == 0) || (strcmp(token, "{") != 0) )
			break;

	//	Com_Printf ("Com_ParseSteamLibraryFolders: Parsing library VDF (%i)...\n", libraryNum);

		// parse key/value pairs for this library
		while (s < (fileContents + contentsLen))
		{
			key = COM_ParseExt(&s, true);
			if ( !key || (key[0] == 0) || !strcmp(key, "}") ) {
				break;
			}
			else if ( !strcmp(key, "path") ) {
				value = COM_ParseExt(&s, true);
				Q_strncpyz (libraryPath, sizeof(libraryPath), value);
			//	Com_Printf ("Com_ParseSteamLibraryFolders: found library path of %s\n", libraryPath);
			}
			else if ( !strcmp(key, "apps") )
			{
			//	Com_Printf ("Com_ParseSteamLibraryFolders: Parsing apps list for library path %s...\n", libraryPath);
				// get opening brace for this apps section
				token = COM_ParseExt(&s, true);
				if ( !token || (token[0] == 0) || (strcmp(token, "{") != 0) )
					break;

				// parse key/value pairs for this apps section
				while (s < (fileContents + contentsLen))
				{
					key = COM_ParseExt(&s, true);
					if ( !key || (key[0] == 0) || !strcmp(key, "}") )
						break;

					Q_strncpyz (keySave, sizeof(keySave), key);
					value = COM_ParseExt(&s, true);

					if ( !strcmp(keySave, appID) ) {
						Q_strncpyz (gamePath, sizeof(gamePath), libraryPath);
						Q_strncatz (gamePath, sizeof(gamePath), relPath);
						foundPath = true;
					//	Com_Printf ("Com_ParseSteamLibraryFolders: found matching appID\n");
					}

					if ( foundPath && (gamePath[0] != 0) )
						break;
				}
			}
			else {
				// just grab the value for this key pair
				value = COM_ParseExt(&s, true);
			}

			if ( foundPath && (gamePath[0] != 0) )
				break;
		}

		if ( foundPath && (gamePath[0] != 0) ) {
		//	Com_Printf ("Com_ParseSteamLibraryFolders: Found game path %s...\n", gamePath);
			break;
		}
	}

	if (gamePath[0] != 0)
		return gamePath;
	else
		return NULL;
}

//============================================================================

/*
=================
StripHighBits

String parsing function from r1q2
=================
*/
void StripHighBits (char *string, int highbits)
{
	byte		high;
	byte		c;
	char		*p;

	p = string;

	if (highbits)
		high = 127;
	else
		high = 255;

	while (string[0])
	{
		c = *(string++);

		if (c >= 32 && c <= high)
			*p++ = c;
	}

	p[0] = '\0';
}


/*
=================
IsValidChar

Security function from r1q2
=================
*/
qboolean IsValidChar (int c)
{
	if (!isalnum(c) && c != '_' && c != '-')
		return false;
	return true;
}

/*
=================
ExpandNewLines

String parsing function from r1q2
=================
*/
void ExpandNewLines (char *string)
{
	char *q = string;
	char *s = q;

	if (!string[0])
		return;

	while (*(q+1))
	{
		if (*q == '\\' && *(q+1) == 'n')
		{
			*s++ = '\n';
			q++;
		}
		else
		{
			*s++ = *q;
		}
		q++;

		//crashfix, check if we reached eol on an expansion.
		if (!*q)
			break;
	}

	if (*q)
		*s++ = *q;
	*s = '\0';
}

/*
=================
StripQuotes

String parsing function from r1q2
=================
*/
char *StripQuotes (char *string)
{
	size_t	i;

	if (!string[0])
		return string;

	i = strlen(string);

	if (string[0] == '"' && string[i-1] == '"')
	{
		string[i-1] = 0;
		return string + 1;
	}

	return string;
}

/*
=================
MakePrintable

String parsing function from r1q2
=================
*/
const char *MakePrintable (const void *subject, size_t numchars)
{
	int			len;
	static char printable[4096];
	char		tmp[8];
	char		*p;
	const byte	*s;

	if (!subject)
	{
	//	strncpy (printable, "(null)");
		Q_strncpyz (printable, sizeof(printable), "(null)");
		return printable;
	}

	s = (const byte *)subject;
	p = printable;
	len = 0;

	if (!numchars)
		numchars = strlen((const char *) s);

	while (numchars--)
	{
		if (isprint(s[0]))
		{
			*p++ = s[0];
			len++;
		}
		else
		{
			snprintf (tmp, sizeof(tmp), "%.3d", s[0]);
			*p++ = '\\';
			*p++ = tmp[0];
			*p++ = tmp[1];
			*p++ = tmp[2];
			len += 4;
		}

		if (len >= sizeof(printable)-5)
			break;

		s++;
	}

	printable[len] = 0;
	return printable;
}
