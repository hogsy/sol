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

// unix_main.c

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>		// needed for GCC to avoid dlopen errors
#include <sys/resource.h>

#include <SDL.h>

#include "../qcommon/qcommon.h"
#include "unixquake.h"

cvar_t *nostdout;

unsigned	sys_frame_time;

uid_t saved_euid;
qboolean stdin_active = true;
int			ActiveApp;

// Knightmare- added exe / pref dirs
static char exe_dir[MAX_OSPATH];
static char pref_dir[MAX_OSPATH];
static char	download_dir[MAX_OSPATH];

// =======================================================================
// General routines
// =======================================================================

void Sys_ConsoleOutput (char *string)
{
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}

void Sys_Sleep (int msec)
{
	if (msec < 0)	return;
	usleep ((unsigned int)msec * 1000);
}

unsigned Sys_TickCount (void)
{
	return clock();
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	unsigned char		*p;

	va_start (argptr, fmt);
//	vsprintf (text, fmt, argptr);
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	if (strlen(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

    if (nostdout && nostdout->value)
        return;

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
}

void Sys_Quit (void)
{
	CL_Shutdown ();
	Qcommon_Shutdown ();
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	_exit(0);
}

void Sys_Init(void)
{
#if id386
//	Sys_SetFPCW();
#endif
}

void Sys_Error (const char *error, ...)
{ 
	va_list     argptr;
	char        string[1024];

// change stdin to non blocking
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	CL_Shutdown ();
	Qcommon_Shutdown ();

	va_start (argptr, error);
//	vsprintf (string, error, argptr);
	Q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	_exit (1);
} 

void Sys_Warn (char *warning, ...)
{ 
	va_list     argptr;
	char        string[1024];

	va_start (argptr, warning);
//	vsprintf (string, warning, argptr);
	Q_vsnprintf (string, sizeof(string), warning, argptr);
	va_end (argptr);
	fprintf(stderr, "Warning: %s", string);
} 

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

void floating_point_exception_handler(int whatever)
{
//	Sys_Warn("floating point exception\n");
	signal(SIGFPE, floating_point_exception_handler);
}

char *Sys_ConsoleInput(void)
{
    static char text[256];
    int     len;
	fd_set	fdset;
    struct timeval timeout;

	if (!dedicated || !dedicated->value)
		return NULL;

	if (!stdin_active)
		return NULL;

	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
		return NULL;

	len = read (0, text, sizeof(text));
	if (len == 0) { // eof!
		stdin_active = false;
		return NULL;
	}

	if (len < 1)
		return NULL;
	text[len-1] = 0;    // rip off the /n and terminate

	return text;
}

// Knightmare added
void Sys_ShowConsole (qboolean show)
{
}

/*****************************************************************************/

/*
=================
Sys_LoadLibrary

From Yamagi Q2
=================
*/
void *Sys_LoadLibrary (const char *libPath, const char *initFuncName, void **libHandle)
{
	void	*hLibrary, *funcPtr;

	*libHandle = NULL;

#ifdef USE_SANITIZER
	hLibrary = dlopen (libPath, RTLD_LAZY | RTLD_NODELETE);
#else
	hLibrary = dlopen (libPath, RTLD_LAZY);
#endif

	if ( !hLibrary ) {
		Com_DPrintf("Sys_LoadLibrary: failure on %s, dlopen returned %s\n", libPath, dlerror());
		return NULL;
	}

	if (initFuncName != NULL)
	{
		funcPtr = dlsym (hLibrary, initFuncName);

		if ( !funcPtr ) {
			Com_DPrintf("Sys_LoadLibrary: failure in %s on %s, dlclose returned %s\n", libPath, initFuncName, dlerror());
			dlclose (hLibrary);
			return NULL;
		}
	}
	else {
		funcPtr = NULL;
	}

	*libHandle = hLibrary;

	Com_DPrintf("Sys_LoadLibrary: sucessfully loaded %s\n", libPath);

	return funcPtr;
}


/*
=================
Sys_FreeLibrary

From Yamagi Q2
=================
*/
void Sys_FreeLibrary (void *libHandle)
{
	if ( libHandle && dlclose(libHandle) )
	{
		Com_Error(ERR_FATAL, "dlclose failed on %p: %s", libHandle, dlerror());
	}
}


/*
=================
Sys_GetProcAddress

From Yamagi Q2
=================
*/
void *Sys_GetProcAddress (void *libHandle, const char *initFuncName)
{
    if (libHandle == NULL)
    {
#ifdef RTLD_DEFAULT
        return dlsym(RTLD_DEFAULT, initFuncName);
#else
        /* POSIX suggests that this is a portable equivalent */
        static void *global_namespace = NULL;

        if (global_namespace == NULL)
            global_namespace = dlopen(NULL, RTLD_GLOBAL|RTLD_LAZY);

        return dlsym (global_namespace, initFuncName);
#endif
    }
    return dlsym (libHandle, initFuncName);
}

/*****************************************************************************/

static void *game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (game_library) 
		SDL_UnloadObject (game_library);
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
	char	name[MAX_OSPATH];
	char	curpath[MAX_OSPATH];
	char	*path;

#ifdef __APPLE__
	// KMQ2 MacOSX port uses the Fruitz of Dojo plug system.  So this will go unused.
	#define LIB_SUFFIX "dylib"
#else
	#define LIB_SUFFIX "so"
#endif

	// Knightmare- changed game library name for better cohabitation
#if defined (_M_X64) || defined (_M_AMD64) || defined (__x86_64__)
	const char *gamename = "kmq2gamex64." LIB_SUFFIX;
#elif defined (__i386__)
	const char *gamename = "kmq2gamei386." LIB_SUFFIX;
#elif defined (__ia64__)
	const char *gamename = "kmq2gameia64." LIB_SUFFIX;
#elif defined (__alpha__)
	const char *gamename = "kmq2gameaxp." LIB_SUFFIX;
#elif defined (__arm__)
	const char *gamename = "kmq2gamearm32." LIB_SUFFIX;
#elif defined (__aarch64__)
	const char *gamename = "kmq2gamearm64." LIB_SUFFIX;
#elif defined (__powerpc__)
	const char *gamename = "kmq2gameppc." LIB_SUFFIX;
#elif defined (__sparc__)
	const char *gamename = "kmq2gamesparc." LIB_SUFFIX;
#else
#error Unknown arch
#endif

	setreuid(getuid(), getuid());
	setegid(getgid());

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	getcwd(curpath, sizeof(curpath));

	Com_Printf("------- Loading %s -------\n", gamename);

	// now run through the search paths
	path = NULL;
	while (1)
	{
		path = FS_NextPath (path);
		if (!path)
			return NULL;		// couldn't find one anywhere
//		Com_sprintf (name, sizeof(name), "%s/%s/%s", curpath, path, gamename);
		if (path[0] == '/') {
			// Path is rooted, override curpath
			Com_sprintf (name, sizeof(name), "%s/%s", path, gamename);
		}
		else {
			Com_sprintf (name, sizeof(name), "%s/%s/%s", curpath, path, gamename);
		}
		game_library = SDL_LoadObject (name);
		if (game_library)
		{
			Com_Printf ("LoadLibrary (%s)\n",name);
			break;
		}
	}

	GetGameAPI = (void *)SDL_LoadFunction (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
}

/*****************************************************************************/

void Sys_AppActivate (void)
{
}

void HandleEvents(void);
void Sys_SendKeyEvents (void)
{
#ifndef DEDICATED_ONLY
	HandleEvents();
#endif

	// grab frame time 
	sys_frame_time = Sys_Milliseconds();
}

/*****************************************************************************/

// Knightmare- adapted from DK 1.3 Linux port
const char* Sys_ExeDir (void)
{
	return exe_dir;
}

const char* Sys_PrefDir (void)
{
	return pref_dir;
}

const char* Sys_DownloadDir (void)
{
	return download_dir;
}

void Init_ExeDir (const char* argv0)
{
	char *basedir = SDL_GetBasePath();
	if (basedir == NULL)
	{
		/* Oh well. */
		memset(exe_dir, 0, sizeof(exe_dir));
		Q_snprintfz(exe_dir, sizeof(exe_dir), ".");
		return;
	}
	Q_strncpyz(exe_dir, sizeof(exe_dir), basedir);
	SDL_free(basedir);

	/* Intentionally cut off the final separator */
	exe_dir[strlen(exe_dir) - 1] = '\0';
}

void Sys_InitPrefDir (void)
{
	char *pp = SDL_GetPrefPath(NULL, SAVENAME);

	if (pp != NULL)
	{
		Q_strncpyz(pref_dir, sizeof(pref_dir), pp);
		Q_strncpyz(download_dir, sizeof(download_dir), pp);
		SDL_free(pp);

		/* Intentionally cut off the final separator */
		pref_dir[strlen(pref_dir) - 1] = '\0';
		download_dir[strlen(download_dir) - 1] = '\0';
		return;
	}

	printf("WARNING: SDL_GetPrefPath failed, defaulting to fs_basedir!\n");
	Q_strncpyz (pref_dir, sizeof(pref_dir), fs_basedir->string);
	Q_strncpyz (download_dir, sizeof(download_dir), fs_basedir->string);
}
// end Knightmare

/*****************************************************************************/

int main (int argc, char **argv)
{
	int 	time, oldtime, newtime, result;
	// Knightmare added
//	const rlim_t	minStackSize = 16L * 1024L * 1024L;	// min stack size of 16MB
//	struct rlimit	rl;

	// go back to real user for config loads
	saved_euid = geteuid();
	seteuid (getuid());

	printf ("\n");	
	printf ("========= Initialization =================\n");
	printf ("KMQuake2 -- Version 0.20u8\n");
	printf ("Linux Port by QuDos\n");
	printf ("SDL2 Port by flibitijibibo\n");
	printf ("http://qudos.quakedev.com/\n");
	printf ("Compiled: "__DATE__" -- "__TIME__"\n");
	printf ("==========================================\n\n");

	// Knightmare- set minimum stack size of 16MB
/*	printf ("Checking stack size...");
	result = getrlimit(RLIMIT_STACK, &rl);
	if (result == 0)
	{
		if (rl.rlim_cur < minStackSize)
		{
			printf (" succeeded.\nIncreasing stack size to %i...", minStackSize);
			rl.rlim_cur = minStackSize;
			result = setrlimit(RLIMIT_STACK, &rl);
			if (result != 0) {
				printf (" failed!\nsetrlimit() returned result of %i.\n", result);
			}
			else {
				printf (" succeeded.\n");
			}
		}
		else {
			printf (" succeeded.  Stack size is sufficient.\n");
		}
	}
	else {
		printf (" failed!\ngetrlimit() returned result of %i.\n", result);
	} */

	// Knightmare- init exe dir
	Init_ExeDir (argv[0]);

	Qcommon_Init (argc, argv);

	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
	//	printf ("Linux Quake II -- Version %0.3f\n", KMQUAKE2_VERSION);
	}

    oldtime = Sys_Milliseconds ();
    while (1)
    {
		// find time spent rendering last frame
		do {
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);
        Qcommon_Frame (time);
		oldtime = newtime;
    }

}
