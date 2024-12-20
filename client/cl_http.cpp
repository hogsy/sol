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

#include "client.h"

#ifdef USE_CURL

enum
{
	HTTPDL_ABORT_NONE,
	HTTPDL_ABORT_SOFT,
	HTTPDL_ABORT_HARD
};

static CURLM	*multi_handle = NULL;
static int		handleCount = 0;
static int		pendingCount = 0;
static int		abortDownloads = HTTPDL_ABORT_NONE;
static qboolean	downloading_pak = false;
static qboolean	httpDown = false;
static qboolean	thisMapAbort = false;		// Knightmare- whether to fall back to UDP for this map
static int		prevSize;					// Knightmare- for KBps counter
static qboolean	downloadFileList = true;	// YQ2 addition for downloading filelist once

/*
===============================
R1Q2 HTTP Downloading Functions
===============================
HTTP downloading is used if the server provides a content
server URL in the connect message. Any missing content the
client needs will then use the HTTP server instead of auto
downloading via UDP. CURL is used to enable multiple files
to be downloaded in parallel to improve performance on high
latency links when small files such as textures are needed.
Since CURL natively supports gzip content encoding, any files
on the HTTP server should ideally be gzipped to conserve
bandwidth.
*/


// Knightmare- store the names of last HTTP downloads from this server that failed
#define NUM_HTTP_FAIL_DLDS 512
failedDownload_t lastFailedHTTPDownload[NUM_HTTP_FAIL_DLDS];
static unsigned failed_HTTP_Dl_ListIndex;

/*
===============
CL_InitFailedHTTPDownloadList
===============
*/
void CL_InitFailedHTTPDownloadList (void)
{
	int		i;

	for (i=0; i<NUM_HTTP_FAIL_DLDS; i++) {
		snprintf (lastFailedHTTPDownload[i].fileName, sizeof(lastFailedHTTPDownload[i].fileName), "\0");
		lastFailedHTTPDownload[i].failCount = 0;
		lastFailedHTTPDownload[i].isDuplicated = false;
	}

	failed_HTTP_Dl_ListIndex = 0;
}


/*
===============
CL_CheckHTTPDownloadFailed
===============
*/
qboolean CL_CheckHTTPDownloadFailed (const char *name)
{
	int		i;

	if ( !name || (name[0] == '\0') )
		return true;

	for (i=0; i<NUM_HTTP_FAIL_DLDS; i++)
	{
		if ( (strlen(lastFailedHTTPDownload[i].fileName) > 0) && !strcmp(name, lastFailedHTTPDownload[i].fileName) )
		{	// We already tried downloading this, server didn't have it
			// If this entry is a duplicated download, only return true if it has failed twice.
			if ( lastFailedHTTPDownload[i].isDuplicated ) {
				if (lastFailedHTTPDownload[i].failCount > 1)
					return true;
				else
					return false;
			}
			else {	// Not a duplicated download
				return true;
			}
		}
	}

	return false;
}


/*
===============
CL_AddToFailedHTTPDownloadList
===============
*/
void CL_AddToFailedHTTPDownloadList (const char *name, qboolean isDuplicated)
{
	int			i;
	qboolean	found = false;

	if ( !name || (name[0] == '\0') )
		return;

	// check if this name is already in the table
	for (i=0; i<NUM_HTTP_FAIL_DLDS; i++)
	{
		if ( (strlen(lastFailedHTTPDownload[i].fileName) > 0) && !strcmp(name, lastFailedHTTPDownload[i].fileName) )
		{
			// increment failCount for duplicated download
			if (lastFailedHTTPDownload[i].isDuplicated) {
				lastFailedHTTPDownload[i].failCount++;
			}
			found = true;
			break;
		}
	}

	// if it isn't already in the table, then we need to add it
	if (!found)
	{
		snprintf (lastFailedHTTPDownload[failed_HTTP_Dl_ListIndex].fileName, sizeof(lastFailedHTTPDownload[failed_HTTP_Dl_ListIndex].fileName), "%s", name);
		lastFailedHTTPDownload[failed_HTTP_Dl_ListIndex].failCount = 1;
		lastFailedHTTPDownload[failed_HTTP_Dl_ListIndex].isDuplicated = isDuplicated;
		failed_HTTP_Dl_ListIndex++;

		// wrap around to start of list
		if (failed_HTTP_Dl_ListIndex >= NUM_HTTP_FAIL_DLDS)
			failed_HTTP_Dl_ListIndex = 0;
	}	
}


/*
===============
CL_HTTP_GetQ2ProGameDir

Gets the gamedir to be used by the URL generator
to determine the remote file path.
===============
*/
const char *CL_HTTP_GetQ2ProGameDir (void)
{
	if (cl.gamedir[0] == '\0')
		return BASEDIRNAME;
	else
		return cl.gamedir;
}
// end Knightmare


/*
===============
CL_HTTP_Reset_KBps_counter

Just a wrapper for CL_Download_Reset_KBps_counter(),
also resets prevSize.
===============
*/
static void CL_HTTP_Reset_KBps_counter (void)
{
	prevSize = 0;
	CL_Download_Reset_KBps_counter ();
}


/*
===============
CL_HTTP_Calculate_KBps

Essentially a wrapper for CL_Download_Calcualte_KBps(),
calcuates bytes since last update and calls the above.
===============
*/
static void CL_HTTP_Calculate_KBps (int curSize, int totalSize)
{
	int byteDistance = curSize - prevSize;

	CL_Download_Calculate_KBps (byteDistance, totalSize);
	prevSize = curSize;
}


/*
===============
CL_HTTP_Progress

libcurl callback to update progress info. Mainly just used as
a way to cancel the transfer if required.
===============
*/
static int /*EXPORT*/ CL_HTTP_Progress (void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	dlhandle_t *dl;

	dl = (dlhandle_t *)clientp;

	dl->position = (unsigned)dlnow;

	// don't care which download shows as long as something does :)
	if (!abortDownloads)
	{
		Q_strncpyz (cls.downloadname, sizeof(cls.downloadname), dl->queueEntry->quakePath);
		cls.downloadposition = dl->position;

		if (dltotal) {
			CL_HTTP_Calculate_KBps ((int)dlnow, (int)dltotal);	// Knightmare- added KB/s counter
			cls.downloadpercent = (int)((dlnow / dltotal) * 100.0f);
		}
		else
			cls.downloadpercent = 0;
	}

	return abortDownloads;
}


