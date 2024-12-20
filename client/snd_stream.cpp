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

// snd_stream.c -- Ogg Vorbis stuff


#include "client.h"
#include "snd_loc.h"

#ifdef OGG_SUPPORT

#define BUFFER_SIZE		16384

static bgTrack_t	s_bgTrack;

static channel_t	*s_streamingChannel;

qboolean		ogg_first_init = true;	// First initialization flag
qboolean		ogg_started = false;	// Initialization flag
ogg_status_t	ogg_status;		// Status indicator

#define			MAX_OGGLIST 512

#define			USE_OGGLIST_STRUCT

typedef struct {
	char	name[MAX_QPATH];
	char	*importFilePath;
} oggListItem_t;

#ifdef USE_OGGLIST_STRUCT
oggListItem_t	*ogg_filelist = NULL;	// List of Ogg Vorbis files
#else
char			**ogg_filelist = NULL;	// List of Ogg Vorbis files
#endif	// USE_OGGLIST_STRUCT
int				ogg_curfile;			// Index of currently played file
int				ogg_numfiles = 0;		// Number of Ogg Vorbis files
int				ogg_numImportFiles = 0;	// Number of imported Ogg Vorbis files
int				ogg_loopcounter;

cvar_t			*ogg_loopcount;
cvar_t			*ogg_ambient_track;

extern cvar_t	*fs_quakeimportpath;	// Install path of Quake1 for content mounting, id1 folder paks are automatically added
extern cvar_t	*fs_quake2rrimportpath;	// Install path of Quake2RR for content mounting, baseq2 folder paks are automatically added

/*
=======================================================================

OGG VORBIS STREAMING

=======================================================================
*/


static size_t ovc_read (void *ptr, size_t size, size_t nmemb, void *datasource)
{
	bgTrack_t	*track = (bgTrack_t *)datasource;

	if (!size || !nmemb)
		return 0;
#ifdef OGG_DIRECT_FILE
	return fread(ptr, 1, size * nmemb, track->file) / size;
#else
//	return FS_Read(ptr, size * nmemb, track->file) / size;
	// should use FS_FRead instead
	return FS_FRead(ptr, size * nmemb, 1, track->file) / size;
#endif
}


static int ovc_seek (void *datasource, ogg_int64_t offset, int whence)
{
	bgTrack_t	*track = (bgTrack_t *)datasource;

	switch (whence)
	{
	case SEEK_SET:
#ifdef OGG_DIRECT_FILE
		fseek(track->file, (int)offset, SEEK_SET);
		break;
	case SEEK_CUR:
		fseek(track->file, (int)offset, SEEK_CUR);
		break;
	case SEEK_END:
		fseek(track->file, (int)offset, SEEK_END);
#else
		FS_Seek(track->file, (int)offset, FS_SEEK_SET);
		break;
	case SEEK_CUR:
		FS_Seek(track->file, (int)offset, FS_SEEK_CUR);
		break;
	case SEEK_END:
		FS_Seek(track->file, (int)offset, FS_SEEK_END);
#endif
		break;
	default:
		return -1;
	}

	return 0;
}


static int ovc_close (void *datasource)
{
	return 0;
}


static long ovc_tell (void *datasource)
{
	bgTrack_t	*track = (bgTrack_t *)datasource;

#ifdef OGG_DIRECT_FILE
	return ftell(track->file);
#else
	return FS_Tell(track->file);
#endif
}


