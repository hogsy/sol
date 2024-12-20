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

// sv_user.c -- server code for moving users

#include "server.h"

edict_t	*sv_player;

/*
============================================================

USER STRINGCMD EXECUTION

sv_client and sv_player will be valid.
============================================================
*/

/*
==================
SV_BeginDemoServer
==================
*/
void SV_BeginDemoserver (void)
{
	char		name[MAX_OSPATH];

	snprintf (name, sizeof(name), "demos/%s", sv.name);
	FS_FOpenFile (name, &sv.demofile, FS_READ);
	if (!sv.demofile)
		Com_Error (ERR_DROP, "Couldn't open %s\n", name);
}

/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_New_f (void)
{
	char		*gamedir;
	int			playernum;
	edict_t		*ent;

	Com_DPrintf ("New() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("New not valid -- already spawned\n");
		return;
	}

	// demo servers just dump the file message
	if (sv.state == ss_demo)
	{
		SV_BeginDemoserver ();
		return;
	}

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	gamedir = Cvar_VariableString ("gamedir");

	// send the serverdata
	MSG_WriteByte (&sv_client->netchan.message, svc_serverdata);
	MSG_WriteLong (&sv_client->netchan.message, PROTOCOL_VERSION);
	MSG_WriteLong (&sv_client->netchan.message, svs.spawncount);
	MSG_WriteByte (&sv_client->netchan.message, sv.attractloop);
	MSG_WriteString (&sv_client->netchan.message, gamedir);

	if (sv.state == ss_cinematic || sv.state == ss_pic)
		playernum = -1;
	else
		playernum = sv_client - svs.clients;
	MSG_WriteShort (&sv_client->netchan.message, playernum);

	// send full levelname
	MSG_WriteString (&sv_client->netchan.message, sv.configstrings[CS_NAME]);

	//
	// game server
	// 
	if (sv.state == ss_game)
	{
		// set up the entity for the client
		ent = EDICT_NUM(playernum+1);
		ent->s.number = playernum+1;
		sv_client->edict = ent;
		memset (&sv_client->lastcmd, 0, sizeof(sv_client->lastcmd));

		// begin fetching configstrings
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd configstrings %i 0\n",svs.spawncount) );
	}

}

// Knightmare added
/*
==================
SV_SetMaxBaselinesSize
==================
*/
int SV_SetMaxBaselinesSize (void)
{
	//	bounds check sv_baselines_maxlen
//	if (sv_baselines_maxlen->value < 400)
	if (sv_baselines_maxlen->integer < 400)
	{
		Com_Printf (S_COLOR_YELLOW"WARNING: sv_baselines_maxlen is less than 400!  Setting to default value of 1200.\n");
		Cvar_Set ("sv_baselines_maxlen", "1200");
	}
//	if (sv_baselines_maxlen->value > MAX_MSGLEN)
	if (sv_baselines_maxlen->integer > MAX_MSGLEN)
	{
		Com_Printf (S_COLOR_YELLOW"WARNING: sv_baselines_maxlen is larger than MAX_MSGLEN of %i!  Setting to default value of 1200.\n", MAX_MSGLEN);
		Cvar_Set ("sv_baselines_maxlen", "1200");
	}
//	if (sv_baselines_maxlen->value > 1400)
	if (sv_baselines_maxlen->integer > 1400)
	{
		Com_Printf (S_COLOR_YELLOW"WARNING: sv_baselines_maxlen is larger than 1400!  Setting to default value of 1200.\n");
		Cvar_Set ("sv_baselines_maxlen", "1200");
	}

	// use MAX_MSGLEN/2 for SP and local clients
//	if ( (sv_client->netchan.remote_address.type == NA_LOOPBACK) || (maxclients->value == 1) )
	if ( (sv_client->netchan.remote_address.type == NA_LOOPBACK) || (maxclients->integer == 1) )
		return MAX_MSGLEN/2;
	else
		return sv_baselines_maxlen->integer;
}