/*
===============
CL_HTTP_Header

libcurl callback to update header info.
===============
*/
static size_t /*EXPORT*/ CL_HTTP_Header (void *ptr, size_t size, size_t nmemb, void *stream)
{
	char	headerBuff[1024];
	size_t	bytes;
	size_t	len;

	bytes = size * nmemb;

	if (bytes <= 16)
		return bytes;

	//memset (headerBuff, 0, sizeof(headerBuff));
	//memcpy (headerBuff, ptr, min(bytes, sizeof(headerBuff)-1));
	if (bytes < sizeof(headerBuff)-1)
		len = bytes;
	else
		len = sizeof(headerBuff)-1;

	Q_strncpyz (headerBuff, len, (char*)ptr);

	if (!Q_strncasecmp ( headerBuff, "Content-Length: ", 16 ) )
	{

		dlhandle_t *dl = static_cast< dlhandle_t * >( stream );

		if (dl->file)
			dl->fileSize = strtoul (headerBuff + 16, NULL, 10);
	}

	return bytes;
}


/*
===============
CL_EscapeHTTPPath

Properly escapes a path with HTTP %encoding. libcurl's function
seems to treat '/' and such as illegal chars and encodes almost
the entire URL...
===============
*/
static void CL_EscapeHTTPPath (const char *filePath, char *escaped, size_t escapedSize)
{
	int		i;
	size_t	len, remainingSize;
	char	*p;

	p = escaped;
	remainingSize = escapedSize;

	len = strlen (filePath);
	for (i = 0; i < len; i++)
	{
		if (!isalnum (filePath[i]) && filePath[i] != ';' && filePath[i] != '/' &&
			filePath[i] != '?' && filePath[i] != ':' && filePath[i] != '@' && filePath[i] != '&' &&
			filePath[i] != '=' && filePath[i] != '+' && filePath[i] != '$' && filePath[i] != ',' &&
			filePath[i] != '[' && filePath[i] != ']' && filePath[i] != '-' && filePath[i] != '_' &&
			filePath[i] != '.' && filePath[i] != '!' && filePath[i] != '~' && filePath[i] != '*' &&
			filePath[i] != '\'' && filePath[i] != '(' && filePath[i] != ')')
		{
			snprintf (p, remainingSize, "%%%02x", filePath[i]);
			p += 3;
			remainingSize -= 3;
		}
		else
		{
			*p = filePath[i];
			p++;
			remainingSize--;
		}
	}
	p[0] = 0;

	// using ./ in a url is legal, but all browsers condense the path and some IDS / request
	// filtering systems act a bit funky if http requests come in with uncondensed paths.
	len = strlen(escaped);
	p = escaped;
	while ((p = strstr (p, "./")))
	{
		memmove (p, p+2, len - (p - escaped) - 1);
		len -= 2;
	}
}


/*
===============
CL_HTTP_CurlWrite

Adapted from Yamagi Quake2.
libcurl callback for file writing.
===============
*/
static size_t CL_HTTP_CurlWrite (char *data, size_t size, size_t nmemb, void *userdata)
{
	dlhandle_t	*dl;

	dl = (dlhandle_t *)userdata;
	
	return fwrite(data, size, nmemb, dl->file);
}


/*
===============
CL_HTTP_Recv

libcurl callback for filelists.
===============
*/
static size_t /*EXPORT*/ CL_HTTP_Recv (void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t		bytes;
	dlhandle_t	*dl;

	dl = (dlhandle_t *)stream;

	bytes = size * nmemb;

	if (!dl->fileSize)
	{
		dl->fileSize = bytes > 131072 ? bytes : 131072;
	//	dl->tempBuffer = Z_TagMalloc ((int)dl->fileSize, TAGMALLOC_CLIENT_DOWNLOAD);
		dl->tempBuffer = static_cast< char * >( Z_TagMalloc( ( int ) dl->fileSize, 0 ) );
	}
	else if (dl->position + bytes >= dl->fileSize - 1)
	{
		char		*tmp;

		tmp = dl->tempBuffer;

	//	dl->tempBuffer = Z_TagMalloc ((int)(dl->fileSize*2), TAGMALLOC_CLIENT_DOWNLOAD);
		dl->tempBuffer = static_cast< char * >( Z_TagMalloc( ( int ) ( dl->fileSize * 2 ), 0 ) );
		memcpy (dl->tempBuffer, tmp, dl->fileSize);
		Z_Free (tmp);
		dl->fileSize *= 2;
	}

	memcpy (dl->tempBuffer + dl->position, ptr, bytes);
	dl->position += bytes;
	dl->tempBuffer[dl->position] = 0;

	return bytes;
}


int /*EXPORT*/ CL_CURL_Debug (CURL *c, curl_infotype type, char *data, size_t size, void * ptr)
{
	if (type == CURLINFO_TEXT)
	{
		char	buff[4096];
	//	if (size > sizeof(buff)-1)
	//		size = sizeof(buff)-1;
		if (size > sizeof(buff))	// Q_strncpyz takes size, not size-1
			size = sizeof(buff);
		Q_strncpyz (buff, size, data);
		Com_Printf ("[HTTP] DEBUG: %s\n", buff);
	}

	return 0;
}


/*void CL_RemoveHTTPDownload (const char *quakePath)
{

}*/


/*
===============
CL_RemoveDownloadFromQueue

Adapted from Yamagi Quake2.
Removes an entry from the download queue.
===============
*/
qboolean CL_RemoveDownloadFromQueue (dlqueue_t *entry)
{
	dlqueue_t	*last = &cls.downloadQueue;
	dlqueue_t	*cur = last->next;

	while (cur)
	{
		if (last->next == entry)
		{
			last->next = cur->next;
			Z_Free (cur);
			cur = NULL;
			return true;
		}
		last = cur;
		cur = cur->next;
	}
	return false;
}


