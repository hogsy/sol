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

// win_main.c

#include "../qcommon/qcommon.h"
#include "winquake.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <shlobj.h>
#include "win_conproc.h"

// [Slipyx] mingw support for _controlfp. from float.h. these would be defined
// if __STRICT_ANSI__ was undefined before including float.h, but i just copied
// out these specific definitions instead since they're the only ones used.
#if defined(__MINGW32__)
#define _MCW_PC 0x00030000
#define _PC_24 0x00020000
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _controlfp (unsigned int unNew, unsigned int unMask);
#endif // __MINGW32__

#define MINIMUM_WIN_MEMORY	0x0a00000
#define MAXIMUM_WIN_MEMORY	0x1000000

qboolean s_win95;

qboolean	s_win9X;
qboolean	s_winNT;
qboolean	s_winNT6;

int			starttime;
int			ActiveApp;
qboolean	Minimized;

static HANDLE		hinput, houtput;

unsigned	sys_msg_time;
unsigned	sys_frame_time;


static HANDLE		qwclsemaphore;

#define	MAX_NUM_ARGVS	128
int			argc;
char		*argv[MAX_NUM_ARGVS];

#define NT5_SAVEDIR "My Games/KMQuake2"
#define NT6_SAVEDIR "KMQuake2"
#define NT5_DLDIR "My Downloads/KMQuake2"
#define NT6_DLDIR "KMQuake2"
static char exe_dir[MAX_OSPATH];
static char	pref_dir[MAX_OSPATH];
static char	download_dir[MAX_OSPATH];

qboolean Detect_WinNT5orLater (void);
qboolean Detect_WinNT6orLater (void);

typedef HRESULT (WINAPI *SHGETFOLDERPATHW) (HWND hwnd, int CSIDL, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
SHGETFOLDERPATHW fnSHGetFolderPathW = NULL;

// from DarkPlaces
const GUID qFOLDERID_SavedGames = {0x4C5C32FF, 0xBB9D, 0x43b0, {0xB5, 0xB4, 0x2D, 0x72, 0xE5, 0x4E, 0xAA, 0xA4}}; 
const GUID qFOLDERID_Downloads = {0x374DE290, 0x123F, 0x4565, {0x91, 0x64, 0x39, 0xC4, 0x92, 0x5E, 0x46, 0x7B}}; 
#define qREFKNOWNFOLDERID const GUID *
#define qKF_FLAG_CREATE 0x8000
#define qKF_FLAG_NO_ALIAS 0x1000
static HRESULT (WINAPI *fnSHGetKnownFolderPath) (qREFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
static HRESULT (WINAPI *fnCoInitializeEx)(LPVOID pvReserved, DWORD dwCoInit);
static void (WINAPI *fnCoUninitialize)(void);
static void (WINAPI *fnCoTaskMemFree)(LPVOID pv);
// end DarkPlaces code


#ifndef NEW_DED_CONSOLE
/*
===============================================================================

DEDICATED CONSOLE

===============================================================================
*/
static char	console_text[256];
static int	console_textlen;

/*
================
Sys_InitConsole
================
*/
void Sys_InitConsole (void)
{
//	if (!dedicated->value)
	if (!dedicated->integer)
		return;

	if (!AllocConsole ())
		Sys_Error ("Couldn't create dedicated server console");
	hinput = GetStdHandle (STD_INPUT_HANDLE);
	houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	
	// let QHOST hook in
	InitConProc (argc, argv);
}


/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	INPUT_RECORD	recs[1024];
	int		dummy;
	int		ch, numread, numevents;

//	if (!dedicated || !dedicated->value)
	if (!dedicated || !dedicated->integer)
		return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}
						break;

					case '\b':
						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						}
						break;

					default:
						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text)-2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);	
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}
						break;
				}
			}
		}
	}
	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string)
{
	int		dummy;
	char	text[256];

//	if (!dedicated || !dedicated->value)
	if (!dedicated || !dedicated->integer)
		return;

	if (console_textlen)
	{
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}

void Sys_ShowConsole (qboolean show)
{
	if (!show)
		Sys_RemoveStartupLogo ();
}

//================================================================
#endif // NEW_DED_CONSOLE

/*
================
Sys_Sleep
================
*/
void Sys_Sleep (int msec)
{
	Sleep (msec);
}

/*
================
Sys_TickCount
================
*/
unsigned Sys_TickCount (void)
{
	return GetTickCount();
}

/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();
		sys_msg_time = msg.time;
      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// grab frame time 
	sys_frame_time = timeGetTime();	// FIXME: should this be at start?
}


/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 )
	{
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 )
		{
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) 
			{
				data = malloc( GlobalSize( hClipboardData ) + 1 );
			//	strncpy( data, cliptext );
				Q_strncpyz (data, GlobalSize( hClipboardData ) + 1, cliptext);
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
	return data;
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

#ifndef NEW_DED_CONSOLE
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	CL_Shutdown ();
	Qcommon_Shutdown ();

	va_start (argptr, error);
//	vsprintf (text, error, argptr);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	MessageBox(NULL, text, "Error", 0 /* MB_OK */ );

	if (qwclsemaphore)
		CloseHandle (qwclsemaphore);

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (1);
}
#endif // NEW_DED_CONSOLE


void Sys_Quit (void)
{
	timeEndPeriod( 1 );

	CL_Shutdown();
	Qcommon_Shutdown ();
	CloseHandle (qwclsemaphore);
//	if (dedicated && dedicated->value)
	if (dedicated && dedicated->integer)
		FreeConsole ();

#ifdef NEW_DED_CONSOLE
	Sys_ShutdownConsole();
#else
// shut down QHOST hooks if necessary
	DeinitConProc ();
#endif

	exit (0);
}


