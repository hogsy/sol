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

// cl_download.c  -- client autodownload code
// moved from cl_main.c and cl_parse.c

#include "client.h"

extern	cvar_t *allow_download;
extern	cvar_t *allow_download_players;
extern	cvar_t *allow_download_models;
extern	cvar_t *allow_download_sounds;
extern	cvar_t *allow_download_maps;
// Knightmare- whether to allow downloading 24-bit textures
extern	cvar_t *allow_download_textures_24bit;

int precache_check; // for autodownload of precache items
int precache_spawncount;
int precache_tex;
int precache_model_skin;
int precache_pak;	// Knightmare added

byte *precache_model; // used for skin checking in alias models

#define PLAYER_MULT 22

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)

// Knightmare- old configstrings for version 34 client compatibility
#define OLD_ENV_CNT (OLD_CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define OLD_TEXTURE_CNT (OLD_ENV_CNT+13)

static const char *env_suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

void CL_InitFailedDownloadList (void);

//=========================================================

/*
=================
CL_RequestNextDownload
=================
*/
void CL_RequestNextDownload (void)
{
	unsigned	map_checksum;		// for detecting cheater maps
	char		fn[MAX_OSPATH];
	dmd2_t		*md2header;
	dmd3_t		*md3header;
	dmd3mesh_t	*md3mesh;
	dspr2_t		*sp2header;
	char		*skinname;
	int			cs_sounds, cs_playerskins, cs_images;
	int			max_models, max_sounds, max_images;
	int			env_cnt, texture_cnt;
	// from qcommon/cmodel.c
	extern int			numtexinfo;
	extern mapsurface_t	map_surfaces[];

	if (cls.state != ca_connected)
		return;

	// clear failed download list
	if (precache_check == CS_MODELS)
		CL_InitFailedDownloadList ();

	// Knightmare- hack for connected to server using old protocol
	// Changed config strings require different parsing
	if ( LegacyProtocol() )
	{
		cs_sounds		= OLD_CS_SOUNDS;
		cs_playerskins	= OLD_CS_PLAYERSKINS;
		cs_images		= OLD_CS_IMAGES;
		max_models		= OLD_MAX_MODELS;
		max_sounds		= OLD_MAX_SOUNDS;
		max_images		= OLD_MAX_IMAGES;
		env_cnt			= OLD_ENV_CNT;
		texture_cnt		= OLD_TEXTURE_CNT;
	}
	else
	{
		cs_sounds		= CS_SOUNDS;
		cs_playerskins	= CS_PLAYERSKINS;
		cs_images		= CS_IMAGES;
		max_models		= MAX_MODELS;
		max_sounds		= MAX_SOUNDS;
		max_images		= MAX_IMAGES;
		env_cnt			= ENV_CNT;
		texture_cnt		= TEXTURE_CNT;
	}

//	Com_Printf ("Restarting precache cycle, precache_check is %i.\n", precache_check);

	// Skip to loading map if downloading disabled or on local server
	if ( (Com_ServerState() || !allow_download->integer) && (precache_check < env_cnt) )
		precache_check = env_cnt;

	// Try downloading pk3 file for current map from server, hack by Jay Dolan
	if ( !LegacyProtocol() && (precache_check == CS_MODELS) && (precache_pak == 0) )
	{
		precache_pak++;
		if (strlen(cl.configstrings[CS_PAKFILE])) {
			if ( !CL_CheckOrDownloadFile(cl.configstrings[CS_PAKFILE]) )
				return;  // started a download
		}
	}

	// ZOID
	if (precache_check == CS_MODELS) { // confirm map
		precache_check = CS_MODELS+2; // 0 isn't used
		if (allow_download_maps->integer) {
			if ( !CL_CheckOrDownloadFile(cl.configstrings[CS_MODELS+1]) )
				return; // started a download
		}
	}
	if ( (precache_check >= CS_MODELS) && (precache_check < CS_MODELS+max_models) )
	{
		if (allow_download_models->integer)
		{
			while ( (precache_check < CS_MODELS+max_models) && cl.configstrings[precache_check][0] )
			{
				if ( (cl.configstrings[precache_check][0] == '*') ||
					(cl.configstrings[precache_check][0] == '#') ) {
					precache_check++;
					continue;
				}
				if (precache_model_skin == 0)
				{
					if ( !CL_CheckOrDownloadFile(cl.configstrings[precache_check]) ) {
						precache_model_skin = 1;
						return; // started a download
					}
					precache_model_skin = 1;
				}

#ifdef USE_CURL	// HTTP downloading from R1Q2
				// pending downloads (models), let's wait here before we can check skins.
				if ( CL_PendingHTTPDownloads() ) {
					return;
				}
#endif	// USE_CURL

				// checking for skins in the model
				if (!precache_model)
				{
					FS_LoadFile (cl.configstrings[precache_check], (void **)&precache_model);
					if (!precache_model) {
						precache_model_skin = 0;
						precache_check++;
						continue; // couldn't load it
					}
					if (LittleLong(*(unsigned *)precache_model) == IDMD2HEADER)		// is it an md2?
					{	// get md2 header
						md2header = (dmd2_t *)precache_model;
						if (LittleLong (md2header->version) != MD2_ALIAS_VERSION)
						{	// not a recognized md2
							FS_FreeFile (precache_model);
							precache_model = 0;
							precache_check++;
							precache_model_skin = 0;
							continue; // couldn't load it
						}
					}
					else if (LittleLong(*(unsigned *)precache_model) == IDMD3HEADER)	// is it an md3?
					{	// get md3 header
						md3header = (dmd3_t *)precache_model;
						if (LittleLong (md3header->version) != MD3_ALIAS_VERSION)
						{	// not a recognized md3
							FS_FreeFile (precache_model);
							precache_model = 0;
							precache_check++;
							precache_model_skin = 0;
							continue; // couldn't load it
						}
					}
					else if (LittleLong(*(unsigned *)precache_model) != IDSP2HEADER)	// is it an sp2?
					{	// get sprite header
						sp2header = (dspr2_t *)precache_model;
						if (LittleLong (sp2header->version) != SP2_VERSION)
						{	// not a recognized sprite
							FS_FreeFile (precache_model);
							precache_model = 0;
							precache_check++;
							precache_model_skin = 0;
							continue; // couldn't load it
						}
					}
					else	// not a recognized model with external skins
					{
						FS_FreeFile (precache_model);
						precache_model = 0;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
				}

				if (LittleLong(*(unsigned *)precache_model) == IDMD2HEADER) // md2
				{
					md2header = (dmd2_t *)precache_model;
					while (precache_model_skin - 1 < LittleLong(md2header->num_skins))
					{
						skinname = (char *)precache_model + LittleLong(md2header->ofs_skins) + 
									(precache_model_skin - 1)*MD2_MAX_SKINNAME;

						// r1ch: spam warning for models that are broken
						if (strchr (skinname, '\\'))
							Com_Printf ("Warning, model %s with incorrectly linked skin: %s\n", cl.configstrings[precache_check], skinname);
						else if (strlen(skinname) > MD2_MAX_SKINNAME-1)
							Com_Error (ERR_DROP, "Model %s has too long a skin path: %s", cl.configstrings[precache_check], skinname);

						if ( !CL_CheckOrDownloadFile(skinname) )
						{
							precache_model_skin++;
							return; // started a download
						}
						precache_model_skin++;
					}
				}
				else if (LittleLong(*(unsigned *)precache_model) == IDMD3HEADER) // md3
				{
					md3header = (dmd3_t *)precache_model;
					while (precache_model_skin - 1 < LittleLong(md3header->num_skins))
					{
						int	i;
						md3mesh = (dmd3mesh_t *)((byte *)md3header + LittleLong(md3header->ofs_meshes));
						for ( i = 0; i < md3header->num_meshes; i++)
						{
							if (precache_model_skin - 1 >= LittleLong(md3header->num_skins))
								break;
							skinname = (char *)precache_model + LittleLong(md3mesh->ofs_skins) + 
										(precache_model_skin - 1)*MD3_MAX_PATH;

							// r1ch: spam warning for models that are broken
							if (strchr (skinname, '\\'))
								Com_Printf ("Warning, model %s with incorrectly linked skin: %s\n", cl.configstrings[precache_check], skinname);
							else if (strlen(skinname) > MD3_MAX_PATH-1)
								Com_Error (ERR_DROP, "Model %s has too long a skin path: %s", cl.configstrings[precache_check], skinname);

							if ( !CL_CheckOrDownloadFile(skinname) )
							{
								precache_model_skin++;
								return; // started a download
							}
							precache_model_skin++;

							md3mesh = (dmd3mesh_t *)((byte *)md3mesh + LittleLong (md3mesh->meshsize));
						}
					}
				}
				else	// if (LittleLong(*(unsigned *)precache_model) == IDSP2HEADER)	// sp2
				{
					sp2header = (dspr2_t *)precache_model;
					while (precache_model_skin - 1 < LittleLong(sp2header->numframes))
					{
						skinname = sp2header->frames[(precache_model_skin - 1)].name;

						// r1ch: spam warning for models that are broken
						if (strchr (skinname, '\\'))
							Com_Printf ("Warning, sprite %s with incorrectly linked skin: %s\n", cl.configstrings[precache_check], skinname);
						else if (strlen(skinname) > MD2_MAX_SKINNAME-1)
							Com_Error (ERR_DROP, "Sprite %s has too long a skin path: %s", cl.configstrings[precache_check], skinname);

						if ( !CL_CheckOrDownloadFile(skinname) )
						{
							precache_model_skin++;
							return; // started a download
						}
						precache_model_skin++;
					}
				}

				if (precache_model) { 
					FS_FreeFile(precache_model);
					precache_model = 0;
				}
				precache_model_skin = 0;
				precache_check++;
			}
		}
		precache_check = cs_sounds;
	}
	if ( (precache_check >= cs_sounds) && (precache_check < cs_sounds+max_sounds) )
	{ 
		if (allow_download_sounds->integer)
		{
			if (precache_check == cs_sounds)
				precache_check++; // zero is blank
			while ( (precache_check < cs_sounds+max_sounds) && cl.configstrings[precache_check][0] )
			{
				if (cl.configstrings[precache_check][0] == '*') {
					precache_check++;
					continue;
				}
				Com_sprintf (fn, sizeof(fn), "sound/%s", cl.configstrings[precache_check++]);
				if ( !CL_CheckOrDownloadFile(fn) )
					return; // started a download
			}
		}
		precache_check = cs_images;
	}
	if ( (precache_check >= cs_images) && (precache_check < cs_images+max_images) )
	{
		if (precache_check == cs_images)
			precache_check++; // zero is blank
		while ( (precache_check < cs_images+max_images) && cl.configstrings[precache_check][0] ) {
			Com_sprintf (fn, sizeof(fn), "pics/%s.pcx", cl.configstrings[precache_check++]);
			if ( !CL_CheckOrDownloadFile(fn) )
				return; // started a download
		}
		precache_check = cs_playerskins;
	}

#ifdef USE_CURL	// HTTP downloading from R1Q2
	// pending downloads (sounds/images), let's wait here.
	if ( CL_PendingHTTPDownloads() ) {
	//	Com_Printf ("Pausing precache cycle after sounds/images queued.  precache_check is %i.\n", precache_check);
		return;
	}
#endif	// USE_CURL

	// skins are special, since a player has three things to download:
	// model, weapon model and skin
	// so precache_check is now *3
	if ( (precache_check >= cs_playerskins) && (precache_check < cs_playerskins + MAX_CLIENTS * PLAYER_MULT) )
	{
		if (allow_download_players->integer)
		{
			while (precache_check < cs_playerskins + MAX_CLIENTS * PLAYER_MULT)
			{
				int			i, j, n, len;
				char		model[MAX_QPATH], skin[MAX_QPATH], *p;
				qboolean	badSkinName = false;

				i = (precache_check - cs_playerskins)/PLAYER_MULT;
				n = (precache_check - cs_playerskins)%PLAYER_MULT;

				// from R1Q2- skip invalid player skins data
				if (i >= cl.maxclients) {
					precache_check = env_cnt;
					continue;
				}

				if (!cl.configstrings[cs_playerskins+i][0]) {
					precache_check = cs_playerskins + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.configstrings[cs_playerskins+i], '\\')) != NULL) {
					p++;
				}
				else {
					p = cl.configstrings[cs_playerskins+i];
				}

				Q_strncpyz (model, sizeof(model), p);
				p = strchr(model, '/');
				if (!p)
					p = strchr(model, '\\');

				if (p)
				{
					*p++ = 0;
					if (!p[0] || !model[0]) {
						precache_check = cs_playerskins + (i + 1) * PLAYER_MULT;
						continue;
					}
					else {
						Q_strncpyz (skin, sizeof(skin), p);
					}
				}
				else {
				//	*skin = 0;
					precache_check = cs_playerskins + (i + 1) * PLAYER_MULT;
					continue;
				}

				// from R1Q2: check model and skin name for invalid chars
				len = (int)strlen(model);
				for (j = 0; j < len; j++)
				{
					if ( !IsValidChar(model[j]) ) {
						Com_Printf (S_COLOR_YELLOW"Bad character '%c' in player model '%s'\n", model[j], model);
						badSkinName = true;
					}
				}

				len = (int)strlen(skin);
				for (j = 0; j < len; j++)
				{
					if ( !IsValidChar(skin[j]) ) {
						Com_Printf (S_COLOR_YELLOW"Bad character '%c' in player skin '%s'\n", skin[j], skin);
						badSkinName = true;
					}
				}
				if (badSkinName) {
					precache_check = cs_playerskins + (i + 1) * PLAYER_MULT;
					continue;
				}
				// end R1Q2 name check

				switch (n)
				{
				case 0: // model
					Com_sprintf (fn, sizeof(fn), "players/%s/tris.md2", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 1;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 1: // weapon model
					Com_sprintf (fn, sizeof(fn), "players/%s/weapon.md2", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 2;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 2: // weapon skin
					Com_sprintf (fn, sizeof(fn), "players/%s/weapon.pcx", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 3;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 3: // skin
					Com_sprintf (fn, sizeof(fn), "players/%s/%s.pcx", model, skin);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 4;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 4: // skin_i
					Com_sprintf (fn, sizeof(fn), "players/%s/%s_i.pcx", model, skin);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 5;
						return; // started a download
					}
			// Knightmare- added support for downloading player sounds
					n++;
					/*FALL THROUGH*/

				case 5: // death1.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/death1.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 6;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 6: // death2.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/death2.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 7;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 7: // death3.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/death3.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 8;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 8: // death4.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/death4.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 9;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 9: // fall1.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/fall1.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 10;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 10: // fall2.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/fall2.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 11;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 11: // gurp1.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/gurp1.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 12;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 12: // gurp2.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/gurp2.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 13;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 13: // jump1.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/jump1.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 14;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 14: // pain25_1.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/pain25_1.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 15;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 15: // pain25_2.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/pain25_2.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 16;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 16: // pain50_1.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/pain50_1.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 17;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 17: // pain50_2.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/pain50_2.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 18;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 18: // pain75_1.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/pain75_1.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 19;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 19: // pain75_2.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/pain75_2.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 20;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 20: // pain100_1.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/pain100_1.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 21;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/
					
				case 21: // pain100_2.wav
					Com_sprintf (fn, sizeof(fn), "players/%s/pain100_2.wav", model);
					if ( !CL_CheckOrDownloadFile(fn) ) {
						precache_check = cs_playerskins + i * PLAYER_MULT + 22;
						return; // started a download
					}
				//	n = 23;
			// end Knightmare

					// move on to next model
				//	precache_check = cs_playerskins + (i + 1) * PLAYER_MULT;
				}
				// move on to next model
				precache_check = cs_playerskins + (i + 1) * PLAYER_MULT;
			}
		}
		// precache phase completed
		precache_check = env_cnt;
	}

#ifdef USE_CURL	// HTTP downloading from R1Q2
	// pending downloads (player models/skins/sounds), let's wait here.
	if ( CL_PendingHTTPDownloads() ) {
	//	Com_Printf ("Pausing precache cycle after player models/skins/sounds queued.  precache_check is %i.\n", precache_check);
		return;
	}
#endif	// USE_CURL

	if (precache_check == env_cnt)
	{
		// if downloading disabled or on local server, skip checking textures
		if ( Com_ServerState() || !allow_download->integer  )
			precache_check = texture_cnt+999;
		else
			precache_check = env_cnt + 1;

		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);

		if (map_checksum != atoi(cl.configstrings[CS_MAPCHECKSUM])) {
			Com_Error (ERR_DROP, "Local map version differs from server: %i != '%s'\n",
				map_checksum, cl.configstrings[CS_MAPCHECKSUM]);
			return;
		}
	}

	if ( (precache_check > env_cnt) && (precache_check < texture_cnt) )
	{
		if (allow_download->integer && allow_download_maps->integer)
		{
			while (precache_check < texture_cnt)
			{
				int n = precache_check++ - env_cnt - 1;

				if (n & 1)
					Com_sprintf (fn, sizeof(fn), "env/%s%s.pcx", 
						cl.configstrings[CS_SKY], env_suf[n/2]);
				else
					Com_sprintf (fn, sizeof(fn), "env/%s%s.tga", 
						cl.configstrings[CS_SKY], env_suf[n/2]);
				if ( !CL_CheckOrDownloadFile(fn) )
					return; // started a download
			}
		}
		precache_check = texture_cnt;
	}

	if (precache_check == texture_cnt) {
		precache_check = texture_cnt+1;
		precache_tex = 0;
	}

	// confirm existance of .wal textures, download any that don't exist
	if (precache_check == texture_cnt+1)
	{
		if (allow_download->integer && allow_download_maps->integer)
		{
			while (precache_tex < numtexinfo)
			{
				char	fn[MAX_OSPATH];

				Com_sprintf (fn, sizeof(fn), "textures/%s.wal", map_surfaces[precache_tex++].rname);
				if ( !CL_CheckOrDownloadFile(fn) )
					return; // started a download
			}
		}
	//	precache_check = texture_cnt+999;
		precache_check = texture_cnt+2;
		precache_tex = 0;
	}

#ifdef USE_CURL	// HTTP downloading from R1Q2
	// pending downloads (possibly textures), let's wait here.
	if ( CL_PendingHTTPDownloads() ) {
	//	Com_Printf ("Pausing precache cycle after WAL textures queued.  precache_check is %i.\n", precache_check);
		return;
	}
#endif	// USE_CURL

	// confirm existance of .tga textures, try to download any that don't exist
	if (precache_check == texture_cnt+2)
	{
		if (allow_download->integer && allow_download_maps->integer
			&& allow_download_textures_24bit->integer)
		{
			while (precache_tex < numtexinfo)
			{
				char	fn[MAX_OSPATH];

				Com_sprintf (fn, sizeof(fn), "textures/%s.tga", map_surfaces[precache_tex++].rname);
				if ( !CL_CheckOrDownloadFile(fn) )
					return; // started a download
			}
		}
		precache_check = texture_cnt+3;
		precache_tex = 0;
	}

#ifdef USE_CURL	// HTTP downloading from R1Q2
	// pending downloads (possibly textures), let's wait here.
	if ( CL_PendingHTTPDownloads() ) {
	//	Com_Printf ("Pausing precache cycle after TGA textures queued.  precache_check is %i.\n", precache_check);
		return;
	}
#endif	// USE_CURL

#ifdef PNG_SUPPORT
	// confirm existance of .png textures, try to download any that don't exist
	if (precache_check == texture_cnt+3)
	{
		if (allow_download->integer && allow_download_maps->integer
			&& allow_download_textures_24bit->integer)
		{
			while (precache_tex < numtexinfo)
			{
				char	fn[MAX_OSPATH];

				Com_sprintf (fn, sizeof(fn), "textures/%s.png", map_surfaces[precache_tex++].rname);
				if ( !CL_CheckOrDownloadFile(fn) )
					return; // started a download
			}
		}
		precache_check = texture_cnt+4;
		precache_tex = 0;
	}

#ifdef USE_CURL	// HTTP downloading from R1Q2
	// pending downloads (possibly textures), let's wait here.
	if ( CL_PendingHTTPDownloads() ) {
	//	Com_Printf ("Pausing precache cycle after PNG textures queued.  precache_check is %i.\n", precache_check);
		return;
	}
#endif	// USE_CURL

	// confirm existance of .jpg textures, try to download any that don't exist
	if (precache_check == texture_cnt+4)
#else	// PNG_SUPPORT
	// confirm existance of .jpg textures, try to download any that don't exist
	if (precache_check == texture_cnt+3)
#endif	// PNG_SUPPORT
	{
		if (allow_download->integer && allow_download_maps->integer
			&& allow_download_textures_24bit->integer)
		{
			while (precache_tex < numtexinfo)
			{
				char	fn[MAX_OSPATH];

				Com_sprintf (fn, sizeof(fn), "textures/%s.jpg", map_surfaces[precache_tex++].rname);
				if ( !CL_CheckOrDownloadFile(fn) )
					return; // started a download
			}
		}
		precache_check = texture_cnt+999;
		precache_tex = 0;
	}
// ZOID

#ifdef USE_CURL	// HTTP downloading from R1Q2
	// pending downloads (possibly textures), let's wait here.
	if ( CL_PendingHTTPDownloads() ) {
	//	Com_Printf ("Pausing precache cycle after JPG textures queued.  precache_check is %i.\n", precache_check);
		return;
	}
#endif	// USE_CURL

	CL_RegisterSounds ();
	CL_PrepRefresh ();

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("begin %i\n", precache_spawncount) );
	cls.forcePacket = true;
}