/*
===============
CL_StartHTTPDownload

Actually starts a download by adding it to the curl multi
handle.
===============
*/
static void CL_StartHTTPDownload (dlqueue_t *entry, dlhandle_t *dl)
{
	size_t		len;
	char		remoteFilePath[MAX_OSPATH];		// Knightmare added
	char		escapedFilePath[MAX_QPATH*4];
	
	// yet another hack to accomodate filelists, how i wish i could push :(
	// NULL file handle indicates filelist.
	len = strlen (entry->quakePath);
	if ( (len > 9) && !strcmp (entry->quakePath + len - 9, ".filelist"))
	{
		dl->file = NULL;
		CL_EscapeHTTPPath (entry->quakePath, escapedFilePath, sizeof(escapedFilePath));
	}
	else
	{
		CL_HTTP_Reset_KBps_counter ();	// Knightmare- for KB/s counter

		snprintf (dl->filePath, sizeof(dl->filePath), "%s/%s", FS_Downloaddir(), entry->quakePath);	// was FS_Gamedir()
	//	snprintf (remoteFilePath, sizeof(remoteFilePath), "/%s/%s", cl.gamedir, entry->quakePath);	// always use cl.gamedir (with leading slash) for remote server path
		// Knightmare- cvar-switchable download path
		if ( !entry->useQ2ProPath )	{	// R1Q2 path
			snprintf (remoteFilePath, sizeof(remoteFilePath), "/%s/%s", cl.gamedir, entry->quakePath);
		}
		else {	// Q2Pro path
			snprintf (remoteFilePath, sizeof(remoteFilePath), "/%s/%s", CL_HTTP_GetQ2ProGameDir(), entry->quakePath);	
		}
		// end Knightmare

	//	CL_EscapeHTTPPath (dl->filePath, escapedFilePath, sizeof(escapedFilePath));
		CL_EscapeHTTPPath (remoteFilePath, escapedFilePath, sizeof(escapedFilePath));

		// Knightmare- if this is an alt queue entry, use alternate temp filename
		if ( entry->isDuplicated && entry->isAltEntry )
			Q_strncatz (dl->filePath, sizeof(dl->filePath), ".tmp2");
		else
			Q_strncatz (dl->filePath, sizeof(dl->filePath), ".tmp");

		FS_CreatePath (dl->filePath);

		// don't bother with http resume... too annoying if server doesn't support it.
		dl->file = fopen (dl->filePath, "wb");
		if (!dl->file)
		{
			Com_Printf ("CL_StartHTTPDownload: Couldn't open %s for writing.\n", dl->filePath);
			entry->state = DLQ_STATE_DONE;
			pendingCount--;	// FS: fix for curl_update limbo
		//	CL_RemoveHTTPDownload (entry->quakePath);
			return;
		}
	}

	dl->tempBuffer = NULL;
	dl->speed = 0;
	dl->fileSize = 0;
	dl->position = 0;
	dl->queueEntry = entry;

	if (!dl->curl)
		dl->curl = qcurl_easy_init ();

	snprintf (dl->URL, sizeof(dl->URL), "%s%s", cls.downloadServer, escapedFilePath);

	qcurl_easy_setopt (dl->curl, CURLOPT_ENCODING, "");
//	qcurl_easy_setopt (dl->curl, CURLOPT_DEBUGFUNCTION, CL_CURL_Debug);
//	qcurl_easy_setopt (dl->curl, CURLOPT_VERBOSE, 1);
	qcurl_easy_setopt (dl->curl, CURLOPT_NOPROGRESS, 0);
	qcurl_easy_setopt (dl->curl, CURLOPT_WRITEDATA, dl);	// Yamagi Quake2 moved here
	if (dl->file)
	{
		qcurl_easy_setopt (dl->curl, CURLOPT_WRITEFUNCTION, CL_HTTP_CurlWrite);	// from Yamagi Quake2
	}
	else
	{
		qcurl_easy_setopt (dl->curl, CURLOPT_WRITEFUNCTION, CL_HTTP_Recv);
	}
	qcurl_easy_setopt (dl->curl, CURLOPT_PROXY, cl_http_proxy->string);
	qcurl_easy_setopt (dl->curl, CURLOPT_FOLLOWLOCATION, 1);
	qcurl_easy_setopt (dl->curl, CURLOPT_MAXREDIRS, 5);
	qcurl_easy_setopt (dl->curl, CURLOPT_WRITEHEADER, dl);
	qcurl_easy_setopt (dl->curl, CURLOPT_HEADERFUNCTION, CL_HTTP_Header);
	qcurl_easy_setopt (dl->curl, CURLOPT_PROGRESSFUNCTION, CL_HTTP_Progress);
	qcurl_easy_setopt (dl->curl, CURLOPT_PROGRESSDATA, dl);
	qcurl_easy_setopt (dl->curl, CURLOPT_USERAGENT, Cvar_VariableString ("version"));
	qcurl_easy_setopt (dl->curl, CURLOPT_REFERER, cls.downloadReferer);
	qcurl_easy_setopt (dl->curl, CURLOPT_URL, dl->URL);

	if (qcurl_multi_add_handle (multi_handle, dl->curl) != CURLM_OK)
	{
		Com_Printf ("curl_multi_add_handle: error\n");
		dl->queueEntry->state = DLQ_STATE_DONE;
		return;
	}

	handleCount++;
//	Com_Printf ("started dl: hc = %d\n", LOG_GENERAL, handleCount);
	Com_DPrintf  ("CL_StartHTTPDownload: Fetching %s...\n", dl->URL);
	dl->queueEntry->state = DLQ_STATE_RUNNING;
}


/*
===============
CL_InitHTTPDownloads

Init libcurl and multi handle.
===============
*/
void CL_InitHTTPDownloads (void)
{
	if ( !qcurl_initialized )	// check if qcurl not initialized
		return;

	Com_Printf ("Initializing curl state...");
	qcurl_global_init (CURL_GLOBAL_NOTHING);
	Com_Printf (" done.\n");

//	Com_Printf ("%s initialized.\n", LOG_CLIENT, qcurl_version());
}