void WinError (void)
{
	LPVOID lpMsgBuf;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	// Display the string.
	MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

//================================================================

/*
================
Sys_ScanForCD

================
*/
char *Sys_ScanForCD (void)
{
	static char	cddir[MAX_OSPATH];
	static qboolean	done;
	char		drive[4];
	FILE		*f;
	char		test[MAX_QPATH];
	qboolean	missionpack = false; // Knightmare added
	int			i; // Knightmare added

	if (done)		// don't re-check
		return cddir;

	// no abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	Com_Printf("\nScanning for game CD data path...");

	done = true;

	// Knightmare- check if mission pack gamedir is set
	for (i=0; i<argc; i++)
		if (!strcmp(argv[i], "game") && (i+1<argc))
		{
			if (!strcmp(argv[i+1], "rogue") || !strcmp(argv[i+1], "xatrix"))
				missionpack = true;
			break; // game parameter only appears once in command line
		}

	// scan the drives
	for (drive[0] = 'c' ; drive[0] <= 'z' ; drive[0]++)
	{
		// where activision put the stuff...
		if (missionpack) // Knightmare- mission packs have cinematics in different path
		{
			Com_sprintf (cddir, sizeof(cddir), "%sdata\\max", drive);
			Com_sprintf (test, sizeof(test), "%sdata\\patch\\quake2.exe", drive);
		}
		else
		{
			Com_sprintf (cddir, sizeof(cddir), "%sinstall\\data", drive);
			Com_sprintf (test, sizeof(test), "%sinstall\\data\\quake2.exe", drive);
		}
		f = fopen(test, "r");
		if (f)
		{
			fclose (f);
			if (GetDriveType (drive) == DRIVE_CDROM) {
				Com_Printf(" found %s\n", cddir);
				return cddir;
			}
		}
	}

	Com_Printf(" could not find %s on any CDROM drive!\n", test);

	cddir[0] = 0;
	
	return NULL;
}

//================================================================

/*
=================
Sys_DetectCPU
l33t CPU detection
Borrowed from Q2E
=================
*/
static qboolean Sys_DetectCPU (char *cpuString, int maxSize)
{
#if ( defined (_M_IX86) || defined (_M_X64) || defined (_M_AMD64) || defined (__x86_64__) ) && !defined(__GNUC__)	// [Slipyx] mingw support

	char				vendor[16];
//	int					numLogicalCores=1, numCores=1;
	int					maxExtFunc, stdBits, features, moreFeatures, extFeatures;
	int					family, extFamily, model, extModel, stepping;
	unsigned __int64	start, end, counter, stop, frequency;
	unsigned			speed;
	qboolean			hasMMX, hasMMXExt, has3DNow, has3DNowExt, hasSSE, hasSSE2, hasSSE3, hasSSE41, hasSSE42, hasSSE4a, hasAVX;
	SYSTEM_INFO			sysInfo;

#if defined (_M_X64) || defined (_M_AMD64) || defined (__x86_64__)
	#define rdtsc	__asm __emit 0fh __asm __emit 031h
	int					registers[4];

	// Get vendor identifier
	__cpuid(registers, 0);
	memset(vendor, 0, sizeof(vendor));
	memcpy(vendor+0, &registers[1], sizeof(char)*4);	// ebx
	memcpy(vendor+4, &registers[3], sizeof(char)*4);	// edx
	memcpy(vendor+8, &registers[2], sizeof(char)*4);	// ecx

	// Get standard bits and features
	__cpuid(registers, 1);
	stdBits = registers[0];
	moreFeatures = registers[2];
	features = registers[3];
//	numLogicalCores = (registers[1] >> 16) & 255;

	// Check if extended functions are present
	__cpuid(registers, 0x80000000);
	maxExtFunc = registers[0];

	// Get extended features
	if (maxExtFunc >= 0x80000001)
	{
		__cpuid(registers, 0x80000001);
		extFeatures = registers[3];
	}
/*	
	// Get non-threaded core count
	if (!Q_stricmp(vendor, "AuthenticAMD"))
	{
		if (maxExtFunc >= 0x80000008) {
			__cpuid(registers, 0x80000008);
			numCores = 1 + (registers[2] & 255);
		}
	}
	else if (!Q_stricmp(vendor, "GenuineIntel"))
	{
		__cpuid(registers, 4);
		numCores = 1 + ( (registers[0] >> 26) & 63 );
	}
*/
#elif defined (_M_IX86)
//	int		tempThreads, tempCores;
	// Check if CPUID instruction is supported
	__try {
		__asm {
			mov eax, 0
			cpuid
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}

	// Get CPU info
	__asm {
		; // Get vendor identifier
		mov eax, 0
		cpuid
		mov dword ptr[vendor+0], ebx
		mov dword ptr[vendor+4], edx
		mov dword ptr[vendor+8], ecx
		mov dword ptr[vendor+12], 0

		; // Get standard bits and features
		mov eax, 1
		cpuid
		mov stdBits, eax
		mov moreFeatures, ecx ; // Knightmare added
		mov features, edx
//		mov tempThreads, ebx ; // Knightmare added

		; // Check if extended functions are present
		mov extFeatures, 0
		mov eax, 80000000h
		cpuid
		mov maxExtFunc, eax	;	// Knightmare added
		cmp eax, 80000000h
		jbe NoExtFunction

		; // Get extended features
		mov eax, 80000001h
		cpuid
		mov extFeatures, edx

NoExtFunction:
	}
/*
	numLogicalCores = (tempThreads >> 16) & 255;

	// Get non-threaded core count
	if (!Q_stricmp(vendor, "AuthenticAMD"))
	{
		if (maxExtFunc >= 0x80000008)
		{
			__asm {
				mov eax, 80000008h
				cpuid
				mov tempCores, ecx
			}
			numCores = 1 + (tempCores & 255);
		}
	}
	else if (!Q_stricmp(vendor, "GenuineIntel"))
	{
		__asm {
			mov eax, 4
			cpuid
			mov tempCores, eax
		}
		numCores = 1 + ( (tempCores >> 26) & 63 );
	}
*/
#endif

	// Get CPU name
	family = (stdBits >> 8) & 15;
	model = (stdBits >> 4) & 15;
	if ( (family == 15) || (family == 6) ) {
	//	extFamily = ( ((stdBits >> 20) & 15) << 4 ) + family;
		extFamily = ((stdBits >> 20) & 255) + family;
		extModel = ( ((stdBits >> 16) & 15) << 4 ) + model;
	}
	else {
		extFamily = (stdBits >> 20) & 255;
		extModel = (stdBits >> 16) & 15;
	}
	stepping = (stdBits) & 15;

	if (!Q_stricmp(vendor, "AuthenticAMD"))
	{
		Q_strncpyz(cpuString, maxSize, "AMD");
	//	Com_sprintf(cpuString, maxSize, "AMD Family %i ExtFamily %i Model %i ExtModel %i", family, extFamily, model, extModel);

		switch (family)
		{
		case 4:
			Q_strncatz(cpuString, maxSize, " 5x86");
			break;
		case 5:
			switch (model)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				Q_strncatz(cpuString, maxSize, " K5");
				break;
			case 6:
			case 7:
				Q_strncatz(cpuString, maxSize, " K6");
				break;
			case 8:
				Q_strncatz(cpuString, maxSize, " K6-2");
				break;
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				Q_strncatz(cpuString, maxSize, " K6-III");
				break;
			}
			break;
		case 6: // K7 family
			switch (model)
			{
			case 1:		// 250nm core
			case 2:		// 180nm core
			case 4:		// Thunderbird core
			case 6:		// Palomino core
			case 8:		// Thoroughbred core
			case 10:	// Barton core
				Q_strncatz(cpuString, maxSize, " Athlon");
				break;
			case 3:		// Spitfire core
			case 7:		// Morgan core
				Q_strncatz(cpuString, maxSize, " Duron");
				break;
			default:
				Q_strncatz(cpuString, maxSize, " K7");
				break;
			}
			break;
		case 15: // refer to extended family
			if (extFamily == 0x0F) // K8 family
			{
				switch (model)
				{
			//	case 0:
			//	case 2:
			//	case 6:
				case 4:		// Clawhammer/Newark
				case 7:		// San Diego/Newcastle
				case 12:	// Newcastle/Albany
					Q_strncatz(cpuString, maxSize, " Athlon 64");
					break;
				case 3:		// Toledo
				case 11:	// Manchester/Brisbane
				case 15:	// Winchester/Venice
					Q_strncatz(cpuString, maxSize, " Athlon 64 X2");
					break;
				case 1:
				case 5:
					Q_strncatz(cpuString, maxSize, " Athlon 64 FX / Opteron");
					break;
				default:
					Q_strncatz(cpuString, maxSize, " K8");
					break;
				}
			}
			else if (extFamily == 0x10) // K10 family
			{
				switch (model)
				{
				case 0: // Barcelona A0-A2
				case 2: // Barcelona B0-B3
					Q_strncatz(cpuString, maxSize, " Phenom / Opteron");
					break;
				case 4: // Deneb / Shanghai
				case 10: // Thuban
					Q_strncatz(cpuString, maxSize, " Phenom II / Opteron");
					break;
				case 5: // Propus
				case 6: // Regor
					Q_strncatz(cpuString, maxSize, " Athlon II");
					break;
				default:
					Q_strncatz(cpuString, maxSize, " K10");
					break;
				}
			}
			else if (extFamily == 0x12) // Stars Fusion family
			{
				switch (model)
				{
				case 1:	// Llano
					Q_strncatz(cpuString, maxSize, " A8 APU");
					break;
				default:
					Q_strncatz(cpuString, maxSize, " A series APU");
					break;
				}
			}
			else if (extFamily == 0x14) // Bobcat family
			{
				switch (model)
				{
				case 1:	// Zacate
					Q_strncatz(cpuString, maxSize, " E-350 APU");
					break;
				default:
					Q_strncatz(cpuString, maxSize, " E series APU");
					break;
				}
			}
			else if (extFamily == 0x15) // Bulldozer family
			{
				switch (model)
				{
				case 0:
					if (extModel == 0x10)	// Trinity
						Q_strncatz(cpuString, maxSize, " A series APU");
					else if (extModel == 0x30) // Kaveri
						Q_strncatz(cpuString, maxSize, " A series APU");
					else // Zambezi
						Q_strncatz(cpuString, maxSize, " FX series");
					break;
				case 1:
					if (extModel == 1)	// Zambezi
						Q_strncatz(cpuString, maxSize, " FX series");
					else if (extModel == 0x60)	// Carrizo
						Q_strncatz(cpuString, maxSize, " A series APU");
					break;
				case 2:
					if (extModel == 2) // Zambezi / Vishera
						Q_strncatz(cpuString, maxSize, " FX series");
					break;
				case 3:
					if (extModel == 0x13)	// Richland
						Q_strncatz(cpuString, maxSize, " A series APU");
					break;
				case 5:
					if (extModel == 0x65)	// Bristol Ridge
						Q_strncatz(cpuString, maxSize, " A series APU");
					break;
				default:
					Q_strncatz(cpuString, maxSize, " FX series");
					break;
				}
			}
			else if (extFamily == 0x17)	// Ryzen Family
			{
				switch (model)
				{
				case 0:
					if (extModel == 0x60)	// Renoir
						Q_strncatz(cpuString, maxSize, " Ryzen 7/5/3 4x00G");
					break;
				case 1:
					if (extModel == 1)	// Summit Ridge
						Q_strncatz(cpuString, maxSize, " Ryzen 7/5/3 1x00");
					else if (extModel == 0x11)	// Raven Ridge
						Q_strncatz(cpuString, maxSize, " Ryzen 5/3 2x00G");
					else if (extModel == 0x71)	// Matisse (Zen 2)
						Q_strncatz(cpuString, maxSize, " Ryzen 9/7/5/3 3x00");
					break;
				case 8:
					if (extModel == 8)	// Pinnacle Ridge
						Q_strncatz(cpuString, maxSize, " Ryzen 7/5/3 2x00");
					else if (extModel == 0x18)	// Picasso
						Q_strncatz(cpuString, maxSize, " Ryzen 5/3 3x00G");
					break;
				default:
					Q_strncatz(cpuString, maxSize, " Zen/Zen+/Zen2");
					break;
				}
			}
			else if (extFamily == 0x19)	// Ryzen Zen3 Family
			{
				switch (model)
				{
				case 0:
					if (extModel == 0x50)	// Cezanne
						Q_strncatz(cpuString, maxSize, " Ryzen 7/5/3 5x00G");
					break;
				case 1:
					if (extModel == 0x21)	// Vermeer (Zen3)
						Q_strncatz(cpuString, maxSize, " Ryzen 9/7/5/3 5x00");
					else if (extModel == 0x61)	// Raphael (Zen4)
						Q_strncatz(cpuString, maxSize, " Ryzen 9/7/5/3 7x00");
					break;
				case 4:
					if (extModel == 0x44)	// Rembrandt (Zen3+)
						Q_strncatz(cpuString, maxSize, " Ryzen 9/7/5/3 6x00");
					break;
				default:
					Q_strncatz(cpuString, maxSize, " Zen3");
					break;
				}
			}
			break;
		default: // unknown family
			break;
		}
	}
	else if (!Q_stricmp(vendor, "CyrixInstead"))
	{
		Q_strncpyz(cpuString, maxSize, "Cyrix");

		switch (family)
		{
		case 4:
			Q_strncatz(cpuString, maxSize, " 5x86");
			break;
		case 5:
			switch (model)
			{
			case 2:
				Q_strncatz(cpuString, maxSize, " 6x86");
				break;
			case 4:
				Q_strncatz(cpuString, maxSize, " MediaGX");
				break;
			default:
				Q_strncatz(cpuString, maxSize, " 6x86 / MediaGX");
				break;
			}
			break;
		case 6:
			Q_strncatz(cpuString, maxSize, " 6x86MX");
			break;
		default: // unknown family
			break;
		}
	}
	else if (!Q_stricmp(vendor, "CentaurHauls"))
	{
		Q_strncpyz(cpuString, maxSize, "Centaur");

		switch (family)
		{
		case 5:
			switch (model)
			{
			case 4:
				Q_strncatz(cpuString, maxSize, " C6");
				break;
			case 8:
				Q_strncatz(cpuString, maxSize, " C2");
				break;
			case 9:
				Q_strncatz(cpuString, maxSize, " C3");
				break;
			default: // unknown model
				break;
			}
			break;
		default: // unknown family
			break;
		}
	}
	else if (!Q_stricmp(vendor, "NexGenDriven"))
	{
		Q_strncpyz(cpuString, maxSize, "NexGen");

		switch (family)
		{
		case 5:
			switch (model)
			{
			case 0:
				Q_strncatz(cpuString, maxSize, " Nx586 / Nx586FPU");
				break;
			default: // unknown model
				break;
			}
			break;
		default: // unknown family
			break;
		}
	}
	else if (!Q_stricmp(vendor, "GenuineIntel"))
	{
		Q_strncpyz(cpuString, maxSize, "Intel");
	//	Com_sprintf(cpuString, maxSize, "Intel Family %i ExtFamily %i Model %i ExtModel %i", family, extFamily, model, extModel);

		switch (family)
		{
		case 5: // Pentium family
			switch (model)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 7:
			case 8:
			default:
				Q_strncatz(cpuString, maxSize, " Pentium");
				break;
			}
			break;
		case 6:
			if (model == extModel)	// P6 family, before extModel was used
			{
				switch (model)
				{
				case 0:
				case 1:
					Q_strncatz(cpuString, maxSize, " Pentium Pro");
					break;
				// Actual differentiation depends on cache settings
				case 3:		// Klamath
				case 5:		// Deschutes
					Q_strncatz(cpuString, maxSize, " Pentium II");
					break;
				case 6:
					Q_strncatz(cpuString, maxSize, " Celeron");
					break;
				// Actual differentiation depends on cache settings
				case 7:		// Katmai
				case 8:		// Coppermine
				case 10:	// Coppermine
				case 11:	// Tualatin
					Q_strncatz(cpuString, maxSize, " Pentium III");
					break;
				case 12:	// Silverthorne
					Q_strncatz(cpuString, maxSize, " Atom");
					break;
				case 9:		// Banias
				case 13:	// Dothan
					Q_strncatz(cpuString, maxSize, " Pentium M");
					break;
				case 14:	// Yonah
					Q_strncatz(cpuString, maxSize, " Core");
					break;
				case 15:	// Conroe / Kentsfield
					Q_strncatz(cpuString, maxSize, " Core 2");
					break;
				default:
					Q_strncatz(cpuString, maxSize, " P6");
					break;
				}
			}
			else	// Newer CPUs
			{
				switch (model)
				{
				case 5:
					if (extModel == 0x25)		// Clarkdale / Arrandale
						Q_strncatz(cpuString, maxSize, " Core i5/i3 6xx / Core i3 5xx");
					else if (extModel == 0x45)	// Haswell ULT
						Q_strncatz(cpuString, maxSize, " Core i7/i5/i3 4xxxU");
					else if (extModel == 0x55) {
						if (stepping == 0x4)		// Skylake-X, stepping = 4
							Q_strncatz(cpuString, maxSize, " Core i9/i7 79xx/78xx or 99xx/98xx");
						else if (stepping == 0x7)	// Cascade Lake-X, stepping = 7
							Q_strncatz(cpuString, maxSize, " Core i9 10xxx");
					}
					else if (extModel == 0xA5)	// Comet Lake
						Q_strncatz(cpuString, maxSize, " Core i9/i7/i5/i3 10xxx");
					break;
				case 7:		
					if (extModel == 0x17)		// Wolfdale / Yorkfield (Penryn)
						Q_strncatz(cpuString, maxSize, " Core 2");
					else if (extModel == 0x47)	// Broadwell
						Q_strncatz(cpuString, maxSize, " Core i7/i5 5xxx");
					else if (extModel == 0x97)	// Alder Lake
						Q_strncatz(cpuString, maxSize, " Core i9/i7/i5/i3 12xxx");
					else if (extModel == 0xA7)	// Rocket Lake
						Q_strncatz(cpuString, maxSize, " Core i9/i7/i5/i3 11xxx");
					else if (extModel == 0xB7)	// Raptor Lake
						Q_strncatz(cpuString, maxSize, " Core i9/i7/i5/i3 13xxx");
					break;
				case 10:	
					if (extModel == 0x2A)		// Sandy Bridge
						Q_strncatz(cpuString, maxSize, "  Core i7/i5/i3 2xxx");
					else if (extModel == 0x3A)	// Ivy Bridge
						Q_strncatz(cpuString, maxSize, "  Core i7/i5/i3 3xxx");
					else if (extModel == 0x1A)	// Bloomfield
						Q_strncatz(cpuString, maxSize, " Core i7 9xx");
					else if (extModel == 0x7A)	// Gemini Lake
						Q_strncatz(cpuString, maxSize, " Pentium J4xxx");
					break;
				case 12:
					if (extModel == 0x2C)		// Gulftown
						Q_strncatz(cpuString, maxSize, " Core i7 9xx");
					else if (extModel == 0x3C)	// Haswell
						Q_strncatz(cpuString, maxSize, "  Core i7/i5/i3 4xxx");
					else if (extModel == 0x8C)	// Tiger Lake
						Q_strncatz(cpuString, maxSize, "  Core i7/i5/i3 11xxG7");
					else						// Silverthorne
						Q_strncatz(cpuString, maxSize, " Atom");
					break;
				case 13:	
					if (extModel == 0x2D)		// Sandy Bridge-E
						Q_strncatz(cpuString, maxSize, " Core i7 39xx / 38xx");
					break;
				case 14:
					if (extModel == 0x1E)		// Lynnfield
						Q_strncatz(cpuString, maxSize, " Core i7 8xx / Core i5 7xx");
					else if (extModel == 0x3E)	// Ivy Bridge-E
						Q_strncatz(cpuString, maxSize, " Core i7 49xx / 48xx");
					else if (extModel == 0x4E)	// Skylake U/Y
						Q_strncatz(cpuString, maxSize, " Core i7/i5/i3 6xxxU");
					else if (extModel == 0x5E)	// Skylake
						Q_strncatz(cpuString, maxSize, " Core i7/i5/i3 6xxx");
					else if (extModel == 0x7E)	// Ice lake
						Q_strncatz(cpuString, maxSize, " Core i7/i5/i3 10xxG7");
					else if (extModel == 0x8E)	// Kaby Lake U/Y
						Q_strncatz(cpuString, maxSize, " Core i7/i5/i3 7xxxU");
					else if (extModel == 0x9E) {
						if (stepping == 0x9)		// Kaby Lake, stepping = 9
							Q_strncatz(cpuString, maxSize, " Core i7/i5/i3 7xxx");
						else if (stepping == 0xA)	// Coffee Lake, stepping = 10
							Q_strncatz(cpuString, maxSize, " Core i7/i5/i3 8xxx");
						else if (stepping == 0xC)	// Coffee Lake refresh, stepping = 12
							Q_strncatz(cpuString, maxSize, " Core i9/i7/i5/i3 9xxx");
					}
					break;
				case 15:	
					if (extModel == 0x0F)		// Conroe / Kentsfield (Merom)
						Q_strncatz(cpuString, maxSize, " Core 2");
					else if (extModel == 0x3F)	// Haswell-E
						Q_strncatz(cpuString, maxSize, " Core i7 59xx / 58xx");
					else if (extModel == 0x4F)	// Broadwell-E
						Q_strncatz(cpuString, maxSize, " Core i7 69xx / 68xx");
					break;
				default:
					break;
				}
			}
			break;
		case 7: // Itanium family
			switch (model)
			{
			default:
				Q_strncatz(cpuString, maxSize, " Itanium");
				break;
			}
			break;
		case 15: // refer to extended family
			if (extFamily == 0x0F) // NetBurst family
			{
				switch (model)
				{
				case 0:		// Williamette
				case 1:		// Williamette
				case 2:		// Northwood
				case 3:		// Prescott
				case 4:		// Smithfield
				case 6:		// Cedar Mill / Presler
					Q_strncatz(cpuString, maxSize, " Pentium 4");
					break;
			//	case 4:		// Smithfield
			//	case 6:		// Cedar Mill / Presler
			//		Q_strncatz(cpuString, " Pentium D", maxSize);
			//		break;
				default:
					Q_strncatz(cpuString, maxSize, " NetBurst");
					break;
				}
			}
			else if (extFamily == 0x1F || extFamily == 0x2F) // Itanium 2 family
			{
				switch (model)
				{
				default:
					Q_strncatz(cpuString, maxSize, " Itanium 2");
					break;
				}
			}
			break;
		default: // unknown family
			break;
		}
	}
	else
		return false;

	// Check if RDTSC instruction is supported
	if ((features >> 4) & 1)
	{
		// Measure CPU speed
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);

#if defined (_M_X64) || defined (_M_AMD64) || defined (__x86_64__)
		start = __rdtsc();
#elif defined (_M_IX86)
		__asm {
			rdtsc
			mov dword ptr[start+0], eax
			mov dword ptr[start+4], edx
		}
#endif

		QueryPerformanceCounter((LARGE_INTEGER *)&stop);
		stop += frequency;

		do {
			QueryPerformanceCounter((LARGE_INTEGER *)&counter);
		} while (counter < stop);

#if defined (_M_X64) || defined (_M_AMD64) || defined (__x86_64__)
		end = __rdtsc();
#elif defined (_M_IX86)
		__asm {
			rdtsc
			mov dword ptr[end+0], eax
			mov dword ptr[end+4], edx
		}
#endif

		speed = (unsigned)((end - start) / 1000000);

		if (speed > 1000)
			Q_strncatz(cpuString, maxSize, va(" %4.2f GHz", ((float)speed/1000.0f)));
		else
			Q_strncatz(cpuString, maxSize, va(" %u MHz", speed));
	}

	// get number of logical processors
	GetSystemInfo(&sysInfo);
	if (sysInfo.dwNumberOfProcessors > 1)
		Q_strncatz(cpuString, maxSize, va(" (%u logical CPUs)", sysInfo.dwNumberOfProcessors));
