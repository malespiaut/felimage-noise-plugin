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

/* 3D */
typedef struct SNoiseBasisCache3DStr {
	double p[3][MAX_FEATURES]; 
	guint32   last_seed;
} SNoiseBasisCache3D;

double SNoise3D(double a0, double a1, double a2, SNoiseBasisCache3D *cache);

SNoiseBasisCache3D *InitSNoiseBasis3D();
void FinishSNoiseBasis3D(SNoiseBasisCache3D *cache);

/* 4D */
typedef struct SNoiseBasisCache4DStr {
	double p[4][MAX_FEATURES]; 
	guint32   last_seed;
} SNoiseBasisCache4D;

double SNoise4D(double a0, double a1, double a2, double a3, SNoiseBasisCache4D *cache);

SNoiseBasisCache4D *InitSNoiseBasis4D();
void FinishSNoiseBasis4D(SNoiseBasisCache4D *cache);

/* 5D */
typedef struct SNoiseBasisCache5DStr {
	double p[5][MAX_FEATURES]; 
	guint32   last_seed;
} SNoiseBasisCache5D;

double SNoise5D(double a0, double a1, double a2, double a3, double a4, SNoiseBasisCache5D *cache);

SNoiseBasisCache5D *InitSNoiseBasis5D();
void FinishSNoiseBasis5D(SNoiseBasisCache5D *cache);