/*
===============
CL_SetHTTPServer

A new server is specified, so we nuke all our state.
===============
*/
void CL_SetHTTPServer (const char *URL)
{
	dlqueue_t	*q, *last;
	char		*fixedURL = NULL;
	size_t		URLlen;

	if ( !qcurl_initialized ) {	// check if qcurl not initialized
	//	cls.downloadServer[0] = '\0';
		return;
	}

	CL_HTTP_Cleanup (false);

	q = &cls.downloadQueue;

	last = NULL;

	while (q->next)
	{
		q = q->next;

		if (last)
			Z_Free (last);

		last = q;
	}

	if (last)
		Z_Free (last);

	if (multi_handle)
		Com_Error (ERR_DROP, "CL_SetHTTPServer: Still have old handle");

	multi_handle = qcurl_multi_init ();
	
	memset (&cls.downloadQueue, 0, sizeof(cls.downloadQueue));

	abortDownloads = HTTPDL_ABORT_NONE;
	handleCount = pendingCount = 0;

//	strncpy (cls.downloadServer, URL, sizeof(cls.downloadServer)-1);

	// from YQ2: remove trailing '/' from URL
	URLlen = strlen(URL);
	fixedURL = CopyString ((char *)URL);
	if (fixedURL[URLlen-1] == '/') {
		fixedURL[URLlen-1] = '\0';
	}

	// From Q2Pro- ignore non-HTTP DL server URLs
	if ( (strncmp(fixedURL, "http://", 7) != 0) && (strncmp(fixedURL, "https://", 8) != 0) ) {
		Com_Printf("[HTTP] Ignoring download server with non-HTTP protocol.\n");
		return;
	}

	Q_strncpyz (cls.downloadServer, sizeof(cls.downloadServer), fixedURL);
	Z_Free (fixedURL);
	fixedURL = NULL;

	// FS: Added because Whale's Weapons HTTP server rejects you after a lot of 404s.  Then you lose HTTP until a hard reconnect.
	cls.downloadServerRetry[0] = 0;
	downloadFileList = true;	// YQ2 addition- re-enable generic file list

	CL_InitFailedHTTPDownloadList ();	// Knightmare- init failed HTTP downloads list
}


/*
===============
CL_CancelHTTPDownloads

Cancel all downloads and nuke the queue.
===============
*/
void CL_CancelHTTPDownloads (qboolean permKill)
{
	dlqueue_t	*q;

	if ( !qcurl_initialized )	// check if qcurl not initialized
		return;

	if (permKill)
	{
		CL_ResetPrecacheCheck ();
		abortDownloads = HTTPDL_ABORT_HARD;
	}
	else
		abortDownloads = HTTPDL_ABORT_SOFT;

	q = &cls.downloadQueue;

	while (q->next)
	{
		q = q->next;
		if (q->state == DLQ_STATE_NOT_STARTED)
			q->state = DLQ_STATE_DONE;
	}

	if ( !pendingCount && !handleCount && (abortDownloads == HTTPDL_ABORT_HARD) )
		cls.downloadServer[0] = 0;

	pendingCount = 0;
}


/*
===============
CL_AllocDLQueueEntry

Allocates a new DL queue entry
===============
*/
dlqueue_t *CL_AllocDLQueueEntry (const char *quakePath, qboolean isPak, qboolean isFilelist, qboolean useQ2ProPath, qboolean allowDuplicate)
{
	dlqueue_t	*q, *check, *last;
	
	if (isFilelist)	// Knightmare- always insert filelist at head of queue
	{
		q = static_cast< dlqueue_t * >( Z_TagMalloc( sizeof( dlqueue_t ), 0 ) );
		q->next = cls.downloadQueue.next;
		cls.downloadQueue.next = q;
	}
	else if (isPak)	// Knightmare- insert paks near head of queue, before first non-pak
	{
		last = &cls.downloadQueue;
		check = cls.downloadQueue.next;
		while (check)
		{
			// avoid sending duplicate requests
			if ( !strcmp (quakePath, check->quakePath) && !allowDuplicate )
				return NULL;

			if (!check->isPak)	// insert before this entry
				break;

			last = check;
			check = check->next;
		}
		q = static_cast< dlqueue_t * >( Z_TagMalloc( sizeof( dlqueue_t ), 0 ) );
		q->next = check;
		last->next = q;
	}
	else
	{
		q = &cls.downloadQueue;
		while (q->next)
		{
			q = q->next;

			// avoid sending duplicate requests
			if ( !strcmp (quakePath, q->quakePath) && !allowDuplicate )
				return NULL;
		}
	//	q->next = Z_TagMalloc (sizeof(*q), TAGMALLOC_CLIENT_DOWNLOAD);
		q->next = static_cast< dlqueue_t * >( Z_TagMalloc( sizeof( dlqueue_t ), 0 ) );
		q = q->next;
		q->next = NULL;
	}

	q->state = DLQ_STATE_NOT_STARTED;
	Q_strncpyz (q->quakePath, sizeof(q->quakePath), quakePath);
	q->isPak = isPak;				// Knightmare added
	q->useQ2ProPath = useQ2ProPath;	// Knightmare added
	q->isDuplicated = false;		// Knightmare added
	q->isAltEntry = false;			// Knightmare added

	return q;
}


