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

double LNoise5D(double x, double y, double z, double s, double t, guint16 *shuffle_table) {
	double xif, yif, zif, sif, tif;
	int   xi,yi,zi,si,ti;
	double xf,yf,zf,sf,tf;
	double cyf, cxf, czf, csf, ctf;
	double dp[32];
	double vx,vy,vz,vs,vt;
	int h;
	int i;
	int x_idx, y_idx, z_idx, s_idx, t_idx;
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
	
	sif = floor(s);
	si = (int) sif;
	sf = s - sif;
		
	tif = floor(t);
	ti = (int) tif;
	tf = t - tif;
	

	for (i = 0; i < 32; i++) { /* vertices = 2^dimension, edges = 2^(dimension-1) * dimension */
		x_idx =  i & 1;
		y_idx = (i & 2)>>1;
		z_idx = (i & 4)>>2;
		s_idx = (i & 8)>>3;
		t_idx = (i & 16)>>4;

		/* adjust depending on which vertex we're modifying */
		vx = xf - x_idx;
		vy = yf - y_idx;
		vz = zf - z_idx;
		vs = sf - s_idx;
		vt = tf - t_idx;

		/* Calculate the "dot product" */
		h = Hash5(xi+x_idx, yi+y_idx, zi+z_idx, si+s_idx, ti+t_idx);
		/* with the table size of 10 bits, (h>>4) has a range R of 0..63
		 * and R % 5 has the following probabilities of ocurring:
		 * 0 -> 13/64
		 * 1 -> 13/64
		 * 2 -> 13/64
		 * 3 -> 13/64
		 * 4 -> 12/64 <-- this one is assymetric, it goes, then, into the last parameter, t (the phase)
		 * */
		switch ((h>>4) % 5) { 
			case 0: dp[i] = ((h&1)?vx:-vx) + ((h&2)?vy:-vy) + ((h&4)?vz:-vz) + ((h&8)?vt:-vt); break;
			case 1: dp[i] = ((h&1)?vx:-vx) + ((h&2)?vy:-vy) + ((h&4)?vs:-vs) + ((h&8)?vt:-vt); break;
			case 2: dp[i] = ((h&1)?vx:-vx) + ((h&2)?vs:-vs) + ((h&4)?vz:-vz) + ((h&8)?vt:-vt); break;
			case 3: dp[i] = ((h&1)?vs:-vs) + ((h&2)?vy:-vy) + ((h&4)?vz:-vz) + ((h&8)?vt:-vt); break;
				
			case 4: dp[i] = ((h&1)?vx:-vx) + ((h&2)?vy:-vy) + ((h&4)?vz:-vz) + ((h&8)?vs:-vs); break;
		}
	}

	cxf = Curve(xf);
	cyf = Curve(yf);
	czf = Curve(zf);
	csf = Curve(sf);
	ctf = Curve(tf);

	for (i = 0; i < 32; i+=2) {
		dp[i] = Lerp(cxf,dp[i],dp[i+1]);
	}

	for (i = 0; i < 32; i+=4) {
		dp[i] = Lerp(cyf,dp[i],dp[i+2]);
	}

	for (i = 0; i < 32; i+=8) {
		dp[i] = Lerp(czf,dp[i],dp[i+4]);
	}
	dp[0]  = Lerp(csf,dp[0],dp[8]);
	dp[16] = Lerp(csf,dp[16],dp[24]);
	
	return Lerp(ctf,dp[0],dp[16]);
}

