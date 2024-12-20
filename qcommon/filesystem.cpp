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

// filesystem.c -- game filesystem and PAK/PK3 file loading

#include "qcommon.h"
#include "filesystem.h"

#ifdef _WIN32
#include "../win32/winquake.h"
#endif

// enables faster binary pak searck, still experimental
#define BINARY_PACK_SEARCH

#pragma warning (disable : 4715)

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

/*
All of Quake's data access is through a hierchal file system, but the
contents of the file system can be transparently merged from several
sources.

The "base directory" is the path to the directory holding the
quake.exe and all game directories.  The sys_* files pass this
to host_init in quakeparms_t->basedir.  This can be overridden
with the "+set game" command line parm to allow code debugging
in a different directory.  The base directory is only used
during filesystem initialization.

The "game directory" is the first tree on the search path and directory
that all generated files (savegames, screenshots, demos, config files)
will be saved to.  This can be overridden with the "-game" command line
parameter.  The game directory can never be changed while quake is
executing.  This is a precacution against having a malicious server
instruct clients to write files over areas they shouldn't.
*/

int		file_from_protected_pak = 0;			// from Yamagi Q2
int		file_from_pak = 0;		// This is set by FS_FOpenFile
int		file_from_pk3 = 0;		// This is set by FS_FOpenFile
char	last_pk3_name[MAX_QPATH];	// This is set by FS_FOpenFile
int		fs_numPakItemRemaps = 0;
char	fs_pakRemapScriptName[MAX_OSPATH];

void CDAudio_Stop (void);
void Com_FileExtension (const char *path, char *dst, int dstSize);


/*
=================
FS_FilePath

Returns the path up to, but not including the last /
=================
*/
void FS_FilePath (const char *path, char *dst, int dstSize)
{
	const char	*s, *last;

	s = last = path + strlen(path);
	while (*s != '/' && *s != '\\' && s != path){
		last = s-1;
		s--;
	}

	Q_strncpyz(dst, dstSize, path);
	if (last-path < dstSize)
		dst[last-path] = 0;
}


char *type_extensions[] =
{
	"bsp",
	"mdl",
	"md2",
	"md3",
	"spr",
	"sp2",
	"dm2",
	"cin",
	"roq",
	"wav",
	"ogg",
	"pcx",
	"wal",
	"wal_json",
	"tga",
	"jpg",
	"png",
	"cfg",
	"txt",
	"def",
	"alias",
	"arena",
	"script",
//	"shader",
	"hud",
//	"menu",
//	"efx",
	"lmp",
	nullptr
};


/*
=================
FS_TypeFlagForPakItem
Returns bit flag based on pak item's extension.
=================
*/
unsigned int FS_TypeFlagForPakItem (const char *itemName)
{
	int		i;
	char	extension[8];

	Com_FileExtension (itemName, extension, sizeof(extension));
	for (i=0; type_extensions[i]; i++) {
		if ( !Q_stricmp( extension, type_extensions[ i ] ) )
			return (1<<i);
	}
	return 0;
}


/*
=================
FS_FileLength
=================
*/
int FS_FileLength (FILE *f)
{
	int cur, end;

	cur = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, cur, SEEK_SET);

	return end;
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath (const char *path)
{
	char	tmpBuf[MAX_OSPATH];
	char	*ofs;

	FS_DPrintf("FS_CreatePath( %s )\n", path);

	if (strstr(path, "..") || strstr(path, "::") || strstr(path, "\\\\") || strstr(path, "//"))
	{
		Com_Printf(S_COLOR_YELLOW"WARNING: refusing to create relative path '%s'\n", path);
		return;
	}
	Q_strncpyz (tmpBuf, sizeof(tmpBuf), path);

	for (ofs = tmpBuf+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\') // Q2E changed
		//if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_Mkdir (tmpBuf);
			*ofs = '/';
		}
	}
}


// Psychospaz's mod detector
qboolean FS_ModType (const char *name)
{
/*	fsSearchPath_t	*search;

	for (search = fs_searchPaths ; search ; search = search->next)
	{
		if (strstr (search->path, name))
			return true;
	}
	return false; */
	// The above trips on fs_basegame, etc.
	// Comparing against fs_gamedir is faster and more direct.
	char *p;

	if (strlen(fs_gamedir) == 0)
		return false;
	p = strrchr(fs_gamedir, '/');
	if (!p)
		return false;
	p++;

	return (Q_stricmp( ( char * ) name, p ) == 0);
}


// This enables Xatrix menu options
qboolean FS_XatrixPath (void)
{
	if ( FS_ModType("xatrix") || fs_xatrixgame->integer )
		return true;
	return false;
}


// This enables Rogue menu options for Q2MP4
qboolean FS_RoguePath (void)
{
	if (FS_ModType("rogue") || fs_roguegame->integer)
		return true;
	return false;
}


/*
=================
FS_DPrintf
=================
*/
void FS_DPrintf (const char *format, ...)
{
	char		msg[1024];
	va_list		argPtr;

//	if (!fs_debug->value)
	if (!fs_debug->integer)
		return;

	va_start(argPtr, format);
//	vsprintf(msg, format, argPtr);
	Q_vsnprintf (msg, sizeof(msg), format, argPtr);
	va_end(argPtr);

	Com_Printf("%s", msg);
}


/*
=================
FS_GameDir

Called to find where to write a file (demos, savegames, etc...)
=================
*/
char *FS_GameDir (void)
{
	return fs_gamedir;
}

/*char *FS_Gamedir (void)
{
	return fs_gamedir;
}*/


/*
=================
FS_SaveGameDir

Called to find where to write and read save games.
Either fs_savegamedir (which results in Sys_PrefDir()/<gamedir>) or FS_Gamedir()
=================
*/
char *FS_SaveGameDir (void)
{
	return (strlen(fs_savegamedir) > 0) ? fs_savegamedir : FS_GameDir();
}


/*
=================
FS_DownloadDir

Called to find where to download game content.
Either fs_downloaddir (which results in Sys_DownloadDir()/<gamedir>) or FS_Gamedir()
=================
*/
char *FS_DownloadDir (void)
{
	return (strlen(fs_downloaddir) > 0) ? fs_downloaddir : FS_GameDir();
}


/*
=================
FS_RootDataPath

Called to find game root path
=================
*/
char *FS_RootDataPath (void)
{
	return fs_basedir->string;
}


/*
=================
FS_HomePath

Called to find home path
=================
*/
char *FS_HomePath (void)
{
	return fs_homepath->string;
}


/*
=================
FS_DeletePath

TODO: delete tree contents
=================
*/
void FS_DeletePath (const char *path)
{
	FS_DPrintf("FS_DeletePath( %s )\n", path);

	Sys_Rmdir(path);
}


/*
=================
FS_FileForHandle

Returns a FILE * for a fileHandle_t
=================
*/
FILE *FS_FileForHandle (fileHandle_t f)
{
	fsHandle_t	*handle;

	handle = FS_GetFileByHandle(f);

	if (handle->zip || handle->writeZip)
		Com_Error(ERR_DROP, "FS_FileForHandle: can't get FILE on zip file");

	if (!handle->file)
		Com_Error(ERR_DROP, "FS_FileForHandle: NULL");

	return handle->file;
}


/*
=================
FS_HandleForFile

Finds a free fileHandle_t
=================
*/
fsHandle_t *FS_HandleForFile( const char *path, fileHandle_t *f )
{
	fsHandle_t *handle = fs_handles;
	for ( int i = 0; i < MAX_HANDLES; i++, handle++ )
	{
		if ( !handle->file && !handle->zip && !handle->writeZip )
		{
			//	strncpy(handle->name, path);
			Q_strncpyz( handle->name, sizeof( handle->name ), path );
			*f = i + 1;
			return handle;
		}
	}

	// Failed
	Com_Error( ERR_DROP, "FS_HandleForFile: none free" );
	return nullptr;
}


/*
=================
FS_GetFileByHandle

Returns a fsHandle_t * for the given fileHandle_t
=================
*/
fsHandle_t *FS_GetFileByHandle (fileHandle_t f)
{
	if (f <= 0 || f > MAX_HANDLES)
		Com_Error(ERR_DROP, "FS_GetFileByHandle: out of range");

	return &fs_handles[f-1];
}


#ifdef BINARY_PACK_SEARCH
/*
=================
FS_FindPackItem

Performs a binary search by hashed filename
to find pack items in a sorted pack
=================
*/
int FS_FindPackItem (fsPack_t *pack, char *itemName, unsigned int itemHash)
{
	int		smax, smin, smidpt;	//, counter = 0;
	int		i;	//, matchStart, matchEnd;

	// catch null pointers
	if ( !pack || !itemName )
		return -1;

	smin = 0;	smax = pack->numFiles;
	while ( (smax - smin) > 5 )	//&& counter < pack->numFiles )
	{
		smidpt = (smax + smin) / 2;
		if (pack->files[smidpt].hash > itemHash)	// before midpoint
			smax = smidpt;
		else if (pack->files[smidpt].hash < itemHash)	// after midpoint
			smin = smidpt;
		else	// pack->files[smidpt].hash == itemHash
			break;
	//	counter++;
	}
	for (i=smin; i<smax; i++)
	{	// make sure this entry is not blacklisted & compare filenames
		if ( pack->files[i].hash == itemHash && !pack->files[i].ignore
			&& !Q_stricmp( pack->files[ i ].name, itemName ) )
			return i;
	}
	// found a match, scan for identical hashes
/*	if (pack->files[smidpt].hash == itemHash)
	{
		for (matchStart = smidpt-1; matchStart >= 0 && pack->files[matchStart].hash == itemHash; matchStart--);
		for (matchEnd = smidpt+1; matchEnd < pack->numFiles && pack->files[matchEnd].hash == itemHash; matchEnd++);
		for (i = matchStart; i < matchEnd; i++)
		{	// make sure this entry is not blacklisted & compare filenames
			if ( pack->files[i].hash == itemHash && !pack->files[i].ignore
				&& !Q_stricmp(pack->files[i].name, itemName) )
				return i;
		}
	}*/
	return -1;
}
#endif	// BINARY_PACK_SEARCH


/*
=================
FS_FOpenFileAppend

Returns file size or -1 on error
=================
*/
int FS_FOpenFileAppend (fsHandle_t *handle)
{
	char	path[MAX_OSPATH];

//	FS_CreatePath(handle->name);
	// include game path, but check for leading /
	if (handle->name[0] == '/')
		FS_CreatePath (va("%s%s", fs_savegamedir, handle->name));	// was fs_gamedir
	else
		FS_CreatePath (va("%s/%s", fs_savegamedir, handle->name));	// was fs_gamedir

	snprintf(path, sizeof(path), "%s/%s", fs_savegamedir, handle->name);	// was fs_gamedir

	handle->file = fopen(path, "ab");
	if (handle->file)
	{
	//	if (fs_debug->value)
		if (fs_debug->integer)
			Com_Printf("FS_FOpenFileAppend: %s\n", path);

		return FS_FileLength(handle->file);
	}

//	if (fs_debug->value)
	if (fs_debug->integer)
		Com_Printf("FS_FOpenFileAppend: couldn't open %s\n", path);

	return -1;
}


/*
=================
FS_FOpenFileWrite

Always returns 0 or -1 on error
=================
*/
int FS_FOpenFileWrite (fsHandle_t *handle)
{
	char	path[MAX_OSPATH];

//	FS_CreatePath(handle->name);
	// include game path, but check for leading /
	if (handle->name[0] == '/')
		FS_CreatePath (va("%s%s", fs_savegamedir, handle->name));	// was fs_gamedir
	else
		FS_CreatePath (va("%s/%s", fs_savegamedir, handle->name));	// was fs_gamedir

	snprintf(path, sizeof(path), "%s/%s", fs_savegamedir, handle->name);	// was fs_gamedir

	handle->file = fopen(path, "wb");
	if (handle->file)
	{
	//	if (fs_debug->value)
		if (fs_debug->integer)
			Com_Printf("FS_FOpenFileWrite: %s\n", path);
		return 0;
	}

//	if (fs_debug->value)
	if (fs_debug->integer)
		Com_Printf("FS_FOpenFileWrite: couldn't open %s\n", path);

	return -1;
}


/*
=================
FS_FOpenFileRead

Returns file size or -1 if not found.
Can open separate files as well as files inside pack files (both PAK
and PK3).
=================
*/
int FS_FOpenFileRead (fsHandle_t *handle)
{
	fsSearchPath_t	*search;
	fsPack_t		*pack;
	char			path[MAX_OSPATH];
	unsigned int	hash;
	int				i;
	unsigned int	typeFlag;

	// Knightmare- hack global vars for autodownloads
	file_from_protected_pak = 0;			// from Yamagi Q2
	file_from_pak = 0;
	file_from_pk3 = 0;
	snprintf(last_pk3_name, sizeof(last_pk3_name), "\0");
	hash = Com_HashFileName(handle->name, 0, false);
	typeFlag = FS_TypeFlagForPakItem(handle->name);

	// Search through the path, one element at a time
	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)
		{	// Search inside a pack file
			pack = search->pack;

			// skip if pack doesn't contain this type of file
			if ((typeFlag != 0)) {
				if (!(pack->contentFlags & typeFlag))
					continue;
			}

#ifdef BINARY_PACK_SEARCH
			// find index of pack item
			i = FS_FindPackItem (pack, handle->name, hash);
			// found it!
			if ( i != -1 && i >= 0 && i < pack->numFiles )
			{
#else
			for (i = 0; i < pack->numFiles; i++)
			{
				if (pack->files[i].ignore)	// skip blacklisted files
					continue;
				if (hash != pack->files[i].hash)	// compare hash first
					continue;
#endif	// 	BINARY_PACK_SEARCH
				if (!Q_stricmp( pack->files[ i ].name, handle->name ) )
				{
					// Found it!
					FS_FilePath (pack->name, fs_fileInPath, sizeof(fs_fileInPath));
					fs_fileInPack = true;

				//	if (fs_debug->value)
					if (fs_debug->integer)
						Com_Printf("FS_FOpenFileRead: %s (found in %s)\n", handle->name, pack->name);

					if (pack->pak)
					{	// PAK
						file_from_pak = 1; // Knightmare added
						file_from_protected_pak = pack->isProtectedPak ? 1 : 0;		// from Yamagi Q2
						handle->file = fopen(pack->name, "rb");
						handle->pakFile = &pack->files[i];	// set pakfile pointer
						if (handle->file)
						{
							fseek(handle->file, pack->files[i].offset, SEEK_SET);

							return pack->files[i].size;
						}
					}
					else if (pack->pk3)
					{	// PK3
						file_from_pk3 = 1; // Knightmare added
						file_from_protected_pak = pack->isProtectedPak ? 1 : 0;		// from Yamagi Q2
						snprintf(last_pk3_name, sizeof(last_pk3_name), strrchr(pack->name, '/')+1); // Knightmare added
						handle->zip = static_cast< unzFile * >( unzOpen( pack->name ) );
						if (handle->zip)
						{
							if (unzLocateFile(handle->zip, handle->name, 2) == UNZ_OK)
							{
								if (unzOpenCurrentFile(handle->zip) == UNZ_OK)
									return pack->files[i].size;
							}

							unzClose (handle->zip);
						}
					}

					Com_Error(ERR_FATAL, "Couldn't reopen %s", pack->name);
				}
				else
					Com_Printf("FS_FOpenFileRead: different filenames with identical hash (%s, %s)!\n", pack->files[i].name, handle->name);
			}
		}
		else
		{	// Search in a directory tree
			snprintf(path, sizeof(path), "%s/%s", search->path, handle->name);

			handle->file = fopen(path, "rb");
			if (handle->file)
			{	// Found it!
				Q_strncpyz(fs_fileInPath, sizeof(fs_fileInPath), search->path);
				fs_fileInPack = false;

			//	if (fs_debug->value)
				if (fs_debug->integer)
					Com_Printf("FS_FOpenFileRead: %s (found in %s)\n", handle->name, search->path);

				return FS_FileLength(handle->file);
			}
		}
	}

	// Not found!
	fs_fileInPath[0] = 0;
	fs_fileInPack = false;