//=============================================================================

/*
===============
CL_DownloadFileName
===============
*/
void CL_DownloadFileName (char *dest, int destlen, const char *fn)
{
	if ( !Q_stricmp(FS_Downloaddir(), FS_Gamedir()) )	// use basedir/gamedir if fs_downloaddir is the same as fs_gamedir
	{
		if (strncmp(fn, "players", 7) == 0)
			Com_sprintf (dest, destlen, "%s/%s", BASEDIRNAME, fn);
		else
			Com_sprintf (dest, destlen, "%s/%s", FS_Gamedir(), fn);	// was FS_Gamedir()
	}
	else
		Com_sprintf (dest, destlen, "%s/%s", FS_Downloaddir(), fn);
}


// Knightmare- store the names of last downloads that failed
#define NUM_FAIL_DLDS 512
failedDownload_t lastFailedDownload[NUM_FAIL_DLDS];
static unsigned failedDlListIndex;

/*
===============
CL_InitFailedDownloadList
===============
*/
void CL_InitFailedDownloadList (void)
{
	int		i;

	for (i=0; i<NUM_FAIL_DLDS; i++) {
		Com_sprintf (lastFailedDownload[i].fileName, sizeof(lastFailedDownload[i].fileName), "\0");
		lastFailedDownload[i].failCount = 0;
		lastFailedDownload[i].isDuplicated = false;
	}

	failedDlListIndex = 0;
}


