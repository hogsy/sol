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

// cmd.c -- Quake script command processing module

#include "qcommon.h"

#ifdef LOC_SUPPORT // Xile/NiceAss LOC
void CL_LocPlace (void);
#endif	// LOC_SUPPORT

void Cmd_ForwardToServer (void);
void CL_ForcePacket (void);

#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
	char	name[MAX_ALIAS_NAME];
	char	*value;
} cmdalias_t;

cmdalias_t	*cmd_alias;

qboolean	cmd_wait;

#define	ALIAS_LOOP_COUNT	16
int		alias_count;		// for detecting runaway loops


//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void)
{
	cmd_wait = true;
}


/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

sizebuf_t	cmd_text;
byte		cmd_text_buf[32768]; // Knightmare increased, was 8192

byte		defer_text_buf[32768]; // Knightmare increased, was 8192

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	SZ_Init (&cmd_text, cmd_text_buf, sizeof(cmd_text_buf));
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (const char *text)
{
	int		l;
	
	l = (int)strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf ("Cbuf_AddText: overflow\n");
		return;
	}
	SZ_Write (&cmd_text, text, (int)strlen (text));
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (char *text)
{
	char	*temp;
	int		templen;

// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if (templen)
	{
		temp = static_cast< char * >( Z_Malloc( templen ) );
		memcpy (temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}
	else
		temp = nullptr;	// shut up compiler
		
// add the entire text of the file
	Cbuf_AddText (text);
	
// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text, temp, templen);
		Z_Free (temp);
	}
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer (void)
{
	memcpy(defer_text_buf, cmd_text_buf, cmd_text.cursize);
	defer_text_buf[cmd_text.cursize] = 0;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer (void)
{
	Cbuf_InsertText ((char*)defer_text_buf);
	defer_text_buf[0] = 0;
}


/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText (int exec_when, char *text)
{
	switch (exec_when)
	{
	case EXEC_NOW:
		Cmd_ExecuteString (text);
		break;
	case EXEC_INSERT:
		Cbuf_InsertText (text);
		break;
	case EXEC_APPEND:
		Cbuf_AddText (text);
		break;
	default:
		Com_Error (ERR_FATAL, "Cbuf_ExecuteText: bad exec_when");
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;

	alias_count = 0;		// don't allow infinite alias loops

	while (cmd_text.cursize)
	{
// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		// [SkulleR]'s fix for overflow vulnerability
		if (i > sizeof(line) - 1)
			i =  sizeof(line) - 1; 		
		memcpy (line, text, i); // stack buffer overflow possible
		line[i] = 0;
		
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove (text, text+i, cmd_text.cursize);
		}

// execute the command line
		Cmd_ExecuteString (line);
		
		if (cmd_wait)
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
		//	if (!dedicated->value)
			if (!dedicated->integer)
				CL_ForcePacket ();
			cmd_wait = false;
			break;
		}
	}
}


/*
===============
Cbuf_AddEarlyCommands

Adds command line parameters as script statements
Commands lead with a +, and continue until another +

Set commands are added early, so they are guaranteed to be set before
the client and server initialize for the first time.

Other commands are added late, after all initialization is complete.
===============
*/
void Cbuf_AddEarlyCommands (qboolean clear)
{
	int		i;
	char	*s;

	for (i=0 ; i<COM_Argc() ; i++)
	{
		s = COM_Argv(i);
		if (strcmp (s, "+set"))
			continue;
		Cbuf_AddText (va("set %s %s\n", COM_Argv(i+1), COM_Argv(i+2)));
		if (clear)
		{
			COM_ClearArgv(i);
			COM_ClearArgv(i+1);
			COM_ClearArgv(i+2);
		}
		i+=2;
	}
}

/*
=================
Cbuf_AddLateCommands

Adds command line parameters as script statements
Commands lead with a + and continue until another + or -
quake +vid_ref gl +map amlev1

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Cbuf_AddLateCommands (void)
{
	int			i, j, s;
	char		*text, *build, c;
	int			argc;
	qboolean	ret;

// build the combined string to parse from
	s = 0;
	argc = COM_Argc();
	for (i=1 ; i<argc ; i++)
	{
		s += (int)strlen (COM_Argv(i)) + 1;
	}
	if (!s)
		return false;
		
	text = static_cast< char * >( Z_Malloc( s + 1 ) );
	text[0] = 0;
	for (i=1 ; i<argc ; i++)
	{
	//	strncat (text,COM_Argv(i));
		Q_strncatz (text, s+1, COM_Argv(i));
		if (i != argc-1)
		//	strncat (text, " ");
			Q_strncatz (text, s+1, " ");
	}
	
	// start quake2:// support
	// R1ch: awful hack to prevent arbitrary cmd execution with quake2:// links due to Q2's bad quote parser
	if (!strncmp (text, "+connect \"quake2://", 19))
	{
		if (strchr (text + 1, '+'))
			Com_Error (ERR_FATAL, "Attempt to use multiple commands in a quake2:// protocol handler:\n\n%s", text);
	}
	// end quake2:// support 

// pull out the commands
	build = static_cast< char * >( Z_Malloc( s + 1 ) );
	build[0] = 0;
	
	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;

			c = text[j];
			text[j] = 0;
			
		//	strncat (build, text+i);
		//	strncat (build, "\n");
			Q_strncatz (build, s+1, text+i);
			Q_strncatz (build, s+1, "\n");
			text[j] = c;
			i = j-1;
		}
	}

	ret = (build[0] != 0);
	if (ret)
		Cbuf_AddText (build);
	
	Z_Free (text);
	Z_Free (build);

	return ret;
}


/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/


/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f, *f2;
	int		len;

	if (Cmd_Argc () != 2)
	{
		Com_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	len = FS_LoadFile (Cmd_Argv(1), (void **)&f);
	if (!f)
	{
		Com_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	Com_Printf ("execing %s\n",Cmd_Argv(1));
	
	// the file doesn't have a trailing 0, so we need to copy it off
	f2 = static_cast< char * >( Z_Malloc( len + 2 ) ); // Echon fix- was len+1
	memcpy (f2, f, len);
	f2[len] = '\n';  // Echon fix added
	f2[len+1] = '\0'; // Echon fix- was len, = 0

	Cbuf_InsertText (f2);

	Z_Free (f2);
	FS_FreeFile (f);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
	int		i;
	
	for (i=1 ; i<Cmd_Argc() ; i++)
		Com_Printf ("%s ",Cmd_Argv(i));
	Com_Printf ("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
void Cmd_Alias_f (void)
{
	cmdalias_t	*a;
	char		cmd[1024];
	int			i, c;
	char		*s;

	if (Cmd_Argc() == 1)
	{
		Com_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Com_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv(1);
	if (strlen(s) >= MAX_ALIAS_NAME)
	{
		Com_Printf ("Alias name is too long\n");
		return;
	}

	// if the alias already exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			Z_Free (a->value);
			break;
		}
	}

	if (!a)
	{
		a = static_cast< cmdalias_t * >( Z_Malloc( sizeof( cmdalias_t ) ) );
		a->next = cmd_alias;
		cmd_alias = a;
	}
//	strncpy (a->name, s);	
	Q_strncpyz (a->name, sizeof(a->name), s);	

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
	//	strncat (cmd, Cmd_Argv(i));
		Q_strncatz (cmd, sizeof(cmd), Cmd_Argv(i));
		if (i != (c - 1))
		//	strncat (cmd, " ");
			Q_strncatz (cmd, sizeof(cmd), " ");
	}
//	strncat (cmd, "\n");
	Q_strncatz (cmd, sizeof(cmd), "\n");
	
	a->value = CopyString (cmd);
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s *next;
	std::string            name;
	xcommand_t             function;
} cmd_function_t;

static	int			cmd_argc;
static	char		*cmd_argv[MAX_STRING_TOKENS];
static	char		*cmd_null_string = "";
static	char		cmd_args[MAX_STRING_CHARS];

static	cmd_function_t	*cmd_functions;		// possible commands to execute

/*
============
Cmd_Argc
============
*/
int		Cmd_Argc (void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv (int arg)
{
	if ( (unsigned)arg >= cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];	
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char		*Cmd_Args (void)
{
	return cmd_args;
}


/*
======================
Cmd_MacroExpandString
======================
*/
char *Cmd_MacroExpandString (char *text)
{
	int		i, j, count, len;
	qboolean	inquote;
	char	*scan;
	static	char	expanded[MAX_STRING_CHARS];
	char	temporary[MAX_STRING_CHARS];
	char	*token, *start;

	inquote = false;
	scan = text;

	len = (int)strlen (scan);
	if (len >= MAX_STRING_CHARS)
	{
		Com_Printf ("Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
		return nullptr;
	}

	count = 0;

	for (i=0 ; i<len ; i++)
	{
		if (scan[i] == '"')
			inquote ^= 1;
		if (inquote)
			continue;	// don't expand inside quotes
		if (scan[i] != '$')
			continue;
		// scan out the complete macro
		start = scan+i+1;
		token = COM_Parse (&start);
		if (!start)
			continue;
	
		token = Cvar_VariableString (token);

		j = (int)strlen(token);
		len += j;
		if (len >= MAX_STRING_CHARS)
		{
			Com_Printf ("Expanded line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
			return nullptr;
		}

		strncpy (temporary, scan, i);
	//	strncpy (temporary+i, token);
	//	strncpy (temporary+i+j, start);
		Q_strncpyz (temporary+i, sizeof(temporary)-i, token);
		Q_strncpyz (temporary+i+j, sizeof(temporary)-i-j, start);

	//	strncpy (expanded, temporary);
		Q_strncpyz (expanded, sizeof(expanded), temporary);
		scan = expanded;
		i--;

		if (++count == 100)
		{
			Com_Printf ("Macro expansion loop, discarded.\n");
			return nullptr;
		}
	}

	if (inquote)
	{
		Com_Printf ("Line has unmatched quote, discarded.\n");
		return nullptr;
	}

	return scan;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
$Cvars will be expanded unless they are in a quoted token
============
*/
void Cmd_TokenizeString (char *text, qboolean macroExpand)
{
	int		i;
	size_t	cmd_argSize;
	char	*com_token;

// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);
		
	cmd_argc = 0;
	cmd_args[0] = 0;
	
	// macro expand the text
	if (macroExpand)
	{
#ifdef LOC_SUPPORT	// Xile/NiceAss LOC
#ifndef DEDICATED_ONLY
		CL_LocPlace();
#endif
#endif	// LOC_SUPPORT
		text = Cmd_MacroExpandString (text);
	}
	if (!text)
		return;

	while (1)
	{
// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
		{
			text++;
		}
		
		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;

		// set cmd_args to everything after the first arg
		if (cmd_argc == 1)
		{
			int		l;

			// [SkulleR]'s fix for overflow vulnerability
			//strncpy (cmd_args, text);
			Q_strncpyz (cmd_args, sizeof(cmd_args), text);
			cmd_args[sizeof(cmd_args)-1] = 0; 

			// strip off any trailing whitespace
			l = (int)strlen(cmd_args) - 1;
			for ( ; l >= 0 ; l--)
				if (cmd_args[l] <= ' ')
					cmd_args[l] = 0;
				else
					break;
		}
			
		com_token = COM_Parse (&text);
		if (!text)
			return;

		if (cmd_argc < MAX_STRING_TOKENS)
		{
			cmd_argSize = strlen(com_token)+1;
			cmd_argv[cmd_argc] = static_cast< char * >( Z_Malloc( cmd_argSize ) );
		//	strncpy (cmd_argv[cmd_argc], com_token);
			Q_strncpyz (cmd_argv[cmd_argc], cmd_argSize, com_token);
			cmd_argc++;
		}
	}
	
}


/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand( const char *cmd_name, xcommand_t function )
{
	cmd_function_t *cmd;

	// fail if the command is a variable name
	if ( Cvar_VariableString( cmd_name )[ 0 ] )
	{
		Com_Printf( "Cmd_AddCommand: %s already defined as a var\n", cmd_name );
		return;
	}

	// fail if the command already exists
	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( cmd->name == cmd_name )
		{
			Com_Printf( "Cmd_AddCommand: %s already defined\n", cmd_name );
			return;
		}
	}

	cmd           = static_cast< cmd_function_t * >( Z_Malloc( sizeof( cmd_function_t ) ) );
	cmd->name     = cmd_name;
	cmd->function = function;
	cmd->next     = cmd_functions;
	cmd_functions = cmd;
}

/*
============
Cmd_RemoveCommand
============
*/
void	Cmd_RemoveCommand ( const char *cmd_name )
{
	cmd_function_t **back = &cmd_functions;
	while (true)
	{
		cmd_function_t *cmd = *back;
		if (!cmd)
		{
			Com_Printf ("Cmd_RemoveCommand: %s not added\n", cmd_name);
			return;
		}
		if (!strcmp (cmd_name, cmd->name.c_str()))
		{
			*back = cmd->next;
			Z_Free (cmd);
			return;
		}
		back = &cmd->next;
	}
}

/*
============
Cmd_Exists
============
*/
qboolean	Cmd_Exists ( const char *cmd_name)
{
	for ( const cmd_function_t *cmd = cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!strcmp (cmd_name,cmd->name.c_str()))
			return true;
	}

	return false;
}





/*
============
Cmd_CompleteCommand
============
*/

/*char *Cmd_CompleteCommand (char *partial)
{
	cmd_function_t	*cmd;
	int				len;
	cmdalias_t		*a;
	
	len = (int)strlen(partial);
	
	if (!len)
		return NULL;
		
// check for exact match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!strcmp (partial,cmd->name))
			return cmd->name;
	for (a=cmd_alias ; a ; a=a->next)
		if (!strcmp (partial, a->name))
			return a->name;

// check for partial match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!strncmp (partial,cmd->name, len))
			return cmd->name;
	for (a=cmd_alias ; a ; a=a->next)
		if (!strncmp (partial, a->name, len))
			return a->name;

	return NULL;
}*/

// Knightmare - added command auto-complete
char        retval[ 256 ];
const char *Cmd_CompleteCommand( const char *partial )
{
	cmd_function_t	*cmd;
	int                  i;
	cmdalias_t		*a;
	cvar_t			*cvar;
	const char			*pmatch[1024];
	qboolean		diff = false;
	
	int len = ( int ) strlen( partial );
	if (!len)
		return nullptr;
		
// check for exact match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!Q_strcasecmp (partial,cmd->name.c_str()))
			return cmd->name.c_str();
	for (a=cmd_alias ; a ; a=a->next)
		if (!Q_strcasecmp (partial, a->name))
			return a->name;
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!Q_strcasecmp (partial,cvar->name))
			return cvar->name;

	for (i=0; i<1024; i++)
		pmatch[i] = nullptr;
	i=0;

// check for partial match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	//	if (!_strnicmp (partial, cmd->name, len)) {
		if (!Q_strncasecmp (partial, cmd->name.c_str(), len)) {
			pmatch[i]=cmd->name.c_str();
			i++;
		}
	for (a=cmd_alias ; a ; a=a->next)
	//	if (!_strnicmp (partial, a->name, len)) {
		if (!Q_strncasecmp ( partial, a->name, len ) ) {
			pmatch[i]=a->name;
			i++;
		}
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
	//	if (!_strnicmp (partial, cvar->name, len)) {
		if (!Q_strncasecmp ( partial, cvar->name, len ) ) {
			pmatch[i]=cvar->name;
			i++;
		}

	if (i) {
		int p;
		int o;
		if (i == 1)
			return pmatch[0];

		Com_Printf("\nListing matches for '%s'...\n",partial);
		for (o=0; o<i; o++)
			Com_Printf("  %s\n",pmatch[o]);

	//	strncpy(retval,""); p=0;
		Q_strncpyz(retval, sizeof(retval), ""); p=0;
		while (!diff && p < 256) {
			retval[p]=pmatch[0][p];
			for (o=0; o<i; o++) {
				if (p > strlen(pmatch[o]))
					continue;
				if (retval[p] != pmatch[o][p]) {
					retval[p] = 0;
					diff=false;
				}
			}
			p++;
		}
		Com_Printf("Found %i matches\n",i);
		return retval;
	}

	return nullptr;
}

qboolean Cmd_IsComplete ( const char *command)
{
	// check for exact match
	for ( const cmd_function_t *cmd = cmd_functions ; cmd ; cmd=cmd->next)
		if (!Q_strcasecmp (command,cmd->name.c_str()))
			return true;
	for ( const cmdalias_t *a = cmd_alias ; a ; a=a->next)
		if (!Q_strcasecmp (command, a->name))
			return true;
	for ( const cvar_t *cvar = cvar_vars ; cvar ; cvar=cvar->next)
		if (!Q_strcasecmp (command,cvar->name))
			return true;

	return false;
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void	Cmd_ExecuteString (char *text)
{	
	cmd_function_t	*cmd;
	cmdalias_t		*a;

	Cmd_TokenizeString (text, true);
			
	// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

	// check functions
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcasecmp (cmd_argv[0],cmd->name.c_str()))
		{
			if (!cmd->function)
			{	// forward to server command
				Cmd_ExecuteString (va("cmd %s", text));
			}
			else
				cmd->function ();
			return;
		}
	}

	// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!Q_strcasecmp (cmd_argv[0], a->name))
		{
			if (++alias_count == ALIAS_LOOP_COUNT)
			{
				Com_Printf ("ALIAS_LOOP_COUNT\n");
				return;
			}
			Cbuf_InsertText (a->value);
			return;
		}
	}
	
	// check cvars
	if (Cvar_Command ())
		return;

	// send it as a server command if we are connected
	Cmd_ForwardToServer ();
}

/*
============
Cmd_List_f
============
*/
void Cmd_List_f()
{
	int i = 0;
	for ( cmd_function_t *cmd = cmd_functions; cmd; cmd = cmd->next, i++ )
		Com_Printf( "%s\n", cmd->name.c_str() );

	Com_Printf( "%i commands\n", i );
}

/*
============
Cmd_Init
============
*/
void Cmd_Init()
{
	//
	// register our commands
	//
	Cmd_AddCommand( "cmdlist", Cmd_List_f );
	Cmd_AddCommand( "exec", Cmd_Exec_f );
	Cmd_AddCommand( "echo", Cmd_Echo_f );
	Cmd_AddCommand( "alias", Cmd_Alias_f );
	Cmd_AddCommand( "wait", Cmd_Wait_f );
}