//	if (fs_debug->value)
	if (fs_debug->integer)
		Com_Printf("FS_FOpenFileRead: couldn't find %s\n", handle->name);

	return -1;
}


/*
=================
FS_FOpenFile

Opens a file for "mode".
Returns file size or -1 if an error occurs/not found.
Can open separate files as well as files inside pack files (both PAK
and PK3).
=================
*/
int FS_FOpenFile (const char *name, fileHandle_t *f, fsMode_t mode)
{
	fsHandle_t	*handle = nullptr;
	int			size = 0;

	handle = FS_HandleForFile(name, f);

	Q_strncpyz(handle->name, sizeof(handle->name), name);
	handle->mode = mode;

	switch (mode)
	{
	case FS_READ:
		size = FS_FOpenFileRead(handle);
		break;
	case FS_WRITE:
		size = FS_FOpenFileWrite(handle);
		break;
	case FS_APPEND:
		size = FS_FOpenFileAppend(handle);
		break;
	default:
		Com_Error(ERR_FATAL, "FS_FOpenFile: bad mode (%i)", mode);
	}

	if (size != -1)
		return size;

	// Couldn't open, so free the handle
	memset(handle, 0, sizeof(*handle));

	*f = 0;
	return -1;
}


/*
=================
FS_FOpenDirectFile

Opens a file for "mode".
Returns file size or -1 if an error occurs/not found.
Opens separate files in absolute paths only.
=================
*/
int FS_FOpenDirectFile (const char *name, fileHandle_t *f, fsMode_t mode)
{
	fsHandle_t	*handle = nullptr;
	int			size = -1;

	handle = FS_HandleForFile(name, f);

	Q_strncpyz(handle->name, sizeof(handle->name), name);
	handle->mode = mode;

	switch (mode)
	{
	case FS_READ:
		handle->file = fopen(name, "rb");
		if (handle->file) {
			fs_fileInPack = false;
			size = FS_FileLength(handle->file);
		}
		break;
	case FS_WRITE:
		handle->file = fopen(name, "wb");
		if (handle->file) {
			size = 0;
		}
		break;
	case FS_APPEND:
		handle->file = fopen(name, "ab");
		if (handle->file) {
			size = FS_FileLength(handle->file);
		}
		break;
	default:
		Com_Error (ERR_FATAL, "FS_FOpenDirectFile: bad mode (%i)", mode);
	}

	if (fs_debug->integer)
		Com_Printf ("FS_FOpenDirectFile: %s\n", handle->name);

	if (size != -1)
		return size;

	// Couldn't open, so free the handle
	memset(handle, 0, sizeof(*handle));

	*f = 0;
	return -1;
}


/*
=================
FS_FOpenCompressedFileWrite

Always returns 0 or -1 on error
Opens files directly from inside a specified zip file,
bypassing the pak/searchpath system, and looking only in the current gamedir.
=================
*/
int FS_FOpenCompressedFileWrite (fsHandle_t *handle, const char *zipName, const char *fileName, qboolean add)
{
	char		path[MAX_OSPATH];
	int			append;

//	FS_CreatePath (va("%s", zipName));
	// include game path, but check for leading /
	if (*zipName == '/')
		FS_CreatePath (va("%s%s", fs_savegamedir, zipName));	// was fs_gamedir
	else
		FS_CreatePath (va("%s/%s", fs_savegamedir, zipName));	// was fs_gamedir

	snprintf(path, sizeof(path), "%s/%s", fs_savegamedir, zipName);	// was fs_gamedir

	append = add ? (FS_SaveFileExists (zipName) ? 2 : 0) : 0;	// was FS_LocalFileExists()
	handle->writeZip = static_cast< zipFile * >( zipOpen( path, append ) );
	if (handle->writeZip)
	{
		if (zipOpenNewFileInZip(handle->writeZip, fileName, nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) == ZIP_OK)
		{
		//	if (fs_debug->value)
			if (fs_debug->integer)
				Com_Printf("FS_FOpenCompressedFileWrite: %s/%s\n", path, fileName);
			return 0;
		}
		zipClose(handle->writeZip, nullptr );
	}

//	if (fs_debug->value)
	if (fs_debug->integer)
		Com_Printf("FS_FOpenCompressedFileWrite: couldn't open %s/%s\n", path, fileName);

	return -1;
}


/*
=================
FS_FOpenCompressedFileRead

Returns file size or -1 if not found.
Opens files directly from inside a specified zip file,
bypassing the pak system.
=================
*/
int FS_FOpenCompressedFileRead (fsHandle_t *handle, const char *zipName, const char *fileName)
{
	char			path[MAX_OSPATH];
	unz_file_info	info;

	// Search through the path, one element at a time
	for ( fsSearchPath_t *search = fs_searchPaths; search; search = search->next)
	{
		if (!search->pack) // Search only in a directory tree
		{
			snprintf(path, sizeof(path), "%s/%s", search->path, zipName);
			handle->zip = static_cast< unzFile * >( unzOpen( path ) );
			if (handle->zip)
			{
				if (unzLocateFile(handle->zip, fileName, 2) == UNZ_OK)
				{
					if (unzOpenCurrentFile(handle->zip) == UNZ_OK)
					{	// Found it!
						Q_strncpyz(fs_fileInPath, sizeof(fs_fileInPath), search->path);
						fs_fileInPack = false;
					//	if (fs_debug->value)
						if (fs_debug->integer)
							Com_Printf("FS_FOpenCompressedFileRead: %s (found in %s/%s)\n", fileName, search->path, zipName);

						unzGetCurrentFileInfo(handle->zip, &info, nullptr, 0, nullptr, 0, nullptr, 0);
						return info.uncompressed_size;
					}
				}
				unzClose (handle->zip);
			}
		}
	}

	// Not found!
	fs_fileInPath[0] = 0;
	fs_fileInPack = false;

//	if (fs_debug->value)
	if (fs_debug->integer)
		Com_Printf("FS_FOpenCompressedFileRead: couldn't find %s\n", handle->name);

	return -1;
}


/*
=================
FS_FOpenCompressedFile

Opens a zip file for "mode".
Returns file size or -1 if an error occurs/not found.
Opens files directly from inside a specified zip file,
bypassing the pak system.
=================
*/
int FS_FOpenCompressedFile (const char *zipName, const char *fileName, fileHandle_t *f, fsMode_t mode)
{
	fsHandle_t	*handle = nullptr;
	char		name[MAX_OSPATH];
	int			size = 0;

	snprintf (name, sizeof(name), "%s/%s", zipName, fileName);
	handle = FS_HandleForFile(name, f);

	if (!handle) {	// case of no handles available
		*f = 0;
		return -1;
	}

	Q_strncpyz(handle->name, sizeof(handle->name), name);
	handle->mode = mode;

	switch (mode)
	{
	case FS_READ:
		size = FS_FOpenCompressedFileRead(handle, zipName, fileName);
		break;
	case FS_WRITE:
		size = FS_FOpenCompressedFileWrite(handle, zipName, fileName, false);
		break;
	case FS_APPEND:
		size = FS_FOpenCompressedFileWrite(handle, zipName, fileName, true);
		break;
	default:
		Com_Error(ERR_FATAL, "FS_FOpenCompressedFile: bad mode (%i)", mode);
	}

	if (size != -1)
		return size;

	// Couldn't open, so free the handle
	memset(handle, 0, sizeof(*handle));

	*f = 0;
	return -1;
}


/*
=================
FS_FCloseFile
=================
*/
void FS_FCloseFile (fileHandle_t f)
{
	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->file)
		fclose (handle->file);
	else if (handle->zip) {
		unzCloseCurrentFile (handle->zip);
		unzClose (handle->zip);
	}
	else if (handle->writeZip)
	{
		zipCloseFileInZip (handle->writeZip);
		zipClose (handle->writeZip, nullptr );
	}

	memset(handle, 0, sizeof(*handle));
}


/*
=================
FS_Read

Properly handles partial reads
=================
*/
int FS_Read (void *buffer, int size, fileHandle_t f)
{
	fsHandle_t	*handle;
	int			remaining, r;
	byte		*buf;
	qboolean	tried = false;

	handle = FS_GetFileByHandle(f);

	// Read
	remaining = size;
	buf = (byte *)buffer;

	while (remaining)
	{
		if (handle->file)
			r = (int)fread(buf, 1, remaining, handle->file);
		else if (handle->zip)
			r = unzReadCurrentFile(handle->zip, buf, remaining);
		else
			return 0;

		if (r == 0)
		{
			if (!tried)
			{	// We might have been trying to read from a CD
				CDAudio_Stop();
				tried = true;
			}
			else
			{	// Already tried once
				//Com_Error(ERR_FATAL, va("FS_Read: 0 bytes read from %s", handle->name));
				Com_DPrintf(S_COLOR_YELLOW"FS_Read: 0 bytes read from %s\n", handle->name);
				return size - remaining;
			}
		}
		else if (r == -1)
			Com_Error(ERR_FATAL, "FS_Read: -1 bytes read from %s", handle->name);

		remaining -= r;
		buf += r;
	}

	return size;
}


/*
=================
FS_FRead

Properly handles partial reads of size up to count times
No error if it can't read
=================
*/
int FS_FRead (void *buffer, int size, int count, fileHandle_t f)
{
	fsHandle_t	*handle;
	int			loops, remaining, r;
	byte		*buf;
	qboolean	tried = false;

	handle = FS_GetFileByHandle(f);

	// Read
	loops = count;
	//remaining = size;
	buf = (byte *)buffer;

	while (loops)
	{	// Read in chunks
		remaining = size;
		while (remaining)
		{
			if (handle->file)
				r = (int)fread(buf, 1, remaining, handle->file);
			else if (handle->zip)
				r = unzReadCurrentFile(handle->zip, buf, remaining);
			else
				return 0;

			if (r == 0)
			{
				if (!tried)
				{	// We might have been trying to read from a CD
					CDAudio_Stop();
					tried = true;
				}
				else {
					//Com_Printf(S_COLOR_RED"FS_FRead: 0 bytes read from %s\n", handle->name);
					return size - remaining;
				}
			}
			else if (r == -1)
				Com_Error(ERR_FATAL, "FS_FRead: -1 bytes read from %s", handle->name);

			remaining -= r;
			buf += r;
		}
		loops--;
	}
	return size;
}


/*
=================
FS_Write

Properly handles partial writes
=================
*/
int FS_Write (const void *buffer, int size, fileHandle_t f)
{
	fsHandle_t	*handle = nullptr;
	int			remaining = 0, w = 0;
	byte		*buf = nullptr;

	handle = FS_GetFileByHandle(f);

	// Write
	remaining = size;
	buf = (byte *)buffer;

	while (remaining)
	{
		if (handle->file)
			w = (int)fwrite(buf, 1, remaining, handle->file);
		else if (handle->writeZip)
		{
			if (zipWriteInFileInZip(handle->writeZip, buf, remaining) == ZIP_OK)
				w = remaining;
		}
		else if (handle->zip)
			Com_Error(ERR_FATAL, "FS_Write: can't write to zip file %s", handle->name);
		else
			return 0;

		if (w == 0)
		{
			Com_Printf(S_COLOR_RED"FS_Write: 0 bytes written to %s\n", handle->name);
			return size - remaining;
		}
		else if (w == -1)
			Com_Error(ERR_FATAL, "FS_Write: -1 bytes written to %s", handle->name);

		remaining -= w;
		buf += w;
	}

	return size;
}


/*
=================
FS_CompressFile
=================
*/
int FS_CompressFile (const char *fileName, const char *zipName, const char *internalName)
{
	int				size, partSize;
	fileHandle_t	f;
	FILE			*fp;
	byte			buf[8192];
	fsMode_t		mode;

	fp = fopen (fileName, "rb");
	if (!fp)
		return -1;

	mode = FS_SaveFileExists((char *)zipName) ? FS_APPEND : FS_WRITE;	// was FS_LocalFileExists()
	size = FS_FOpenCompressedFile (zipName, internalName, &f, mode);
	if (size == -1) {
		fclose (fp);
		return -1;
	}

	do {
		partSize = (int)fread (&buf, 1, sizeof(buf), fp);
		if (partSize > 0)
			FS_Write (&buf, partSize, f);
	} while (partSize > 0);

	FS_FCloseFile (f);
	fclose (fp);

	return size;
}


/*
=================
FS_DecompressFile
=================
*/
int FS_DecompressFile (const char *fileName, const char *zipName, const char *internalName)
{
	int				size, partSize;
	fileHandle_t	f;
	FILE			*fp;
	byte			buf[8192];

	size = FS_FOpenCompressedFile (zipName, internalName, &f, FS_READ);
	if (size == -1)
		return -1;

	fp = fopen (fileName, "wb");
	if (!fp) {
		FS_FCloseFile (f);
		return -1;
	}

	do {
		partSize = FS_Read (&buf, sizeof(buf), f);
		if (partSize > 0)
			fwrite (&buf, 1, partSize, fp);
	} while (partSize > 0);

	fclose (fp);
	FS_FCloseFile (f);

	return size;
}


/*
=================
FS_FTell
=================
*/
int FS_FTell (fileHandle_t f)
{

	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->pakFile) {	// inside .pak file uses offset/size
		int	pos = ftell(handle->file);
		if (pos != -1)
			pos -= handle->pakFile->offset;
		return pos;
	}
	else if (handle->file)
		return ftell(handle->file);
	else if (handle->zip)
		return unztell(handle->zip);

	return 0;
}


/*
=================
FS_ListPak

Generates a listing of the contents of a pak file
=================
*/
char **FS_ListPak (const char *find, int *num)
{
	fsSearchPath_t	*search;
	//char			netpath[MAX_OSPATH];
	fsPack_t		*pak;

	int nfiles = 0, nfound = 0;
	char **list = nullptr;
	int i;

	// now check pak files
	for (search = fs_searchPaths; search; search = search->next)
	{
		if (!search->pack)
			continue;

		pak = search->pack;

		// now find and build list
		for (i=0 ; i<pak->numFiles ; i++) {
			if (!pak->files[i].ignore)
				nfiles++;
		}
	}

	list = static_cast< char ** >( malloc( sizeof( char * ) * nfiles ) );
	memset(list, 0, sizeof(char *) * nfiles);

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (!search->pack)
			continue;

		pak = search->pack;

		// now find and build list
		for (i=0 ; i<pak->numFiles ; i++)
		{
			if (!pak->files[i].ignore && strstr(pak->files[i].name, find))
			{
				list[nfound] = strdup(pak->files[i].name);
				nfound++;
			}
		}
	}

	*num = nfound;

	return list;
}