/*
==================
SV_Configstrings_f
==================
*/
void SV_Configstrings_f (void)
{
	int			startPos, start;
	int		maxLen;		// Knightmare added

	Com_DPrintf ("Configstrings() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("configstrings not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Configstrings_f from different level\n");
		SV_New_f ();
		return;
	}

	//	Knightmare- use sv_baselines_maxlen for proper bounding in multiplayer
	maxLen = SV_SetMaxBaselinesSize();

//	start = atoi(Cmd_Argv(2));
	startPos = atoi(Cmd_Argv(2));
	if (startPos < 0) // r1ch's fix for negative index
	{
		Com_Printf ("Illegal configstrings request (negative index) from %s[%s], dropping client\n", sv_client->name, NET_AdrToString(sv_client->netchan.remote_address));
		SV_DropClient (sv_client);
		return;
	}
	start = startPos;

	// write a packet full of data
	//	Knightmare- use maxLen for proper bounding
//	while ( sv_client->netchan.message.cursize < MAX_MSGLEN/2 && start < MAX_CONFIGSTRINGS)
	while ( sv_client->netchan.message.cursize < maxLen && start < MAX_CONFIGSTRINGS)
	{
		if (sv.configstrings[start][0])
		{
			MSG_WriteByte (&sv_client->netchan.message, svc_configstring);
			MSG_WriteShort (&sv_client->netchan.message, start);
			MSG_WriteString (&sv_client->netchan.message, sv.configstrings[start]);
		}
		start++;
	}

	// send next command

	if (start == MAX_CONFIGSTRINGS)
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd baselines %i 0\n",svs.spawncount) );
	}
	else
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd configstrings %i %i\n",svs.spawncount, start) );
	}
}

/*
==================
SV_Baselines_f
==================
*/
void SV_Baselines_f (void)
{
	int				startPos, start;
	int				maxLen;	// Knightmare added
	centity_state_t	nullstate;
	centity_state_t	*base;

	Com_DPrintf ("Baselines() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("baselines not valid -- already spawned\n");
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Baselines_f from different level\n");
		SV_New_f ();
		return;
	}

	//	Knightmare- use sv_baselines_maxlen for proper bounding in multiplayer
	maxLen = SV_SetMaxBaselinesSize();

//	start = atoi(Cmd_Argv(2));
	startPos = atoi(Cmd_Argv(2));
	if (startPos < 0) // r1ch's fix for negative index
	{
		Com_Printf ("Illegal baselines request (negative index) from %s[%s], dropping client\n", sv_client->name, NET_AdrToString(sv_client->netchan.remote_address));
		SV_DropClient (sv_client);
		return;
	}
	start = startPos;

	memset (&nullstate, 0, sizeof(nullstate));

	// write a packet full of data
	//	Knightmare- use maxLen for proper bounding
//	while ( sv_client->netchan.message.cursize <  MAX_MSGLEN/2 && start < MAX_EDICTS)
	while ( sv_client->netchan.message.cursize < maxLen && start < MAX_EDICTS)
	{
		base = &sv.baselines[start];
		if (base->modelindex || base->sound || base->effects)
		{
			MSG_WriteByte (&sv_client->netchan.message, svc_spawnbaseline);
			MSG_WriteDeltaEntity (&nullstate, base, &sv_client->netchan.message, true, true);
		}
		start++;
	}

	// send next command

	if (start == MAX_EDICTS)
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("precache %i\n", svs.spawncount) );
	}
	else
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd baselines %i %i\n",svs.spawncount, start) );
	}
}

/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f (void)
{
	Com_DPrintf ("Begin() from %s\n", sv_client->name);

	// r1ch: could be abused to respawn or cause spam/other mod specific problems
	if (sv_client->state != cs_connected)
	{
		Com_Printf ("EXPLOIT: Illegal 'begin' from %s[%s] (already spawned), client dropped.\n", sv_client->name, NET_AdrToString (sv_client->netchan.remote_address));
		SV_DropClient (sv_client);
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Begin_f from different level\n");
		SV_New_f ();
		return;
	}

	sv_client->state = cs_spawned;
	
	// call the game begin function
	ge->ClientBegin (sv_player);

	// Knightmare- set default player speeds here, if
	// the game DLL hasn't already set them
#ifdef NEW_PLAYER_STATE_MEMBERS
	if (!sv_player->client->ps.maxspeed)
		sv_player->client->ps.maxspeed = DEFAULT_MAXSPEED;
	if (!sv_player->client->ps.duckspeed)
		sv_player->client->ps.duckspeed = DEFAULT_DUCKSPEED;
	if (!sv_player->client->ps.waterspeed)
		sv_player->client->ps.waterspeed = DEFAULT_WATERSPEED;
	if (!sv_player->client->ps.accel)
		sv_player->client->ps.accel = DEFAULT_ACCELERATE;
	if (!sv_player->client->ps.stopspeed)
		sv_player->client->ps.stopspeed = DEFAULT_STOPSPEED;
#endif
	// end Knightmare

	Cbuf_InsertFromDefer ();
}

//=============================================================================