/*
===============
CL_CheckDownloadFailed
===============
*/
qboolean CL_CheckDownloadFailed (const char *name)
{
	int		i;

	if ( !name || (name[0] == '\0') )
		return true;

	for (i=0; i<NUM_FAIL_DLDS; i++)
		if ( (strlen(lastFailedDownload[i].fileName) > 0) && !strcmp(name, lastFailedDownload[i].fileName) )
		{	// we already tried downloading this, server didn't have it
			return true;
		}

	return false;
}


/*
===============
CL_AddToFailedDownloadList
===============
*/
void CL_AddToFailedDownloadList (const char *name)
{
	int			i;
	qboolean	found = false;

	if ( !name || (name[0] == '\0') )
		return;

	// check if this name is already in the table
	for (i=0; i<NUM_FAIL_DLDS; i++)
		if ( (strlen(lastFailedDownload[i].fileName) > 0) && !strcmp(name, lastFailedDownload[i].fileName) )
		{
			found = true;
			break;
		}

	// if it isn't already in the table, then we need to add it
	if (!found)
	{
		Com_sprintf (lastFailedDownload[failedDlListIndex].fileName, sizeof(lastFailedDownload[failedDlListIndex].fileName), "%s", name);
		lastFailedDownload[failedDlListIndex].failCount = 1;
		failedDlListIndex++;

		// wrap around to start of list
		if (failedDlListIndex >= NUM_FAIL_DLDS)
			failedDlListIndex = 0;
	}	
}