/*
===============
CL_QueueHTTPDownload

Called from the precache check to queue a download. Return value of
false will cause standard UDP downloading to be used instead.
===============
*/
qboolean CL_QueueHTTPDownload (const char *quakePath)
{
	size_t		len;
	char		quakePathFixed[MAX_OSPATH];
	dlqueue_t	*q, *q2;
	char		*tmp;
	qboolean	needList = false, isPak = false, isFilelist = false;

	if ( !qcurl_initialized )	// check if qcurl not initialized
		return false;

	// no http server (or we got booted)
	if (!cls.downloadServer[0] || abortDownloads || thisMapAbort || !cl_http_downloads->integer)
		return false;

//	needList = false;

	// first download queued, so we want the mod filelist
//	if ( !cls.downloadQueue.next && cl_http_filelists->integer ) {
	if ( downloadFileList && cl_http_filelists->integer ) {
		needList = true;
		downloadFileList = false;
	}

	Q_strncpyz (quakePathFixed, sizeof(quakePathFixed), quakePath);
	// Convert filename to lowercase if enabled
	if ( cl_http_download_lowercase->integer )
		Q_strlwr (quakePathFixed);
	// Change any '\\' in filename to '/'
	tmp = &quakePathFixed[0];
	while ( *tmp != 0 )
	{
		if ( *tmp == '\\' ) 
			*tmp = '/';
		tmp++;
	}

	len = strlen (quakePathFixed);
	if ( (len > 4) && ( !Q_stricmp( ( char * ) quakePathFixed + len - 4, ".pak" ) || !Q_stricmp( ( char * ) quakePathFixed + len - 4, ".pk3" ) ) )
		isPak = true;
	if ( (len > 9) && !Q_stricmp( ( char * ) quakePathFixed + len - 9, ".filelist" ) )
		isFilelist = true;

	// Knightmare- don't try again to download via HTTP a file that failed
	if ( !isFilelist /*&& !needList*/ ) {
		if (CL_CheckHTTPDownloadFailed((char *)quakePathFixed)) {
		//	Com_Printf ("[HTTP] Refusing to download %s again, already in failed HTTP download list.\n", quakePathFixed);
			return true;
		}
	}

//	q = CL_AllocDLQueueEntry (quakePathFixed, isPak, isFilelist, false, false);
	// Knightmare- cvar-switchable download path
	switch (cl_http_pathtype->integer)
	{
	default:
	case 0:	// R1Q2 path
		q = CL_AllocDLQueueEntry (quakePathFixed, isPak, isFilelist, false, false);
		break;
	case 1:	// Q2Pro path
		q = CL_AllocDLQueueEntry (quakePathFixed, isPak, isFilelist, true, false);
		break;
	case 2: // attempt both filelist paths
		q = CL_AllocDLQueueEntry (quakePathFixed, isPak, isFilelist, false, false);
		// Gamedir is not set (stock Q2 behavior for baseq2), this is the case where R1Q2 and Q2Pro file paths diverge
		if ( (q != NULL) && (cl.gamedir[0] == '\0') && !isFilelist ) {
		//	Com_Printf ("[HTTP] Adding Q2Pro path mirror entry for %s...\n", quakePathFixed);
			q2 = CL_AllocDLQueueEntry (quakePathFixed, isPak, isFilelist, true, true);
			if (q2 != NULL) {
				q->isDuplicated = q2->isDuplicated = true;
				q2->isAltEntry = true;
			}
		}
		break;
	}
	// handle duplicate entry case where CL_AllocDLQueueEntry() returns NULL
	if (q == NULL)
		return true;
	// end Knightmare

	if (needList)
	{
		// grab the filelist
	//	CL_QueueHTTPDownload (va("/%s.filelist", cl.gamedir));
		// Knightmare- cvar-switchable download path
		switch (cl_http_pathtype->integer)
		{
		default:
		case 0:	// R1Q2 path
			CL_QueueHTTPDownload (va("/%s.filelist", cl.gamedir));
			break;
		case 1:	// Q2Pro path
			CL_QueueHTTPDownload (va("/%s.filelist", CL_HTTP_GetQ2ProGameDir()));
			break;
		case 2: // attempt both filelist paths
			CL_QueueHTTPDownload (va("/%s.filelist", cl.gamedir));
			// Gamedir is not set (stock Q2 behavior for baseq2), this is the case where R1Q2 and Q2Pro file paths diverge
			if (cl.gamedir[0] == '\0') {
				CL_QueueHTTPDownload (va("/%s.filelist", CL_HTTP_GetQ2ProGameDir()));
			}
			break;
		}
		// end Knightmare

		// this is a nasty hack to let the server know what we're doing so admins don't
		// get confused by a ton of people stuck in CNCT state. it's assumed the server
		// is running r1q2 if we're even able to do http downloading so hopefully this
		// won't spew an error msg.
	//	MSG_BeginWriting (clc_stringcmd);
	//	MSG_WriteString ("download http\n");
	//	MSG_EndWriting (&cls.netchan.message);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "download http\n");
	}

	// special case for map file lists, I really wanted a server-push mechanism for this, but oh well
	len = strlen (quakePathFixed);
	if ( cl_http_filelists->integer && (len > 4) && !Q_stricmp ( ( char * ) ( quakePathFixed + len - 4 ), ".bsp" ) )
	{
		char	listPath[MAX_OSPATH];
		char	filePath[MAX_OSPATH];

	/*	snprintf (filePath, sizeof(filePath), "%s/%s", cl.gamedir, quakePathFixed);
		COM_StripExtension (filePath, listPath, sizeof(listPath));
		Q_strncatz (listPath, sizeof(listPath), ".filelist");
		CL_QueueHTTPDownload (listPath); */
		// Knightmare- cvar-switchable download path
		switch (cl_http_pathtype->integer)
		{
		default:
		case 0:	// R1Q2 path
			snprintf (filePath, sizeof(filePath), "%s/%s", cl.gamedir, quakePathFixed);
			COM_StripExtension (filePath, listPath, sizeof(listPath));
			Q_strncatz (listPath, sizeof(listPath), ".filelist");
			CL_QueueHTTPDownload (listPath);
			break;
		case 1:	// Q2Pro path
			snprintf (filePath, sizeof(filePath), "/%s/%s", CL_HTTP_GetQ2ProGameDir(), quakePathFixed);
			COM_StripExtension (filePath, listPath, sizeof(listPath));
			Q_strncatz (listPath, sizeof(listPath), ".filelist");
			CL_QueueHTTPDownload (listPath);
			break;
		case 2: // attempt both filelist paths
			snprintf (filePath, sizeof(filePath), "%s/%s", cl.gamedir, quakePathFixed);
			COM_StripExtension (filePath, listPath, sizeof(listPath));
			Q_strncatz (listPath, sizeof(listPath), ".filelist");
			CL_QueueHTTPDownload (listPath);
			
			// Gamedir is not set (stock Q2 behavior for baseq2), this is the case where R1Q2 and Q2Pro file paths diverge
			if (cl.gamedir[0] == '\0') {
				snprintf (filePath, sizeof(filePath), "/%s/%s", CL_HTTP_GetQ2ProGameDir(), quakePathFixed);
				COM_StripExtension (filePath, listPath, sizeof(listPath));
				Q_strncatz (listPath, sizeof(listPath), ".filelist");
				CL_QueueHTTPDownload (listPath);
			}
			break;
		}
		// end Knightmare
	}

	// if a download entry has made it this far, CL_FinishHTTPDownload is guaranteed to be called.
	pendingCount++;

	return true;
}


/*
===============
CL_PendingHTTPDownloads

See if we're still busy with some downloads. Called by precacher just
before it loads the map since we could be downloading the map. If we're
busy still, it'll wait and CL_FinishHTTPDownload will pick up from where
it left.
===============
*/
qboolean CL_PendingHTTPDownloads (void)
{
//	dlqueue_t	*q;

	if ( !qcurl_initialized )	// check if qcurl not initialized
		return false;

	if (!cls.downloadServer[0])
		return false;

	return pendingCount + handleCount;

#if 0	// FS: unreachable code
	q = &cls.downloadQueue;

	while (q->next)
	{
		q = q->next;
		if (q->state != DLQ_STATE_DONE)
			return true;
	}

	return false;
#endif
}


