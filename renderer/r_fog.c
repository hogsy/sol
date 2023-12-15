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

// r_fog.c -- fog handling
// moved from r_main.c

#include "r_local.h"

// global fog vars w/ defaults
int FogModels[3] = {GL_LINEAR, GL_EXP, GL_EXP2};

static	qboolean r_fogenable = false;
static	qboolean r_fogsuspended = false;
static	int		r_fogmodel = GL_LINEAR;
static	float	r_fogdensity = 50.0;
static	float	r_fogskydensity = 5.0;
static	float	r_fognear = 64.0;
static	float	r_fogfar = 1024.0;
static	float	r_fogskyfar = 10240.0;
static	GLfloat r_fogColor[4] = {1.0, 1.0, 1.0, 1.0};

/*
================
R_SetFog
================
*/
void R_SetFog (void)
{
	R_SetFogVars (r_newrefdef.foginfo.enabled, r_newrefdef.foginfo.model,
		r_newrefdef.foginfo.density, r_newrefdef.foginfo.start, r_newrefdef.foginfo.end,
		r_newrefdef.foginfo.red, r_newrefdef.foginfo.green, r_newrefdef.foginfo.blue); 

	if (!r_fogenable)	// engine fog not enabled
		return;			// leave fog enabled if set by game DLL

	r_fogColor[3] = 1.0;
	qglEnable(GL_FOG);
	qglClearColor (r_fogColor[0], r_fogColor[1], r_fogColor[2], r_fogColor[3]); // Clear the background color to the fog color
	qglFogi(GL_FOG_MODE, r_fogmodel);
	qglFogfv(GL_FOG_COLOR, r_fogColor);
	if (r_fogmodel == GL_LINEAR)
	{
		qglFogf(GL_FOG_START, r_fognear); 
		qglFogf(GL_FOG_END, r_fogfar);
	}
	else
		qglFogf(GL_FOG_DENSITY, r_fogdensity/10000.f);
	qglHint (GL_FOG_HINT, GL_NICEST);

	// nVidia radial fog
	if (glConfig.nvFogAvailable) {
		qglFogi(GL_FOG_DISTANCE_MODE_NV, glConfig.nvFogMode);
	}
}


/*
================
R_SetFog2D
================
*/
void R_SetFog2D (void)
{
	// turn off nVidia radial fog to prevent fogging of 2D elements
	if (glConfig.nvFogAvailable) {
		qglFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE);
	}
}


/*
================
R_SetSkyFog
================
*/
void R_SetSkyFog (qboolean setSkyFog)
{
	if (!r_fogenable)	// engine fog not enabled
		return;			// leave fog enabled if set by game DLL

	if (setSkyFog)
	{
		if (r_fogmodel == GL_LINEAR)
			qglFogf(GL_FOG_END, r_fogskyfar);
		else
			qglFogf(GL_FOG_DENSITY, r_fogskydensity/10000.f);
	}
	else
	{
		if (r_fogmodel == GL_LINEAR)
			qglFogf(GL_FOG_END, r_fogfar);
		else
			qglFogf(GL_FOG_DENSITY, r_fogdensity/10000.f);
	}
}


/*
================
R_SuspendFog
================
*/
void R_SuspendFog (void)
{
	// check if fog is enabled; if so, disable it
	if (qglIsEnabled(GL_FOG)) 
	{
		r_fogsuspended = true;
		qglDisable(GL_FOG);
	}
}


/*
================
R_ResumeFog
================
*/
void R_ResumeFog (void)
{
	// re-enable fog if it was on
	if (r_fogsuspended)
	{
		r_fogsuspended = false;
		qglEnable(GL_FOG);
	}
}