/*
=================
S_OpenBackgroundTrack
=================
*/
static qboolean S_OpenBackgroundTrack (const char *name, bgTrack_t *track)
{
	OggVorbis_File	*vorbisFile;
	vorbis_info		*vorbisInfo;
	ov_callbacks	vorbisCallbacks = {ovc_read, ovc_seek, ovc_close, ovc_tell};
#ifdef OGG_DIRECT_FILE
	char	filename[1024];
	char	*path = NULL;
#endif
	int		i;

//	Com_Printf ("Opening background track: %s\n", name);

#ifdef OGG_DIRECT_FILE
	do {
		path = FS_NextPath( path );
		snprintf( filename, sizeof(filename), "%s/%s", path, name );
		if ( (track->file = fopen(filename, "rb")) != 0)
			break;
	} while ( path );
#else	// OGG_DIRECT_FILE
	FS_FOpenFile (name, &track->file, FS_READ);

#ifdef USE_OGGLIST_STRUCT
	if ( !track->file )
	{	// Check for import path for this file in ogg list
		for (i = 0; i < ogg_numfiles; i++)
		{
			if ( !Q_stricmp( ogg_filelist[ i ].name, ( char * ) name ) ) {
				if ( (ogg_filelist[i].importFilePath != NULL) && (ogg_filelist[i].importFilePath[0] != 0) ) {
					FS_FOpenDirectFile (ogg_filelist[i].importFilePath, &track->file, FS_READ);
				}
				break;
			}
		}
	}
#endif	// USE_OGGLIST_STRUCT

#endif	// OGG_DIRECT_FILE
	if ( !track->file ) {
		Com_Printf (S_COLOR_YELLOW"S_OpenBackgroundTrack: couldn't find %s\n", name);
		return false;
	}

	track->vorbisFile = vorbisFile = static_cast< OggVorbis_File * >( Z_Malloc( sizeof( OggVorbis_File ) ) );

//	Com_Printf ("Opening callbacks for background track\n");

	// bombs out here- ovc_read, FS_Read 0 bytes error
	if (ov_open_callbacks(track, vorbisFile, NULL, 0, vorbisCallbacks) < 0)
	{
		Com_Printf (S_COLOR_YELLOW"S_OpenBackgroundTrack: couldn't open OGG stream (%s)\n", name);
		return false;
	}

//	Com_Printf ("Getting info for background track\n");

	vorbisInfo = ov_info(vorbisFile, -1);
	if (vorbisInfo->channels != 1 && vorbisInfo->channels != 2)
	{
		Com_Printf (S_COLOR_YELLOW"S_OpenBackgroundTrack: only mono and stereo OGG files supported (%s)\n", name);
		return false;
	}

	track->start = ov_raw_tell(vorbisFile);
	track->rate = vorbisInfo->rate;
	track->width = 2;
	track->channels = vorbisInfo->channels; // Knightmare added
	track->format = (vorbisInfo->channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

//	Com_Printf ("Vorbis info: frequency: %i channels: %i bitrate: %i\n",
//		vorbisInfo->rate, vorbisInfo->channels, vorbisInfo->bitrate_nominal);

	return true;
}


/*
=================
S_CloseBackgroundTrack
=================
*/
static void S_CloseBackgroundTrack (bgTrack_t *track)
{

	if (track->vorbisFile)
	{
		ov_clear( static_cast< OggVorbis_File * >( track->vorbisFile ) );

		Z_Free(track->vorbisFile);
		track->vorbisFile = NULL;
	}

	if (track->file)
	{
	#ifdef OGG_DIRECT_FILE
		fclose(track->file);
	#else
		FS_FCloseFile(track->file);
	#endif
		track->file = 0;
	}
}

#if 0
/*
=================
S_StreamBackgroundTrack
=================
*/
void S_StreamBackgroundTrack (void)
{
	byte		data[BUFFER_SIZE];
	int			queued = 0; //, processed, state;
	int			size, read, dummy;
	//unsigned	buffer;
	int			samples; // Knightmare added

	if ( !ogg_started )
		return;

	if ( !s_bgTrack.file || !s_musicvolume->value )
		return;

	if ( !s_streamingChannel )
		return;

	// Unqueue and delete any processed buffers
/*	qalGetSourcei(s_streamingChannel->sourceNum, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0){
		while (processed--){
			qalSourceUnqueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);
			qalDeleteBuffers(1, &buffer);
		}
	} */

	//Com_Printf("Streaming background track\n");

	// Make sure we always have at least 4 buffers in the queue
	//qalGetSourcei(s_streamingChannel->sourceNum, AL_BUFFERS_QUEUED, &queued);
	while (queued < 4)
	{
		size = 0;
		// Stream from disk
		while (size < BUFFER_SIZE)
		{
			read = ov_read(s_bgTrack.vorbisFile, data + size, BUFFER_SIZE - size, 0, 2, 1, &dummy);
			if (read == 0)
			{	// End of file
				if (!s_bgTrack.looping)
				{	// Close the intro track
					S_CloseBackgroundTrack(&s_bgTrack);

					// Open the loop track
					if (!S_OpenBackgroundTrack(s_bgTrack.loopName, &s_bgTrack))
					{
						S_StopBackgroundTrack();
						return;
					}
					s_bgTrack.looping = true;
				}

				// Restart the track, skipping over the header
				ov_raw_seek(s_bgTrack.vorbisFile, (ogg_int64_t)s_bgTrack.start);

				// Try streaming again
				read = ov_read(s_bgTrack.vorbisFile, data + size, BUFFER_SIZE - size, 0, 2, 1, &dummy);
			}

			if (read <= 0)
			{	// An error occurred
				S_StopBackgroundTrack();
				return;
			}

			size += read;
		}

		// Knightmare added
		samples = size / (s_bgTrack.width * s_bgTrack.channels);
		S_RawSamples(samples, s_bgTrack.rate,s_bgTrack. width, s_bgTrack.channels, data, true);

		// Upload and queue the new buffer
	/*	qalGenBuffers(1, &buffer);
		qalBufferData(buffer, s_bgTrack.format, data, size, s_bgTrack.rate);
		qalSourceQueueBuffers(s_streamingChannel->sourceNum, 1, &buffer); */

		queued++;
	}


	// Update volume
	//qalSourcef(s_streamingChannel->sourceNum, AL_GAIN, s_musicVolume->value);

	// If not playing, then do so
/*	qalGetSourcei(s_streamingChannel->sourceNum, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING)
		qalSourcePlay(s_streamingChannel->sourceNum); */
}

#else

/*
============
S_StreamBackgroundTrack
============
*/
void S_StreamBackgroundTrack (void)
{
	int		samples, maxSamples;
	int		read, maxRead, total, dummy;
	float	scale;
	byte	data[MAX_RAW_SAMPLES*4];

	if ( !ogg_started )
		return;

	if ( !s_bgTrack.file || !s_musicvolume->value )
		return;

	if ( !s_streamingChannel )
		return;

	if (s_rawend < paintedtime)
		s_rawend = paintedtime;

	scale = (float)s_bgTrack.rate / dma.speed;
	maxSamples = sizeof(data) / s_bgTrack.channels / s_bgTrack.width;

	while (1)
	{
		samples = (paintedtime + MAX_RAW_SAMPLES - s_rawend) * scale;
		if (samples <= 0)
			return;
		if (samples > maxSamples)
			samples = maxSamples;
		maxRead = samples * s_bgTrack.channels * s_bgTrack.width;

		total = 0;
		while (total < maxRead)
		{
			read = ov_read(s_bgTrack.vorbisFile, (char*)data + total, maxRead - total, 0, 2, 1, &dummy);
			if (!read)
			{	// End of file
				if (!s_bgTrack.looping)
				{	// Close the intro track
					S_CloseBackgroundTrack(&s_bgTrack);

					// Open the loop track
					if (!S_OpenBackgroundTrack(s_bgTrack.loopName, &s_bgTrack)) {
						S_StopBackgroundTrack();
						return;
					}
					s_bgTrack.looping = true;
				}
				else
				{	// check if it's time to switch to the ambient track
					if ( ++ogg_loopcounter >= ogg_loopcount->integer
						&& (!cl.configstrings[CS_MAXCLIENTS][0] || !strcmp(cl.configstrings[CS_MAXCLIENTS], "1")) )
					{	// Close the loop track
						S_CloseBackgroundTrack(&s_bgTrack);

						if (!S_OpenBackgroundTrack(s_bgTrack.ambientName, &s_bgTrack)) {
							if (!S_OpenBackgroundTrack(s_bgTrack.loopName, &s_bgTrack)) {
								S_StopBackgroundTrack();
								return;
							}
						}
						else
							s_bgTrack.ambient_looping = true;
					}
				}

				// Restart the track, skipping over the header
				ov_raw_seek(s_bgTrack.vorbisFile, (ogg_int64_t)s_bgTrack.start);
			}

		/*	if (s_bgTrack.read)
				read = s_bgTrack->read( s_bgTrack, data + total, maxRead - total );
			else
				read = FS_Read( data + total, maxRead - total, s_bgTrack->file );

			if (!read)
			{
				if (s_bgTrackIntro.file != s_bgTrackLoop.file)
				{
					if (s_bgTrackIntro.close)
						s_bgTrackIntro.close(&s_bgTrackIntro);
					else
						FS_FCloseFile(s_bgTrackIntro.file);
					s_bgTrackIntro = s_bgTrackLoop;
				}
				s_bgTrack = &s_bgTrackLoop;

				if (s_bgTrack->seek)
					s_bgTrack->seek( s_bgTrack, s_bgTrack->info.dataofs );
				else
					FS_Seek(s_bgTrack->file, s_bgTrack->info.dataofs, FS_SEEK_SET);
			} */
			total += read;
		}
		S_RawSamples (samples, s_bgTrack.rate, s_bgTrack.width, s_bgTrack.channels, data, true );
	}
}
#endif

/*
============
S_UpdateBackgroundTrack

Streams background track
============
*/
void S_UpdateBackgroundTrack (void)
{
	// stop music if paused
	if (ogg_status == PLAY)// && !cl_paused->value)
		S_StreamBackgroundTrack ();
}

// =====================================================================


/*
=================
S_StartBackgroundTrack
=================
*/
void S_StartBackgroundTrack (const char *introTrack, const char *loopTrack)
{
	if ( !ogg_started )
		return;

	// Stop any playing tracks
	S_StopBackgroundTrack();

	// Start it up
	Q_strncpyz (s_bgTrack.introName, sizeof(s_bgTrack.introName), introTrack);
	Q_strncpyz (s_bgTrack.loopName, sizeof(s_bgTrack.loopName), loopTrack);
	Q_strncpyz (s_bgTrack.ambientName, sizeof(s_bgTrack.ambientName), va("music/%s.ogg", ogg_ambient_track->string));

	// set a loop counter so that this track will change to the ambient track later
	ogg_loopcounter = 0;

	S_StartStreaming();

	// Open the intro track
	if (!S_OpenBackgroundTrack(s_bgTrack.introName, &s_bgTrack))
	{
		S_StopBackgroundTrack();
		return;
	}

	ogg_status = PLAY;

	S_StreamBackgroundTrack();
}

/*
=================
S_StopBackgroundTrack
=================
*/
void S_StopBackgroundTrack (void)
{
	if ( !ogg_started )
		return;

	S_StopStreaming ();

	S_CloseBackgroundTrack (&s_bgTrack);

	ogg_status = STOP;

	memset(&s_bgTrack, 0, sizeof(bgTrack_t));
}

/*
=================
S_StartStreaming
=================
*/
void S_StartStreaming (void)
{
	if ( !ogg_started )
		return;

	if ( s_streamingChannel )
		return;		// Already started

	s_streamingChannel = S_PickChannel(0, 0);
	if ( !s_streamingChannel )
		return;

	s_streamingChannel->streaming = true;

	// FIXME: OpenAL bug?
	/*qalDeleteSources(1, &s_streamingChannel->sourceNum);
	qalGenSources(1, &s_streamingChannel->sourceNum);*/

	// Set up the source
	/*qalSourcei(s_streamingChannel->sourceNum, AL_BUFFER, 0);
	qalSourcei(s_streamingChannel->sourceNum, AL_LOOPING, AL_FALSE);
	qalSourcei(s_streamingChannel->sourceNum, AL_SOURCE_RELATIVE, AL_TRUE);
	qalSourcefv(s_streamingChannel->sourceNum, AL_POSITION, vec3_origin);
	qalSourcefv(s_streamingChannel->sourceNum, AL_VELOCITY, vec3_origin);
	qalSourcef(s_streamingChannel->sourceNum, AL_REFERENCE_DISTANCE, 1.0);
	qalSourcef(s_streamingChannel->sourceNum, AL_MAX_DISTANCE, 1.0);
	qalSourcef(s_streamingChannel->sourceNum, AL_ROLLOFF_FACTOR, 0.0);*/
}

/*
=================
S_StopStreaming
=================
*/
void S_StopStreaming (void)
{
//	int			processed;
//	unsigned	buffer;

	if ( !ogg_started )
		return;

	if ( !s_streamingChannel )
		return;		// Already stopped

	s_streamingChannel->streaming = false;

	// Clean up the source
/*	qalSourceStop(s_streamingChannel->sourceNum);

	qalGetSourcei(s_streamingChannel->sourceNum, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0){
		while (processed--){
			qalSourceUnqueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);
			qalDeleteBuffers(1, &buffer);
		}
	}

	qalSourcei(s_streamingChannel->sourceNum, AL_BUFFER, 0);

	// FIXME: OpenAL bug?
	qalDeleteSources(1, &s_streamingChannel->sourceNum);
	qalGenSources(1, &s_streamingChannel->sourceNum); */

	s_streamingChannel = NULL;
}


// =====================================================================


/*
==========
S_OGG_Init

Initialize the Ogg Vorbis subsystem
Based on code by QuDos
==========
*/
void S_OGG_Init (void)
{
	if ( ogg_started )
		return;

	// Cvars
	ogg_loopcount = Cvar_Get ( "ogg_loopcount", "5", CVAR_ARCHIVE );
	Cvar_SetDescription ( "ogg_loopcount", "Sets number of music track loops until the ambient music track is played." );
	ogg_ambient_track = Cvar_Get ( "ogg_ambient_track", "track11", CVAR_ARCHIVE );
	Cvar_SetDescription ( "ogg_ambient_track", "Sets the name of the Ogg Vorbis file used for the ambient music track." );

	// Console commands
	Cmd_AddCommand ( "ogg", S_OGG_ParseCmd );

	// Build list of files
	Com_Printf("Searching for Ogg Vorbis files...\n");
	ogg_numfiles = 0;
	S_OGG_LoadFileList ();
//	Com_Printf ("%d Ogg Vorbis files found.\n", ogg_numfiles);
	Com_Printf ("%d Ogg Vorbis files found (%i imported).\n", ogg_numfiles, ogg_numImportFiles);

	// Initialize variables
	if (ogg_first_init) {
	//	srand(time(NULL));
	//	ogg_curfile = -1;
		ogg_status = STOP;
		ogg_first_init = false;
	}

	ogg_started = true;
}


/*
==========
S_OGG_Shutdown

Shutdown the Ogg Vorbis subsystem
Based on code by QuDos
==========
*/
void S_OGG_Shutdown (void)
{
	int		i;

	if ( !ogg_started )
		return;

	S_StopBackgroundTrack ();

	// Free the list of files
	for (i = 0; i < ogg_numfiles; i++)
	{
#ifdef USE_OGGLIST_STRUCT
		if (ogg_filelist[i].importFilePath) {
			Z_Free (ogg_filelist[i].importFilePath);
			ogg_filelist[i].importFilePath = NULL;
		}
#else	// USE_OGGLIST_STRUCT
		Z_Free (ogg_filelist[i]);
		ogg_filelist[i] = NULL;
#endif	// USE_OGGLIST_STRUCT
	}
	if (ogg_numfiles > 0)
		Z_Free (ogg_filelist);
	ogg_filelist = NULL;
	ogg_numfiles = 0;
	ogg_numImportFiles = 0;

	// Remove console commands
	Cmd_RemoveCommand ( "ogg" );

	ogg_started = false;
}


/*
==========
S_OGG_Restart

Reinitialize the Ogg Vorbis subsystem
Based on code by QuDos
==========
*/
void S_OGG_Restart (void)
{
	S_OGG_Shutdown ();
	S_OGG_Init ();
}


/*
==========
S_OGG_ItemInOggList

Checks if a file is in the Ogg Vorbis filelist
==========
*/
qboolean S_OGG_ItemInOggList (const char *check, int num, oggListItem_t *list)
{
	int		i;

	if ( !check || !list )
		return false;

	for (i = 0; i <num; i++)
	{
		if ( !Q_strcasecmp((char *)check, (char *)list[i].name) )
			return true;
	}

	return false;
}


/*
==========
S_OGG_LoadFileList

Load list of Ogg Vorbis files in music/
Based on code by QuDos
==========
*/
void S_OGG_LoadFileList (void)
{
	char		*p, *path = NULL;
	char		**list;			// List of .ogg files
//	char		findname[MAX_OSPATH];
	char		lastPath[MAX_OSPATH];	// Knightmare added
	int			i, j, len, numfiles = 0;
	// Knightmare added
	oggImport_t	oi;
	int			ogg_numImports = 0;		// Number of ogg_json imports
	int			jsonSize;
	byte		*jsonData = NULL;
	char		*jsonStr = NULL;
	qboolean	json_parsed = false;
	const char	*importRootPath = NULL;
	char		importFullPath[MAX_OSPATH];
	size_t		nameLen;

#ifdef USE_OGGLIST_STRUCT
	ogg_filelist = static_cast< oggListItem_t * >( Z_Malloc( sizeof( oggListItem_t ) * MAX_OGGLIST ) );
#else
	ogg_filelist = Z_Malloc(sizeof(char *) * MAX_OGGLIST);
#endif	// USE_OGGLIST_STRUCT
	lastPath[0] = 0;

	list = FS_GetFileList("music", "ogg", &numfiles);
	// Add valid Ogg Vorbis files to the list
	for (i=0; i < numfiles && ogg_numfiles < MAX_OGGLIST; i++)
	{
		if (!list || !list[i])
			continue;

		len = (int)strlen(list[i]);
		if ( strcmp(list[i]+max(len-4, 0), ".ogg") )
			continue;

		p = list[i];

#ifdef USE_OGGLIST_STRUCT
		if ( !S_OGG_ItemInOggList(p, ogg_numfiles, ogg_filelist) ) // check if already in list
		{
			Q_strncpyz (ogg_filelist[ogg_numfiles].name, sizeof(ogg_filelist[ogg_numfiles].name), p);
			ogg_numfiles++;
		}
#else	// USE_OGGLIST_STRUCT
		if ( !FS_ItemInList(p, ogg_numfiles, ogg_filelist) ) // check if already in list
		{
			ogg_filelist[ogg_numfiles] = CopyString (p);
			ogg_numfiles++;
		}
#endif	// USE_OGGLIST_STRUCT
	}

	// Free ogg filelist
	if (numfiles) {
		FS_FreeFileList (list, numfiles);
		list = NULL;
		numfiles = 0;
	}

	// Load imported Ogg files based on ogg_json scripts
#ifdef USE_OGGLIST_STRUCT
	list = FS_GetFileList("music", "ogg_json", &numfiles);
	// Add valid ogg_json files to the list
	for (i=0; i < numfiles && ogg_numfiles < MAX_OGGLIST; i++)
	{
		if (!list || !list[i])
			continue;

		len = (int)strlen(list[i]);
		if ( strcmp(list[i]+max(len-9, 0), ".ogg_json") )
			continue;

		path = list[i];

		jsonSize = FS_LoadFile (path, (void**)&jsonData);
		jsonStr = (char *)jsonData;
		if (jsonStr)
		{
			json_parsed = Com_ParseOggJSON (path, jsonStr, jsonSize, &oi, false);
			if (json_parsed)
			{
				if ( (oi.virtualName[0] != 0) && (oi.importGame[0] != 0) && (oi.importPath[0] != 0) )
				{
					ogg_numImports++;
					importRootPath = NULL;
					Com_DPrintf ("virtualName: %s\n", oi.virtualName);
					Com_DPrintf ("importGame: %s\n", oi.importGame);
					for (j = 0; j < MAX_OGG_IMPORT_PATHS; j++) {
						if (oi.importPath[j][0] != 0)
							Com_DPrintf ("importPath%i: %s\n", j, oi.importPath[j]);
					}

					// Get import root path for this ogg_json script
					if ( ( !Q_stricmp( oi.importGame, "Quake" ) || !Q_stricmp( oi.importGame, "Quake1" ) || !Q_stricmp( oi.importGame, "Quake1RR" ) ) &&
						(fs_quakeimportpath != NULL) && (fs_quakeimportpath->string[0] != 0) ) {
						importRootPath = fs_quakeimportpath->string;
					}
					else if ( !Q_stricmp( oi.importGame, "Quake2RR" )  &&
						(fs_quake2rrimportpath != NULL) && (fs_quake2rrimportpath->string[0] != 0) ) {
						importRootPath = fs_quake2rrimportpath->string;
					}
					// Add support for any other import root paths here

					// Check import paths for actual ogg file, add to ogg_filelist if found
					if (importRootPath != NULL)
					{
						for (j = 0; j < MAX_OGG_IMPORT_PATHS; j++)
						{
							if (oi.importPath[j][0] != 0)
							{
								snprintf (importFullPath, sizeof(importFullPath), "%s/%s", importRootPath, oi.importPath[j]);
								if ( FS_DirectFileExists(importFullPath) )
								{
									nameLen = strlen(oi.virtualName);
									if ( Q_stricmp( oi.virtualName + nameLen - 4, ".ogg" ) != 0 ) {
										snprintf (ogg_filelist[ogg_numfiles].name, sizeof(ogg_filelist[ogg_numfiles].name), "%s.ogg", oi.virtualName);
									}
									else {
										Q_strncpyz (ogg_filelist[ogg_numfiles].name, sizeof(ogg_filelist[ogg_numfiles].name), oi.virtualName);
									}
									ogg_filelist[ogg_numfiles].importFilePath = CopyString (importFullPath);
									ogg_numfiles++;
									ogg_numImportFiles++;
									break;
								}
							}
						}
					}
				}
			}
			FS_FreeFile (jsonData);
			jsonData = NULL;
			jsonStr = NULL;
		}
	}
	Com_DPrintf ("%i ogg_json import scripts found.\n", ogg_numImports);

	// Free temp filelist
	if (numfiles) {
		FS_FreeFileList (list, numfiles);
		list = NULL;
		numfiles = 0;
	}
#endif	// USE_OGGLIST_STRUCT
}

// =====================================================================

/*
=================
S_OGG_PlayCmd
Based on code by QuDos
=================
*/
void S_OGG_PlayCmd (void)
{
	char	name[MAX_QPATH];

	if (Cmd_Argc() < 3) {
		Com_Printf("Usage: ogg play {track}\n");
		return;
	}
	snprintf(name, sizeof(name), "music/%s.ogg", Cmd_Argv(2) );
	S_StartBackgroundTrack (name, name);
}


/*
=================
S_OGG_StatusCmd
Based on code by QuDos
=================
*/
void S_OGG_StatusCmd (void)
{
	char	*trackName;

	if (s_bgTrack.ambient_looping)
		trackName = s_bgTrack.ambientName;
	else if (s_bgTrack.looping)
		trackName = s_bgTrack.loopName;
	else
		trackName = s_bgTrack.introName;

	switch (ogg_status) {
	case PLAY:
		Com_Printf("Playing file %s at %0.2f seconds.\n",
		    trackName, ov_time_tell(s_bgTrack.vorbisFile));
		break;
	case PAUSE:
		Com_Printf("Paused file %s at %0.2f seconds.\n",
		    trackName, ov_time_tell(s_bgTrack.vorbisFile));
		break;
	case STOP:
		Com_Printf("Stopped.\n");
		break;
	}
}


/*
==========
S_OGG_ListCmd

List Ogg Vorbis files
Based on code by QuDos
==========
*/
void S_OGG_ListCmd (void)
{
	int i;

	if (ogg_numfiles <= 0) {
		Com_Printf("No Ogg Vorbis files to list.\n");
		return;
	}

	for (i = 0; i < ogg_numfiles; i++)
	{
#ifdef USE_OGGLIST_STRUCT
		if ( (ogg_filelist[i].importFilePath != NULL) && (ogg_filelist[i].importFilePath[0] != 0) )
			Com_Printf ("%d %s (imported from %s)\n", i+1, ogg_filelist[i].name, ogg_filelist[i].importFilePath);
		else
			Com_Printf ("%d %s\n", i+1, ogg_filelist[i].name);
#else	// USE_OGGLIST_STRUCT
		Com_Printf ("%d %s\n", i+1, ogg_filelist[i]);
#endif	// USE_OGGLIST_STRUCT
	}

	Com_Printf("%d Ogg Vorbis files.\n", ogg_numfiles);
}


/*
=================
S_OGG_ParseCmd

Parses OGG commands
Based on code by QuDos
=================
*/
void S_OGG_ParseCmd (void)
{
	char	*command;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: ogg {play | pause | resume | stop | status | list}\n");
		return;
	}

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "play") == 0) {
		S_OGG_PlayCmd ();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0) {
		if (ogg_status == PLAY)
			ogg_status = PAUSE;
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0) {
		if (ogg_status == PAUSE)
			ogg_status = PLAY;
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0) {
		S_StopBackgroundTrack ();
		return;
	}

	/*if (Q_strcasecmp(command, "seek") == 0) {
		Com_Printf ("Ogg Vorbis seek command\n");
		return;
	}*/

	if (Q_strcasecmp(command, "status") == 0) {
		S_OGG_StatusCmd ();
		return;
	}

	if (Q_strcasecmp(command, "list") == 0) {
		S_OGG_ListCmd ();
		return;
	}

	Com_Printf("Usage: ogg {play | pause | resume | stop | status | list}\n");
}