/*
	if (numLogicalCores >= 2 || numCores >= 2)
	{
		if (numLogicalCores > numCores)	// Hyperthreading or SMT
			Q_strncatz(cpuString, maxSize, va(" (%u cores, %u threads)", numCores, numLogicalCores));
		else
			Q_strncatz(cpuString, maxSize, va(" (%u cores)", numCores));
	}
*/

	// Get extended instruction sets supported
	hasMMX = (features >> 23) & 1;
	hasMMXExt = (extFeatures >> 22) & 1;
	has3DNow = (extFeatures >> 31) & 1;
	has3DNowExt = (extFeatures >> 30) & 1;
	hasSSE = (features >> 25) & 1;
	hasSSE2 = (features >> 26) & 1;
	hasSSE3 = (moreFeatures >> 0) & 1;
	hasSSE41 = (moreFeatures >> 19) & 1;
	hasSSE42 = (moreFeatures >> 20) & 1;
	hasSSE4a = (moreFeatures >> 6) & 1;
	hasAVX = (moreFeatures >> 28) & 1;

	if (hasMMX || has3DNow || hasSSE)
	{
		Q_strncatz(cpuString, maxSize, " w/");

		if (hasMMX){
			Q_strncatz(cpuString, maxSize, " MMX");
			if (hasMMXExt)
				Q_strncatz(cpuString, maxSize, "+");
		}
		if (has3DNow){
			Q_strncatz(cpuString, maxSize, " 3DNow!");
			if (has3DNowExt)
				Q_strncatz(cpuString, maxSize, "+");
		}
		if (hasSSE){
			Q_strncatz(cpuString, maxSize, " SSE");
			if (hasSSE42)
				Q_strncatz(cpuString, maxSize, "4.2");
			else if (hasSSE41)
				Q_strncatz(cpuString, maxSize, "4.1");
			else if (hasSSE3)
				Q_strncatz(cpuString, maxSize, "3");
			else if (hasSSE2)
				Q_strncatz(cpuString, maxSize, "2");
		}
		if (hasSSE4a){
			Q_strncatz(cpuString, maxSize, " SSE4a");
		}
		if (hasAVX){
			Q_strncatz(cpuString, maxSize, " AVX");
		}
	}

	return true;

