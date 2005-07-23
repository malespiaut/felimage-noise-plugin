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

double LNoise3D(double x, double y, double z, guint16 *shuffle_table) {
	double xif,yif, zif;
	int   xi,yi,zi;
	double xf,yf,zf;
	double cyf, cxf, czf;
	double dp[8];
	double v1,v2,v3,v4;
	double vx,vy,vz;
	int i;
	int x_idx, y_idx, z_idx;
	double tmp;

	/* Get the integer and fractional part of the coordinates */
	xif= floor(x);
	xi = (int)xif;
	xf = x - xif;
	
	yif = floor(y);
	yi = (int) yif;
	yf = y - yif;
	
	zif = floor(z);
	zi = (int) zif;
	zf = z - zif;
	
	for (i = 0; i < 8; i++) {
		x_idx = i & 1;
		y_idx = (i & 2)>>1;
		z_idx = (i & 4)>>2;

		/* adjust depending on which vertex we're modifying */
		vx = xf - x_idx;
		vy = yf - y_idx;
		vz = zf - z_idx;

		/* Calculate the "dot product" */
		switch (Hash3(xi+x_idx,yi+y_idx,zi+z_idx) & 15) {
			
			case 0 : dp[i] =  vx +  vy;  break;
			case 1 : dp[i] =  vx + -vy;  break;
			case 2 : dp[i] = -vx +  vy;  break;
			case 3 : dp[i] = -vx + -vy;  break;
				
			case 4 : dp[i] =  vx +  vz;  break;
			case 5 : dp[i] =  vx + -vz;  break;
			case 6 : dp[i] = -vx +  vz;  break;
			case 7 : dp[i] = -vx + -vz;  break;

			case 8 : dp[i] =  vy +  vz;  break;
			case 9 : dp[i] =  vy + -vz;  break;
			case 10: dp[i] = -vy +  vz;  break;
			case 11: dp[i] = -vy + -vz;  break;

			case 12: dp[i] =  vx +  vy;  break;
			case 13: dp[i] = -vx +  vy;  break;
			case 14: dp[i] = -vy +  vz;  break;
			case 15: dp[i] = -vy + -vz;  break;

			default: break;
		}
	}

	cxf = Curve(xf);
	cyf = Curve(yf);
	czf = Curve(zf);

	v1 = Lerp(cxf,dp[0],dp[1]);
	v2 = Lerp(cxf,dp[2],dp[3]);
	v3 = Lerp(cxf,dp[4],dp[5]);
	v4 = Lerp(cxf,dp[6],dp[7]);

	v1 = Lerp(cyf, v1, v2);
	v3 = Lerp(cyf, v3, v4);

	return Lerp(czf,v1,v3);
	
}

