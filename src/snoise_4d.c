/*  Felimage Noise Plugin for the GIMP
 *  Copyright (C) 2005 Guillermo Romero Franco <drirr_gato@users.sourceforge.net>
 *
 *  This file is part of the Felimage Noise Plugin for the GIMP
 *  
 *  Felimage Noise Plugin for the Gimp is free software; 
 *  you can redistribute it and/or modify it under the terms of 
 *  the GNU General Public License as published by the Free Software 
 *  Foundation; either version 2 of the License, or (at your option) 
 *  any later version.
 *
 *  Felimage Noise Plugin for the Gimp is distributed in the hope 
 *  that it will be useful, but WITHOUT ANY WARRANTY; without even 
 *  the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 *  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with fimg-noise; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
*/

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


#ifdef CALIBRATE
#include <glib.h>
#include <string.h>
#else
#include <libgimp/gimp.h>
#endif

#include "poisson.h"
#include "snoise_int.h"
#include "snoise.h"
#include "random.h"


SNoiseBasisCache4D *InitSNoiseBasis4D() {
	SNoiseBasisCache4D *cache;


	cache = g_malloc(sizeof(SNoiseBasisCache4D) * (3*3*3*3)); 
	memset(cache,0,sizeof(SNoiseBasisCache4D) * (3*3*3*3));

	
	return cache;
}

void FinishSNoiseBasis4D(SNoiseBasisCache4D *cache) {
	g_free(cache);
}

double SNoise4D(double a0, double a1, double a2, double a3, SNoiseBasisCache4D *cache) {
	int a[4];
	guint32 seed[4];
	int count,j;
	double d[4];
	double f[4];
	gint32 int_at[4];
	double dist;
	double r;
	double fa[4];

	
	int_at[0]=(a0<0.0) ? (gint32)a0-1 : (gint32)a0; 
	int_at[1]=(a1<0.0) ? (gint32)a1-1 : (gint32)a1; 
	int_at[2]=(a2<0.0) ? (gint32)a2-1 : (gint32)a2; 
	int_at[3]=(a3<0.0) ? (gint32)a3-1 : (gint32)a3; 

	r = 0;
	for (a[3] = -1; a[3] <=1; a[3]++) {
		seed[3] = Hash1(a[3] + int_at[3]);
		fa[3] = a[3]+int_at[3] - a3;
		for (a[2] = -1; a[2] <=1; a[2]++) {
			seed[2] = Hash1(seed[3] + a[2] + int_at[2]);
			fa[2] = a[2]+int_at[2] - a2;
			for (a[1] = -1; a[1] <=1; a[1]++) {
				seed[1] = Hash1(seed[2] + a[1] + int_at[1]);
				fa[1] = a[1]+int_at[1] - a1;
				for (a[0] = -1; a[0] <=1; a[0]++) {
					seed[0] = Hash1(seed[1] + a[0] + int_at[0]);
					fa[0] = a[0]+int_at[0] - a0;

					count = Poisson_count[seed[0]&255];	

					if ( 1 || (seed[0] != cache->last_seed) || (seed[0] == 0)) {
						cache->last_seed = seed[0];	

						for (j=0; j<count; j++) {
							
							seed[0]=RANDOM(seed[0]);
							f[0] = (seed[0]+0.5)*(1.0/4294967296.0); 
							
							seed[0]=RANDOM(seed[0]);
							f[1] = (seed[0]+0.5)*(1.0/4294967296.0);
							
							seed[0]=RANDOM(seed[0]);
							f[2] =(seed[0]+0.5)*(1.0/4294967296.0);

							seed[0]=RANDOM(seed[0]);
							f[3] =(seed[0]+0.5)*(1.0/4294967296.0);

							d[0] = (cache->p[0][j] = f[0])+fa[0]; 
							d[1] = (cache->p[1][j] = f[1])+fa[1];
							d[2] = (cache->p[2][j] = f[2])+fa[2];
							d[3] = (cache->p[3][j] = f[3])+fa[3];

							dist = d[0]*d[0] + d[1]*d[1] + d[2]*d[2] + d[3]*d[3];

							if (dist<1.00) {
								r += KERNEL(dist);
							}
						}
					} else {
						for (j=0; j<count; j++) {
							d[0] = cache->p[0][j]+fa[0];
							d[1] = cache->p[1][j]+fa[1];
							d[2] = cache->p[2][j]+fa[2];
							d[3] = cache->p[3][j]+fa[3];
							
							dist = d[0]*d[0] + d[1]*d[1] + d[2]*d[2] + d[3]*d[3];

							if (dist<1.00) {
								r += KERNEL(dist);
							}
						}
					}	
					
					cache++;
				}
			}
		}
	}

	return r;
}