#else

//	Q_strncpyz(cpuString, maxSize, "Alpha AXP");
	Q_strncpyz(cpuString, maxSize, CPUSTRING); // [Slipyx] mingw support
	return true;
#endif
}


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	OSVERSIONINFOEX	osInfo;
	SYSTEM_INFO		sysInfo;
	MEMORYSTATUS	memStatus;
	char			string[128];
#if (_MSC_VER >= 1300)
	MEMORYSTATUSEX	memStatusEX;
	char			stringEx[128];
#endif

#if 0
	// allocate a named semaphore on the client so the
	// front end can tell if it is alive

	// mutex will fail if semephore already exists
    qwclsemaphore = CreateMutex(
        NULL,         /* Security attributes */
        0,            /* owner       */
        "qwcl"); /* Semaphore name      */
	if (!qwclsemaphore)
		Sys_Error ("QWCL is already running on this system");
	CloseHandle (qwclsemaphore);

    qwclsemaphore = CreateSemaphore(
        NULL,         /* Security attributes */
        0,            /* Initial count       */
        1,            /* Maximum count       */
        "qwcl"); /* Semaphore name      */
#endif

	timeBeginPeriod( 1 );

	osInfo.dwOSVersionInfoSize = sizeof(osInfo);

	if (!GetVersionEx ((OSVERSIONINFO*) &osInfo))
		Sys_Error ("Couldn't get OS info");

	GetSystemInfo(&sysInfo);

	if (osInfo.dwMajorVersion < 4)
		Sys_Error ("KMQuake2 requires windows version 4 or greater");
	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("KMQuake2 doesn't run on Win32s");
	else if ( osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
		s_win95 = true;

	// from Q2E - OS & CPU detection
	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		
// wProductType field not supported in MSVC6
#if (_MSC_VER < 1300)
		if (osInfo.dwMajorVersion == 4) {
			Q_strncpyz (string, sizeof(string), "Windows NT");
		}
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 0) {
			Q_strncpyz (string, sizeof(string), "Windows 2000");
		}
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 1) {
			Q_strncpyz (string, sizeof(string), "Windows XP");
		}
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 2) {
			Q_strncpyz (string, sizeof(string), "Windows XP");
		}
		else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 0) {
			Q_strncpyz (string, sizeof(string), "Windows Vista");
		}
		else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 1) {
			Q_strncpyz (string, sizeof(string), "Windows 7");
		}
		else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 2) {
			Q_strncpyz (string, sizeof(string), "Windows 8");
		}
		else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 3) {
			Q_strncpyz (string, sizeof(string), "Windows 8.1");
		}
		else if (osInfo.dwMajorVersion == 10 && osInfo.dwMinorVersion == 0) {
			if (osInfo.dwBuildNumber >= 22000)
				Q_strncpyz (string, sizeof(string), "Windows 11");
			else
				Q_strncpyz (string, sizeof(string), "Windows 10");
		}