/*
==================
SV_NextDownload_f
==================
*/
void SV_NextDownload_f (void)
{
	int		r;
	int		percent;
	int		size;

	if (!sv_client->download)
		return;

	r = sv_client->downloadsize - sv_client->downloadcount;
	if (r > 1024)
		r = 1024;

	MSG_WriteByte (&sv_client->netchan.message, svc_download);
	MSG_WriteShort (&sv_client->netchan.message, r);

	sv_client->downloadcount += r;
	size = sv_client->downloadsize;
	if (!size)
		size = 1;
	percent = sv_client->downloadcount*100/size;
	MSG_WriteByte (&sv_client->netchan.message, percent);
	SZ_Write (&sv_client->netchan.message,
		sv_client->download + sv_client->downloadcount - r, r);

	if (sv_client->downloadcount != sv_client->downloadsize)
		return;

	FS_FreeFile (sv_client->download);
	sv_client->download = NULL;
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f(void)
{
	char		*name, *p;
	size_t		length;
	qboolean	valid;
//	extern int	file_from_pak; // ZOID did file come from pak?

	int offset = 0;

	name = Cmd_Argv(1);

	if (Cmd_Argc() > 2)
		offset = atoi(Cmd_Argv(2)); // downloaded offset

	// r1ch fix: name is always filtered for security reasons
	StripHighBits (name, 1);

	// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check

	// r1ch fix: for  some ./ references in maps, eg ./textures/map/file
	length = strlen(name);
	p = name;
	while ((p = strstr (p, "./")))
	{
		memmove (p, p+2, length - (p - name) - 1);
		length -= 2;
	}

	// r1ch fix: block the really nasty ones - \server.cfg will download from mod root on win32, .. is obvious
	if (name[0] == '\\' || strstr (name, ".."))
	{
		Com_Printf ("Refusing illegal download path %s to %s\n", name, sv_client->name);
		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		Com_Printf ("Client %s[%s] tried to download illegal path: %s\n", sv_client->name, NET_AdrToString (sv_client->netchan.remote_address), name);
		SV_DropClient (sv_client);
		return;
	}
	else if (offset < 0) // r1ch fix: negative offset will crash on read
	{
		Com_Printf ("Refusing illegal download offset %d to %s\n", offset, sv_client->name);
		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		Com_Printf ("Client %s[%s] supplied illegal download offset for %s: %d\n", sv_client->name, NET_AdrToString (sv_client->netchan.remote_address), name, offset);
		SV_DropClient (sv_client);
		return;
	}
	else if ( !length || name[0] == 0 // empty name, maybe as result of ./ normalize
			|| !IsValidChar(name[0])
			// r1ch: \ is bad in general, client won't even write properly if we do sent it
			|| strchr (name, '\\')
			// MUST be in a subdirectory, unless a pk3	
			|| (!strstr (name, "/") && strcmp(name+strlen(name)-4, ".pk3"))
			// r1ch: another bug, maps/. will fopen(".") -> crash
			|| !IsValidChar(name[length-1]) )
/*	if (strstr (name, "..") || !allow_download->value
		// leading dot is no good
		|| *name == '.' 
		// leading slash bad as well, must be in subdir
		|| *name == '/'
		// next up, skin check
		|| (strncmp(name, "players/", 8) == 0 && !allow_download_players->value)
		// now models
		|| (strncmp(name, "models/", 7) == 0 && !allow_download_models->value)
		// now sounds
		|| (strncmp(name, "sound/", 6) == 0 && !allow_download_sounds->value)
		// now maps (note special case for maps, must not be in pak)
		|| (strncmp(name, "maps/", 5) == 0 && !allow_download_maps->value)
		// MUST be in a subdirectory, unless a pk3	
		|| (!strstr (name, "/") && strcmp(name+strlen(name)-4, ".pk3")) )	*/
	{	// don't allow anything with .. path
		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}

	valid = true;

/*	if ( !allow_download->value
		|| (strncmp(name, "players/", 8) == 0 && !allow_download_players->value)
		|| (strncmp(name, "models/", 7) == 0 && !allow_download_models->value)
		|| (strncmp(name, "sound/", 6) == 0 && !allow_download_sounds->value)
		|| (strncmp(name, "maps/", 5) == 0 && !allow_download_maps->value)
		|| (strncmp(name, "pics/", 5) == 0 && !allow_download_pics->value)
		|| ( ((strncmp(name, "env/", 4) == 0 || strncmp(name, "textures/", 9) == 0)) && !allow_download_textures->value ) )*/
	if ( !allow_download->integer
		|| (strncmp(name, "players/", 8) == 0 && !allow_download_players->integer)
		|| (strncmp(name, "models/", 7) == 0 && !allow_download_models->integer)
		|| (strncmp(name, "sound/", 6) == 0 && !allow_download_sounds->integer)
		|| (strncmp(name, "maps/", 5) == 0 && !allow_download_maps->integer)
		|| (strncmp(name, "pics/", 5) == 0 && !allow_download_pics->integer)
		|| ( ((strncmp(name, "env/", 4) == 0 || strncmp(name, "textures/", 9) == 0)) && !allow_download_textures->integer ) )
		valid = false;

	if (!valid)
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}

	if (sv_client->download)
		FS_FreeFile (sv_client->download);

	sv_client->downloadsize = FS_LoadFile (name, (void **)&sv_client->download);
	sv_client->downloadcount = offset;

	if (offset > sv_client->downloadsize)
		sv_client->downloadcount = sv_client->downloadsize;

	// ZOID- special check for maps, if it came from a pak file, don't allow download  
//	if (!sv_client->download || (strncmp(name, "maps/", 5) == 0 && file_from_pak))
	if (!sv_client->download || (strncmp(name, "maps/", 5) == 0 && file_from_protected_pak))	// protected pak check from Yamagi Q2
	{
		Com_DPrintf ("Couldn't download %s to %s\n", name, sv_client->name);
		if (sv_client->download)
		{
			FS_FreeFile (sv_client->download);
			sv_client->download = NULL;
		}

		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}

	SV_NextDownload_f ();
	Com_DPrintf ("Downloading %s to %s\n", name, sv_client->name);
}



