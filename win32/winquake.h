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

// winquake.h: Win32-specific Quake2 header file

#pragma warning( disable : 4229 )  // mgraph gets this

#include <windows.h>

#include <dsound.h>

//#define	WINDOW_STYLE	(WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE)
#define	WINDOW_STYLE	(WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_VISIBLE)

// defines for MSVC6
#if (_MSC_VER < 1300)
#ifndef LONG_PTR
#define LONG_PTR LONG
#endif

#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLong
#endif

#ifndef GetWindowLongPtr
#define GetWindowLongPtr GetWindowLong
#endif

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC GWL_WNDPROC
#endif

#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64 9
#endif
#endif	// _MSC_VER

extern	HINSTANCE	global_hInstance;

extern LPDIRECTSOUND pDS;
extern LPDIRECTSOUNDBUFFER pDSBuf;

extern DWORD gSndBufSize;

extern HWND			cl_hwnd;
extern qboolean		ActiveApp, Minimized;

extern int		window_center_x, window_center_y;
extern RECT		window_rect;

// win_main.c
extern cvar_t		*win_use_profile_dir;

// win_wndproc.c
void WIN_SetAltTab (void);
LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// win_vid.c
extern qboolean		kmgl_active;
extern cvar_t		*win_noalttab;
extern cvar_t		*win_alttab_restore_desktop;	// Knightmare- whether to restore desktop resolution on alt-tab
extern cvar_t		*vid_xpos;
extern cvar_t		*vid_ypos;
extern cvar_t		*vid_fullscreen;

// win_input.c
int IN_MapKey (int key);
void IN_Activate (qboolean active);
void IN_MouseEvent (int mstate);
void IN_MouseWheel (int dir);

// win_dedconsole.c
#define NEW_DED_CONSOLE // enable new dedicated console

#ifdef NEW_DED_CONSOLE
void Sys_ShowConsole (qboolean show);
void Sys_ShutdownConsole (void);
void Sys_InitDedConsole (void);
#endif // NEW_DED_CONSOLE
