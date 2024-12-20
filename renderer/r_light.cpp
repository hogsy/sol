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

// r_light.c

#include "r_local.h"

int	r_dlightframecount;

gllightmapstate_t gl_lms;

static void		LM_InitBlock (void);
static void		LM_UploadBlock (void);
static qboolean	LM_AllocBlock (int w, int h, int *x, int *y);

//#define	DLIGHT_CUTOFF	64	// Knightmare- no longer hard-coded

/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

#define DLIGHTRADUIS 16.0 // was 32.0

/*
=============
R_AddDlight
=============
*/
void R_AddDlight (dlight_t *light)
{
	int		i, j;
	float	a;
	vec3_t	v;
	float	rad;

	rad = light->intensity * 0.35;

	VectorSubtract (light->origin, r_origin, v);

	for (i=0; i<3; i++)
		v[i] = light->origin[i] - vpn[i]*rad;

	if (RB_CheckArrayOverflow (DLIGHTRADUIS+1, DLIGHTRADUIS*3)) 
		RB_RenderMeshGeneric (true);

	for (i = 1; i <= DLIGHTRADUIS; i++) {
		indexArray[rb_index++] = rb_vertex;
		indexArray[rb_index++] = rb_vertex+i;
		indexArray[rb_index++] = rb_vertex+1+((i<DLIGHTRADUIS)?i:0);
	}

	VA_SetElem3(vertexArray[rb_vertex], v[0], v[1], v[2]);
	VA_SetElem4(colorArray[rb_vertex], light->color[0]*0.2, light->color[1]*0.2, light->color[2]*0.2, 1.0);
	rb_vertex++;

	for (i=DLIGHTRADUIS; i>0; i--)
	{
		a = i/DLIGHTRADUIS * M_PI*2;
		for (j=0; j<3; j++)
			v[j] = light->origin[j] + vright[j]*cos(a)*rad + vup[j]*sin(a)*rad;

		VA_SetElem3(vertexArray[rb_vertex], v[0], v[1], v[2]);
		VA_SetElem4(colorArray[rb_vertex], 0, 0, 0, 1.0);
		rb_vertex++;
	}
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights (void)
{
	int		i;
	dlight_t	*l;

	if (!r_flashblend->integer)
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											// advanced yet for this frame
	GL_DepthMask (0);
	GL_DisableTexture (0);
	GL_ShadeModel (GL_SMOOTH);
	GL_Enable (GL_BLEND);
	GL_BlendFunc (GL_ONE, GL_ONE);

	rb_vertex = rb_index = 0;

	l = r_newrefdef.dlights;
	for (i=0; i<r_newrefdef.num_dlights; i++, l++)
		R_AddDlight (l);

	RB_RenderMeshGeneric (true);

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_Disable (GL_BLEND);
	GL_ShadeModel (GL_FLAT);
	GL_EnableTexture (0);
	GL_DepthMask (1);
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights (dlight_t *light, int num, mnode_t *node)
{
	cplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;
	int			sidebit; //Knightmare added

	if (node->contents != -1)
		return;

	splitplane = node->plane;
	dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;
	
	if (dist > light->intensity - r_lightcutoff->value)	//** DMP var dynalight cutoff
	{
		R_MarkLights (light, num, node->children[0]);
		return;
	}
	if (dist < -light->intensity + r_lightcutoff->value)	//** DMP var dynalight cutoff
	{
		R_MarkLights (light, num, node->children[1]);
		return;
	}
		
// mark the polygons
	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0; i<node->numsurfaces; i++, surf++)
	{
		// Knightmare- Discoloda's dynamic light clipping
		if (r_dlights_normal->integer)
		{
			dist = DotProduct (light->origin, surf->plane->normal) - surf->plane->dist;
			if (dist >= 0)
				sidebit = 0;
			else
				sidebit = SURF_PLANEBACK;

			if ( (surf->flags & SURF_PLANEBACK) != sidebit)
				continue;
		}
		// end Knightmare

		if (surf->dlightframe != r_dlightframecount)
		{
		//	surf->dlightbits = bit; // was 0
			memset (surf->dlightbits, 0, sizeof(surf->dlightbits));
			surf->dlightbits[num >> 5] = 1 << (num & 31);	// was 0, fixes hyperblaster tearing
			surf->dlightframe = r_dlightframecount;
		}
		else
		//	surf->dlightbits |= bit;
			surf->dlightbits[num >> 5] |= 1 << (num & 31);
	}

	R_MarkLights (light, num, node->children[0]);
	R_MarkLights (light, num, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int		i;
	dlight_t	*l;

	if (r_flashblend->integer)
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	l = r_newrefdef.dlights;
	for (i=0; i<r_newrefdef.num_dlights; i++, l++)
		R_MarkLights ( l, i, r_worldmodel->nodes );
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

vec3_t			pointcolor;
cplane_t		*lightplane;		// used as shadow plane
vec3_t			lightspot;

/*
===============
RecursiveLightPoint
===============
*/
int RecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	int			maps;
	int			r;

	if (node->contents != -1)
		return -1;		// didn't hit anything
	
	// calculate mid point

	// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;
	
	if ( (back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
	// go down front side	
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something
		
	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing
		
	// check for impact on this node
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
	// Knightmare- allow warp surfaces with light samples
#ifdef WARP_LIGHTMAPS
		if ( (surf->flags & SURF_DRAWSKY) || ((surf->flags & SURF_DRAWTURB) && !surf->isLightmapped) )
			continue;	// no lightmaps
		if ( (surf->texinfo->flags & SURF_SKY) || ((surf->texinfo->flags & SURF_WARP) && !surf->isLightmapped) )
			continue;	// no lightmaps
		if ( (node->contents & MASK_WATER) && ((surf->flags & SURF_DRAWTURB) || (surf->texinfo->flags & SURF_WARP))  )
			continue;	// skip inside-liquid warp surfaces, as they will have darker lightmaps
	//	if ((surf->flags & SURF_DRAWTURB) || (surf->texinfo->flags & SURF_WARP))
	//		continue;	// skip warp surfaces if warp lightmap loading is enabled, as water lightmaps may screw up model lighting
#else	// WARP_LIGHTMAPS
		if (surf->flags & (SURF_DRAWTURB|SURF_DRAWSKY)) 
			continue;	// no lightmaps
		if (surf->texinfo->flags & (SURF_SKY|SURF_WARP))
			continue;	// no lightmaps
#endif	// WARP_LIGHTMAPS

		tex = surf->texinfo;
		
		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		VectorCopy (vec3_origin, pointcolor);
		if (lightmap)
		{
			vec3_t scale;

			lightmap += 3*(dt * ((surf->extents[0]>>4)+1) + ds);

			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				if ( !r_newrefdef.lightstyles || !surf->styles )
					break;
				for (i = 0; i < 3; i++)
					scale[i] = r_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

				pointcolor[0] += lightmap[0] * scale[0] * (1.0/255);
				pointcolor[1] += lightmap[1] * scale[1] * (1.0/255);
				pointcolor[2] += lightmap[2] * scale[2] * (1.0/255);
				lightmap += 3*((surf->extents[0]>>4)+1) *
						((surf->extents[1]>>4)+1);
			}
		}
		
		return 1;
	}

	// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}


/*
===============
R_MaxColorVec

Psychospaz's lighting on alpha surfaces
===============
*/
void R_MaxColorVec (vec3_t color)
{
	int j;
	float lightest = 0.0f;

	for (j=0;j<3;j++)
		if (color[j]>lightest)
			lightest= color[j];
	if (lightest>255)
		for (j=0;j<3;j++)
			color[j]*= 255/lightest;

	for (j=0;j<3;j++)
	{
		if (color[j]>1) color[j] = 1;
		if (color[j]<0) color[j] = 0;
	}
}


/*
===============
R_LightPoint
===============
*/
void R_LightPoint (vec3_t p, vec3_t color, qboolean isEnt)
{
	vec3_t		end;
	float		r;
	int			lnum, i;
	dlight_t	*dl;
	float		light;
	vec3_t		dist;
	float		add;
	
	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	r = RecursiveLightPoint (r_worldmodel->nodes, p, end);
	
	if (r == -1)
	{
		VectorCopy (vec3_origin, color);
	}
	else
	{
		VectorCopy (pointcolor, color);
	}

	// this catches too bright modulated color
	for (i=0; i<3; i++)
		if (color[i]>1) color[i] = 1;

	//
	// add dynamic lights
	//
	light = 0;
	dl = r_newrefdef.dlights;
	for (lnum=0; lnum < r_newrefdef.num_dlights; lnum++, dl++)
	{
		if (dl->spotlight) // spotlights
			continue;

		if (isEnt)
			VectorSubtract (currententity->origin, dl->origin, dist);
		else
			VectorSubtract (p, dl->origin, dist);

		add = dl->intensity - VectorLength(dist);
		add *= (1.0/256);
		if (add > 0)
			VectorMA (color, add, dl->color, color);
	}

	// scale up light color by r_modulate if enabled
	if (r_entity_doublelight->integer) {
		VectorScale (color, r_modulate->value, color);
	}
//	VectorScale (color, r_modulate->value, color); // Knightmare- this makes ents too bright
//	VectorScale (color, r_modulate->value*1.5f, color); // Knightmare- this makes ents too bright
}


/*
===============
R_LightPointDynamics
===============
*/
void R_LightPointDynamics (vec3_t p, vec3_t color, m_dlight_t *list, int *amount, int max)
{
	vec3_t		end;
	float		r;
	int			lnum, i, m_dl;
	dlight_t	*dl;
	vec3_t		dist, dlColor;
	float		add;
	
	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	r = RecursiveLightPoint (r_worldmodel->nodes, p, end);
	
	if (r == -1)
	{
		VectorCopy (vec3_origin, color);
	}
	else
	{
		VectorCopy (pointcolor, color);
	}

	// this catches too bright modulated color
	for (i=0;i<3;i++)
		if (color[i]>1) color[i] = 1;

	//
	// add dynamic lights
	//
	m_dl = 0;
	dl = r_newrefdef.dlights;
	for (lnum=0; lnum<r_newrefdef.num_dlights; lnum++, dl++)
	{
		if (dl->spotlight) // spotlights
			continue;

		VectorSubtract (dl->origin, p, dist);

		add = dl->intensity - VectorNormalize(dist);
		add *= (DIV256);
		if (add > 0)
		{
			float highest = -1;

			VectorScale(dl->color, add, dlColor);
			for (i=0; i<3; i++)
				if (highest<dlColor[i]) highest = dlColor[i];

			if (m_dl<max)
			{
				list[m_dl].strength = highest;
				VectorCopy(dist, list[m_dl].direction);
				VectorCopy(dlColor, list[m_dl].color);
				m_dl++;
			}
			else
			{
				float	least_val = 10;
				int		least_index = 0;

				for (i=0; i<m_dl; i++)
					if (list[i].strength < least_val)
					{


						least_val = list[i].strength;
						least_index = i;
					}

				VectorAdd (color, list[least_index].color, color);
				list[least_index].strength = highest;
				VectorCopy(dist, list[least_index].direction);
				VectorCopy(dlColor, list[least_index].color);
			}
		}
	}

	// scale up light color by r_modulate if enabled
	if (r_entity_doublelight->integer) {
		VectorScale (color, r_modulate->value, color);
	}

	*amount = m_dl;
}


/*
===============
R_SurfLightPoint
===============
*/
void R_SurfLightPoint (msurface_t *surf, vec3_t p, vec3_t color, qboolean vertexLightBase)
{
	vec3_t		start, end, dist, bestColor;
	vec3_t		dlorigin, temp, forward, right, up;
	vec3_t		startOffset[9] = { {0,0,0}, {-1,0,0}, {-1,-1,0}, {0,-1,0}, {1,-1,0}, {1,0,0}, {1,1,0}, {0,1,0}, {-1,1,0} };
	float		r, light, add;
	int			lnum, i;
	dlight_t	*dl;
	// Knightmare- fix for moving surfaces
	qboolean	rotated = false;
	entity_t	*hostent = NULL;

	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}

	// Knightmare- fix for moving surfaces
	if (surf)
		hostent = surf->entity;
	if (hostent && (hostent->angles[0] || hostent->angles[1] || hostent->angles[2]))
	{
		rotated = true;
		AngleVectors (hostent->angles, forward, right, up);
	}

	if (vertexLightBase)
	{
		VectorCopy (vec3_origin, bestColor);
		for (i=0; i<9; i++) // test multiple offset points to avoid dark corners, select brightest
		{
			vec3_t tempOffset;
			VectorCopy (p, start);
			VectorScale (startOffset[i], min(max(r_warp_lighting_sample_offset->value,1.0f),16.0f), tempOffset);
			VectorAdd (start, tempOffset, start);
			end[0] = start[0];
			end[1] = start[1];
			end[2] = start[2] - 2048;

			r = RecursiveLightPoint (r_worldmodel->nodes, start, end);

			if ( (r != -1) && (VectorLength(pointcolor) > VectorLength(bestColor)) )
				VectorCopy (pointcolor, bestColor);
		}
		/*
		end[0] = p[0];
		end[1] = p[1];
		end[2] = p[2] - 2048;
		
		r = RecursiveLightPoint (r_worldmodel->nodes, p, end);
		
		if (r == -1)
			VectorCopy (vec3_origin, color);
		else
			VectorCopy (pointcolor, color);*/
			VectorCopy (bestColor, color);

		// this catches too bright modulated color
		for (i=0; i<3; i++)
			if (color[i]>1) color[i] = 1;
	}
	else
	{
		VectorClear(color);
		//
		// add dynamic lights
		//
		light = 0;
		dl = r_newrefdef.dlights;
		for (lnum=0; lnum<r_newrefdef.num_dlights; lnum++, dl++)
		{
			if (dl->spotlight) // spotlights
				continue;
			if (r_flashblend->integer || !r_dynamic->integer) // no dlight casting
				continue;

			// Knightmare- fix for moving surfaces
			VectorCopy (dl->origin, dlorigin);
			if (hostent)
				VectorSubtract (dlorigin, hostent->origin, dlorigin);
			if (rotated)
			{
				VectorCopy (dlorigin, temp);
				dlorigin[0] = DotProduct (temp, forward);
				dlorigin[1] = -DotProduct (temp, right);
				dlorigin[2] = DotProduct (temp, up);
			}
			VectorSubtract (p, dlorigin, dist);
			// end Knightmare
		//	VectorSubtract (p, dl->origin, dist);

			add = dl->intensity - VectorLength(dist);
			add *= (DIV256);
			if (add > 0)
			{
				VectorMA (color, add, dl->color, color);
			}
		}
	}
}

//===================================================================
// Knightmare- added Psychospaz's dynamic light-based shadows

/*
===============
R_ShadowLight
===============
*/
void R_ShadowLight (vec3_t pos, vec3_t lightAdd)
{
	int			lnum;
	dlight_t	*dl;
	vec3_t		dist, angle;
	float		add, shadowdist;

	if (!r_worldmodel)
		return;
	if (!r_worldmodel->lightdata) // keep old lame shadow
		return;
	
	VectorClear(lightAdd);

	//
	// add dynamic light shadow angles
	//
	if (r_shadows->integer == 2)
	{
		dl = r_newrefdef.dlights;
		for (lnum=0; lnum<r_newrefdef.num_dlights; lnum++, dl++)
		{
			if (dl->spotlight) //spotlights
				continue;

			VectorSubtract (dl->origin, pos, dist);
			add = 0.20*sqrt(dl->intensity - VectorLength(dist));
			if (add > 0) {
				VectorNormalize(dist);
				VectorScale(dist, add, dist);
				VectorAdd (lightAdd, dist, lightAdd);
			}
		}
	}

#if 0
	shadowdist = VectorNormalize(lightAdd);
	if (shadowdist>2048) shadowdist=2048;
	if (shadowdist<=0) return;

	// now rotate according to model yaw
	vectoangles2 (lightAdd, angle);
	angle[YAW] = -(currententity->angles[YAW]-angle[YAW]);
	AngleVectors (angle, dist, NULL, NULL);
	VectorScale (dist, shadowdist, lightAdd); 
#else
	// Barnes improved code
	shadowdist = VectorNormalize(lightAdd);
	if (shadowdist > 4) shadowdist = 4;
	if (shadowdist < 1) // old style static shadow
	{
		//angle[PITCH] = currententity->angles[PITCH];
		//angle[YAW] =   -currententity->angles[YAW];
		//angle[ROLL] =   currententity->angles[ROLL];
		add = currententity->angles[1]/180*M_PI;
		dist[0] = cos(-add);
		dist[1] = sin(-add);
		dist[2] = 1;
		VectorNormalize (dist);
		shadowdist = 1;
	}
	else // shadow from dynamic lights
	{
		vectoangles2 (lightAdd, angle);
		angle[YAW] -= currententity->angles[YAW];
		AngleVectors (angle, dist, NULL, NULL);
	}
	VectorScale (dist, shadowdist, lightAdd); 
#endif
}

//===================================================================

static float s_blocklights[136*136*3]; //Knightmare-  was [34*34*3], supports max chop size of 2048?
/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (msurface_t *surf)
{
	int			lnum;
	int			sd, td;
	float		fdist, frad, fminlight;
	vec3_t		impact, local, dlorigin, temp, entOrigin, entAngles;
	int			s, t;
	int			i;
	int			smax, tmax;
	mtexinfo_t	*tex;
	dlight_t	*dl;
	float		*pfBL;
	float		fsacc, ftacc;
	qboolean rotated = false;
	vec3_t	forward, right, up;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	// currententity is not valid for trans surfaces
	if (tex->flags & (SURF_TRANS33|SURF_TRANS66)) {
		if (surf->entity) {
			VectorCopy (surf->entity->origin, entOrigin);
			VectorCopy (surf->entity->angles, entAngles);
		}
		else {
			VectorCopy (vec3_origin, entOrigin);
			VectorCopy (vec3_origin, entAngles);
		}
	}
	else {
		VectorCopy (currententity->origin, entOrigin);
		VectorCopy (currententity->angles, entAngles);
	}

	if (entAngles[0] || entAngles[1] || entAngles[2])
	{
		rotated = true;
		AngleVectors (entAngles, forward, right, up);
	}

	for (lnum=0; lnum<r_newrefdef.num_dlights; lnum++)
	{
	//	if ( !(surf->dlightbits & (1<<lnum) ) )
		if ( !(surf->dlightbits[lnum >> 5] & (1 << (lnum & 31)) ) )
			continue;		// not lit by this light

		dl = &r_newrefdef.dlights[lnum];

		if (dl->spotlight) //spotlights
			continue;

		frad = dl->intensity;

		VectorCopy (dl->origin, dlorigin);
		VectorSubtract (dlorigin, entOrigin, dlorigin);

		if (rotated)
		{
			VectorCopy (dlorigin, temp);
			dlorigin[0] = DotProduct (temp, forward);
			dlorigin[1] = -DotProduct (temp, right);
			dlorigin[2] = DotProduct (temp, up);
		}

		fdist = DotProduct (dlorigin, surf->plane->normal) -
				surf->plane->dist;
		frad -= fabs(fdist);
		// rad is now the highest intensity on the plane

		fminlight = r_lightcutoff->value; 	//** DMP var dynalight cutoff
		if (frad < fminlight)
			continue;
		fminlight = frad - fminlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = dlorigin[i] -
					surf->plane->normal[i]*fdist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

		pfBL = s_blocklights;
		for (t = 0, ftacc = 0; t<tmax; t++, ftacc += 16)
		{
			td = local[1] - ftacc;
			if ( td < 0 )
				td = -td;

			for ( s=0, fsacc = 0; s<smax; s++, fsacc += 16, pfBL += 3)
			{
				sd = Q_ftol( local[0] - fsacc );

				if ( sd < 0 )
					sd = -sd;

				if (sd > td)
					fdist = sd + (td>>1);
				else
					fdist = td + (sd>>1);

				if ( fdist < fminlight )
				{
					pfBL[0] += ( frad - fdist ) * dl->color[0];
					pfBL[1] += ( frad - fdist ) * dl->color[1];
					pfBL[2] += ( frad - fdist ) * dl->color[2];
				}
			}
		}
	}
}

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

/*
================
LM_InitBlock
================
*/
static void LM_InitBlock (void)
{
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );

#ifdef BATCH_LM_UPDATES
	// alloc lightmap update buffer if needed
	if ( !gl_lms.lightmap_update[gl_lms.current_lightmap_texture] ) {
		gl_lms.lightmap_update[gl_lms.current_lightmap_texture] = Z_Malloc (LM_BLOCK_WIDTH*LM_BLOCK_HEIGHT*LIGHTMAP_BYTES);
	}
#endif	// BATCH_LM_UPDATES
}


/*
================
LM_UploadBlock
================
*/
static void LM_UploadBlock (void)
{
	GL_Bind (glState.lightmap_textures + gl_lms.current_lightmap_texture);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	qglTexImage2D( GL_TEXTURE_2D, 0, 
				   gl_lms.internal_format,
				   LM_BLOCK_WIDTH, LM_BLOCK_HEIGHT, 
				   0, 
	//			   GL_LIGHTMAP_FORMAT, GL_LIGHTMAP_TYPE, 
					   gl_lms.format, gl_lms.type,
#ifdef BATCH_LM_UPDATES
				   gl_lms.lightmap_update[gl_lms.current_lightmap_texture] );
#else
				   gl_lms.lightmap_buffer );
#endif	// BATCH_LM_UPDATES
	if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
		VID_Error( ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
}


/*
================
LM_AllocBlock
returns a texture number and the position inside it
================
*/
static qboolean LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = LM_BLOCK_HEIGHT;

	for (i=0 ; i<LM_BLOCK_WIDTH-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (gl_lms.allocated[i+j] >= best)
				break;
			if (gl_lms.allocated[i+j] > best2)
				best2 = gl_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > LM_BLOCK_HEIGHT)
		return false;

	for (i=0 ; i<w ; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}


/*
===============
R_SetCacheState
===============
*/
void R_SetCacheState (msurface_t *surf)
{
	int maps;

	for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
	{
		surf->cached_light[maps] = r_newrefdef.lightstyles[surf->styles[maps]].white;
	}
#ifdef BATCH_LM_UPDATES
	// mark if dynamicly lit
	surf->cached_dlight = (surf->dlightframe == r_framecount);
#endif
}


/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride)
{
	int			smax, tmax;
	int			r, g, b, a, max;
	int			i, j, size;
	byte		*lightmap;
	float		scale[4];
	int			nummaps;
	float		*bl;
	lightstyle_t	*style;
	int			monolightmap;

#ifdef WARP_LIGHTMAPS
	// Knightmare- allow warp surfaces with light samples
	if ( (surf->texinfo->flags & SURF_SKY) || ((surf->texinfo->flags & SURF_WARP) && !surf->isLightmapped) )
#else	// WARP_LIGHTMAPS
	if ( surf->texinfo->flags & (SURF_SKY|SURF_WARP) )
#endif	// WARP_LIGHTMAPS
		VID_Error ( ERR_DROP, "R_BuildLightMap called for non-lit surface" );

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	// FIXME- can this limit be directly increased?		Yep - Knightmare
	if (size > (sizeof(s_blocklights)>>4) )
		VID_Error ( ERR_DROP, "Bad s_blocklights size: %d", size );

	// set to full bright if no light data
	if (!surf->samples)
	{
		int maps;

		for (i=0 ; i<size*3 ; i++)
			s_blocklights[i] = 255;
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			style = &r_newrefdef.lightstyles[surf->styles[maps]];
		}
		goto store;
	}

	// count the # of maps
	for ( nummaps = 0; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255; nummaps++)
		;

	lightmap = surf->samples;

	// add all the lightmaps
	if ( nummaps == 1 )
	{
		int maps;

		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			bl = s_blocklights;

			for (i = 0; i < 3; i++)
				scale[i] = r_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i = 0; i < size; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0];
					bl[1] = lightmap[i*3+1];
					bl[2] = lightmap[i*3+2];
				}
			}
			else
			{
				for (i = 0; i < size; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0] * scale[0];
					bl[1] = lightmap[i*3+1] * scale[1];
					bl[2] = lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}
	else
	{
		int maps;

		memset( s_blocklights, 0, sizeof( s_blocklights[0] ) * size * 3 );

		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			bl = s_blocklights;

			for (i = 0; i < 3; i++)
				scale[i] = r_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i = 0; i < size; i++, bl+=3 )
				{
					bl[0] += lightmap[i*3+0];
					bl[1] += lightmap[i*3+1];
					bl[2] += lightmap[i*3+2];
				}
			}
			else
			{
				for (i = 0; i < size; i++, bl+=3)
				{
					bl[0] += lightmap[i*3+0] * scale[0];
					bl[1] += lightmap[i*3+1] * scale[1];
					bl[2] += lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}

	// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights (surf);

	// put into texture format
store:
	stride -= (smax<<2);
	bl = s_blocklights;

	monolightmap = r_monolightmap->string[0];

	if ( monolightmap == '0' )
	{
		for (i = 0; i < tmax; i++, dest += stride)
		{
			for (j = 0; j < smax; j++)
			{
				
				r = Q_ftol( bl[0] );
				g = Q_ftol( bl[1] );
				b = Q_ftol( bl[2] );

				// catch negative lights
				if (r < 0)
					r = 0;
				if (g < 0)
					g = 0;
				if (b < 0)
					b = 0;

				//
				// determine the brightest of the three color components
				//
				if (r > g)
					max = r;
				else
					max = g;
				if (b > max)
					max = b;

				//
				// alpha is ONLY used for the mono lightmap case.  For this reason
				// we set it to the brightest of the color components so that 
				// things don't get too dim.
				//
				a = max;

				//
				// rescale all the color components if the intensity of the greatest
				// channel exceeds 1.0
				//
				if (max > 255)
				{
					float t = 255.0F / max;

					r = r*t;
					g = g*t;
					b = b*t;
					a = a*t;
				}
				a = 255;	// fix for alpha test

				if (gl_lms.format == GL_BGRA)
				{
					dest[0] = b;
					dest[1] = g;
					dest[2] = r;
					dest[3] = a;
				}
				else
				{
					dest[0] = r;
					dest[1] = g;
					dest[2] = b;
					dest[3] = a;
				}

				bl += 3;
				dest += 4;
			}
		}
	}
	else
	{
		for (i = 0; i < tmax; i++, dest += stride)
		{
			for (j = 0; j < smax; j++)
			{
				
				r = Q_ftol( bl[0] );
				g = Q_ftol( bl[1] );
				b = Q_ftol( bl[2] );

				// catch negative lights
				if (r < 0)
					r = 0;
				if (g < 0)
					g = 0;
				if (b < 0)
					b = 0;

				//
				// determine the brightest of the three color components
				//
				if (r > g)
					max = r;
				else
					max = g;
				if (b > max)
					max = b;

				//
				// alpha is ONLY used for the mono lightmap case.  For this reason
				// we set it to the brightest of the color components so that 
				// things don't get too dim.
				//
				a = max;

				//
				// rescale all the color components if the intensity of the greatest
				// channel exceeds 1.0
				//
				if (max > 255)
				{
					float t = 255.0F / max;

					r = r*t;
					g = g*t;
					b = b*t;
					a = a*t;
				}

				//
				// So if we are doing alpha lightmaps we need to set the R, G, and B
				// components to 0 and we need to set alpha to 1-alpha.
				//
				switch ( monolightmap )
				{
				case 'L':
				case 'I':
					r = a;
					g = b = 0;
					a = 255;	// fix for alpha test
					break;
				case 'C':
					// try faking colored lighting
					a = 255 - ((r+g+b)/3); //Knightmare changed
					r *= a*0.003921568627450980392156862745098; // /255.0;
					g *= a*0.003921568627450980392156862745098; // /255.0;
					b *= a*0.003921568627450980392156862745098; // /255.0;
					a = 255;	// fix for alpha test
					break;
				case 'A':
				//	r = g = b = 0;
					a = 255 - a;
					r = g = b = a;
					break;
				default:
					r = g = b = a;
					a = 255;	// fix for alpha test
					break;
				}

				if (gl_lms.format == GL_BGRA)
				{
					dest[0] = b;
					dest[1] = g;
					dest[2] = r;
					dest[3] = a;
				}
				else
				{
					dest[0] = r;
					dest[1] = g;
					dest[2] = b;
					dest[3] = a;
				}

				bl += 3;
				dest += 4;
			}
		}
	}
}