/*
=================
FS_FindFiles

Generates a listing of files in the given path with an optional extension.
Lists all files if extension is NULL.
=================
*/
char **FS_FindFiles (const char *path, const char *extension, int *num)
{
	fsSearchPath_t	*search;
	fsPack_t		*pak;
	char			dir[MAX_OSPATH], ext[16], findName[1024];
	char			*name, **itemFiles, *tmpList[MAX_FIND_FILES], **outList = nullptr;
	int				nFound = 0;
	int				i, nItems;	// len, extLen;

	memset (tmpList, 0, sizeof(tmpList));

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)	// search inside a pak/pk3 file
		{
			pak = search->pack;
			for (i=0 ; i<pak->numFiles ; i++)
			{
				// skip blacklisted pak entries
				if (pak->files[i].ignore)
					continue;

				// check path
				FS_FilePath (pak->files[i].name, dir, sizeof(dir));
				if ( Q_stricmp( ( char * ) path, dir ) )
					continue;

				// check extension
				if ( (extension != nullptr ) && (strlen(extension) > 0) ) {
					Com_FileExtension(pak->files[i].name, ext, sizeof(ext));
					if ( Q_stricmp( ( char * ) extension, ext ) )
						continue;
				}

				// found something
				name = pak->files[i].name;
				if (nFound < (MAX_FIND_FILES-1))
				{
					if (!FS_ItemInList(name, nFound, (const char **)tmpList)) // check if already in list
					{
						tmpList[nFound] = strdup(name);
						nFound++;
					}
				}
			}
		}
		else	// search in a directory tree
		{
			if ( (extension != nullptr ) && (strlen(extension) > 0) )
				snprintf (findName, sizeof(findName), "%s/%s/*.%s", search->path, path, extension);
			else
				snprintf (findName, sizeof(findName), "%s/%s/*.*", search->path, path);

			itemFiles = FS_ListFiles(findName, &nItems, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

			for (i=0; i < nItems; i++)
			{
				if (!itemFiles || !itemFiles[i])
					continue;

				// check extension
			/*	if ( (extension != NULL) && (strlen(extension) > 0) ) {
					len = (int)strlen(itemFiles[i]);
					extLen = (int)strlen(extension);
					if ( strcmp(itemFiles[i]+max(len-(extLen+1),0), va(".%s", extension)) )
						continue;
				}*/

				// found something
				name = itemFiles[i] + strlen(search->path) + 1; // skip over search path and /
				if (nFound < (MAX_FIND_FILES-1))
				{
					if (!FS_ItemInList(name, nFound, (const char **)tmpList)) // check if already in list
					{
						tmpList[nFound] = strdup(name);
						nFound++;
					}
				}
			}
			if (nItems)
				FS_FreeFileList (itemFiles, nItems);
		}
	}

	// sort the list
	qsort(tmpList, nFound, sizeof(char *), Q_SortStrcmp);

	// alloc and copy output list
	outList = static_cast< char ** >( malloc( sizeof( char * ) * ( nFound + 1 ) ) );
	memset(outList, 0, sizeof(char *) * (nFound+1));
	for (i=0; i<nFound; i++) {
		outList[i] = tmpList[i];
	}

	if (num)
		*num = nFound;
	return outList;
}


/*
=================
FS_FilteredFindFiles

Generates a listing of files that matches the given filter/widlcards.
=================
*/
char **FS_FilteredFindFiles (const char *pattern, int *num)
{
	fsSearchPath_t	*search;
	fsPack_t		*pak;
	char			findName[1024];
	char			*name, **itemFiles, *tmpList[MAX_FIND_FILES], **outList = nullptr;
	int				nFound = 0;
	int				i, nItems;

	memset (tmpList, 0, sizeof(tmpList));

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)	// search inside a pak/pk3 file
		{
			pak = search->pack;
			for (i=0 ; i<pak->numFiles ; i++)
			{
				// skip blacklisted pak entries
				if (pak->files[i].ignore)
					continue;

				// match pattern
				if ( !Q_GlobMatch(pattern, pak->files[i].name, false) )
					continue;

				// found something
				name = pak->files[i].name;
				if (nFound < (MAX_FIND_FILES-1))
				{
					if (!FS_ItemInList(name, nFound, (const char **)tmpList)) // check if already in list
					{
						tmpList[nFound] = strdup(name);
						nFound++;
					}
				}
			}
		}
		else	// search in a directory tree
		{
			snprintf (findName, sizeof(findName), "%s/%s", search->path, pattern);
			itemFiles = FS_ListFiles(findName, &nItems, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

			for (i=0; i < nItems; i++)
			{
				if (!itemFiles || !itemFiles[i])
					continue;

				// match pattern
				if ( !Q_GlobMatch(pattern, itemFiles[i] + strlen(search->path) + 1, false) )
					continue;

				// found something
				name = itemFiles[i] + strlen(search->path) + 1; // skip over search path and /
				if (nFound < (MAX_FIND_FILES-1))
				{
					if (!FS_ItemInList(name, nFound, (const char **)tmpList)) // check if already in list
					{
						tmpList[nFound] = strdup(name);
						nFound++;
					}
				}
			}
			if (nItems)
				FS_FreeFileList (itemFiles, nItems);
		}
	}

	// sort the list
	qsort(tmpList, nFound, sizeof(char *), Q_SortStrcmp);

	// alloc and copy output list
	outList = static_cast< char ** >( malloc( sizeof( char * ) * ( nFound + 1 ) ) );
	memset(outList, 0, sizeof(char *) * (nFound+1));
	for (i=0; i<nFound; i++) {
		outList[i] = tmpList[i];
	}

	if (num)
		*num = nFound;
	return outList;
}


/*
=================
FS_GetFileList

Generates a listing of files in the given path with the specified optional extension
If extension is NULL, retuns all files in the path.  Also does filtered search based on wildcards.
=================
*/
char **FS_GetFileList (const char *path, const char *extension, int *num)
{
	// If wildcards are in path, use filtered search instead of extension
	if ( strchr(path, '*') || strchr(path, '?') || strchr(path, '[') || strchr(path, ']') )
		return FS_FilteredFindFiles (path, num);
	else
		return FS_FindFiles (path, extension, num);
}


/*
=================
FS_Seek
=================
*/
void FS_Seek (fileHandle_t f, int offset, fsOrigin_t origin)
{
	fsHandle_t		*handle = nullptr;
	unz_file_info	info;
	int				remaining = 0, r, len;
	byte			dummy[0x8000];

	handle = FS_GetFileByHandle(f);

	if (handle->pakFile)	// inside .pak file uses offset/size
	{
		switch (origin)
		{
		case FS_SEEK_SET:
			fseek(handle->file, handle->pakFile->offset + offset, SEEK_SET);
			break;
		case FS_SEEK_CUR:
			fseek(handle->file, offset, SEEK_CUR);
			break;
		case FS_SEEK_END:
			fseek(handle->file, handle->pakFile->offset + handle->pakFile->size, SEEK_SET);
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin (%i)", origin);
		}
	}
	else if (handle->file)
	{
		switch (origin)
		{
		case FS_SEEK_SET:
			fseek(handle->file, offset, SEEK_SET);
			break;
		case FS_SEEK_CUR:
			fseek(handle->file, offset, SEEK_CUR);
			break;
		case FS_SEEK_END:
			fseek(handle->file, offset, SEEK_END);
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin (%i)", origin);
		}
	}
	else if (handle->zip)
	{
		switch (origin)
		{
		case FS_SEEK_SET:
			remaining = offset;
			break;
		case FS_SEEK_CUR:
			remaining = offset + unztell(handle->zip);
			break;
		case FS_SEEK_END:
			unzGetCurrentFileInfo(handle->zip, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			remaining = offset + info.uncompressed_size;
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin (%i)", origin);
		}

		// Reopen the file
		unzCloseCurrentFile(handle->zip);
		unzOpenCurrentFile(handle->zip);

		// Skip until the desired offset is reached
		while (remaining)
		{
			len = remaining;
			if (len > sizeof(dummy))
				len = sizeof(dummy);

			r = unzReadCurrentFile(handle->zip, dummy, len);
			if (r <= 0)
				break;

			remaining -= r;
		}
	}
}


/*
=================
FS_Tell

Returns -1 if an error occurs
=================
*/
int FS_Tell (fileHandle_t f)
{
	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->pakFile) {	// inside .pak file uses offset/size
		int	pos = ftell(handle->file);
		if (pos != -1)
			pos -= handle->pakFile->offset;
		return pos;
	}
	else if (handle->file)
		return ftell(handle->file);
	else if (handle->zip)
		return unztell(handle->zip);
	else
		return -1;
}


/*
=================
FS_FileExists
================
*/
qboolean FS_FileExists (const char *path)
{
	fileHandle_t f;

	FS_FOpenFile(path, &f, FS_READ);
	if (f)
	{
		FS_FCloseFile(f);
		return true;
	}
	return false;
}


/*
=================
FS_DirectFileExists

Similar to FS_FileExists,
but only takes absolute paths.
Does not use FS_FOpenFile() or searchpaths.
================
*/
qboolean FS_DirectFileExists (const char *rawPath)
{
	FILE		*f;

	f = fopen (rawPath, "rb");
	if (f) {
		fclose (f);
		return true;
	}
	return false;
}


/*
=================
FS_LocalFileExists

Similar to FS_FileExists,
but only looks under fs_gamedir.
Does not use FS_FOpenFile() or searchpaths.
================
*/
qboolean FS_LocalFileExists (const char *path)
{
	char		realPath[MAX_OSPATH];
	FILE		*f;

	snprintf (realPath, sizeof(realPath), "%s/%s", FS_GameDir(), path);
	f = fopen (realPath, "rb");
	if (f) {
		fclose (f);
		return true;
	}
	return false;
}


/*
=================
FS_SaveFileExists

Similar to FS_FileExists,
but only looks under fs_savegamedir.
Does not use FS_FOpenFile() or searchpaths.
================
*/
qboolean FS_SaveFileExists (const char *path)
{
	char		realPath[MAX_OSPATH];
	FILE		*f;

	snprintf (realPath, sizeof(realPath), "%s/%s", FS_SaveGameDir(), path);	// was FS_GameDir()
	f = fopen (realPath, "rb");
	if (f) {
		fclose (f);
		return true;
	}
	return false;
}


/*
=================
FS_DownloadFileExists

Similar to FS_FileExists,
but only looks under fs_downloaddir.
Does not use FS_FOpenFile() or searchpaths.
================
*/
qboolean FS_DownloadFileExists (const char *path)
{
	char		realPath[MAX_OSPATH];
	FILE		*f;

	snprintf (realPath, sizeof(realPath), "%s/%s", FS_DownloadDir(), path);
	f = fopen (realPath, "rb");
	if (f) {
		fclose (f);
		return true;
	}
	return false;
}


/*
================
FS_CopyFile
================
*/
void FS_CopyFile (const char *src, const char *dst)
{
	FILE	*f1, *f2;
	size_t	l;
	byte	buffer[65536];

	Com_DPrintf ("FS_CopyFile (%s, %s)\n", src, dst);

	f1 = fopen (src, "rb");
	if (!f1)
		return;
	f2 = fopen (dst, "wb");
	if (!f2)
	{
		fclose (f1);
		return;
	}

	while (1)
	{
		l = fread (buffer, 1, sizeof(buffer), f1);
		if (!l)
			break;
		fwrite (buffer, 1, l, f2);
	}

	fclose (f1);
	fclose (f2);
}


/*
=================
FS_RenameFile
=================
*/
void FS_RenameFile (const char *oldPath, const char *newPath)
{
	FS_DPrintf("FS_RenameFile( %s, %s )\n", oldPath, newPath);

	if (rename(oldPath, newPath))
		FS_DPrintf("FS_RenameFile: failed to rename %s to %s\n", oldPath, newPath);
}


/*
=================
FS_DeleteFile
=================
*/
void FS_DeleteFile (const char *path)
{
	FS_DPrintf("FS_DeleteFile( %s )\n", path);

	if (remove(path))
		FS_DPrintf("FS_DeleteFile: failed to delete %s\n", path);
}


/*
=================
FS_LoadFile

"path" is relative to the Quake search path.
Returns file size or -1 if the file is not found.
A NULL buffer will just return the file size without loading.
=================
*/
int FS_LoadFile (const char *path, void **buffer)
{
	fileHandle_t	f;
	byte			*buf;
	int				size;

	buf = nullptr;

	size = FS_FOpenFile(path, &f, FS_READ);
	if (size == -1 || size == 0)
	{
		if (buffer)
			*buffer = nullptr;
		return size;
	}
	if (!buffer)
	{
		FS_FCloseFile(f);
		return size;
	}
	buf = static_cast< byte * >( Z_Malloc( size ) );
	*buffer = buf;

	FS_Read(buf, size, f);

	FS_FCloseFile(f);

	return size;
}


/*
=================
FS_FreeFile
=================
*/
void FS_FreeFile (void *buffer)
{
	if (!buffer)
	{
		FS_DPrintf("FS_FreeFile: NULL buffer\n");
		return;
	}
	Z_Free (buffer);
}