/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qboolean CL_CheckOrDownloadFile (const char *filename)
{
	FILE	*fp;
	char	name[MAX_OSPATH];
	int		len; // Knightmare added
	char	s[128];
	//int	i;

	if ( !filename || (filename[0] == '\0') )
		return true;

	if (strstr (filename, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return true;
	}

	if (FS_LoadFile (filename, NULL) != -1)
	{	// it exists, no need to download
		return true;
	}

	// don't try again to download a file that just failed
//	if (CL_CheckDownloadFailed(filename))
//		return true;

#ifdef PNG_SUPPORT
	// don't download a .png texture which already has a .tga counterpart
	len = (int)strlen(filename); 
	Q_strncpyz (s, sizeof(s), filename); 
	if (strstr(s, "textures/") && !strcmp(s+len-4, ".png")) // look if we have a .png texture 
	{ 
		s[len-3]='t'; s[len-2]='g'; s[len-1]='a'; // replace extension 
		if (FS_LoadFile (s, NULL) != -1)	// check for .tga counterpart
			return true;
	}
#endif	// PNG_SUPPORT

	// don't download a .jpg texture which already has a .tga or .png counterpart
	len = (int)strlen(filename); 
	Q_strncpyz (s, sizeof(s), filename); 
	if (strstr(s, "textures/") && !strcmp(s+len-4, ".jpg")) // look if we have a .jpg texture 
	{ 
		s[len-3]='t'; s[len-2]='g'; s[len-1]='a'; // replace extension 
		if (FS_LoadFile (s, NULL) != -1)	// check for .tga counterpart
			return true;
#ifdef PNG_SUPPORT
		s[len-3]='p'; s[len-2]='n'; s[len-1]='g'; // replace extension 
		if (FS_LoadFile (s, NULL) != -1)	// check for .png counterpart
			return true;
#endif	// PNG_SUPPORT
	}

#ifdef USE_CURL	// HTTP downloading from R1Q2
	if ( CL_QueueHTTPDownload(filename) )
	{
		// Knightmare- If we downloaded a player model, force reload of player models in UI
		if ( strncmp(filename, "players/", 8) == 0 ) {
			Com_DPrintf ("CL_CheckOrDownloadFile: downloading a player model, forcing reload of player models in UI.\n");
			cls.refreshPlayerModels = true;
		}
		// We return true so that the precache check keeps feeding us more files.
		// Since we have multiple HTTP connections we want to minimize latency
		// and be constantly sending requests, not one at a time.
		cls.forcePacket = true;	// Maraakate says this will improve ramp-up of DL speed
		return true;
	}
	else
	{
#endif	// USE_CURL

	// don't try again to download a file that just failed
	if (CL_CheckDownloadFailed(filename))
		return true;

	Q_strncpyz (cls.downloadname, sizeof(cls.downloadname), filename);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname, sizeof(cls.downloadtempname));
	Q_strncatz (cls.downloadtempname, sizeof(cls.downloadtempname), ".tmp");

//ZOID
	// check to see if we already have a tmp for this file, if so, try to resume
	// open the file if not opened yet
	CL_DownloadFileName (name, sizeof(name), cls.downloadtempname);

//	FS_CreatePath (name);

	fp = fopen (name, "r+b");
	if (fp)
	{	// it exists
		int len;
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		cls.download = fp;

		// give the server an offset to start the download
		Com_Printf ("Resuming %s\n", cls.downloadname);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message,
			va("download %s %i", cls.downloadname, len));
	}
	else {
		Com_Printf ("Downloading %s\n", cls.downloadname);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, va("download %s", cls.downloadname));
	}

	cls.downloadnumber++;
	cls.forcePacket = true;

	// Knightmare- If we downloaded a player model, force reload of player models in UI
	if ( strncmp(filename, "players/", 8) == 0 ) {
		Com_DPrintf ("CL_CheckOrDownloadFile: downloading a player model, forcing reload of player models in UI.\n");
		cls.refreshPlayerModels = true;
	}

	return false;