/*
===============
CL_CheckAndQueueDownload

Validate a path supplied by a filelist.
===============
*/
static void CL_CheckAndQueueDownload (char *path)
{
	size_t		length;
	char		*ext;
	qboolean	pak;
	qboolean	gameLocal;

	StripHighBits (path, 1);

	length = strlen(path);

	if (length >= MAX_QPATH)
		return;

	ext = strrchr (path, '.');

	if (!ext)
		return;

	ext++;

	if (!ext[0])
		return;

	Q_strlwr (ext);

	if ( !strcmp (ext, "pak") || !strcmp (ext, "pk3") )
	{
		Com_Printf ("[HTTP] NOTICE: Filelist is requesting a pak file (%s)\n", path);
		pak = true;
	}
	else
		pak = false;

	if (!pak && strcmp (ext, "pcx") && strcmp (ext, "wal") && strcmp (ext, "wav") && strcmp (ext, "md2") &&
		strcmp (ext, "sp2") && strcmp (ext, "tga") && strcmp (ext, "png") && strcmp (ext, "jpg") &&
		strcmp (ext, "bsp") && strcmp (ext, "ent") && strcmp (ext, "txt") && strcmp (ext, "dm2") &&
		strcmp (ext, "loc") && strcmp (ext, "md3") && strcmp (ext, "script") && strcmp (ext, "shader"))
	{
		Com_Printf ("[HTTP] WARNING: Illegal file type '%s' in filelist.\n", MakePrintable(path, length));
		return;
	}

	if (path[0] == '@')
	{
		if (pak)
		{
			Com_Printf ("[HTTP] WARNING: @ prefix used on a pak file (%s) in filelist.\n", MakePrintable(path, length));
			return;
		}
		gameLocal = true;
		path++;
		length--;
	}
	else
		gameLocal = false;

	if (strstr (path, "..") || !IsValidChar (path[0]) || !IsValidChar (path[length-1]) || strstr(path, "//") ||
		strchr (path, '\\') || (!pak && !strchr (path, '/')) || (pak && strchr(path, '/')))
	{
		Com_Printf ("[HTTP] WARNING: Illegal path '%s' in filelist.\n", MakePrintable(path, length));
		return;
	}

	// by definition paks are game-local
	if (gameLocal || pak)
	{
		qboolean	exists;
		FILE		*f;
		char		gamePath[MAX_OSPATH];

		if (pak)
		{
			snprintf (gamePath, sizeof(gamePath),"%s/%s", FS_Downloaddir(), path);	// was FS_Gamedir()
			f = fopen (gamePath, "rb");
			if (!f)
			{
				if ( !Q_stricmp( FS_Downloaddir(), FS_Gamedir() ) )	// if fs_gamedir and fs_downloaddir are the same, don't bother trying fs_gamedir
				{
					exists = false;
				}
				else
				{
					snprintf (gamePath, sizeof(gamePath),"%s/%s", FS_Gamedir(), path);
					f = fopen (gamePath, "rb");
					if (!f)
					{
						exists = false;
					}
					else
					{
					//	Com_Printf ("[HTTP] NOTICE: pak file (%s) specified in filelist already exists\n", gamePath);
						exists = true;
						fclose (f);
					}
				}
			}
			else
			{
			//	Com_Printf ("[HTTP] NOTICE: pak file (%s) specified in filelist already exists\n", gamePath);
				exists = true;
				fclose (f);
			}
		}
		else
		{
		//	exists = FS_ExistsInGameDir (path);
			exists = (FS_DownloadFileExists (path) || FS_LocalFileExists(path));
		}

		if (!exists)
		{
			if ( CL_QueueHTTPDownload (path) )
			{
				// Paks get bumped to the top and HTTP switches to single downloading.
				// This prevents someone on 28k dialup trying to do both the main .pak
				// and referenced configstrings data at once.
				// Knightmare- moved this functionality inside CL_QueueHTTPDownload()
			/*	if (pak)
				{
					dlqueue_t	*q, *last;

					last = q = &cls.downloadQueue;

					while (q->next)
					{
						last = q;
						q = q->next;
					}

					last->next = NULL;
					q->next = cls.downloadQueue.next;
					cls.downloadQueue.next = q;
				}*/
			}
		}
		else
		{
		//	Com_Printf ("[HTTP] NOTICE: requested file (%s) already exists\n", path);
		}
	}
	else
	{
		CL_CheckOrDownloadFile (path);
	}
}


/*
===============
CL_ParseFileList

A filelist is in memory, scan and validate it and queue up the files.
===============
*/
static void CL_ParseFileList (dlhandle_t *dl)
{
	char	 *list;
	char	*p;

	if (!cl_http_filelists->integer)
		return;

	list = dl->tempBuffer;

	for (;;)
	{
		p = strchr (list, '\n');
		if (p)
		{
			p[0] = 0;
			if (list[0])
				CL_CheckAndQueueDownload (list);
			list = p + 1;
		}
		else
		{
			if (list[0])
				CL_CheckAndQueueDownload (list);
			break;
		}
	}

	Z_Free (dl->tempBuffer);
	dl->tempBuffer = NULL;
}


/*
===============
CL_ReVerifyHTTPQueue

A pak file just downloaded, let's see if we can remove some stuff from
the queue which is in the .pak.
===============
*/
static void CL_ReVerifyHTTPQueue (void)
{
	dlqueue_t	*q;

	q = &cls.downloadQueue;

	pendingCount = 0;

	while (q->next)
	{
		q = q->next;
		if (q->state == DLQ_STATE_NOT_STARTED)
		{
			// Knightmare- don't check for paks inside other paks!
			if (!q->isPak && FS_LoadFile (q->quakePath, NULL) != -1)
				q->state = DLQ_STATE_DONE;
			else
				pendingCount++;
		}
	}
}


/*
===============
CL_HTTP_Cleanup

Quake II is exiting or we're changing servers. Clean up.
===============
*/
void CL_HTTP_Cleanup (qboolean fullShutdown)
{
	dlhandle_t	*dl;
	int			i;

	if ( !qcurl_initialized )	// check if qcurl not initialized
		return;

	if (fullShutdown && httpDown)
		return;

	for (i = 0; i < MAX_HTTP_HANDLES; i++)
	{
		dl = &cls.HTTPHandles[i];

		if (dl->file)
		{
			fclose (dl->file);
			remove (dl->filePath);
			dl->file = NULL;
		}

		if (dl->tempBuffer)
		{
			Z_Free (dl->tempBuffer);
			dl->tempBuffer = NULL;
		}

		if (dl->curl)
		{
			if (multi_handle)
				qcurl_multi_remove_handle (multi_handle, dl->curl);
			qcurl_easy_cleanup (dl->curl);
			dl->curl = NULL;
		}

		dl->queueEntry = NULL;
	}

	if (multi_handle)
	{
		qcurl_multi_cleanup (multi_handle);
		multi_handle = NULL;
	}

	if (fullShutdown)
	{
		Com_Printf ("Cleaning up curl state...");
		qcurl_global_cleanup ();
		Com_Printf (" done.\n");

		httpDown = true;
	}
}