//============================================================================


/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Disconnect_f (void)
{
//	SV_EndRedirect ();
	SV_DropClient (sv_client);	
}


/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
void SV_ShowServerinfo_f (void)
{
//	Info_Print (Cvar_Serverinfo());

	// r1ch: this is a client issued command !
	char *s;
	char *p;
	int flip;

	s = Cvar_Serverinfo();

	// skip beginning \\ char
	s++;

	flip = 0;
	p = s;

	// make it more readable
	while (p[0])
	{
		if (p[0] == '\\')
		{
			if (flip)
				p[0] = '\n';
			else
				p[0] = '=';
			flip ^= 1;
		}
		p++;
	}

	SV_ClientPrintf (sv_client, PRINT_HIGH, "%s\n", s);
	// end r1ch fix
}


void SV_Nextserver (void)
{
	char	*v;

	// ZOID, ss_pic can be nextserver'd in coop mode
	if (sv.state == ss_game || (sv.state == ss_pic && !Cvar_VariableValue("coop")))
		return;		// can't nextserver while playing a normal game

	svs.spawncount++;	// make sure another doesn't sneak in
	v = Cvar_VariableString ("nextserver");
	if (!v[0])
	{
	//	Com_DPrintf ("SV_Nextserver: Null nextserver!\n");
		Cbuf_AddText ("killserver\n");
	}
	else
	{
	//	Com_DPrintf ("SV_Nextserver: Changing to nextserver %s.\n", v);
		Cbuf_AddText (v);
		Cbuf_AddText ("\n");
	}
	Cvar_Set ("nextserver","");
}

/*
==================
SV_Nextserver_f

A cinematic has completed or been aborted by a client, so move
to the next server,
==================
*/
void SV_Nextserver_f (void)
{
	if ( atoi(Cmd_Argv(1)) != svs.spawncount ) {
		Com_DPrintf ("Nextserver() from wrong level, from %s\n", sv_client->name);
		return;		// leftover from last server
	}

	Com_DPrintf ("Nextserver() from %s\n", sv_client->name);

	SV_Nextserver ();
}

typedef struct
{
	char	*name;
	void	(*func) (void);
} ucmd_t;

ucmd_t ucmds[] =
{
	// auto issued
	{"new", SV_New_f},
	{"configstrings", SV_Configstrings_f},
	{"baselines", SV_Baselines_f},
	{"begin", SV_Begin_f},

	{"nextserver", SV_Nextserver_f},

	{"disconnect", SV_Disconnect_f},

	// issued by hand at client consoles	
	{"info", SV_ShowServerinfo_f},

	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},

	{NULL, NULL}
};