#ifdef USE_CURL	// HTTP downloading from R1Q2
	}
#endif	// USE_CURL
}


/*
===============
CL_Download_f

Request a download from the server
===============
*/
void CL_Download_f (void)
{
	char	filename[MAX_OSPATH];
	int		filenamelen;

	if (Cmd_Argc() != 2) {
		Com_Printf ("Usage: download <filename>\n");
		return;
	}

	Com_sprintf (filename, sizeof(filename), "%s", Cmd_Argv(1));
	filenamelen = (int)strlen(filename);

	if (strstr (filename, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return;
	}

	// Knightmare- don't download .dll files (or .so on Linux)!
#ifdef _WIN32
	if ( (filenamelen > 4) && !Q_strcasecmp(filename+filenamelen-4, ".dll") )
	{
		Com_Printf ("Refusing to download a DLL file (%s)!\n", filename);
		return;
	}
#else
	if ( (filenamelen > 3) && !Q_strcasecmp(filename+filenamelen-3, ".so") )
	{
		Com_Printf ("Refusing to download a .so file (%s)!\n", filename);
		return;
	}
#endif
	// end Knightmare

	if (FS_LoadFile (filename, NULL) != -1)
	{	// it exists, no need to download
		Com_Printf ("File already exists.\n");
		return;
	}

	Q_strncpyz (cls.downloadname, sizeof(cls.downloadname), filename);
	Com_Printf ("Downloading %s\n", cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname, sizeof(cls.downloadtempname));
	Q_strncatz (cls.downloadtempname, sizeof(cls.downloadtempname), ".tmp");

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("download %s", cls.downloadname));

	cls.downloadnumber++;
}