// Knightmare added
/*
===============
CL_HTTP_ResetMapAbort

Resets the thisMapAbort boolean.
===============
*/
void CL_HTTP_ResetMapAbort (void)
{
	thisMapAbort = false;
}
// end Knightmare


/*
===============
CL_FinishHTTPDownload

A download finished, find out what it was, whether there were any errors and
if so, how severe. If none, rename file and other such stuff.
===============
*/
static void CL_FinishHTTPDownload (void)
{
	size_t		i;
	int			msgs_in_queue;
	CURLMsg		*msg = NULL;
	CURLcode	result;
	dlhandle_t	*dl = NULL;
	CURL		*curl = NULL;
	long		responseCode = 0;
	double		timeTaken = 0.0;
	double		fileSize = 0.0;
	char		tempName[MAX_OSPATH];
	qboolean	isFile = false;
	size_t		len;

	do
	{
		msg = qcurl_multi_info_read (multi_handle, &msgs_in_queue);

		if (!msg)
		{
			Com_Printf ("CL_FinishHTTPDownload: Odd, no message for us...\n");
			return;
		}

		if (msg->msg != CURLMSG_DONE)
		{
			Com_Printf ("CL_FinishHTTPDownload: Got some weird message...\n");
			continue;
		}

		curl = msg->easy_handle;

		// curl doesn't provide reverse-lookup of the void * ptr, so search for it
		for (i = 0; i < MAX_HTTP_HANDLES; i++)
		{
			if (cls.HTTPHandles[i].curl == curl)
			{
				dl = &cls.HTTPHandles[i];
				break;
			}
		}

		if (i == MAX_HTTP_HANDLES)
			Com_Error (ERR_DROP, "CL_FinishHTTPDownload: Handle not found");

		// we mark everything as done even if it errored to prevent multiple
		// attempts.
		dl->queueEntry->state = DLQ_STATE_DONE;

		// filelist processing is done on read
		if (dl->file)
			isFile = true;
		else
			isFile = false;

		if (isFile)
		{
			fclose (dl->file);
			dl->file = NULL;
		}

		// might be aborted
		if (pendingCount)
			pendingCount--;
		handleCount--;
	//	Com_Printf ("finished dl: hc = %d\n", LOG_GENERAL, handleCount);
		cls.downloadname[0] = 0;
		cls.downloadposition = 0;

		result = msg->data.result;

		switch (result)
		{
			// for some reason curl returns CURLE_OK for a 404...
			case CURLE_HTTP_RETURNED_ERROR:
			case CURLE_OK:
			
				qcurl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &responseCode);
				if (responseCode == 404)
				{
					len = strlen (dl->queueEntry->quakePath);
					if ( (len > 4) && ( !strcmp (dl->queueEntry->quakePath + len - 4, ".pak") || !strcmp (dl->queueEntry->quakePath + len - 4, ".pk3") ) )
						downloading_pak = false;

					if (isFile) {
						remove (dl->filePath);
					}
					// Knightmare- specify which server path failed if we're in dual-path mode
					if (dl->queueEntry->isDuplicated) {
						if (dl->queueEntry->isAltEntry)
							Com_Printf ("[HTTP] (%s, Q2Pro path): 404 File Not Found [%d remaining files]\n", dl->queueEntry->quakePath, pendingCount);
						else
							Com_Printf ("[HTTP] (%s, R1Q2 path): 404 File Not Found [%d remaining files]\n", dl->queueEntry->quakePath, pendingCount);
					}
					else {
						Com_Printf ("[HTTP] (%s): 404 File Not Found [%d remaining files]\n", dl->queueEntry->quakePath, pendingCount);
					}
				/*	qcurl_easy_getinfo (curl, CURLINFO_SIZE_DOWNLOAD, &fileSize);

					// Knightmare- ignore this, doesn't need to be fatal
					if (fileSize > 512)
					{
						// ick
						isFile = false;
						result = CURLE_FILESIZE_EXCEEDED;
						Com_Printf ("Oversized 404 body received (%d bytes), aborting HTTP downloading.\n", (int)fileSize);
					}
					else */
					{
						qcurl_multi_remove_handle (multi_handle, dl->curl);

						// Fall back to UDP download for this map if failure on .bsp
					/*	if ( !strncmp(dl->queueEntry->quakePath, "maps/", 5) && !strcmp(dl->queueEntry->quakePath + len - 4, ".bsp") )
						{
							Com_Printf ("[HTTP]: failed to download %s, falling back to UDP until next map.\n", dl->queueEntry->quakePath);
							thisMapAbort = true;
							CL_CancelHTTPDownloads (false);
							CL_ResetPrecacheCheck ();
						}
						else { */
							// Knightmare- add this entry to failed HTTP downloads list
							CL_AddToFailedHTTPDownloadList (dl->queueEntry->quakePath, dl->queueEntry->isDuplicated);

							// Remove queue entry from CURL multihandle queue
							CL_RemoveDownloadFromQueue (dl->queueEntry);
							dl->queueEntry = NULL;
					//	}
						// end Knightmare
						continue;
					}
				}
				else if (responseCode == 200)
				{
					if (!isFile && !abortDownloads)
						CL_ParseFileList (dl);
					break;
				}

				// every other code is treated as fatal, fallthrough here
				Com_Printf ("[HTTP] Bad response code %d for %s, aborting HTTP downloading.\n", responseCode, dl->queueEntry->quakePath);

			// fatal error, disable http
			case CURLE_COULDNT_RESOLVE_HOST:
			case CURLE_COULDNT_CONNECT:
			case CURLE_COULDNT_RESOLVE_PROXY:
				if (isFile) {
					remove (dl->filePath);
				}
				Com_Printf ("[HTTP] Fatal error: %s\n", CURL_ERROR(result));
				qcurl_multi_remove_handle (multi_handle, dl->curl);
				if (abortDownloads)
					continue;
				CL_CancelHTTPDownloads (true);
				continue;
			default:
				len = strlen (dl->queueEntry->quakePath);
				if ( (len > 4) && ( !strcmp (dl->queueEntry->quakePath + len - 4, ".pak") || !strcmp (dl->queueEntry->quakePath + len - 4, ".pk3") ) )
					downloading_pak = false;
				if (isFile) {
					remove (dl->filePath);
				}
				Com_Printf ("[HTTP] download failed: %s\n", CURL_ERROR(result));
				qcurl_multi_remove_handle (multi_handle, dl->curl);
				continue;
		}

		if (isFile)
		{
			// rename the temp file
			snprintf (tempName, sizeof(tempName), "%s/%s", FS_Downloaddir(), dl->queueEntry->quakePath);	// was FS_Gamedir()

			if (rename (dl->filePath, tempName))
			{	// Knightmare- If this download was mirrored, that possibly means the other path
				// also downloaded and finished first.  In that case, delete the temp file.
				if (dl->queueEntry->isDuplicated)
				{
					FILE	*fp;

					fp = fopen (tempName, "rb");
					if (fp != NULL) {
						Com_Printf ("[HTTP] File %s already completed on other path, deleting temp file.\n", tempName);
						remove (dl->filePath);
					}
					else {
						fclose (fp);
						Com_Printf ("[HTTP] Failed to rename %s to %s for some odd reason...", dl->filePath, tempName);
					}
				}
				else
					Com_Printf ("[HTTP] Failed to rename %s to %s for some odd reason...", dl->filePath, tempName);
			}

			// a pak file is very special...
			len = strlen (tempName);
			if ( !strcmp (tempName + len - 4, ".pak") || !strcmp (tempName + len - 4, ".pk3") )
			{
			//	FS_FlushCache ();
			//	FS_ReloadPAKs ();
				// Knightmare- just add the pk3 / pak file
				if (!strcmp (tempName + len - 4, ".pk3")) 
					FS_AddPK3File (tempName, false);
				else
					FS_AddPAKFile (tempName, false, false);

				CL_ReVerifyHTTPQueue ();
				downloading_pak = false;
			}
		}

		// show some stats
		qcurl_easy_getinfo (curl, CURLINFO_TOTAL_TIME, &timeTaken);
		qcurl_easy_getinfo (curl, CURLINFO_SIZE_DOWNLOAD, &fileSize);

		// FIXME:
		// Technically I shouldn't need to do this as curl will auto reuse the
		// existing handle when you change the URL. however, the handleCount goes
		// all weird when reusing a download slot in this way. if you can figure
		// out why, please let me know.
		qcurl_multi_remove_handle (multi_handle, dl->curl);

		Com_Printf ("[HTTP] (%s): %.f bytes, %.2fkB/sec [%d remaining files]\n", dl->queueEntry->quakePath, fileSize, (fileSize / 1024.0) / timeTaken, pendingCount);
	}
	while (msgs_in_queue > 0);