/*
================
R_InitFogVars
================
*/
void R_InitFogVars (void)
{
	r_fogenable = false;
	r_fogsuspended = false;
	r_fogmodel = GL_LINEAR;
	r_fogdensity = 50.0;
	r_fogskydensity = 5.0;
	r_fognear = 64.0;
	r_fogfar = 1024.0;
	r_fogskyfar = 10240.0;
	r_fogColor[0] = r_fogColor[1] = r_fogColor[2] = r_fogColor[3] = 1.0f;
}


/*
================
R_SetFogVars
================
*/
void R_SetFogVars (qboolean enable, int model, int density,
				   int start, int end, int red, int green, int blue)
{
	int		temp;
	float	maxFogFar, skyRatio;

	//VID_Printf( PRINT_ALL, "Setting fog variables: model %i density %i near %i far %i red %i green %i blue %i\n",
	//	model, density, start, end, red, green, blue );

	// Skip this if QGL subsystem is already down
	if (!qglDisable)	return;

	r_fogenable = enable;
	if (!r_fogenable) { // recieved fog disable message
		qglDisable(GL_FOG);
		return;
	}
	temp = model;
	if ((temp > 2) || (temp < 0)) temp = 0;

	if (r_skydistance && r_skydistance->value)
		maxFogFar = r_skydistance->value;
	else
		maxFogFar = 10000.0f;
	maxFogFar = max(maxFogFar, 1024.0f);

	if ( r_fog_skyratio && r_skydistance && r_skydistance->value )
	{
		if (r_fog_skyratio->integer < 0)	// -1 is auto distance based on old Q2 sky distance of 2300
			skyRatio = (r_skydistance->value / OLD_Q2_SKYDIST);
		else if (r_fog_skyratio->value >= 1.0f)
			skyRatio = r_fog_skyratio->value;
		else
			skyRatio = 10.0f;
	}
	else
		skyRatio = 10.0f;
	skyRatio = max (skyRatio, 1.0f);
	skyRatio = min (skyRatio, 100.0f);

	r_fogmodel = FogModels[temp];
	r_fogdensity = (float)density;
	r_fogskydensity = r_fogdensity / skyRatio;
	if (temp == 0) {	// GL_LINEAR
		r_fognear = (float)start;
		r_fogfar = (float)end;
		r_fogskyfar = r_fogfar * skyRatio;
	}
	r_fogColor[0] = ((float)red)/255.0f;
	r_fogColor[1] = ((float)green)/255.0f;
	r_fogColor[2] = ((float)blue)/255.0f;

	// clamp vars
	r_fogdensity = max(r_fogdensity, 0.0f);
	r_fogdensity = min(r_fogdensity, 100.0f);
	r_fogskydensity = max(r_fogskydensity, 0.0f);
	r_fogskydensity = min(r_fogskydensity, (100.0f / skyRatio));
	r_fognear = max(r_fognear, 0.0f);
	r_fognear = min(r_fognear, 10000.0f - 64.0f);
	r_fogfar = max(r_fogfar, r_fognear + 64.0f);
//	r_fogfar = min(r_fogfar, 10000.0);
	r_fogfar = min(r_fogfar, maxFogFar);
	r_fogskyfar = max(r_fogskyfar, (r_fognear + 64.0f) * skyRatio);
	r_fogskyfar = min(r_fogskyfar, maxFogFar*skyRatio);
	r_fogColor[0] = max(r_fogColor[0], 0.0f);
	r_fogColor[0] = min(r_fogColor[0], 1.0f);
	r_fogColor[1] = max(r_fogColor[1], 0.0f);
	r_fogColor[1] = min(r_fogColor[1], 1.0f);
	r_fogColor[2] = max(r_fogColor[2], 0.0f);
	r_fogColor[2] = min(r_fogColor[2], 1.0f);

/*	VID_Printf (PRINT_ALL, "Set fog variables: model %i density %f near %f far %f red %f green %f blue %f\n",
		model, r_fogdensity, r_fognear, r_fogfar, r_fogColor[0], r_fogColor[1], r_fogColor[2]); */
}