#else	// (_MSC_VER < 1300)
		if (osInfo.dwMajorVersion == 4) {
			Q_strncpyz (string, sizeof(string), "Windows NT");
		}
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 0) {
			if (osInfo.wProductType == VER_NT_WORKSTATION)
				Q_strncpyz (string, sizeof(string), "Windows 2000");
			else
				Q_strncpyz (string, sizeof(string), "Windows 2000 Server");
		}
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 1) {
			Q_strncpyz (string, sizeof(string), "Windows XP");
		}
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 2) {
		//	if ( (osInfo.wProductType == VER_NT_WORKSTATION) && (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) )
		//		Q_strncpyz (string, "Windows XP x64 Edition", sizeof(string));
			if (osInfo.wProductType == VER_NT_WORKSTATION)
				Q_strncpyz (string, sizeof(string), "Windows XP");
			else
				Q_strncpyz (string, sizeof(string), "Windows Server 2003");
		}
		else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 0) {
			if (osInfo.wProductType == VER_NT_WORKSTATION)
				Q_strncpyz (string, sizeof(string), "Windows Vista");
			else
				Q_strncpyz (string, sizeof(string), "Windows Server 2008");
		}
		else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 1) {
			if (osInfo.wProductType == VER_NT_WORKSTATION)
				Q_strncpyz (string, sizeof(string), "Windows 7");
			else
				Q_strncpyz (string, sizeof(string), "Windows Server 2008 R2");
		}
		else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 2) {
			if (osInfo.wProductType == VER_NT_WORKSTATION)
				Q_strncpyz (string, sizeof(string), "Windows 8");
			else
				Q_strncpyz (string, sizeof(string), "Windows Server 2012");
		}
		else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 3) {
			if (osInfo.wProductType == VER_NT_WORKSTATION)
				Q_strncpyz (string, sizeof(string), "Windows 8.1");
			else
				Q_strncpyz (string, sizeof(string), "Windows Server 2012 R2");
		}
		else if (osInfo.dwMajorVersion == 10 && osInfo.dwMinorVersion == 0) {
			if (osInfo.wProductType == VER_NT_WORKSTATION) {
				if (osInfo.dwBuildNumber >= 22000)
					Q_strncpyz (string, sizeof(string), "Windows 11");
				else
					Q_strncpyz (string, sizeof(string), "Windows 10");
			}
			else {
				if (osInfo.dwBuildNumber == 17763)
					Q_strncpyz (string, sizeof(string), "Windows Server 2019");
				else
					Q_strncpyz (string, sizeof(string), "Windows Server 2016");
			}
		}
