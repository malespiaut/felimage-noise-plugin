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


#ifdef CALIBRATE
#include <glib.h>
#include <string.h>
#include <math.h>
#else
#include <libgimp/gimp.h>
#endif


#include "lnoise_int.h"
#include "random.h"

double LNoise4D(double x, double y, double z, double t, guint16 *shuffle_table) {
	double xif, yif, zif, tif;
	int   xi,yi,zi,ti;
	double xf,yf,zf,tf;
	double cyf, cxf, czf, ctf;
	double dp[16];
	double v1,v2,v3,v4,v5,v6,v7,v8;
	double vx,vy,vz,vt;
	int i;
	int x_idx, y_idx, z_idx, t_idx;
	double tmp;

	/* Get the integer and fractional part of the coordinates */
	xif= floor(x);
	xi = (int) xif;
	xf = x - xif;
	
	yif = floor(y);
	yi = (int) yif;
	yf = y - yif;
	
	zif = floor(z);
	zi = (int) zif;
	zf = z - zif;
	
	tif = floor(t);
	ti = (int) tif;
	tf = t - tif;
	
	for (i = 0; i < 16; i++) {
		x_idx =  i & 1;
		y_idx = (i & 2)>>1;
		z_idx = (i & 4)>>2;
		t_idx = (i & 8)>>3;

		/* adjust depending on which vertex we're modifying */
		vx = xf - x_idx;
		vy = yf - y_idx;
		vz = zf - z_idx;
		vt = tf - t_idx;

		/* Calculate the "dot product" */
		switch (Hash4(xi+x_idx,yi+y_idx,zi+z_idx,ti+t_idx) & 31) {
			
			case 0 : dp[i] =  vx +  vy +  vz;  break;
			case 1 : dp[i] =  vx + -vy +  vz;  break;
			case 2 : dp[i] = -vx +  vy +  vz;  break;
			case 3 : dp[i] = -vx + -vy +  vz;  break;
				
			case 4 : dp[i] =  vx +  vy + -vz;  break;
			case 5 : dp[i] =  vx + -vy + -vz;  break;
			case 6 : dp[i] = -vx +  vy + -vz;  break;
			case 7 : dp[i] = -vx + -vy + -vz;  break;

			case 8 : dp[i] =  vx +  vy +  vt;  break;
			case 9 : dp[i] =  vx + -vy +  vt;  break;
			case 10: dp[i] = -vx +  vy +  vt;  break;
			case 11: dp[i] = -vx + -vy +  vt;  break;
				
			case 12: dp[i] =  vx +  vy + -vt;  break;
			case 13: dp[i] =  vx + -vy + -vt;  break;
			case 14: dp[i] = -vx +  vy + -vt;  break;
			case 15: dp[i] = -vx + -vy + -vt;  break;

			case 16: dp[i] =  vx +  vz +  vt;  break;
			case 17: dp[i] =  vx + -vz +  vt;  break;
			case 18: dp[i] = -vx +  vz +  vt;  break;
			case 19: dp[i] = -vx + -vz +  vt;  break;
				
			case 20: dp[i] =  vx +  vz + -vt;  break;
			case 21: dp[i] =  vx + -vz + -vt;  break;
			case 22: dp[i] = -vx +  vz + -vt;  break;
			case 23: dp[i] = -vx + -vz + -vt;  break;

			case 24: dp[i] =  vy +  vz +  vt;  break;
			case 25: dp[i] =  vy + -vz +  vt;  break;
			case 26: dp[i] = -vy +  vz +  vt;  break;
			case 27: dp[i] = -vy + -vz +  vt;  break;
				
			case 28: dp[i] =  vy +  vz + -vt;  break;
			case 29: dp[i] =  vy + -vz + -vt;  break;
			case 30: dp[i] = -vy +  vz + -vt;  break;
			case 31: dp[i] = -vy + -vz + -vt;  break;
			default: break;
		}
	}

	cxf = Curve(xf);
	cyf = Curve(yf);
	czf = Curve(zf);
	ctf = Curve(tf);

	v1 = Lerp(cxf,dp[0],dp[1]);
	v2 = Lerp(cxf,dp[2],dp[3]);
	v3 = Lerp(cxf,dp[4],dp[5]);
	v4 = Lerp(cxf,dp[6],dp[7]);

	v5 = Lerp(cxf,dp[8],dp[9]);
	v6 = Lerp(cxf,dp[10],dp[11]);
	v7 = Lerp(cxf,dp[12],dp[13]);
	v8 = Lerp(cxf,dp[14],dp[15]);
	
	v1 = Lerp(cyf, v1, v2);
	v3 = Lerp(cyf, v3, v4);
	v5 = Lerp(cyf, v5, v6);
	v7 = Lerp(cyf, v7, v8);
	
	v1 = Lerp(czf, v1, v3);
	v5 = Lerp(czf, v5, v7);

	return Lerp(ctf,v1,v5);
}