#if 0
/*
=================
S_StreamRawSamples

Cinematic streaming
=================
*/
void S_StreamRawSamples (const byte *data, int samples, int rate, int width, int channels)
{
	int			processed, state, size;
	unsigned	format, buffer;

	if (!s_initialized)
		return;

	if (!s_streamingChannel)
		return;

	// Unqueue and delete any processed buffers
	qalGetSourcei(s_streamingChannel->sourceNum, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0){
		while (processed--){
			qalSourceUnqueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);
			qalDeleteBuffers(1, &buffer);
		}
	}

	// Calculate buffer size
	size = samples * width * channels;

	// Set buffer format
	if (width == 2)
	{
		if (channels == 2)
			format = AL_FORMAT_STEREO16;
		else
			format = AL_FORMAT_MONO16;
	}
	else
	{
		if (channels == 2)
			format = AL_FORMAT_STEREO8;
		else
			format = AL_FORMAT_MONO8;
	}

	// Upload and queue the new buffer
	qalGenBuffers(1, &buffer);
	qalBufferData(buffer, format, (byte *)data, size, rate);
	qalSourceQueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);

	// Update volume
	qalSourcef(s_streamingChannel->sourceNum, AL_GAIN, s_sfxVolume->value);

	// If not playing, then do so
	qalGetSourcei(s_streamingChannel->sourceNum, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING)
		qalSourcePlay(s_streamingChannel->sourceNum);
}
#endif

#endif // OGG_SUPPORT