/*
========================
R_CreateSurfaceLightmap
========================
*/
void R_CreateSurfaceLightmap (msurface_t *surf)
{
	int			smax, tmax;
	unsigned	*base;

#ifdef WARP_LIGHTMAPS
	// Knightmare- allow warp surfaces with light samples
	if ( (surf->flags & SURF_DRAWSKY) || ((surf->flags & SURF_DRAWTURB) && !surf->isLightmapped) )
		return;
	if ( (surf->texinfo->flags & SURF_SKY) || ((surf->texinfo->flags & SURF_WARP) && !surf->isLightmapped) )
		return;
#else	// WARP_LIGHTMAPS
	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;
	if (surf->texinfo->flags & (SURF_SKY|SURF_WARP))
		return;
#endif	// WARP_LIGHTMAPS


	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	if ( !LM_AllocBlock (smax, tmax, &surf->light_s, &surf->light_t) )
	{
		LM_UploadBlock ();
		LM_InitBlock();
		if ( !LM_AllocBlock (smax, tmax, &surf->light_s, &surf->light_t) )
		{
			VID_Error ( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	// copy extents
	surf->light_smax = smax;
	surf->light_tmax = tmax;

#ifdef BATCH_LM_UPDATES
	base = gl_lms.lightmap_update[surf->lightmaptexturenum];
#else
	base = gl_lms.lightmap_buffer;
#endif	// BATCH_LM_UPDATES

	base += (surf->light_t * LM_BLOCK_WIDTH + surf->light_s);	// * LIGHTMAP_BYTES

	R_SetCacheState (surf);
	R_BuildLightMap (surf, (void *)base, LM_BLOCK_WIDTH*LIGHTMAP_BYTES);
}


#define gl_tex_solid_format		3
#define gl_tex_alpha_format		4
/*
==================
R_BeginBuildingLightmaps
==================
*/
void R_BeginBuildingLightmaps (model_t *m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;
	unsigned		dummy[LM_BLOCK_WIDTH*LM_BLOCK_HEIGHT];	// 128*128

	memset (gl_lms.allocated, 0, sizeof(gl_lms.allocated));

#ifdef BATCH_LM_UPDATES
	// free lightmap update buffers
	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		if (gl_lms.lightmap_update[i])
			Z_Free (gl_lms.lightmap_update[i]);
		gl_lms.lightmap_update[i] = NULL;
		gl_lms.modified[i] = false;
		gl_lms.lightrect[i].left = LM_BLOCK_WIDTH;
		gl_lms.lightrect[i].right = 0;
		gl_lms.lightrect[i].top = LM_BLOCK_HEIGHT;
		gl_lms.lightrect[i].bottom = 0;
	}
#endif	// BATCH_LM_UPDATES

	r_framecount = 1;		// no dlightcache

	GL_EnableMultitexture (true);
	GL_SelectTexture (1);

	// setup the base lightstyles so the lightmaps won't have to be regenerated
	// the first time they're seen
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	if ( !glState.lightmap_textures )
	{
		glState.lightmap_textures	= TEXNUM_LIGHTMAPS;
	//	glState.lightmap_textures	= glState.texture_extension_number;
	//	glState.texture_extension_number = glState.lightmap_textures + MAX_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;

#ifdef BATCH_LM_UPDATES
	// alloc lightmap update buffer if needed
	if ( !gl_lms.lightmap_update[gl_lms.current_lightmap_texture] ) {
		gl_lms.lightmap_update[gl_lms.current_lightmap_texture] = Z_Malloc (LM_BLOCK_WIDTH*LM_BLOCK_HEIGHT*LIGHTMAP_BYTES);
	}
#endif	// BATCH_LM_UPDATES


	/*
	** if mono lightmaps are enabled and we want to use alpha
	** blending (a,1-a) then we're likely running on a 3DLabs
	** Permedia2.  In a perfect world we'd use a GL_ALPHA lightmap
	** in order to conserve space and maximize bandwidth, however 
	** this isn't a perfect world.
	**
	** So we have to use alpha lightmaps, but stored in GL_RGBA format,
	** which means we only get 1/16th the color resolution we should when
	** using alpha lightmaps.  If we find another board that supports
	** only alpha lightmaps but that can at least support the GL_ALPHA
	** format then we should change this code to use real alpha maps.
	*/

	// Knightmare- old internal formats for compatibility with older GPUs/drivers
	if ( !glConfig.newLMFormat )
	{
		if ( toupper( r_monolightmap->string[0] ) == 'A' )
		{
			gl_lms.internal_format = gl_tex_alpha_format;
		}
		// try to do hacked colored lighting with a blended texture
		else if ( toupper( r_monolightmap->string[0] ) == 'C' )
		{
			gl_lms.internal_format = gl_tex_alpha_format;
		}
		else if ( toupper( r_monolightmap->string[0] ) == 'I' )
		{
			gl_lms.internal_format = GL_INTENSITY8;
		}
		else if ( toupper( r_monolightmap->string[0] ) == 'L' ) 
		{
			gl_lms.internal_format = GL_LUMINANCE8;
		}
		else
		{
			gl_lms.internal_format = gl_tex_solid_format;
		}

		gl_lms.format = GL_RGBA;
		gl_lms.type = GL_UNSIGNED_BYTE;
	}
	else
	{
		if ( toupper( r_monolightmap->string[0] ) == 'I' )
			gl_lms.internal_format = GL_INTENSITY8;
		else if ( toupper( r_monolightmap->string[0] ) == 'L' ) 
			gl_lms.internal_format = GL_LUMINANCE8;
		else
			gl_lms.internal_format = GL_RGBA8;

		gl_lms.format = GL_BGRA;
		gl_lms.type = GL_UNSIGNED_INT_8_8_8_8_REV;
	}

	// initialize the dynamic lightmap texture
	GL_Bind( glState.lightmap_textures + 0 );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D( GL_TEXTURE_2D, 
				   0, 
				   gl_lms.internal_format,
				   LM_BLOCK_WIDTH, LM_BLOCK_HEIGHT, 
				   0, 
				   gl_lms.format,
				   gl_lms.type,
	//			   GL_LIGHTMAP_FORMAT, 
	//			   GL_UNSIGNED_BYTE, 
				   dummy );
}


/*
=======================
R_EndBuildingLightmaps
=======================
*/
void R_EndBuildingLightmaps (void)
{
	LM_UploadBlock ();
	GL_EnableMultitexture (false);
}