#endif	// (_MSC_VER < 1300)
		else {
			Q_strncpyz (string, sizeof(string), va("Windows %i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion));
		}

		if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			Q_strncatz (string, sizeof(string), " x64");
		if (strlen(osInfo.szCSDVersion) > 0)
			Q_strncatz (string, sizeof(string), va(" %s", osInfo.szCSDVersion));
	}
	else if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 0) {
			if (osInfo.szCSDVersion[1] == 'C' || osInfo.szCSDVersion[1] == 'B')
				Q_strncpyz (string, sizeof(string), "Windows 95 OSR2");
			else
				Q_strncpyz (string, sizeof(string), "Windows 95");
		}
		else if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 10) {
			if (osInfo.szCSDVersion[1] == 'A')
				Q_strncpyz (string, sizeof(string), "Windows 98 SE");
			else
				Q_strncpyz (string, sizeof(string), "Windows 98");
		}
		else if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 90)
			Q_strncpyz (string, sizeof(string), "Windows ME");
		else
			Q_strncpyz (string, sizeof(string), va("Windows %i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion));
	}
	else
		Q_strncpyz (string, sizeof(string), va("Windows %i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion));

	if (osInfo.dwBuildNumber > 0)
		Q_strncatz (string, sizeof(string), va(", build %i", osInfo.dwBuildNumber));
	Com_Printf ("OS: %s\n", string);
	Cvar_Get ("sys_osVersion", string, CVAR_NOSET|CVAR_LATCH|CVAR_SAVE_IGNORE);

	// Detect CPU
	Com_Printf ("Detecting CPU... ");
	if (Sys_DetectCPU (string, sizeof(string))) {
		Com_Printf ("Found %s\n", string);
		Cvar_Get ("sys_cpuString", string, CVAR_NOSET|CVAR_LATCH|CVAR_SAVE_IGNORE);
	}
	else {
		Com_Printf ("Unknown CPU found\n");
		Cvar_Get ("sys_cpuString", "Unknown", CVAR_NOSET|CVAR_LATCH|CVAR_SAVE_IGNORE);
	}

	// Get physical memory
	GlobalMemoryStatus (&memStatus);
#if (_MSC_VER >= 1300)
	memStatusEX.dwLength = sizeof(memStatusEX);
	GlobalMemoryStatusEx (&memStatusEX);
	Q_strncpyz (stringEx, sizeof(stringEx), va("%u", (memStatusEX.ullTotalPhys >> 20)));
	Q_strncpyz (string, sizeof(string), va("%u", (memStatus.dwTotalPhys >> 20)));
	Com_Printf ("Memory: %s MB (%s MB accessible)\n", stringEx, string);
	Cvar_Get ("sys_ramMegs", stringEx, CVAR_NOSET|CVAR_LATCH|CVAR_SAVE_IGNORE);
	Cvar_Get ("sys_ramMegs_perApp", string, CVAR_NOSET|CVAR_LATCH|CVAR_SAVE_IGNORE);
#else
	Q_strncpyz (string, sizeof(string), va("%u", (memStatus.dwTotalPhys >> 20)));
	Com_Printf ("Memory: %s MB\n", string);
	Cvar_Get ("sys_ramMegs", string, CVAR_NOSET|CVAR_LATCH|CVAR_SAVE_IGNORE);
	Cvar_Get ("sys_ramMegs_avail", string, CVAR_NOSET|CVAR_LATCH|CVAR_SAVE_IGNORE);
#endif
// end Q2E detection

#ifndef NEW_DED_CONSOLE
	Sys_InitConsole (); // show dedicated console, moved to function
#endif
}


/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
	ShowWindow ( cl_hwnd, SW_RESTORE);
	SetForegroundWindow ( cl_hwnd );
}

/*
========================================================================

GENERIC DLL LOADING

From Yamagi Q2

========================================================================
*/

/*
=================
Sys_LoadLibrary
=================
*/
void *Sys_LoadLibrary (const char *libPath, const char *initFuncName, void **libHandle)
{
//	HINSTANCE	hLibrary;
	HMODULE		hLibrary;
	WCHAR		wLibPath[MAX_OSPATH];
	void		*funcPtr;

	if ( !libPath || (libPath[0] == '\0') || !libHandle )	// catch bad pointers/path
		return NULL;	

	*libHandle = NULL;

	MultiByteToWideChar (CP_UTF8, 0, libPath, -1, wLibPath, MAX_OSPATH);
	hLibrary = LoadLibraryW (wLibPath);
//	hLibrary = LoadLibrary (libPath);

	if ( !hLibrary ) {
		Com_DPrintf ("Sys_LoadLibrary: failure on %s, LoadLibrary returned %lu\n", libPath, GetLastError());
		return NULL;
	}

	if (initFuncName != NULL)
	{
		funcPtr = GetProcAddress (hLibrary, initFuncName);

		if ( !funcPtr ) {
			Com_DPrintf ("Sys_LoadLibrary: failure in %s on %s, GetProcAddress returned %lu\n", libPath, initFuncName, GetLastError());
			FreeLibrary (hLibrary);
			return NULL;
		}
	}
	else {
		funcPtr = NULL;
	}

	*libHandle = hLibrary;

	Com_DPrintf ("Sys_LoadLibrary: sucessfully loaded %s\n", libPath);

	return funcPtr;
}


/*
=================
Sys_FreeLibrary
=================
*/
void Sys_FreeLibrary (void *libHandle)
{
	if (!libHandle)
		return;

	if ( !FreeLibrary(libHandle) ) {
		Com_Error (ERR_FATAL, "FreeLibrary failed for %p", libHandle);
	}
}


/*
=================
Sys_GetProcAddress
=================
*/
void *Sys_GetProcAddress (void *libHandle, const char *funcName)
{
	return GetProcAddress (libHandle, funcName);
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE	game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (!FreeLibrary (game_library))
		Com_Error (ERR_FATAL, "FreeLibrary failed for game library");
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
	char	*path;
	char	cwd[MAX_OSPATH];

#if defined (_M_X64) || defined (_M_AMD64) || defined (__x86_64__)
	const char *gamename = "kmq2gamex64.dll"; 

#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

#elif defined (_M_IX86)
	// Knightmare- changed DLL name for better cohabitation
	const char *gamename = "kmq2gamex86.dll"; 

#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

#elif defined _M_ALPHA
	const char *gamename = "kmq2gameaxp.dll";

#ifdef NDEBUG
	const char *debugdir = "releaseaxp";
#else
	const char *debugdir = "debugaxp";
#endif

#endif

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	// check the current debug directory first for development purposes
	_getcwd (cwd, sizeof(cwd));
	Com_sprintf (name, sizeof(name), "%s/%s/%s", cwd, debugdir, gamename);
	game_library = LoadLibrary ( name );
	if (game_library)
	{
		Com_DPrintf ("LoadLibrary (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof(name), "%s/%s", cwd, gamename);
		game_library = LoadLibrary ( name );
		if (game_library)
		{
			Com_DPrintf ("LoadLibrary (%s)\n", name);
		}
		else
#endif
		{
			// now run through the search paths
			path = NULL;
			while (1)
			{
			//	path = FS_NextPath (path);
				path = FS_NextGamePath (path);
				if (!path)
					return NULL;		// couldn't find one anywhere
				Com_sprintf (name, sizeof(name), "%s/%s", path, gamename);
				game_library = LoadLibrary (name);
				if (game_library)
				{
					Com_DPrintf ("LoadLibrary (%s)\n",name);
					break;
				}
			}
		}
	}

	GetGameAPI = (void *)GetProcAddress (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
}

//=======================================================================


/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine (LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
		}
	}
}

// Knightmare- startup logo, code from TomazQuake
#ifndef NEW_DED_CONSOLE
HWND		hwnd_dialog; // Knightmare added
qboolean	logoRemoved = false;

void Sys_CreateStartupLogo (void)
{
//	if (!(dedicated && dedicated->integer))
	{
		hwnd_dialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);
		RECT			rect; // Knightmare added

		if (hwnd_dialog)
		{
			if (GetWindowRect (hwnd_dialog, &rect))
			{
				if (rect.left > (rect.top * 2))
				{
					SetWindowPos (hwnd_dialog, 0, (rect.left/2) - ((rect.right - rect.left)/2), rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
				}
			}

			ShowWindow (hwnd_dialog, SW_SHOWDEFAULT);
			UpdateWindow (hwnd_dialog);
			SetForegroundWindow (hwnd_dialog);
		}
	}
	// end Knightmare
}

void Sys_RemoveStartupLogo (void)
{
	if (!logoRemoved) {
		DestroyWindow (hwnd_dialog);
		logoRemoved = true;
	}
}
#endif	// NEW_DED_CONSOLE


/*
==================
ReplaceBackSlashes

Replaces backslashes in a path with slashes.
==================
*/
static void ReplaceBackSlashes (char *path)
{
	char		*cur, *old;

	cur = old = path;
	if (strstr(cur, "\\") != NULL)
	{
		while (cur != NULL)
		{
			if ((cur - old) > 1) {
				*cur = '/';
			}
			old = cur;
			cur = strchr(old + 1, '\\');
		}
	}
}


/*
==================
Sys_ExeDir
==================
*/
const char *Sys_ExeDir (void)
{
	return exe_dir;
}


/*
==================
Sys_PrefDir
==================
*/
const char *Sys_PrefDir (void)
{
	return pref_dir;
}


/*
==================
Sys_DownloadDir
==================
*/
const char *Sys_DownloadDir (void)
{
	return download_dir;
}


/*
==================
Sys_InitPrefDir

Adapted from DK 1.3 source
Must be called AFTER cvars are initialized in FS_InitFilesystem()
==================
*/
void Sys_InitPrefDir (void)
{
	if ( win_use_profile_dir && win_use_profile_dir->integer )
	{
		char		profile[MAX_PATH], dlPath[MAX_PATH];
		const char	*reason = "No error!";
		const char	*reason_dl = "No error!";
		int			len, len_dl;
		qboolean	bGotNT6SavedGames=false, bGotNT6Downloads=false;

		// Use Saved Games/KMQuake2 on Win Vista and later, unless "mygames" parameter is set
		if ( Detect_WinNT6orLater() && !COM_CheckParm ("-mygames") && !COM_CheckParm ("+mygames")  )
		{
			WCHAR		*wprofile, *wDLPath;
			HMODULE		hShell32 = LoadLibrary("shell32");
			HMODULE		hOle32 = LoadLibrary("ole32");

			if ( !hShell32 || !hOle32 ) {
				reason = reason_dl = "shell32.dll or ole32.dll couldn't be loaded";
			}
			else
			{
				fnSHGetKnownFolderPath = (void *)GetProcAddress (hShell32, "SHGetKnownFolderPath");
				fnCoInitializeEx = (void *)GetProcAddress (hOle32, "CoInitializeEx");
				fnCoUninitialize = (void *)GetProcAddress (hOle32, "CoUninitialize");
				fnCoTaskMemFree =  (void *)GetProcAddress (hOle32, "CoTaskMemFree");
				if ( !fnSHGetKnownFolderPath || !fnCoInitializeEx || !fnCoUninitialize || !fnCoTaskMemFree ) {
					reason = reason_dl = "functions SHGetKnownFolderPath / CoInitializeEx / CoUninitialize / CoTaskMemFree couldn't be mapped";
				}
				else
				{
					// from DarkPlaces
					memset (profile, 0, sizeof(profile));
					fnCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
					if (fnSHGetKnownFolderPath(&qFOLDERID_SavedGames, qKF_FLAG_CREATE | qKF_FLAG_NO_ALIAS, NULL, &wprofile) == S_OK)
					{
						memset(profile, 0, sizeof(profile));
				#if _MSC_VER >= 1400
						wcstombs_s(NULL, profile, sizeof(profile), wprofile, sizeof(profile)-1);
				#else
						wcstombs(profile, wprofile, sizeof(profile)-1);
				#endif
						fnCoTaskMemFree(wprofile);
					}
					// Also get Downloads Folder from qFOLDERID_Downloads
					if (fnSHGetKnownFolderPath(&qFOLDERID_Downloads, qKF_FLAG_CREATE | qKF_FLAG_NO_ALIAS, NULL, &wDLPath) == S_OK)
					{
						memset(dlPath, 0, sizeof(dlPath));
				#if _MSC_VER >= 1400
						wcstombs_s(NULL, dlPath, sizeof(dlPath), wDLPath, sizeof(dlPath)-1);
				#else
						wcstombs(dlPath, wDLPath, sizeof(dlPath)-1);
				#endif
						fnCoTaskMemFree(wDLPath);
					}
					fnCoUninitialize();
					// end DarkPlaces code

					len = (int)strlen(profile);
					if (len == 0) {
						reason = "SHGetKnownFolderPath()/wcstombs() returned 0 length string";
					}
					else
					{
						// Check if path is too long
						if ( (len + strlen(NT6_SAVEDIR) + 3) >= 256 ) {
							reason = "the resulting path would be too long (>= 256 chars)";
						}
						else
						{
							// Replace backslashes with slashes
							ReplaceBackSlashes (profile);
							Com_sprintf (pref_dir, sizeof(pref_dir), "%s/%s", profile, NT6_SAVEDIR);
							bGotNT6SavedGames = true;
						//	return;
						}
					}

					len_dl = (int)strlen(dlPath);
					if (len_dl == 0) {
						reason_dl = "SHGetKnownFolderPath()/wcstombs() returned 0 length string";
					}
					else
					{
						// Check if path is too long
						if ( (len_dl + strlen(NT6_DLDIR) + 3) >= 256 ) {
							reason_dl = "the resulting path would be too long (>= 256 chars)";
						}
						else
						{
							// Replace backslashes with slashes
							ReplaceBackSlashes (dlPath);
							Com_sprintf (download_dir, sizeof(download_dir), "%s/%s", dlPath, NT6_DLDIR);
							bGotNT6Downloads = true;
						//	return;
						}
					}
				}
			}
			if (bGotNT6SavedGames && bGotNT6Downloads)	// successfully got both dirs, return now
				return;

			// Output reason for why one or both dirs couldn't be found, then fall back on NT5 path
			if (!bGotNT6SavedGames)
				Com_Printf("Couldn't get PrefDir (Saved Games/KMQuake2), because %s.\n", reason);
			if (!bGotNT6Downloads)
				Com_Printf("Couldn't get DownloadDir (Downloads/KMQuake2), because %s.\n", reason_dl);
		}

		// Use My Documents/My Games/KMQuake2 on Win2K / XP
		if ( Detect_WinNT5orLater() )
		{
			WCHAR		sprofile[MAX_PATH];
			WCHAR		uprofile[MAX_PATH];
			HMODULE		hShell32 = LoadLibrary("shell32");

			if (!hShell32) {
				reason = "shell32.dll couldn't be loaded";
			}
			else
			{
				fnSHGetFolderPathW = (SHGETFOLDERPATHW)GetProcAddress (hShell32, "SHGetFolderPathW");
				if (!fnSHGetFolderPathW) {
					reason = "function SHGetFolderPathW couldn't be mapped";
				}
				else
				{
					memset (pref_dir, 0, sizeof(pref_dir));

					/* The following lines implement a horrible
					   hack to connect the UTF-16 WinAPI to the
					   ASCII Quake II. While this should work in
					   most cases, it'll fail if the "Windows to
					   DOS filename translation" is switched off.
					   In that case the function will return NULL
					   and no homedir is used. */

					// Get path to "My Documents" folder
					fnSHGetFolderPathW (NULL, CSIDL_PERSONAL, NULL, 8, uprofile);

					// Create a UTF-16 DOS path
					len = GetShortPathNameW (uprofile, sprofile, sizeof(sprofile));

					if (len == 0) {
						reason = "GetShortPathNameW() returned 0";
					}
					else
					{
						// Since the DOS path contains no UTF-16 characters, just convert it to ASCII
						len = WideCharToMultiByte (CP_ACP, 0, sprofile, -1, profile, sizeof(profile), NULL, NULL);

						if (len == 0) {
							reason = "WideCharToMultiByte() returned 0";
						}
						else
						{
							// Check if path is too long
							if ( ((len + strlen(NT5_SAVEDIR) + 3) >= 256) || ((len + strlen(NT5_DLDIR) + 3) >= 256) ) {
								reason = "The resulting path would be too long (>= 256 chars)";
							}
							else
							{
								// Replace backslashes with slashes
								ReplaceBackSlashes (profile);
								// Allow splitting of dirs from above NT6 section if only one failed
								if (!bGotNT6SavedGames)
									Com_sprintf (pref_dir, sizeof(pref_dir), "%s/%s", profile, NT5_SAVEDIR);
								if (!bGotNT6Downloads)
									Com_sprintf (download_dir, sizeof(download_dir), "%s/%s", profile, NT5_DLDIR);
								return;
							}
						}
					}
				}
			}
			Com_Printf("Couldn't get PrefDir (My Documents/My Games/KMQuake2), because %s.\n", reason);
		}
	}

	Q_strncpyz (pref_dir, sizeof(pref_dir), fs_basedir->string);
	Q_strncpyz (download_dir, sizeof(download_dir), fs_basedir->string);
}


/*
==================
Init_ExeDir
==================
*/
static void Init_ExeDir (void)
{
#if 1
	memset(exe_dir, 0, sizeof(exe_dir));
	Q_snprintfz (exe_dir, sizeof(exe_dir), ".");
#else
	char		buf[MAX_PATH];
	const char	*lastSlash;
	int			dirLen;

	memset(buf, 0, sizeof(buf));
	memset(exe_dir, 0, sizeof(exe_dir));

	GetModuleFileName (NULL, buf, sizeof(buf)-1);

	// get path up to last backslash
	lastSlash = strrchr(buf, '\\');
	if (lastSlash == NULL)
		lastSlash = strrchr(buf, '/');

	dirLen = lastSlash ? (lastSlash - buf) : 0;
	if ( lastSlash == NULL || dirLen == 0 || dirLen >= sizeof(exe_dir) ) {
		Q_snprintfz (exe_dir, sizeof(exe_dir), ".");
	}
	else {
		memcpy(exe_dir, buf, dirLen);
	}
#endif
}


/*
==================
FixWorkingDirectory
==================
*/
void FixWorkingDirectory (void)
{
	int		i;
	char	*p;
	char	curDir[MAX_PATH];

	GetModuleFileName (NULL, curDir, sizeof(curDir)-1);

	p = strrchr (curDir, '\\');
	p[0] = 0;

	for (i = 1; i < argc; i++)
	{
		if (!strcmp (argv[i], "-nopathcheck"))
			goto skipPathCheck;

		if (!strcmp (argv[i], "-nocwdcheck"))
			return;
	}

	if (strlen(curDir) > (MAX_OSPATH - MAX_QPATH))
		Sys_Error ("Current path is too long. Please move your Quake II installation to a shorter path.");

skipPathCheck:

	SetCurrentDirectory (curDir);
}


/*
==================
Detect_WinNT5orLater
==================
*/
qboolean Detect_WinNT5orLater (void)
{
	DWORD	WinVersion;
	DWORD	WinLowByte, WinHighByte;

	WinVersion = GetVersion();
	WinLowByte = (DWORD)(LOBYTE(LOWORD(WinVersion)));
	WinHighByte = (DWORD)(HIBYTE(HIWORD(WinVersion)));

	if (WinLowByte <= 4) {
		Com_DPrintf("Windows 9x or NT 4 detected.\n");
		return false;
	}

	if (WinLowByte >= 5) {
		Com_DPrintf("Windows 5.x or later detected.\n");
		return true;
	}

	return false;
}


/*
==================
Detect_WinNT6orLater
==================
*/
qboolean Detect_WinNT6orLater (void)
{
	DWORD	WinVersion;
	DWORD	WinLowByte, WinHighByte;

	WinVersion = GetVersion();
	WinLowByte = (DWORD)(LOBYTE(LOWORD(WinVersion)));
	WinHighByte = (DWORD)(HIBYTE(HIWORD(WinVersion)));

	if (WinLowByte <= 4) {
		Com_DPrintf("Windows 9x or NT 4 detected.\n");
		return false;
	}

	if (WinLowByte == 5) {
		Com_DPrintf("Windows 5.x (Win2K / XP / Server 2003) detected.\n");
		return false;
	}

	if (WinLowByte >= 6) {
		Com_DPrintf("Windows 6.x (WinVista / 7 / 8) detected.\n");
		return true;
	}

	return false;
}


/*
==================
Sys_SetHighDPIMode

From Yamagi Quake2
==================
*/
typedef enum KMQ2_PROCESS_DPI_AWARENESS {
	KMQ2_PROCESS_DPI_UNAWARE = 0,
	KMQ2_PROCESS_SYSTEM_DPI_AWARE = 1,
	KMQ2_PROCESS_PER_MONITOR_DPI_AWARE = 2
} KMQ2_PROCESS_DPI_AWARENESS;

void Sys_SetHighDPIMode (void)
{
	HINSTANCE userDLL, shcoreDLL;

	/* For Vista, Win7 and Win8 */
	BOOL(WINAPI *SetProcessDPIAware)(void) = NULL;

	/* Win8.1 and later */
	HRESULT(WINAPI *SetProcessDpiAwareness)(KMQ2_PROCESS_DPI_AWARENESS dpiAwareness) = NULL;

	userDLL = LoadLibrary("USER32.DLL");
	if (userDLL)
	{
		SetProcessDPIAware = (BOOL(WINAPI *)(void)) GetProcAddress(userDLL,
				"SetProcessDPIAware");
	}

	shcoreDLL = LoadLibrary("SHCORE.DLL");
	if (shcoreDLL)
	{
		SetProcessDpiAwareness = (HRESULT(WINAPI *)(KMQ2_PROCESS_DPI_AWARENESS))
			GetProcAddress(shcoreDLL, "SetProcessDpiAwareness");
	}

	if (SetProcessDpiAwareness) {
		SetProcessDpiAwareness(KMQ2_PROCESS_PER_MONITOR_DPI_AWARE);
	}
	else if (SetProcessDPIAware) {
		SetProcessDPIAware();
	}
}


/*
==================
WinMain
==================
*/
HINSTANCE	global_hInstance;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG				msg;
	int				time, oldtime, newtime;
	char			*cddir;
	int				i; // Knightmare added
	qboolean		cdscan = false; // Knightmare added

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

	global_hInstance = hInstance;

	ParseCommandLine (lpCmdLine);

	Sys_SetHighDPIMode ();	// setup DPI awareness

	// r1ch: always change to our directory (ugh)
	FixWorkingDirectory ();

	Init_ExeDir ();	// Knightmare added

#ifdef NEW_DED_CONSOLE // init debug console
	Sys_InitDedConsole ();
	Com_Printf("KMQ2 %4.2fu%d %s %s %s %s\n", VERSION, VERSION_UPDATE, CPUSTRING, OS_STRING, COMPILETYPE_STRING, __DATE__);
#else
	Sys_CreateStartupLogo ();
#endif	// NEW_DED_CONSOLE

	// Knightmare- scan for cd command line option
	for (i=0; i<argc; i++)
		if (!strcmp(argv[i], "scanforcd")) {
			cdscan = true;
			break;
		}

	// if we find the CD, add a +set cddir xxx command line
	if (cdscan)
	{
		cddir = Sys_ScanForCD ();
		if (cddir && argc < MAX_NUM_ARGVS - 3)
		{
			int		i;

			// don't override a cddir on the command line
			for (i=0 ; i<argc ; i++)
				if (!strcmp(argv[i], "cddir"))
					break;
			if (i == argc)
			{
				argv[argc++] = "+set";
				argv[argc++] = "cddir";
				argv[argc++] = cddir;
			}
		}
	}

	Qcommon_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

    /* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
	//	if (Minimized || (dedicated && dedicated->value) )
		if (Minimized || (dedicated && dedicated->integer) )
		{
			Sleep (1);
		}

		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0))
				Com_Quit ();
			sys_msg_time = msg.time;
			TranslateMessage (&msg);
   			DispatchMessage (&msg);
		}

		// DarkOne's CPU usage fix
		while (1)
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
			if (time > 0) break;
			Sleep(0); // may also use Sleep(1); to free more CPU, but it can lower your fps
		}
		/*do
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);*/
		//	Con_Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime, oldtime, time);

#if defined(_M_IX86) || defined(__i386__)
		//	_controlfp( ~( _EM_ZERODIVIDE /*| _EM_INVALID*/ ), _MCW_EM );
		// Knightmare- removed this, as it inhibits the double-precision fix for lightmap extents
	//	_controlfp( _PC_24, _MCW_PC );
#endif

		Qcommon_Frame (time);

		oldtime = newtime;
	}

	// never gets here
    return TRUE;
}