/*
=================
FS_LoadPakRemapScript

Parses import pak remap script named <dirName>_pakremap.def
=================
*/
void FS_LoadPakRemapScript (const char *gameDir, const char *importDir)
{
	char		fileName[MAX_OSPATH];
	FILE		*scriptFile = nullptr;
	size_t		fileSize, bufSize, len;
	byte		*pakRemapFileBuf;
	char		*s, *token, *remapStart, *orgName, *remapName;
	int			i, numItemRemaps = 0;
	qboolean	foundPakRemap = false;

	// free any loaded remap script first
	if (fs_pakItemRemaps) {
		Z_Free (fs_pakItemRemaps);
		fs_pakItemRemaps = nullptr;
		fs_numPakItemRemaps = 0;
	}

	// load the remap script
	snprintf (fileName, sizeof(fileName), "%s/%s/%s_pakremap.def", fs_basedir->string, gameDir, importDir);
	Q_strncpyz (fs_pakRemapScriptName, sizeof(fs_pakRemapScriptName), fileName);
	scriptFile = fopen(fileName, "rb");
	if ( !scriptFile ) {
		Com_DPrintf ("FS_LoadPakRemapScript: couldn't load %s\n", fileName);
		return;
	}

	// get size of remap script
	fseek (scriptFile, 0L, SEEK_END);
	fileSize = ftell(scriptFile);
	fseek (scriptFile, 0L, SEEK_SET);

	// allocate file size + 1 for null terminator
	bufSize = fileSize + 1;
	pakRemapFileBuf = static_cast< byte * >( Z_Malloc( bufSize ) );

	// read the script
	len = fread (pakRemapFileBuf, 1, fileSize, scriptFile);
	fclose (scriptFile);
	scriptFile = nullptr;
	if (len != fileSize) {
		Com_Printf ("FS_LoadPakRemapScript: couldn't read %i bytes from %s\n", fileSize, fileName);
		Z_Free (pakRemapFileBuf);
		return;
	}

	// parse it
	s = reinterpret_cast< char * >( pakRemapFileBuf );
	while (s < ((char*)pakRemapFileBuf + len))
	{
		token = COM_ParseExt (&s, true);
		if (!token[0])
			break;

		if ( !Q_strcasecmp(token, "pakRemapList") )
		{
			// only one pakRemapList per file!
			if (foundPakRemap) {
				Com_Printf ("FS_LoadPakRemapScript: found extra 'pakRemapList' in file %s\n", fileName);
				Z_Free (pakRemapFileBuf);
				return;
			}
			foundPakRemap = true;

			token = COM_ParseExt (&s, true);
			if (token[0] != '{') {
				Com_Printf ("FS_LoadPakRemapScript: found %s when expecting '{' in file %s\n", token, fileName);
				Z_Free (pakRemapFileBuf);
				return;
			}

			// save start point in remap list
			remapStart = s;

			// count number of remap pairs
			while (s < ((char*)pakRemapFileBuf + len))
			{
				token = COM_ParseExt (&s, true);
				if ( !token[0] || !s ) {
					Com_Printf ("FS_LoadPakRemapScript: EOF without closing brace in file %s\n", fileName);
					break;
				}
				// closing brace before pair opening brace means we're done
				if (token[0] == '}') {
					break;
				}
				else if (token[0] != '{') {
					Com_Printf ("FS_LoadPakRemapScript: found %s when expecting '{' in file %s\n", token, fileName);
					Z_Free (pakRemapFileBuf);
					return;
				}
				// get orgName
				orgName = COM_ParseExt (&s, true);
				if ( !orgName[0] || !s ) {
					Com_Printf ("FS_LoadPakRemapScript: EOF without closing brace in file %s\n", fileName);
					break;
				}
				// get remapName
				remapName = COM_ParseExt (&s, true);
				if ( /*!remapName[0] ||*/ !s ) {
					Com_Printf ("FS_LoadPakRemapScript: EOF without closing brace in file %s\n", fileName);
					break;
				}
				// get remap pair end brace
				token = COM_ParseExt (&s, true);
				if ( !token[0] || !s ) {
					Com_Printf ("FS_LoadPakRemapScript: EOF without closing brace in file %s\n", fileName);
					break;
				}
				if (token[0] != '}') {
					Com_Printf ("FS_LoadPakRemapScript: found %s when expecting '}' in file %s\n", token, fileName);
					break;
				}
				// at this point, we've successfully parsed a single item remap
				numItemRemaps++;
			}

			// bail out if we couldn't load any remaps
			if (numItemRemaps == 0) {
				Com_Printf ("FS_LoadPakRemapScript: couldn't parse any item remaps in file %s\n", fileName);
				Z_Free (pakRemapFileBuf);
				return;
			}

			// alloc remap list
			fs_pakItemRemaps = static_cast< fsPackItemRemap_t * >( Z_Malloc( sizeof( fsPackItemRemap_t ) * numItemRemaps ) );

			// now go through all the remap pairs and put them in the list
			s = remapStart;
			i = 0;
			while ( (s < ((char*)pakRemapFileBuf + len)) && (i < numItemRemaps) )
			{
				token = COM_ParseExt (&s, true);
				if ( !token[0] || !s ) {
					Com_Printf ("FS_LoadPakRemapScript: EOF without closing brace in file %s\n", fileName);
					break;
				}
				// closing brace before pair opening brace means we're done
				if (token[0] == '}') {
					break;
				}
				else if (token[0] != '{') {
					Com_Printf ("FS_LoadPakRemapScript: found %s when expecting '{' in file %s\n", token, fileName);
					Z_Free (pakRemapFileBuf);
					return;
				}

				// get orgName
				orgName = COM_ParseExt (&s, true);
				if ( !orgName[0] || !s ) {
					Com_Printf ("FS_LoadPakRemapScript: EOF without closing brace in file %s\n", fileName);
					break;
				}
				Q_strncpyz (fs_pakItemRemaps[i].orgName, sizeof(fs_pakItemRemaps[i].orgName), orgName);

				// get remapName
				remapName = COM_ParseExt (&s, true);
				if ( /*!remapName[0] ||*/ !s ) {
					Com_Printf ("FS_LoadPakRemapScript: EOF without closing brace in file %s\n", fileName);
					break;
				}
				// if remap name is an empty string (or ''), use orgName for unchanged filename
				if ( (remapName[0] == '\0') || !Q_stricmp( remapName, "''" ) )
					Q_strncpyz (fs_pakItemRemaps[i].remapName, sizeof(fs_pakItemRemaps[i].remapName), fs_pakItemRemaps[i].orgName);
				else
					Q_strncpyz (fs_pakItemRemaps[i].remapName, sizeof(fs_pakItemRemaps[i].remapName), remapName);

				// get remap pair end brace
				token = COM_ParseExt (&s, true);
				if ( !token[0] || !s ) {
					Com_Printf ("FS_LoadPakRemapScript: EOF without closing brace in file %s\n", fileName);
					break;
				}
				if (token[0] != '}') {
					Com_Printf ("FS_LoadPakRemapScript: found %s when expecting '}' in file %s\n", token, fileName);
					break;
				}

				// increment array position
				i++;
			}
		}

		// ignore any crap after the pakRemapList
		else if ( !foundPakRemap ) {
			Com_Printf ("FS_LoadPakRemapScript: unknown command '%s' while looking for 'pakRemapList' in file %s\n", token, fileName);
			Z_Free (pakRemapFileBuf);
			return;
		}
	}

	// free the buffer
	Z_Free (pakRemapFileBuf);

	Com_Printf ("Loaded %i pak remaps from file %s\n", numItemRemaps, fileName);
	fs_numPakItemRemaps = numItemRemaps;
}


/*
=================
FS_FreePakRemapScript

Frees import pak remap script named <dirName>_pakremap.def
=================
*/
void FS_FreePakRemapScript (void)
{
	int			i;

	// check for unused rename entries
	for (i = 0; i < fs_numPakItemRemaps; i++)
	{
		if (fs_pakItemRemaps[i].timesUsed == 0)
			Com_Printf ("Pak remap orgName %s in %s was not found in any paks\n", fs_pakItemRemaps[i].orgName, fs_pakRemapScriptName);
	}

	if (fs_pakItemRemaps)
		Z_Free (fs_pakItemRemaps);
	fs_pakItemRemaps = nullptr;
	fs_numPakItemRemaps = 0;
	fs_pakRemapScriptName[0] = 0;
}


/*
=================
FS_GetPakFileRemapName

Gets a remapped name for a given pakitem
name from the loaded remap script
=================
*/
static qboolean FS_GetPakFileRemapName (const char *pakItemName, char *remapName, size_t remapNameSize)
{
	qboolean	nameRemapped = false;

	if ( !fs_pakItemRemaps || !fs_numPakItemRemaps )
		return false;

	for ( int i = 0; i < fs_numPakItemRemaps; i++)
	{
		if ( !Q_stricmp( fs_pakItemRemaps[ i ].orgName, pakItemName ) ) {
			Q_strncpyz (remapName, remapNameSize, fs_pakItemRemaps[i].remapName);
			fs_pakItemRemaps[i].timesUsed++;
			nameRemapped = true;
			break;
		}
	}

	return nameRemapped;
}


// Some incompetently packaged mods have these files in their paks!
static const char *pakfile_ignore_names[] =
{
	"save/",
	"scrnshot/",
	"screenshots/",
	"autoexec.cfg",
	"kmq2config.cfg",
	nullptr
};

// These files are sometimes inside paks, but should be loaded externally first
static const char *pakfile_tryExtFirst_names[] =
{
	"players/",
	"maps.lst",
	nullptr
};

/*
=================
FS_FileInPakBlacklist

Checks against a blacklist to see if a file
should not be loaded from a pak.
=================
*/
qboolean FS_FileInPakBlacklist ( const char *filename )
{
	int			i;
	qboolean	ignore = false;

	const char *compare = filename;
	if (compare[0] == '/')	// remove leading slash
		compare++;

	for (i=0; pakfile_ignore_names[i]; i++) {
		if ( !Q_strncasecmp( compare, pakfile_ignore_names[ i ], strlen( pakfile_ignore_names[ i ] ) ) )
			ignore = true;
		// Ogg files can't load from .paks
	//	if ( !isPk3 && !strcmp(COM_FileExtension(compare), "ogg") )
	//		ignore = true;
	}
	if (!ignore)	// see if a file should be loaded from outside paks first
	{
		qboolean loadExtFirst = false;
		for (i=0; pakfile_tryExtFirst_names[i]; i++) {
			if ( !Q_strncasecmp( compare, pakfile_tryExtFirst_names[ i ], strlen( pakfile_tryExtFirst_names[ i ] ) ) )
				loadExtFirst = true;
		}
		if (loadExtFirst)
		{
			if (FS_LocalFileExists(compare)) {
			//	Com_Printf ("FS_LoadPAK: file %s in pack is ignored in favor of external file first.\n", filename);
				ignore = true;
			}
		}
	}

//	if (ignore)
//		Com_Printf ("FS_LoadPAK: file %s blacklisted!\n", filename);
//	else if ( !strncmp (filename, "save/", 5) )
//		Com_Printf ("FS_LoadPAK: file %s not blacklisted.\n", filename);
	return ignore;
}


#ifdef BINARY_PACK_SEARCH
/*
=================
FS_PakFileCompare

Used for sorting pak entries by hash
=================
*/
unsigned int *nameHashes = nullptr;
int FS_PakFileCompare (const void *f1, const void *f2)
{
	if (!nameHashes)
		return 1;

	return (nameHashes[*((int *)(f1))] - nameHashes[*((int *)(f2))]);
}
#endif	// BINARY_PACK_SEARCH


