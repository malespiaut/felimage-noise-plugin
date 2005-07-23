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
typedef struct CellBasisCache3DStr {
	double p[3][MAX_FEATURES]; 
	guint32    id[MAX_FEATURES];
	guint32   last_seed;
} CellBasisCache3D;

void Cells3D(double a0, double a1, double a2, gint32 max_order, double *f, double (*p_delta)[3], guint32 *p_id, CellBasisCache3D *cache);

CellBasisCache3D *InitCellBasis3D();
void FinishCellBasis3D(CellBasisCache3D *cache);

/* 4D */
typedef struct CellBasisCache4DStr {
	double p[4][MAX_FEATURES]; 
	guint32    id[MAX_FEATURES];
	guint32   last_seed;
} CellBasisCache4D;

void Cells4D(double a0, double a1, double a2, double a3, gint32 max_order, double *f, double (*p_delta)[4], guint32 *p_id, CellBasisCache4D *cache);

CellBasisCache4D *InitCellBasis4D();
void FinishCellBasis4D(CellBasisCache4D *cache);

/* 5D */
typedef struct CellBasisCache5DStr {
	double p[5][MAX_FEATURES]; 
	guint32    id[MAX_FEATURES];
	guint32   last_seed;
} CellBasisCache5D;

void Cells5D(double a0, double a1, double a2, double a3, double a4, gint32 max_order, double *f, double (*p_delta)[5], guint32 *p_id, CellBasisCache5D *cache);

CellBasisCache5D *InitCellBasis5D();
void FinishCellBasis5D(CellBasisCache5D *cache);