/*
==================
SV_ExecuteUserCommand
==================
*/
void SV_ExecuteUserCommand (char *s)
{
	ucmd_t	*u;
	
	Cmd_TokenizeString (s, false); //Knightmare- password security fix, was true
									// prevents players from reading rcon_password
	sv_player = sv_client->edict;

//	SV_BeginRedirect (RD_CLIENT);

	for (u=ucmds ; u->name ; u++)
		if (!strcmp (Cmd_Argv(0), u->name) )
		{
			u->func ();
			break;
		}

	// r1ch: do we really want to be passing commands from unconnected players
	// to the game dll at this point? doesn't sound like a good idea to me
	// especially if the game dll does its own banning functions after connect
	// as banned players could spam game commands (eg say) whilst connecting
	if (sv_client->state < cs_spawned)
		return;

	if (!u->name && sv.state == ss_game)
		ge->ClientCommand (sv_player);

//	SV_EndRedirect ();
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/



void SV_ClientThink (client_t *cl, usercmd_t *cmd)

{
	cl->commandMsec -= cmd->msec;

//	if (cl->commandMsec < 0 && sv_enforcetime->value )
	if (cl->commandMsec < 0 && sv_enforcetime->integer )
	{
		Com_DPrintf ("commandMsec underflow from %s\n", cl->name);
		return;
	}

	ge->ClientThink (cl->edict, cmd);
}



#define	MAX_STRINGCMDS	8
/*
===================
SV_ExecuteClientMessage

The current net_message is parsed for the given client
===================
*/
void SV_ExecuteClientMessage (client_t *cl)
{
	int		c;
	char	*s;

	usercmd_t	nullcmd;
	usercmd_t	oldest, oldcmd, newcmd;
	int		net_drop;
	int		stringCmdCount;
	int		checksum, calculatedChecksum;
	int		checksumIndex;
	qboolean	move_issued;
	int		lastframe;

	sv_client = cl;
	sv_player = sv_client->edict;

	// only allow one move command
	move_issued = false;
	stringCmdCount = 0;

	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Printf ("SV_ReadClientMessage: badread\n");
			SV_DropClient (cl);
			return;
		}	

		c = MSG_ReadByte (&net_message);
		if (c == -1)
			break;
				
		switch (c)
		{
		default:
			Com_Printf ("SV_ReadClientMessage: unknown command char\n");
			SV_DropClient (cl);
			return;
						
		case clc_nop:
			break;

		case clc_userinfo:
			strncpy (cl->userinfo, MSG_ReadString (&net_message), sizeof(cl->userinfo)-1);
			SV_UserinfoChanged (cl);
			break;

		case clc_move:
			if (move_issued)
				return;		// someone is trying to cheat...

			move_issued = true;
			checksumIndex = net_message.readcount;
			checksum = MSG_ReadByte (&net_message);
			lastframe = MSG_ReadLong (&net_message);
			if (lastframe != cl->lastframe) {
				cl->lastframe = lastframe;
				if (cl->lastframe > 0) {
					cl->frame_latency[cl->lastframe&(LATENCY_COUNTS-1)] = 
						svs.realtime - cl->frames[cl->lastframe & UPDATE_MASK].senttime;
				}
			}

			memset (&nullcmd, 0, sizeof(nullcmd));
			MSG_ReadDeltaUsercmd (&net_message, &nullcmd, &oldest);
			MSG_ReadDeltaUsercmd (&net_message, &oldest, &oldcmd);
			MSG_ReadDeltaUsercmd (&net_message, &oldcmd, &newcmd);

			if ( cl->state != cs_spawned )
			{
				cl->lastframe = -1;
				break;
			}

			// if the checksum fails, ignore the rest of the packet
			calculatedChecksum = COM_BlockSequenceCRCByte (
				net_message.data + checksumIndex + 1,
				net_message.readcount - checksumIndex - 1,
				cl->netchan.incoming_sequence);

			if (calculatedChecksum != checksum)
			{
				Com_DPrintf ("Failed command checksum for %s (%d != %d)/%d\n", 
					cl->name, calculatedChecksum, checksum, 
					cl->netchan.incoming_sequence);
				return;
			}

		//	if (!sv_paused->value)
			if (!sv_paused->integer)
			{
				net_drop = cl->netchan.dropped;
				if (net_drop < 20)
				{

//if (net_drop > 2)

//	Com_Printf ("drop %i\n", net_drop);
					while (net_drop > 2)
					{
						SV_ClientThink (cl, &cl->lastcmd);

						net_drop--;
					}
					if (net_drop > 1)
						SV_ClientThink (cl, &oldest);

					if (net_drop > 0)
						SV_ClientThink (cl, &oldcmd);

				}
				SV_ClientThink (cl, &newcmd);
			}

			cl->lastcmd = newcmd;
			break;

		case clc_stringcmd:	
			s = MSG_ReadString (&net_message);

			// malicious users may try using too many string commands
			if (++stringCmdCount < MAX_STRINGCMDS)
				SV_ExecuteUserCommand (s);

			if (cl->state == cs_zombie)
				return;	// disconnect command
			break;
		}
	}
}