//	FS_FlushCache ();

	if (handleCount == 0)
	{
		if (abortDownloads == HTTPDL_ABORT_SOFT)
			abortDownloads = HTTPDL_ABORT_NONE;
		else if (abortDownloads == HTTPDL_ABORT_HARD) {
			// FS: Added because Whale's Weapons HTTP server rejects you after a lot of 404s.  Then you lose HTTP until a hard reconnect.
			Q_strncpyz(cls.downloadServerRetry, sizeof(cls.downloadServerRetry), cls.downloadServer); 
			cls.downloadServer[0] = 0;
		}
	}

	// done current batch, see if we have more to dl - maybe a .bsp needs downloaded
	if ( (cls.state == ca_connected) && !CL_PendingHTTPDownloads() )
		CL_RequestNextDownload ();
}


/*
===============
CL_GetFreeDLHandle

Find a free download handle to start another queue entry on.
===============
*/
static dlhandle_t *CL_GetFreeDLHandle (void)
{
	dlhandle_t	*dl;
	int			i;

	for (i = 0; i < MAX_HTTP_HANDLES; i++)
	{
		dl = &cls.HTTPHandles[i];
		if (!dl->queueEntry || dl->queueEntry->state == DLQ_STATE_DONE)
			return dl;
	}

	return NULL;
}


/*
===============
CL_StartNextHTTPDownload

Start another HTTP download if possible.
===============
*/
static void CL_StartNextHTTPDownload (void)
{
	dlqueue_t	*q;
	dlhandle_t	*dl;
	size_t		len;

	q = &cls.downloadQueue;

	while (q->next)
	{
		q = q->next;
		if (q->state == DLQ_STATE_NOT_STARTED)
		{
			dl = CL_GetFreeDLHandle();

			if (!dl)
				return;

			CL_StartHTTPDownload (q, dl);

			// ugly hack for pak file single downloading
			len = strlen (q->quakePath);
			if ( (len > 4) && ( !Q_stricmp ( q->quakePath + len - 4, ".pak" ) || !Q_stricmp ( q->quakePath + len - 4, ".pk3" ) ) )
				downloading_pak = true;

			break;
		}
	}
}


/*
===============
CL_RunHTTPDownloads

This calls curl_multi_perform do actually do stuff. Called every frame while
connecting to minimise latency. Also starts new downloads if we're not doing
the maximum already.
===============
*/
void CL_RunHTTPDownloads (void)
{
	int			newHandleCount;
	CURLMcode	ret;

	if ( !qcurl_initialized )	// check if qcurl not initialized
		return;

	if (!cls.downloadServer[0])
		return;

//	Com_Printf ("handle %d, pending %d\n", LOG_GENERAL, handleCount, pendingCount);

	// not enough downloads running, queue some more!
	if ( pendingCount && (abortDownloads == HTTPDL_ABORT_NONE) &&
		!downloading_pak && (handleCount < cl_http_max_connections->integer) )
		CL_StartNextHTTPDownload ();

	do
	{
		ret = qcurl_multi_perform (multi_handle, &newHandleCount);
		if (newHandleCount < handleCount)
		{
		//	Com_Printf ("runnd dl: hc = %d, nc = %d\n", LOG_GENERAL, handleCount, newHandleCount);
			// hmm, something either finished or errored out.
			CL_FinishHTTPDownload ();
			handleCount = newHandleCount;
		}
	}
	while (ret == CURLM_CALL_MULTI_PERFORM);

	if (ret != CURLM_OK)
	{
		Com_Printf ("curl_multi_perform error. Aborting HTTP downloads.\n");
		CL_CancelHTTPDownloads (true);
	}

	// not enough downloads running, queue some more!
	if ( pendingCount && (abortDownloads == HTTPDL_ABORT_NONE) &&
		!downloading_pak && (handleCount < cl_http_max_connections->integer) )
		CL_StartNextHTTPDownload ();
}
#endif	// USE_CURL