//=============================================================================

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload (void)
{
	int		size, percent;
	char	name[MAX_OSPATH];
	int		r;	// i

	// read the data
	size = MSG_ReadShort (&net_message);
	percent = MSG_ReadByte (&net_message);
	if (size == -1)
	{
		Com_Printf ("Server does not have this file.\n");

		if (cls.downloadname)	// Knightmare- save name of failed download
			CL_AddToFailedDownloadList (cls.downloadname);

		if (cls.download)
		{
			// if here, we tried to resume a file but the server said no
			fclose (cls.download);
			cls.download = NULL;
		}
		CL_RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		CL_Download_Reset_KBps_counter ();	// Knightmare- for KB/s counter

		CL_DownloadFileName (name, sizeof(name), cls.downloadtempname);

		FS_CreatePath (name);

		cls.download = fopen (name, "wb");
		if (!cls.download)
		{
			net_message.readcount += size;
			Com_Printf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
	}

	fwrite (net_message.data + net_message.readcount, 1, size, cls.download);
	net_message.readcount += size;

	if (percent != 100)
	{
		// request next block
// change display routines by zoid
#if 0
		Com_Printf (".");
		if (10*(percent/10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10*(percent/10);
			Com_Printf ("%i%%", cls.downloadpercent);
		}
#endif
		CL_Download_Calculate_KBps (size, 0);	// Knightmare- for KB/s counter
		cls.downloadpercent = percent;

		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "nextdl");
		cls.forcePacket = true;
	}
	else
	{
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

	//	Com_Printf ("100%%\n");

		fclose (cls.download);

		// rename the temp file to it's final name
		CL_DownloadFileName (oldn, sizeof(oldn), cls.downloadtempname);
		CL_DownloadFileName (newn, sizeof(newn), cls.downloadname);
		r = rename (oldn, newn);
		if (r)
			Com_Printf ("failed to rename.\n");

		cls.download = NULL;
		cls.downloadpercent = 0;

		// add new pk3s to search paths, hack by Jay Dolan
		if (strstr(newn, ".pk3")) 
			FS_AddPK3File (newn, false);

		// get another file if needed

		CL_RequestNextDownload ();
	}
}

//=============================================================================

// Download speed counter

typedef struct {
	int		prevTime;
	int		bytesRead;
	int		byteCount;
	float	timeCount;
	float	prevTimeCount;
	float	startTime;
} dlSpeedInfo_t;

dlSpeedInfo_t	dlSpeedInfo;

/*
=====================
CL_Download_Reset_KBps_counter
=====================
*/
void CL_Download_Reset_KBps_counter (void)
{
	dlSpeedInfo.timeCount = dlSpeedInfo.prevTime = dlSpeedInfo.prevTimeCount = dlSpeedInfo.bytesRead = dlSpeedInfo.byteCount = 0;
	dlSpeedInfo.startTime = (float)cls.realtime;
	cls.downloadrate = 0;
}

/*
=====================
CL_Download_Calculate_KBps
=====================
*/
void CL_Download_Calculate_KBps (int byteDistance, int totalSize)
{
	float	timeDistance = (float)(cls.realtime - dlSpeedInfo.prevTime);
	float	totalTime = (dlSpeedInfo.timeCount - dlSpeedInfo.startTime) / 1000.0f;

	dlSpeedInfo.timeCount += timeDistance;
	dlSpeedInfo.byteCount += byteDistance;
	dlSpeedInfo.bytesRead += byteDistance;

	if (totalTime >= 1.0f)
	{
		cls.downloadrate = (float)dlSpeedInfo.byteCount / 1024.0f;
		Com_DPrintf ("Rate: %4.2fKB/s, Downloaded %4.2fKB of %4.2fKB\n", cls.downloadrate, (float)dlSpeedInfo.bytesRead/1024.0, (float)totalSize/1024.0);
		dlSpeedInfo.byteCount = 0;
		dlSpeedInfo.startTime = (float)cls.realtime;
	}
	dlSpeedInfo.prevTime = cls.realtime;
}