/*
=================
FS_LoadPAK

Takes an explicit (not game tree related) path to a pack file.

Loads the header and directory, adding the files at the beginning of
the list so they override previous pack files.
=================
*/
fsPack_t *FS_LoadPAK (const char *packPath, qboolean isQuakeImport)
{
	int				numFiles, i;
	fsPackFile_t	*files;
	fsPack_t		*pack;
	FILE			*handle;
	dpackheader_t	header;
	dpackfile_t		*info = nullptr;		// made this dynamically allocated to avoid stack overflow
	unsigned		contentFlags = 0;
	int				numRemappedItems = 0;
#ifdef BINARY_PACK_SEARCH
	char			**remapNames = nullptr;
	int				*sortIndices;
	unsigned int	*sortHashes;
#else
	char			remapName[MAX_QPATH];
#endif	// BINARY_PACK_SEARCH

	handle = fopen(packPath, "rb");
	if (!handle)
		return nullptr;

	fread(&header, 1, sizeof(dpackheader_t), handle);

	if (LittleLong(header.ident) != IDPAKHEADER) {
		fclose (handle);
		Com_Error(ERR_FATAL, "FS_LoadPAK: %s is not a pack file", packPath);
	}

	header.dirofs = LittleLong(header.dirofs);
	header.dirlen = LittleLong(header.dirlen);

	numFiles = header.dirlen / sizeof(dpackfile_t);
	if (numFiles == 0) {
		fclose (handle);
		Com_Error(ERR_FATAL, "FS_LoadPAK: %s has %i files", packPath, numFiles);
	}

	info = static_cast< dpackfile_t * >( Z_Malloc( header.dirlen ) );
	files = static_cast< fsPackFile_t * >( Z_Malloc( numFiles * sizeof( fsPackFile_t ) ) );

	fseek(handle, header.dirofs, SEEK_SET);
	fread(info, 1, header.dirlen, handle);

#ifdef BINARY_PACK_SEARCH
	// Create remap list
	if ( isQuakeImport ) {
		remapNames = static_cast< char ** >( Z_Malloc( numFiles * sizeof( char * ) ) );
		for (i = 0; i < numFiles; i++) {
			remapNames[i] = static_cast< char * >( Z_Malloc( sizeof( char ) * MAX_QPATH ) );
		}
	}
	// Create sort table
	sortIndices = static_cast< int * >( Z_Malloc( numFiles * sizeof( int ) ) );
	sortHashes  = static_cast< unsigned int * >( Z_Malloc( numFiles * sizeof( unsigned int ) ) );
	nameHashes = sortHashes;
	for (i = 0; i < numFiles; i++)
	{
		sortIndices[i] = i;
		if ( isQuakeImport ) {
			if ( FS_GetPakFileRemapName(info[i].name, remapNames[i], MAX_QPATH) )
				sortHashes[i] = Com_HashFileName(remapNames[i], 0, false);
			else
				sortHashes[i] = Com_HashFileName(info[i].name, 0, false);
		}
		else {
			sortHashes[i] = Com_HashFileName(info[i].name, 0, false);
		}
	}
	qsort((void *)sortIndices, numFiles, sizeof(int), FS_PakFileCompare);

	// Parse the directory
	for (i = 0; i < numFiles; i++)
	{
		Q_strncpyz(files[i].name, sizeof(files[i].name), info[sortIndices[i]].name);
		files[i].hash = sortHashes[sortIndices[i]];
		files[i].offset = LittleLong(info[sortIndices[i]].filepos);
		files[i].size = LittleLong(info[sortIndices[i]].filelen);
		if ( isQuakeImport )
		{	// pak entries in imported Quake paks MUST be remapped, otherwise they are ignored
			if ( remapNames[sortIndices[i]][0] != 0 ) {
				Q_strncpyz (files[i].name, sizeof(files[i].name), remapNames[sortIndices[i]]);
				files[i].ignore = false;
				files[i].isRemapped = true;
				numRemappedItems++;
			}
			else {
				files[i].ignore = true;
				files[i].isRemapped = false;
			}
		}
		else {
			files[i].ignore = FS_FileInPakBlacklist( files[ i ].name );	// check against pak loading blacklist
			files[i].isRemapped = false;
		}
		if (!files[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(files[i].name);
	}

	// Free remap list
	if ( isQuakeImport )
	{
		for (i = 0; i < numFiles; i++) {
			Z_Free (remapNames[i]);
			remapNames[i] = nullptr;
		}
		Z_Free (remapNames);
	}
	// Free sort table
	Z_Free (sortIndices);
	Z_Free (sortHashes);
	nameHashes = nullptr;
#else	// Parse the directory
	for (i = 0; i < numFiles; i++)
	{
		Q_strncpyz(files[i].name, sizeof(files[i].name), info[i].name);
		files[i].hash = Com_HashFileName(info[i].name, 0, false);	// Added to speed up seaching
		files[i].offset = LittleLong(info[i].filepos);
		files[i].size = LittleLong(info[i].filelen);
		if ( isQuakeImport )
		{	// pak entries in imported Quake paks MUST be remapped, otherwise they are ignored
			if ( FS_GetPakFileRemapName(info[i].name, remapName, MAX_QPATH) ) {
				Q_strncpyz (files[i].name, sizeof(files[i].name), remapName);
				files[i].ignore = false;
				files[i].isRemapped = true;
				numRemappedItems++;
			}
			else {
				files[i].ignore = true;
				files[i].isRemapped = false;
			}
		}
		else {
			files[i].ignore = FS_FileInPakBlacklist(info[i].name, false);	// check against pak loading blacklist
			files[i].isRemapped = false;
		}
		if (!files[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(files[i].name);
	}
#endif	// BINARY_PACK_SEARCH

	Z_Free (info);

	pack = static_cast< fsPack_t * >( Z_Malloc( sizeof( fsPack_t ) ) );
	Q_strncpyz(pack->name, sizeof(pack->name), packPath);
	pack->pak = handle;
	pack->pk3 = nullptr;
	pack->numFiles = numFiles;
	pack->files = files;
	pack->contentFlags = contentFlags;
	pack->isQuakeImportPak = isQuakeImport;
	pack->numRemappedFiles = numRemappedItems;

	if ( isQuakeImport && (numRemappedItems > 0) )
		Com_Printf ("Remapped %i items in pak %s\n", numRemappedItems, packPath);

	return pack;
}


/*
=================
FS_AddPAKFile

Adds a Pak file to the searchpath
=================
*/
void FS_AddPAKFile (const char *packPath, qboolean isProtected, qboolean isQuakeImport)
{
	fsPack_t *pack = FS_LoadPAK( packPath, isQuakeImport );
    if (!pack)
        return;
	pack->isProtectedPak = isProtected;	// From Yamagi Q2
    fsSearchPath_t *search = static_cast< fsSearchPath_t * >( Z_Malloc( sizeof( fsSearchPath_t ) ) );
    search->pack = pack;
    search->next = fs_searchPaths;
    fs_searchPaths = search;
}


/*
=================
FS_LoadPK3

Takes an explicit (not game tree related) path to a pack file.

Loads the header and directory, adding the files at the beginning of
the list so they override previous pack files.
=================
*/
fsPack_t *FS_LoadPK3 (const char *packPath)
{
	int				numFiles, i = 0;
	fsPackFile_t	*files;
	fsPack_t		*pack;
	unzFile			*handle;
	unz_global_info	global;
	unz_file_info	info;
	int				status;
	unsigned		contentFlags = 0;
	char			fileName[MAX_QPATH];
#ifdef BINARY_PACK_SEARCH
	fsPackFile_t	*tmpFiles;
	int				*sortIndices;
	unsigned int	*sortHashes;
#endif	// BINARY_PACK_SEARCH

	handle = static_cast< unzFile * >( unzOpen( packPath ) );
	if (!handle)
		return nullptr;

	if (unzGetGlobalInfo(handle, &global) != UNZ_OK) {
		unzClose (handle);
		Com_Error(ERR_FATAL, "FS_LoadPK3: %s is not a pack file", packPath);
	}

	numFiles = global.number_entry;
	if (numFiles == 0) {
		unzClose (handle);
		Com_Error(ERR_FATAL, "FS_LoadPK3: %s has %i files", packPath, numFiles);
	}
	files = static_cast< fsPackFile_t * >( Z_Malloc( numFiles * sizeof( fsPackFile_t ) ) );

#ifdef BINARY_PACK_SEARCH
	// create sort table
	tmpFiles    = static_cast< fsPackFile_t * >( Z_Malloc( numFiles * sizeof( fsPackFile_t ) ) );
	sortIndices = static_cast< int * >( Z_Malloc( numFiles * sizeof( int ) ) );
	sortHashes  = static_cast< unsigned int * >( Z_Malloc( numFiles * sizeof( unsigned ) ) );
	nameHashes  = sortHashes;

	// Parse the directory
	status = unzGoToFirstFile(handle);
	while (status == UNZ_OK)
	{
		fileName[0] = 0;
		unzGetCurrentFileInfo(handle, &info, fileName, MAX_QPATH, nullptr, 0, nullptr, 0);
		sortIndices[i] = i;
	//	strncpy(tmpFiles[i].name, fileName);
		Q_strncpyz(tmpFiles[i].name, sizeof(tmpFiles[i].name), fileName);
		tmpFiles[i].hash = sortHashes[i] = Com_HashFileName(fileName, 0, false);	// Added to speed up seaching
		tmpFiles[i].offset = -1;		// Not used in ZIP files
		tmpFiles[i].size = info.uncompressed_size;
		tmpFiles[i].ignore = FS_FileInPakBlacklist( fileName );	// check against pak loading blacklist
		tmpFiles[i].isRemapped = false;
		if (!tmpFiles[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(tmpFiles[i].name);
		i++;
		status = unzGoToNextFile(handle);
	}

	// sort by hash and copy to final file table
	qsort((void *)sortIndices, numFiles, sizeof(int), FS_PakFileCompare);
	for (i=0; i < numFiles; i++)
	{
	//	strncpy(files[i].name, tmpFiles[sortIndices[i]].name);
		Q_strncpyz(files[i].name, sizeof(files[i].name), tmpFiles[sortIndices[i]].name);
		files[i].hash = tmpFiles[sortIndices[i]].hash;
		files[i].offset = tmpFiles[sortIndices[i]].offset;
		files[i].size = tmpFiles[sortIndices[i]].size;
		files[i].ignore = tmpFiles[sortIndices[i]].ignore;
		files[i].isRemapped = tmpFiles[sortIndices[i]].isRemapped;
	}

	// free sort table
	Z_Free (tmpFiles);
	Z_Free (sortIndices);
	Z_Free (sortHashes);
	nameHashes = nullptr;
#else	// Parse the directory
	status = unzGoToFirstFile(handle);
	while (status == UNZ_OK)
	{
		fileName[0] = 0;
		unzGetCurrentFileInfo(handle, &info, fileName, MAX_QPATH, NULL, 0, NULL, 0);

	//	strncpy(files[i].name, fileName);
		Q_strncpyz(files[i].name, sizeof(files[i].name, fileName);
		files[i].hash = Com_HashFileName(fileName, 0, false);	// Added to speed up seaching
		files[i].offset = -1;		// Not used in ZIP files
		files[i].size = info.uncompressed_size;
		files[i].ignore = FS_FileInPakBlacklist(fileName, true);	// check against pak loading blacklist
		files[i].isRemapped = false;
		if (!files[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(files[i].name);
		i++;

		status = unzGoToNextFile(handle);
	}
#endif	// BINARY_PACK_SEARCH

	pack = static_cast< fsPack_t * >( Z_Malloc( sizeof( fsPack_t ) ) );
//	strncpy(pack->name, packPath);
	Q_strncpyz(pack->name, sizeof(pack->name), packPath);
	pack->pak = nullptr;
	pack->pk3 = handle;
	pack->numFiles = numFiles;
	pack->files = files;
	pack->contentFlags = contentFlags;
	pack->isQuakeImportPak = false;
	pack->numRemappedFiles = 0;

	return pack;
}


/*
=================
FS_AddPK3File

Adds a Pk3 file to the searchpath
=================
*/
void FS_AddPK3File (const char *packPath, qboolean isProtected)
{
	fsSearchPath_t	*search;
	fsPack_t		*pack;

    pack = FS_LoadPK3 (packPath);
    if (!pack)
        return;
	pack->isProtectedPak = isProtected;	// From Yamagi Q2
    search = static_cast< fsSearchPath_t * >( Z_Malloc( sizeof( fsSearchPath_t ) ) );
    search->pack = pack;
    search->next = fs_searchPaths;
    fs_searchPaths = search;
}


/*
=================
FS_AddQuakeImportGameDirectory

Only loads numerical pack files in path.
=================
*/
void FS_AddQuakeImportGameDirectory (const char *dir)
{
	char	packPath[MAX_OSPATH];
	int		i;

	//
	// add any pak files in the format pak0.pak pak1.pak, ...
	//
	for (i=0; i<100; i++)
	{
		snprintf (packPath, sizeof(packPath), "%s/pak%i.pak", dir, i);
		FS_AddPAKFile (packPath, true, true);
	}
}


/*
=================
FS_AddPaksInDirectory

Used by FS_AddGameDirectory() and FS_AddDownloadDirectory().
Loads and adds all the pack files found
(first numerically 0-99 and then in alphabetical order).
PK3 files are loaded later so they override PAK files.
=================
*/
void FS_AddPaksInDirectory (const char *dir)
{
	char	packPath[MAX_OSPATH];
	int		i, j;
	// VoiD -S- *.pak support
	char	findname[1024];
	char	**dirnames;
	int		ndirs;
	char	*tmp;
	// VoiD -E- *.pak support

	//
	// add any pak files in the format pak0.pak pak1.pak, ...
	//
	for (i=0; i<100; i++)    // Pooy - paks can now go up to 100
	{
		snprintf (packPath, sizeof(packPath), "%s/pak%i.pak", dir, i);
		FS_AddPAKFile (packPath, ((i<10) ? true : false), false);	// pak0.pak is protected
	}
    //
    // NeVo - pak3's!
    // add any pk3 files in the format kmq2_pak00.pk3 kmq2_pak01.pk3, ...
    //
    for (i=0; i<100; i++)    // Pooy - paks can now go up to 100
    {
        snprintf (packPath, sizeof(packPath), "%s/kmq2_pak%02i.pk3", dir, i);
        FS_AddPK3File (packPath, false);
    }

    for (i=0; i<2; i++)
    {	// NeVo - Set filetype
        switch (i) {
            case 0:
			default:
                // Standard Quake II pack file '.pak'
                snprintf (findname, sizeof(findname), "%s/%s", dir, "*.pak");
                break;
            case 1:
                // Quake III pack file '.pk3'
                snprintf (findname, sizeof(findname), "%s/%s", dir, "*.pk3");
                break;
        }
		// VoiD -S- *.pack support
        tmp = findname;
        while ( *tmp != 0 )
        {
            if ( *tmp == '\\' )
                *tmp = '/';
            tmp++;
        }
        if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != nullptr )
        {
            for ( j=0; j < ndirs-1; j++ )
            {	// don't reload numbered pak files
				int		k;
				char	buf[16];
				char	buf2[16];
				qboolean numberedpak = false;
				for (k=0; k<100; k++)
				{
					snprintf (buf, sizeof(buf), "/pak%i.pak", k);
					snprintf (buf2, sizeof(buf2), "/kmq2_pak%02i.pk3", k);
					if ( strstr(dirnames[j], buf) || strstr(dirnames[j], buf2) ) {
						numberedpak = true;
						break;
					}
				}
				if (numberedpak)
					continue;
                if ( strrchr( dirnames[j], '/' ) )
                {
					if (i == 1)
						FS_AddPK3File (dirnames[j], false);
					else
						FS_AddPAKFile (dirnames[j], false, false);
                }
                free( dirnames[j] );
            }
            free( dirnames );
        }
        // VoiD -E- *.pack support
    }
}


/*
=================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads any pack files in that path by calling FS_AddPaksInDirectory().
=================
*/
void FS_AddGameDirectory (const char *dir)
{

	Q_strncpyz(fs_gamedir, sizeof(fs_gamedir), dir);

	//
	// Add the directory to the search path
	//
	fsSearchPath_t *search = static_cast< fsSearchPath_t * >( Z_Malloc( sizeof( fsSearchPath_t ) ) );
	Q_strncpyz(search->path, sizeof(search->path), dir);
	search->path[sizeof(search->path)-1] = 0;
	search->next = fs_searchPaths;
	fs_searchPaths = search;

	//
	// Load pack files
	//
	FS_AddPaksInDirectory (dir);
}


/*
=================
FS_AddSaveGameDirectory

Adds the savegame directory to the head of the path.
Should only be called after the final FS_AddGameDirectory() call.
Sets fs_savegamedir, not fs_gamedir, and does not load any pack files.
=================
*/
void FS_AddSaveGameDirectory (const char *dir)
{

	if (!dir)
		return;
	if (strlen(dir) < 1)	// catch 0-length string
		return;

	Q_strncpyz (fs_savegamedir, sizeof(fs_savegamedir), dir);

	if (!Q_stricmp( fs_savegamedir, fs_gamedir ) )	// only add if different from fs_gamedir
		return;

	// create savegamedir if it doesn't yet exist
	FS_CreatePath (va("%s/", fs_savegamedir));

	//
	// Add the directory to the search path
	//
	fsSearchPath_t *search = static_cast< fsSearchPath_t * >( Z_Malloc( sizeof( fsSearchPath_t ) ) );
	Q_strncpyz(search->path, sizeof(search->path), fs_savegamedir);
	search->path[sizeof(search->path)-1] = 0;
	search->next = fs_searchPaths;
	fs_searchPaths = search;
}


/*
=================
FS_AddDownloadDirectory

Adds the download directory to the head of the path.
Should only be called after the final FS_AddGameDirectory() call.
Sets fs_downloaddir, not fs_gamedir, and loads any pack files
in that path by calling FS_AddPaksInDirectory().
=================
*/
void FS_AddDownloadDirectory (const char *dir)
{

	if (!dir)
		return;
	if (strlen(dir) < 1)	// catch 0-length string
		return;

	Q_strncpyz (fs_downloaddir, sizeof(fs_downloaddir), dir);

	if (!Q_stricmp( fs_downloaddir, fs_gamedir ) )	// only add if different from fs_gamedir
		return;

	// create downloaddir if it doesn't yet exist
	FS_CreatePath (va("%s/", fs_downloaddir));

	//
	// Add the directory to the search path
	//
	fsSearchPath_t *search = static_cast< fsSearchPath_t * >( Z_Malloc( sizeof( fsSearchPath_t ) ) );
	Q_strncpyz(search->path, sizeof(search->path), fs_downloaddir);
	search->path[sizeof(search->path)-1] = 0;
	search->next = fs_searchPaths;
	fs_searchPaths = search;

	//
	// Load pack files
	//
	FS_AddPaksInDirectory (dir);
}


/*
=================
FS_NextPath

Allows enumerating all of the directories in the search path
=================
*/
char *FS_NextPath (const char *prevPath)
{
	fsSearchPath_t	*search;
	char			*prev, *firstPath;

	// only use fs_savegamedir if different from fs_gamedir
	if (!Q_stricmp( fs_savegamedir, fs_gamedir ) )
		firstPath = fs_gamedir;
	else
		firstPath = fs_savegamedir;

	if (!prevPath)
		return firstPath;	// was fs_gamedir

	prev = firstPath;	// was fs_gamedir
	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)
			continue;

		if (prevPath == prev)
			return search->path;

		prev = search->path;
	}
	return nullptr;
}


/*
=================
FS_NextGamePath

Allows enumerating all of the directories in the search path
Only called from Sys_GetGameAPI
Skips fs_savegamedir and fs_downloaddir,
so as not to load game library from there.
=================
*/
char *FS_NextGamePath (const char *prevPath)
{
	fsSearchPath_t	*search;
	char			*prev;

	if (!prevPath)
		return fs_gamedir;

	prev = fs_gamedir;
	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)
			continue;

		// explicitly skip fs_savegamedir and fs_downloaddir (if different from fs_gamedir)
		if ( (strlen(search->path) > 0) &&
			(	((Q_stricmp( search->path, fs_savegamedir ) == 0) && (Q_stricmp( fs_savegamedir, fs_gamedir ) != 0)) ||
				((Q_stricmp( search->path, fs_downloaddir ) == 0) && (Q_stricmp( fs_downloaddir, fs_gamedir ) != 0)) ) )
			continue;

		if (prevPath == prev)
			return search->path;

		prev = search->path;
	}
	return nullptr;
}


/*
=================
FS_Path_f
=================
*/
void FS_Path_f (void)
{
	fsSearchPath_t	*search;
	fsHandle_t		*handle;
	fsLink_t		*link;
	int				totalFiles = 0, i;

	Com_Printf("Current search path:\n");

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)
		{
		//	Com_Printf("%s (%i files)\n", search->pack->name, search->pack->numFiles);
			if (search->pack->isQuakeImportPak)
				Com_Printf ("%s (%i files, %i remapped)\n", search->pack->name, search->pack->numFiles,  search->pack->numRemappedFiles);
			else
				Com_Printf ("%s (%i files)\n", search->pack->name, search->pack->numFiles);
			totalFiles += search->pack->numFiles;
		}
		else
			Com_Printf("%s\n", search->path);
	}

//	Com_Printf("\n");
	Com_Printf("Current game dir: %s\n", fs_gamedir);
	Com_Printf("Current savegame dir: %s\n", fs_savegamedir);
	Com_Printf("Current download dir: %s\n", fs_downloaddir);

	for (i = 0, handle = fs_handles; i < MAX_HANDLES; i++, handle++)
	{
		if (handle->file || handle->zip)
			Com_Printf("Handle %i: %s\n", i + 1, handle->name);
	}

	for (i = 0, link = fs_links; link; i++, link = link->next)
		Com_Printf("Link %i: %s -> %s\n", i, link->from, link->to);

	Com_Printf("-------------------------------------\n");

	Com_Printf("%i files in PAK/PK3 files\n\n", totalFiles);
}


/*
=================
FS_Startup

TODO: close open files for game dir
=================
*/
#if 0
void FS_Startup (void)
{
	if (strstr(fs_gamedirvar->string, "..") || strstr(fs_gamedirvar->string, ".")
		|| strstr(fs_gamedirvar->string, "/") || strstr(fs_gamedirvar->string, "\\")
		|| strstr(fs_gamedirvar->string, ":") || !fs_gamedirvar->string[0])
	{
		//Com_Printf("Invalid game directory\n");
		Cvar_ForceSet("game", BASEDIRNAME);
	}

	// Check for game override
	if (stricmp(fs_gamedirvar->string, fs_currentGame))
	{
		fsSearchPath_t	*next;
		fsPack_t		*pack;

		// Free up any current game dir info
		while (fs_searchPaths != fs_baseSearchPaths)
		{
			if (fs_searchPaths->pack)
			{
				pack = fs_searchPaths->pack;

				if (pack->pak)
					fclose (pack->pak);
				if (pack->pk3)
					unzClose (pack->pk3);

				Z_Free (pack->files);
				Z_Free (pack);
			}

			next = fs_searchPaths->next;
			Z_Free (fs_searchPaths);
			fs_searchPaths = next;
		}

		if (!stricmp(fs_gamedirvar->string, BASEDIRNAME))	// Don't add baseq2 again
		//	strncpy(fs_gamedir, fs_basedir->string);
			Q_strncpyz(fs_gamedir, sizeof(fs_gamedir), fs_basedir->string);
		else
		{
			// Add the directories
			FS_AddGameDirectory(va("%s/%s", fs_homepath->string, fs_gamedirvar->string));
		}
	}

//	strncpy(fs_currentGame, fs_gamedirvar->string);
	Q_strncpyz(fs_currentGame, sizeof(fs_currentGame), fs_gamedirvar->string);

	FS_Path_f ();
}
#endif


/*
=================
FS_CopyConfigsToSavegameDir
=================
*/
void FS_CopyConfigsToSavegameDir (void)
{
	FILE	*kmq2ConfigFile;
	char	cfgPattern[MAX_OSPATH];
	char	*srcCfgPath;
	char	dstCfgPath[MAX_OSPATH];
	char	*cfgName;

	// check if fs_savegamedir and fs_gamedir are the same, so we don't try to copy the files over each other
	if (!Q_stricmp( FS_SaveGameDir(), fs_gamedir ) )
		return;

	// check if kmq2config.cfg exists in FS_SaveGameDir() so we can skip copying
	kmq2ConfigFile = fopen(va("%s/kmq2config.cfg", FS_SaveGameDir()), "rb");
	if (kmq2ConfigFile != nullptr )
	{
		fclose (kmq2ConfigFile);
		return;
	}

	// create savegamedir if it doesn't yet exist
	FS_CreatePath (va("%s/", fs_savegamedir));

	snprintf (cfgPattern, sizeof(cfgPattern), "%s/*.cfg", fs_gamedir);
	for (srcCfgPath = Sys_FindFirst(cfgPattern, 0, SFF_SUBDIR|SFF_HIDDEN|SFF_SYSTEM);
		srcCfgPath != nullptr;
		srcCfgPath = Sys_FindNext (0, SFF_SUBDIR|SFF_HIDDEN|SFF_SYSTEM))
	{
		cfgName = strrchr(srcCfgPath, '/');
		if (cfgName == nullptr ) {
			continue;
		}
		++cfgName;	// move to after the '/'
		// Don't copy default.cfg, autoexec.cfg, or configs written by other engines
		// TODO: keep this up to date!
		// config.cfg, aprconfig.cfg, bqconfig.cfg, eglcfg.cfg, maxconfig.cfg, q2config.cfg, q2b_config.cfg, q2econfig.cfg, xpconfig.cfg, yq2.cfg
	/*	if ( (strstr(cfgName, "config.cfg") && (Q_stricmp(cfgName, "kmq2config.cfg") != 0)) ||
			!Q_stricmp(cfgName, "default.cfg") || !Q_stricmp(cfgName, "autoexec.cfg") ||
			!Q_stricmp(cfgName, "eglcfg.cfg") || !Q_stricmp(cfgName, "yq2.cfg") ) { */
		// Only copy kmq2config.cfg
		if (Q_stricmp( cfgName, "kmq2config.cfg" ) != 0) {
			continue;
		}
		snprintf (dstCfgPath, sizeof(dstCfgPath), "%s/%s", FS_SaveGameDir(), cfgName);
		FS_CopyFile (srcCfgPath, dstCfgPath);
	}
	Sys_FindClose();
}


/*
=================
FS_Init
=================
*/
void FS_Dir_f (void);
void FS_Link_f (void);
char *Sys_GetCurrentDirectory (void);

void FS_InitFilesystem (void)
{
	// Init savegame/download dirs as null string
	fs_savegamedir[0] = '\0';
	fs_downloaddir[0] = '\0';

	// Init pakItemRemaps pointer as null
	fs_pakItemRemaps = nullptr;

	// Register our commands and cvars
	Cmd_AddCommand ( "path", FS_Path_f );
	Cmd_AddCommand ( "link", FS_Link_f );
	Cmd_AddCommand ( "dir", FS_Dir_f );

	Com_Printf ("\n----- Filesystem Initialization -----\n");

	// basedir <path>
	// allows the game to run from outside the data tree
#if defined(__linux__) || defined(__APPLE__) || defined(MACOSX)
	fs_basedir = Cvar_Get ( "basedir", ( char * ) Sys_ExeDir(), CVAR_NOSET );
#else
	fs_basedir = Cvar_Get ("basedir", ".", CVAR_NOSET);
#endif
	Cvar_SetDescription ( "basedir", "Sets the root folder where KMQuake2 mounts game directories.  Only settable from the command line with +set basedir <dir>.  Only change this if you want KMQ2 to run with data files outside the Quake2 folder." );

	// cddir <path>
	// Logically concatenates the cddir after the basedir for
	// allows the game to run from outside the data tree
	fs_cddir = Cvar_Get( "cddir", "", CVAR_NOSET );
	Cvar_SetDescription ( "cddir", "Sets the path to where the data files on the game CD are.  Only settable from the command line with +set cddir <path>.  Only used if the full game install was not done." );
	if (fs_cddir->string[0])
		FS_AddGameDirectory (va("%s/" BASEDIRNAME, fs_cddir->string) );

	// Start up with baseq2 by default
	FS_AddGameDirectory (va("%s/" BASEDIRNAME, fs_basedir->string) );

	// Any set gamedirs will be freed up to here
	fs_baseSearchPaths = fs_searchPaths;

//	strncpy(fs_currentGame, BASEDIRNAME);
	Q_strncpyz (fs_currentGame, sizeof(fs_currentGame), BASEDIRNAME);

	// Init cvars
	fs_homepath = Cvar_Get( "homepath", Sys_GetCurrentDirectory(), CVAR_NOSET );
	Cvar_SetDescription ( "homepath", "Current directory that KMQuake2 is running in.  This is a NOSET value." );
	fs_debug = Cvar_Get( "fs_debug", "0", 0 );
	Cvar_SetDescription ( "fs_debug", "Enables console output of filesystem operations." );
	fs_xatrixgame = Cvar_Get( "xatrixgame", "0", CVAR_LATCH );
	Cvar_SetDescription ( "xatrixgame", "Enables Xatrix-specific features in start server menu when not running under the Xatrix gamedir." );
	fs_roguegame = Cvar_Get( "roguegame", "0", CVAR_LATCH );
	Cvar_SetDescription ( "roguegame", "Enables Rogue-specific features in start server menu when not running under the Rogue gamedir." );
	fs_basegamedir1 = Cvar_Get ( "basegame", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "basegame", "Additional game data path.  Use in conjunction with game to load content from one mod while running another." );
	fs_basegamedir2 = Cvar_Get ( "basegame2", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "basegame2", "Second additional game data path.  Use in conjunction with basegame and game to load content from two mods while running another." );
	fs_basegamedir3 = Cvar_Get ( "basegame3", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "basegame3", "Third additional game data path.  Use in conjunction with basegame2, basegame, and game to load content from three mods while running another." );
	fs_gamedirvar = Cvar_Get ( "game", "", CVAR_LATCH | CVAR_SERVERINFO | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "game", "Sets the mod/game dir.  Only set this from the command line with \"+set game <moddir>\".  Use the \"changegame\" command to change game folders while KMQuake2 is running." );

	// Whether to auto-detect Quake1 Steam install path
	fs_quakeimportpath_auto = Cvar_Get( "quake_importpath_auto", "0", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake_importpath_auto", "Whether to auto-detect Steam install path of Quake1 for content mounting." );
	// Whether to auto-detect Quake1RR Steam install path
	fs_quakerrimportpath_auto = Cvar_Get( "quakerr_importpath_auto", "0", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quakerr_importpath_auto", "Whether to auto-detect Steam install path of Quake1 re-release for content mounting." );
	// Install path of Quake1 for content mounting, id1 folder paks are automatically added
	fs_quakeimportpath = Cvar_Get( "quake_importpath", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake_importpath", "Install path of Quake1 for content mounting (e.g. X:/Quake).  Id1 folder is automatically added." );
	// Name override of Quake1 id1 folder, to allow mounting content for other Quake1 engine games such as Hexen2
	fs_quakemaingame = Cvar_Get( "quake_maingame", Q1_MAINDIRNAME, CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake_maingame", "Name override of Quake1 id1 folder.  Allows mounting content for other Quake1 engine games such as Hexen2." );
	// Additional gamedirs for mounting the Quake mission packs and mods' .pak files
	fs_quakegamedir1 = Cvar_Get( "quake_game1", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake_game1", "First additional gamedir under Quake1 install path for content mounting.  Use for Hipnotic/Rogue mission packs or other mods." );
	fs_quakegamedir2 = Cvar_Get( "quake_game2", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake_game2", "Second additional gamedir under Quake1 install path for content mounting.  Use for Hipnotic/Rogue mission packs or other mods." );
	fs_quakegamedir3 = Cvar_Get( "quake_game3", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake_game3", "Third additional gamedir under Quake1 install path for content mounting.  Use for Hipnotic/Rogue mission packs or other mods." );
	fs_quakegamedir4 = Cvar_Get( "quake_game4", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake_game4", "Fourth additional gamedir under Quake1 install path for content mounting.  Use for Hipnotic/Rogue mission packs or other mods." );

#ifdef USE_Q2RR_IMPORT_PATH
	// Whether to auto-detect Quake2RR Steam install path
	fs_quake2rrimportpath_auto = Cvar_Get( "quake2rr_importpath_auto", "0", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake2rr_importpath_auto", "Whether to auto-detect Steam install path of Quake2 re-release for content mounting." );
	// Install path of Quake2RR for content mounting, baseq2 folder paks are automatically added
	fs_quake2rrimportpath = Cvar_Get( "quake2rr_importpath", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake2rr_importpath", "Install path of Quake2 re-release for content mounting (e.g. X:/Quake2RR).  Baseq2 folder is automatically added." );
	// Name override of Quake2RR baseq2 folder, to allow mounting content for other Quake2 engine games
	fs_quake2rrmaingame = Cvar_Get( "quake2rr_maingame", BASEDIRNAME, CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake2rr_maingame", "Name override of Quake2 re-release baseq2 folder.  Allows mounting content for other Quake2 engine games." );
	// Additional gamedirs for mounting Quake2RR mods' .pak files
	fs_quake2rrgamedir1 = Cvar_Get( "quake2rr_game1", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake2rr_game1", "First additional gamedir under Quake2 re-release install path for content mounting.  Use for Q2RR mods." );
	fs_quake2rrgamedir2 = Cvar_Get( "quake2rr_game2", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake2rr_game2", "Second additional gamedir under Quake2 re-release install path for content mounting.  Use for Q2RR mods." );
	fs_quake2rrgamedir3 = Cvar_Get( "quake2rr_game3", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake2rr_game3", "Third additional gamedir under Quake2 re-release install path for content mounting.  Use for Q2RR mods." );
	fs_quake2rrgamedir4 = Cvar_Get( "quake2rr_game4", "", CVAR_LATCH | CVAR_SAVE_IGNORE );
	Cvar_SetDescription ( "quake2rr_game4", "Fourth additional gamedir under Quake2 re-release install path for content mounting.  Use for Q2RR mods." );
#endif	// USE_Q2RR_IMPORT_PATH

	// Set up pref dir under Win32 here
#ifdef _WIN32
	// whether to use user profile dir for savegames, configs, screenshots, etc
	if ( COM_CheckParm ("-portable") || COM_CheckParm ("+portable") || (FS_LoadFile("portable.cfg", NULL) != -1) )
		win_use_profile_dir = Cvar_Get ("win_use_profile_dir", "0", CVAR_NOSET);
	else
		win_use_profile_dir = Cvar_Get ("win_use_profile_dir", "1", CVAR_NOSET);
	Cvar_SetDescription ("win_use_profile_dir", "Internal value that determines whether to use the <userprofile>/Saved Games/KMQuake2 folder on Windows Vista and later for config files, saved games, screenshots, etc.  On Win 2000/XP it uses Documents/My Games/KMQuake2.  To disable this, add -portable to the command line or add an empty portable.cfg file in the Quake2/baseq2 folder.");
#endif

	Sys_InitPrefDir ();	// set up pref dir now instead of calling a function every time it's needed

	// Set our savegame/download dirs with Sys_PrefDir() and baseq2
#ifdef USE_SAVEGAMEDIR
	FS_AddDownloadDirectory (va("%s/%s", Sys_DownloadDir(), BASEDIRNAME));
	FS_AddSaveGameDirectory (va("%s/%s", Sys_PrefDir(), BASEDIRNAME));
#else
	Q_strncpyz(fs_savegamedir, sizeof(fs_savegamedir), fs_gamedir);
	Q_strncpyz(fs_downloaddir, sizeof(fs_downloaddir), fs_gamedir);
#endif	// USE_SAVEGAMEDIR

	// Check for game override
	// Check and load game directory (also sets config/savegame dirs)
	if (fs_gamedirvar->string[0])
		FS_SetGamedir (fs_gamedirvar->string);

	// Copy over configs from gamedir to savegamedir if it's empty
	FS_CopyConfigsToSavegameDir ();

	FS_Path_f (); // output path data
}


/*
=================
FS_Shutdown
=================
*/
void FS_Shutdown (void)
{
	fsHandle_t		*handle;
	fsSearchPath_t	*next;
	fsPack_t		*pack;
	int				i;

	Cmd_RemoveCommand ( "dir" );
//	Cmd_RemoveCommand ("fdir");
	Cmd_RemoveCommand ( "link" );
	Cmd_RemoveCommand ( "path" );

	// Close all files
	for (i = 0, handle = fs_handles; i < MAX_HANDLES; i++, handle++)
	{
		if (handle->file)
			fclose (handle->file);
		if (handle->zip)
		{
			unzCloseCurrentFile (handle->zip);
			unzClose (handle->zip);
		}
	}

	// Free the search paths
	while (fs_searchPaths)
	{
		if (fs_searchPaths->pack)
		{
			pack = fs_searchPaths->pack;

			if (pack->pak)
				fclose (pack->pak);
			if (pack->pk3)
				unzClose (pack->pk3);

			Z_Free (pack->files);
			Z_Free (pack);
		}
		next = fs_searchPaths->next;
		Z_Free (fs_searchPaths);
		fs_searchPaths = next;
	}
}


/*
================
FS_CheckBaseGameVars

Cvar checks moved from FS_SetGamedir().
================
*/
void FS_CheckBaseGameVars (const char *dir)
{
	// check basegame var
	if ( fs_basegamedir1->string[0] )
	{
		if (strstr(fs_basegamedir1->string, "..") || strstr(fs_basegamedir1->string, "/")
			|| strstr(fs_basegamedir1->string, "\\") || strstr(fs_basegamedir1->string, ":"))
		{
			Cvar_Set ("basegame", "");
			Com_Printf ("Basegame should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_basegamedir1->string, BASEDIRNAME ) || !Q_stricmp( fs_basegamedir1->string, ( char * ) dir ) )
		{
			Cvar_Set ("basegame", "");
			Com_Printf ("Basegame should not be the same as " BASEDIRNAME " or gamedir.\n");
		}
	}

	// check basegame2 var
	if ( fs_basegamedir2->string[0] )
	{
		if (strstr(fs_basegamedir2->string, "..") || strstr(fs_basegamedir2->string, "/")
			|| strstr(fs_basegamedir2->string, "\\") || strstr(fs_basegamedir2->string, ":") )
		{
			Cvar_Set ("basegame2", "");
			Com_Printf ("Basegame2 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_basegamedir2->string, BASEDIRNAME ) || !Q_stricmp( fs_basegamedir2->string, ( char * ) dir )
			|| !Q_stricmp( fs_basegamedir2->string, fs_basegamedir1->string ) )
		{
			Cvar_Set ("basegame2", "");
			Com_Printf ("Basegame2 should not be the same as " BASEDIRNAME ", gamedir, or basegame.\n");
		}
	}

	// check basegame3 var
	if ( fs_basegamedir3->string[0] )
	{
		if ( strstr(fs_basegamedir3->string, "..") || strstr(fs_basegamedir3->string, "/")
			|| strstr(fs_basegamedir3->string, "\\") || strstr(fs_basegamedir3->string, ":") )
		{
			Cvar_Set ("basegame3", "");
			Com_Printf ("Basegame3 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_basegamedir3->string, BASEDIRNAME ) || !Q_stricmp( fs_basegamedir3->string, ( char * ) dir )
			|| !Q_stricmp( fs_basegamedir3->string, fs_basegamedir1->string ) || !Q_stricmp( fs_basegamedir3->string, fs_basegamedir2->string ) )
		{
			Cvar_Set ("basegame3", "");
			Com_Printf ("Basegame3 should not be the same as " BASEDIRNAME ", gamedir, basegame, or basegame2.\n");
		}
	}
}


/*
================
FS_CheckImportGameVars

Cvar checks moved from FS_SetGamedir().
================
*/
void FS_CheckImportGameVars (const char *dir)
{
	// check quakeimportpath var
	if ( fs_quakeimportpath->string[0] )
	{
		if (fs_quakeimportpath->string[0] == '.')
		{
			Cvar_Set ("quake_importpath", "");
			Com_Printf ("Quake_importpath should be an absolute path, not a relative one.\n");
		}
	}

	// check quakemaingame var
	if ( fs_quakemaingame->string[0] == 0 ) {
		Cvar_Set ("quake_maingame", "id1");	// must not be 0-length string
	}
	else if ( fs_quakemaingame->string[0] )
	{
		if ( strstr(fs_quakemaingame->string, "..") || strstr(fs_quakemaingame->string, "/")
			|| strstr(fs_quakemaingame->string, "\\") || strstr(fs_quakemaingame->string, ":") )
		{
			Cvar_Set ("quake_maingame", "id1");
			Com_Printf ("Quake_maingame should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quakemaingame->string, fs_quakegamedir1->string ) || !Q_stricmp( fs_quakemaingame->string, fs_quakegamedir2->string )
			|| !Q_stricmp( fs_quakemaingame->string, fs_quakegamedir3->string ) || !Q_stricmp( fs_quakemaingame->string, fs_quakegamedir4->string ) )
		{
			Cvar_Set ("quake_maingame", "id1");
			Com_Printf ("Quake_maingame should not be the same as quake_game1, quake_game2, quake_game3, or quake_game4.\n");
		}
	}

	// check quakegamedir1 var
	if ( fs_quakegamedir1->string[0] )
	{
		if ( strstr(fs_quakegamedir1->string, "..") || strstr(fs_quakegamedir1->string, "/")
			|| strstr(fs_quakegamedir1->string, "\\") || strstr(fs_quakegamedir1->string, ":") )
		{
			Cvar_Set ("quake_game1", "");
			Com_Printf ("Quake_game1 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quakegamedir1->string, fs_quakemaingame->string ) || !Q_stricmp( fs_quakegamedir1->string, fs_quakegamedir2->string )
			|| !Q_stricmp( fs_quakegamedir1->string, fs_quakegamedir3->string ) || !Q_stricmp( fs_quakegamedir1->string, fs_quakegamedir4->string ) )
		{
			Cvar_Set ("quake_game1", "");
			Com_Printf ("Quake_game1 should not be the same as quake_maingame, quake_game2, quake_game3, or quake_game4.\n");
		}
	}

	// check quakegamedir2 var
	if ( fs_quakegamedir2->string[0] )
	{
		if ( strstr(fs_quakegamedir2->string, "..") || strstr(fs_quakegamedir2->string, "/")
			|| strstr(fs_quakegamedir2->string, "\\") || strstr(fs_quakegamedir2->string, ":") )
		{
			Cvar_Set ("quake_game2", "");
			Com_Printf ("Quake_game2 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quakegamedir2->string, fs_quakemaingame->string ) || !Q_stricmp( fs_quakegamedir2->string, fs_quakegamedir1->string )
			|| !Q_stricmp( fs_quakegamedir2->string, fs_quakegamedir3->string ) || !Q_stricmp( fs_quakegamedir2->string, fs_quakegamedir4->string ) )
		{
			Cvar_Set ("quake_game2", "");
			Com_Printf ("Quake_game2 should not be the same as quake_maingame, quake_game1, quake_game3, or quake_game4.\n");
		}
	}

	// check quakegamedir3 var
	if ( fs_quakegamedir3->string[0] )
	{
		if ( strstr(fs_quakegamedir3->string, "..") || strstr(fs_quakegamedir3->string, "/")
			|| strstr(fs_quakegamedir3->string, "\\") || strstr(fs_quakegamedir3->string, ":") )
		{
			Cvar_Set ("quake_game3", "");
			Com_Printf ("Quake_game3 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quakegamedir3->string, fs_quakemaingame->string ) || !Q_stricmp( fs_quakegamedir3->string, fs_quakegamedir1->string )
			|| !Q_stricmp( fs_quakegamedir3->string, fs_quakegamedir2->string ) || !Q_stricmp( fs_quakegamedir3->string, fs_quakegamedir4->string ) )
		{
			Cvar_Set ("quake_game3", "");
			Com_Printf ("Quake_game3 should not be the same as quake_maingame, quake_game1, quake_game2, or quake_game4.\n");
		}
	}

	// check quakegamedir4 var
	if ( fs_quakegamedir4->string[0] )
	{
		if ( strstr(fs_quakegamedir4->string, "..") || strstr(fs_quakegamedir4->string, "/")
			|| strstr(fs_quakegamedir4->string, "\\") || strstr(fs_quakegamedir4->string, ":") )
		{
			Cvar_Set ("quake_game4", "");
			Com_Printf ("Quake_game4 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quakegamedir4->string, fs_quakemaingame->string ) || !Q_stricmp( fs_quakegamedir4->string, fs_quakegamedir1->string )
			|| !Q_stricmp( fs_quakegamedir4->string, fs_quakegamedir2->string ) || !Q_stricmp( fs_quakegamedir4->string, fs_quakegamedir3->string ) )
		{
			Cvar_Set ("quake_game4", "");
			Com_Printf ("Quake_game4 should not be the same as quake_maingame, quake_game1, quake_game2, or quake_game3.\n");
		}
	}

#ifdef USE_Q2RR_IMPORT_PATH
	// check quake2rrimportpath var
	if ( fs_quake2rrimportpath->string[0] )
	{
		if (fs_quake2rrimportpath->string[0] == '.')
		{
			Cvar_Set ("quake2rr_importpath", "");
			Com_Printf ("Quake2rr_importpath should be an absolute path, not a relative one.\n");
		}
	}

	// check quake2rrmaingame var
	if ( fs_quake2rrmaingame->string[0] == 0 ) {
		Cvar_Set ("quake2rr_maingame", BASEDIRNAME);	// must not be 0-length string
	}
	else if ( fs_quake2rrmaingame->string[0] )
	{
		if ( strstr(fs_quake2rrmaingame->string, "..") || strstr(fs_quake2rrmaingame->string, "/")
			|| strstr(fs_quake2rrmaingame->string, "\\") || strstr(fs_quake2rrmaingame->string, ":") )
		{
			Cvar_Set ("quake2rr_maingame", BASEDIRNAME);
			Com_Printf ("Quake2rr_maingame should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quake2rrmaingame->string, fs_quake2rrgamedir1->string ) || !Q_stricmp( fs_quake2rrmaingame->string, fs_quake2rrgamedir2->string )
			|| !Q_stricmp( fs_quake2rrmaingame->string, fs_quake2rrgamedir3->string ) || !Q_stricmp( fs_quake2rrmaingame->string, fs_quake2rrgamedir4->string ) )
		{
			Cvar_Set ("quake2rr_maingame", BASEDIRNAME);
			Com_Printf ("Quake2rr_maingame should not be the same as quake2rr_game1, quake2rr_game2, quake2rr_game3, or quake2rr_game4.\n");
		}
	}

	// check quake2rrgamedir1 var
	if ( fs_quake2rrgamedir1->string[0] )
	{
		if ( strstr(fs_quake2rrgamedir1->string, "..") || strstr(fs_quake2rrgamedir1->string, "/")
			|| strstr(fs_quake2rrgamedir1->string, "\\") || strstr(fs_quake2rrgamedir1->string, ":") )
		{
			Cvar_Set ("quake2rr_game1", "");
			Com_Printf ("Quake2rr_game1 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quake2rrgamedir1->string, fs_quake2rrmaingame->string ) || !Q_stricmp( fs_quake2rrgamedir1->string, fs_quake2rrgamedir2->string )
			|| !Q_stricmp( fs_quake2rrgamedir1->string, fs_quake2rrgamedir3->string ) || !Q_stricmp( fs_quake2rrgamedir1->string, fs_quake2rrgamedir4->string ) )
		{
			Cvar_Set ("quake2rr_game1", "");
			Com_Printf ("Quake2rr_game1 should not be the same as quake2rr_maingame, quake2rr_game2, quake2rr_game3, or quake2rr_game4.\n");
		}
	}

	// check quake2rrgamedir2 var
	if ( fs_quake2rrgamedir2->string[0] )
	{
		if ( strstr(fs_quake2rrgamedir2->string, "..") || strstr(fs_quake2rrgamedir2->string, "/")
			|| strstr(fs_quake2rrgamedir2->string, "\\") || strstr(fs_quake2rrgamedir2->string, ":") )
		{
			Cvar_Set ("quake2rr_game2", "");
			Com_Printf ("Quake2rr_game2 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quake2rrgamedir2->string, fs_quake2rrmaingame->string ) || !Q_stricmp( fs_quake2rrgamedir2->string, fs_quake2rrgamedir1->string )
			|| !Q_stricmp( fs_quake2rrgamedir2->string, fs_quake2rrgamedir3->string ) || !Q_stricmp( fs_quake2rrgamedir2->string, fs_quake2rrgamedir4->string ) )
		{
			Cvar_Set ("quake2rr_game2", "");
			Com_Printf ("Quake2rr_game2 should not be the same as quake2rr_maingame, quake2rr_game1, quake2rr_game3, or quake2rr_game4.\n");
		}
	}

	// check quake2rrgamedir3 var
	if ( fs_quake2rrgamedir3->string[0] )
	{
		if ( strstr(fs_quake2rrgamedir3->string, "..") || strstr(fs_quake2rrgamedir3->string, "/")
			|| strstr(fs_quake2rrgamedir3->string, "\\") || strstr(fs_quake2rrgamedir3->string, ":") )
		{
			Cvar_Set ("quake2rr_game3", "");
			Com_Printf ("Quake2rr_game3 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quake2rrgamedir3->string, fs_quake2rrmaingame->string ) || !Q_stricmp( fs_quake2rrgamedir3->string, fs_quake2rrgamedir1->string )
			|| !Q_stricmp( fs_quake2rrgamedir3->string, fs_quake2rrgamedir2->string ) || !Q_stricmp( fs_quake2rrgamedir3->string, fs_quake2rrgamedir4->string ) )
		{
			Cvar_Set ("quake2rr_game3", "");
			Com_Printf ("Quake2rr_game3 should not be the same as quake2rr_maingame, quake2rr_game1, quake2rr_game2, or quake2rr_game4.\n");
		}
	}

	// check quake2rrgamedir4 var
	if ( fs_quake2rrgamedir4->string[0] )
	{
		if ( strstr(fs_quake2rrgamedir4->string, "..") || strstr(fs_quake2rrgamedir4->string, "/")
			|| strstr(fs_quake2rrgamedir4->string, "\\") || strstr(fs_quake2rrgamedir4->string, ":") )
		{
			Cvar_Set ("quake2rr_game4", "");
			Com_Printf ("Quake2rr_game4 should be a single filename, not a path.\n");
		}
		if ( !Q_stricmp( fs_quake2rrgamedir4->string, fs_quake2rrmaingame->string ) || !Q_stricmp( fs_quake2rrgamedir4->string, fs_quake2rrgamedir1->string )
			|| !Q_stricmp( fs_quake2rrgamedir4->string, fs_quake2rrgamedir2->string ) || !Q_stricmp( fs_quake2rrgamedir4->string, fs_quake2rrgamedir3->string ) )
		{
			Cvar_Set ("quake2rr_game4", "");
			Com_Printf ("Quake2rr_game4 should not be the same as quake2rr_maingame, quake2rr_game1, quake2rr_game2, or quake2rr_game3.\n");
		}
	}
#endif	// USE_Q2RR_IMPORT_PATH
}


/*
================
FS_AddQuakeImportGame

Adds the quakeImportPath / quake2RRImportPath paks
================
*/
void FS_AddQuakeImportGame (const char *dir)
{
	char		tempPath[MAX_OSPATH];
	qboolean	quakegame1_loaded = false, quakegame2_loaded = false, quakegame3_loaded = false;
	qboolean	quakeimportpath_autoset = false;
	qboolean	quake2rrgame1_loaded = false, quake2rrgame2_loaded = false, quake2rrgame3_loaded = false;

	// set QuakeImportPath if quakeimportpath_auto or quakerrimportpath_auto is enabled
#if defined (_WIN32) || (__linux__)
	if (fs_quakerrimportpath_auto->integer) {
		Sys_InitQ1RRSteamInstallDir ();	// auto-detect Q1 Steam install dir
		Q_strncpyz (tempPath, sizeof(tempPath), Sys_Q1RRSteamInstallDir());
		if (tempPath[0] != 0) {
			Cvar_ForceSet ("quake_importpath", tempPath);
			quakeimportpath_autoset = true;
		}
	}
	if ( !quakeimportpath_autoset && fs_quakeimportpath_auto->integer ) {
		Sys_InitQ1SteamInstallDir ();	// auto-detect Q1RR Steam install dir
		Q_strncpyz (tempPath, sizeof(tempPath), Sys_Q1SteamInstallDir());
		if (tempPath[0] != 0)
			Cvar_ForceSet ("quake_importpath", tempPath);
	}
#endif
	// check and load QuakeImportPath/id1 and quakegame dirs
	if ( fs_quakeimportpath->string[0] )
	{
		// Load pak remap script for fs_quakemaingame
		FS_LoadPakRemapScript (dir, fs_quakemaingame->string);
		FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quakeimportpath->string, fs_quakemaingame->string));

		if ( fs_quakegamedir1->string[0] ) {
			FS_LoadPakRemapScript (dir, fs_quakegamedir1->string);
			FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quakeimportpath->string, fs_quakegamedir1->string));
			quakegame1_loaded = true;
		}
		if ( quakegame1_loaded && fs_quakegamedir2->string[0] ) {
			FS_LoadPakRemapScript (dir, fs_quakegamedir2->string);
			FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quakeimportpath->string, fs_quakegamedir2->string));
			quakegame2_loaded = true;
		}
		if ( quakegame2_loaded && fs_quakegamedir3->string[0] ) {
			FS_LoadPakRemapScript (dir, fs_quakegamedir3->string);
			FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quakeimportpath->string, fs_quakegamedir3->string));
			quakegame3_loaded = true;
		}
		if ( quakegame3_loaded && fs_quakegamedir4->string[0] ) {
			FS_LoadPakRemapScript (dir, fs_quakegamedir4->string);
			FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quakeimportpath->string, fs_quakegamedir4->string));
		}

		// Free pak remap script, as we're done with it
		FS_FreePakRemapScript ();
	}

#ifdef USE_Q2RR_IMPORT_PATH
	// set Quake2RRImportPath if quake2rrimportpath_auto is enabled
#if defined (_WIN32) || (__linux__)
	if (fs_quake2rrimportpath_auto->integer) {
		Sys_InitQ2RRSteamInstallDir ();	// auto-detect Q1 Steam install dir
		Q_strncpyz (tempPath, sizeof(tempPath), Sys_Q2RRSteamInstallDir());
		if (tempPath[0] != 0)
			Cvar_ForceSet ("quake2rr_importpath", tempPath);
	}
#endif
	// check and load Quake2RRImportPath/id1 and quake2rrgame dirs
	if ( fs_quake2rrimportpath->string[0] )
	{
		// TODO: load pak remap script for quakebasedirname here
		FS_LoadPakRemapScript (dir, fs_quake2rrmaingame->string);
		FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quake2rrimportpath->string, fs_quake2rrmaingame->string));

		if ( fs_quake2rrgamedir1->string[0] ) {
			FS_LoadPakRemapScript (dir, fs_quake2rrgamedir1->string);
			FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quake2rrimportpath->string, fs_quake2rrgamedir1->string));
			quake2rrgame1_loaded = true;
		}
		if ( quake2rrgame1_loaded && fs_quake2rrgamedir2->string[0] ) {
			FS_LoadPakRemapScript (dir, fs_quake2rrgamedir2->string);
			FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quake2rrimportpath->string, fs_quake2rrgamedir2->string));
			quake2rrgame2_loaded = true;
		}
		if ( quake2rrgame2_loaded && fs_quake2rrgamedir3->string[0] ) {
			FS_LoadPakRemapScript (dir, fs_quake2rrgamedir3->string);
			FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quake2rrimportpath->string, fs_quake2rrgamedir3->string));
			quake2rrgame3_loaded = true;
		}
		if ( quake2rrgame3_loaded && fs_quake2rrgamedir4->string[0] ) {
			FS_LoadPakRemapScript (dir, fs_quake2rrgamedir4->string);
			FS_AddQuakeImportGameDirectory (va("%s/%s", fs_quake2rrimportpath->string, fs_quake2rrgamedir4->string));
		}

		// Free pak remap script, as we're done with it
		FS_FreePakRemapScript ();
	}
#endif	// USE_Q2RR_IMPORT_PATH
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir (const char *dir)
{
	fsSearchPath_t	*next;
	qboolean		basegame1_loaded = false, basegame2_loaded = false;

	if ( strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Com_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}

	//
	// check our basegame/quakegame cvars
	//
	FS_CheckBaseGameVars (dir);
	FS_CheckImportGameVars (dir);

	//
	// free up any current game dir info
	//
	while (fs_searchPaths != fs_baseSearchPaths)
	{
		if (fs_searchPaths->pack)
		{
			if (fs_searchPaths->pack->pak)
				fclose (fs_searchPaths->pack->pak);
			if (fs_searchPaths->pack->pk3)
				unzClose (fs_searchPaths->pack->pk3);
			Z_Free (fs_searchPaths->pack->files);
			Z_Free (fs_searchPaths->pack);
		}
		next = fs_searchPaths->next;
		Z_Free (fs_searchPaths);
		fs_searchPaths = next;
	}

	//
	// flush all data, so it will be forced to reload
	//
	if (dedicated && !dedicated->integer)
		Cbuf_AddText ("vid_restart\nsnd_restart\n");

	if (*dir == 0)	// Knightmare- set to basedir if a blank dir is passed
		snprintf (fs_gamedir, sizeof(fs_gamedir), "%s/" BASEDIRNAME, fs_basedir->string);
	else
		snprintf (fs_gamedir, sizeof(fs_gamedir), "%s/%s", fs_basedir->string, dir);

	if (!strcmp(dir,BASEDIRNAME) || (*dir == 0))
	{
		Cvar_FullSet ("gamedir", "", CVAR_SERVERINFO|CVAR_NOSET|CVAR_SAVE_IGNORE);
		Cvar_FullSet ("game", "", CVAR_LATCH|CVAR_SERVERINFO|CVAR_SAVE_IGNORE);

		// set our savegame/download dirs with Sys_PrefDir() and baseq2
#ifdef USE_SAVEGAMEDIR
		FS_AddDownloadDirectory (va("%s/%s", Sys_DownloadDir(), BASEDIRNAME));
		FS_AddSaveGameDirectory (va("%s/%s", Sys_PrefDir(), BASEDIRNAME));
#else
		Q_strncpyz(fs_savegamedir, sizeof(fs_savegamedir), fs_gamedir);
		Q_strncpyz(fs_downloaddir, sizeof(fs_downloaddir), fs_gamedir);
#endif	// USE_SAVEGAMEDIR
	}
	else
	{
		// check and load base game directory (so mods can be based upon other mods)
		if ( fs_basegamedir1->string[0] )
		{
		//	Com_Printf("Adding basegame path %s/%s\n", fs_basedir->string, fs_basegamedir1->string);
			if (fs_cddir->string[0])
				FS_AddGameDirectory (va("%s/%s", fs_cddir->string, fs_basegamedir1->string) );
			FS_AddGameDirectory (va("%s/%s", fs_basedir->string, fs_basegamedir1->string) );
			basegame1_loaded = true;
		}

		// second basegame so mods can utilize both Rogue and Xatrix assets
		if ( basegame1_loaded && fs_basegamedir2->string[0] )
		{
		//	Com_Printf("Adding basegame2 path %s/%s\n", fs_basedir->string, fs_basegamedir2->string);
			if (fs_cddir->string[0])
				FS_AddGameDirectory (va("%s/%s", fs_cddir->string, fs_basegamedir2->string) );
			FS_AddGameDirectory (va("%s/%s", fs_basedir->string, fs_basegamedir2->string) );
			basegame2_loaded = true;
		}

		// third basegame so mods can utilize Rogue, Xatrix, and Zaero assets
		if ( basegame1_loaded && basegame2_loaded && fs_basegamedir3->string[0] )
		{
		//	Com_Printf("Adding basegame3 path %s/%s\n", fs_basedir->string, fs_basegamedir3->string);
			if (fs_cddir->string[0])
				FS_AddGameDirectory (va("%s/%s", fs_cddir->string, fs_basegamedir3->string) );
			FS_AddGameDirectory (va("%s/%s", fs_basedir->string, fs_basegamedir3->string) );
		}

		// load the gamedir
		Cvar_FullSet ("gamedir", (char *)dir, CVAR_SERVERINFO|CVAR_NOSET|CVAR_SAVE_IGNORE);
		if (fs_cddir->string[0])
			FS_AddGameDirectory (va("%s/%s", fs_cddir->string, dir) );
		FS_AddGameDirectory (va("%s/%s", fs_basedir->string, dir) );

		// load the Quake import paks
		FS_AddQuakeImportGame (dir);

		// set our savegame/download dirs with Sys_PrefDir() and baseq2
#ifdef USE_SAVEGAMEDIR
		FS_AddDownloadDirectory (va("%s/%s", Sys_DownloadDir(), dir));
		FS_AddSaveGameDirectory (va("%s/%s", Sys_PrefDir(), dir));
#else
		Q_strncpyz(fs_savegamedir, sizeof(fs_savegamedir), fs_gamedir);
		Q_strncpyz(fs_downloaddir, sizeof(fs_downloaddir), fs_gamedir);
#endif	// USE_SAVEGAMEDIR
	}
}


/*
================
FS_Link_f

Creates a filelink_t
================
*/
void FS_Link_f (void)
{
	fsLink_t	*l, **prev;

	if (Cmd_Argc() != 3)
	{
		Com_Printf ("USAGE: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	prev = &fs_links;
	for (l=fs_links ; l ; l=l->next)
	{
		if (!strcmp (l->from, Cmd_Argv(1)))
		{
			Z_Free (l->to);
			if (!strlen(Cmd_Argv(2)))
			{	// delete it
				*prev = l->next;
				Z_Free (l->from);
				Z_Free (l);
				return;
			}
			l->to = CopyString (Cmd_Argv(2));
			return;
		}
		prev = &l->next;
	}

	// create a new link
	l = static_cast< fsLink_t * >( Z_Malloc( sizeof( *l ) ) );
	l->next = fs_links;
	fs_links = l;
	l->from = CopyString(Cmd_Argv(1));
	l->length = (int)strlen(l->from);
	l->to = CopyString(Cmd_Argv(2));
}


/*
=================
FS_ExecConfigs

Executes default.cfg and kmq2config.cfg
Encapsulated to avoid redundancy
=================
*/
void FS_ExecConfigs (qboolean unbind)
{
//	char	*cfgfile;

	if (unbind) {
		Cbuf_AddText ("unbindall\n");
	}
	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_AddText ("exec kmq2config.cfg\n");

	// Look for kmq2config.cfg, if not there, try config.cfg
	// Removed because some settings in existing config.cfgs may cause problems
/*	FS_LoadFile ("kmq2config.cfg", (void **)&cfgfile);
	if (cfgfile)
	{
		Cbuf_AddText ("exec kmq2config.cfg\n");
		FS_FreeFile (cfgfile);
	}
	else
		Cbuf_AddText ("exec config.cfg\n");
*/
}


/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec (void)
{
	char	*dir;
	char	name[MAX_OSPATH];

	dir = Cvar_VariableString("gamedir");
	if (*dir) {
		snprintf(name, sizeof(name), "%s/%s/autoexec.cfg", fs_basedir->string, dir);
	}
	else {
		snprintf(name, sizeof(name), "%s/%s/autoexec.cfg", fs_basedir->string, BASEDIRNAME);
	}
	if ( Sys_FindFirst(name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM) ) {
		Cbuf_AddText ("exec autoexec.cfg\n");
	}
	Sys_FindClose ();
}


/*
================
FS_ListFiles
================
*/
char **FS_ListFiles (const char *findname, int *numfiles, unsigned musthave, unsigned canthave)
{
	char *s;
	int nfiles = 0;
	char **list = nullptr;

	s = Sys_FindFirst( (char *)findname, musthave, canthave );
	while ( s )
	{
		if ( s[strlen(s)-1] != '.' )
			nfiles++;
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();

	if ( !nfiles ) {
		*numfiles = 0;
		return nullptr;
	}

	nfiles++; // add space for a guard
	*numfiles = nfiles;

	list = static_cast< char ** >( malloc( sizeof( char * ) * nfiles ) );
	memset(list, 0, sizeof(char *) * nfiles);

	s = Sys_FindFirst( const_cast< char * >( findname ), musthave, canthave );
	nfiles = 0;
	while ( s )
	{
		if ( s[strlen(s)-1] != '.' )
		{
			list[nfiles] = strdup( s );
#ifdef _WIN32
			strlwr( list[nfiles] );
#endif
			nfiles++;
		}
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();

	// sort the list
	qsort(list, nfiles, sizeof(char *), Q_SortStrcmp);

	return list;
}


/*
=================
FS_FreeFileList
=================
*/
void FS_FreeFileList (char **list, int n)
{
	int i;

	for (i = 0; i < n; i++)
	{
		if (list && list[i])
		{
			free(list[i]);
			list[i] = nullptr;
		}
	}
	free(list);
}


/*
=================
FS_ItemInList
=================
*/
qboolean FS_ItemInList (const char *check, int num, const char **list)
{
	int		i;

	if (!check || !list)
		return false;
	for (i=0; i<num; i++)
	{
		if (!list[i])
			continue;
		if ( !Q_strcasecmp((char *)check, (char *)list[i]) )
			return true;
	}
	return false;
}


/*
================
FS_Dir_f
================
*/
void FS_Dir_f (void)
{
	char	*path = nullptr;
	char	findname[1024];
	char	wildcard[1024] = "*.*";
	char	**dirnames;
	int		ndirs;

	if ( Cmd_Argc() != 1 )
	{
	//	strncpy(wildcard, Cmd_Argv(1));
		Q_strncpyz (wildcard, sizeof(wildcard), Cmd_Argv(1));
	}

	while ( ( path = FS_NextPath( path ) ) != nullptr )
	{
		char *tmp = findname;

		snprintf( findname, sizeof(findname), "%s/%s", path, wildcard );

		while ( *tmp != 0 )
		{
			if ( *tmp == '\\' )
				*tmp = '/';
			tmp++;
		}
		Com_Printf( "Directory of %s\n", findname );
		Com_Printf( "----\n" );

		if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != nullptr )
		{
			int i;

			for ( i = 0; i < ndirs-1; i++ )
			{
				if ( strrchr( dirnames[i], '/' ) )
					Com_Printf( "%s\n", strrchr( dirnames[i], '/' ) + 1 );
				else
					Com_Printf( "%s\n", dirnames[i] );

				free( dirnames[i] );
			}
			free( dirnames );
		}
		Com_Printf( "\n" );
	};
}

std::string FS_GetExtension( const std::string &filename )
{
	std::string::size_type pos = filename.find_last_of( '.' );
	if ( pos == std::string::npos )
	{
		return "";
	}

	return filename.substr( pos + 1 );
}

std::string FS_SanitizePath( const std::string &path )
{
	std::string newPath = path;
	for ( auto &i : newPath )
	{
		if ( i == '\\' )
		{
			i = '/';
		}
	}

	return newPath;
}
