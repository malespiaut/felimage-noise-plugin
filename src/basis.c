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

#include "config.h"

#ifdef CALIBRATE
#include <glib.h>
#include <stdio.h>
#include <math.h>
#else
#include <libgimp/gimp.h>
#endif

#include "main.h"

#include "poisson.h"
#include "cell.h"
#include "snoise.h"
#include "lnoise.h"
#include "basis.h"

#include "random.h"

#include "render.h"

static int octaves;
static double lacunarity;
static double oct_frac;
static double *weight=NULL;
static double exponent;

typedef void *init_fn_type();
typedef void  deinit_fn_type(void *);

typedef struct {
	init_fn_type *init_fn;
	deinit_fn_type *deinit_fn;
	basis_fn_type *sample_fn;
#ifdef CALIBRATE
	const char *name;
#endif
} basis_struct;


static int data_type = 0;
static void *data = NULL;

#ifdef CALIBRATE
FILE *cal_file;
double st_sum,st_min,st_max;
#endif


/**********************/

#include "calibration.h"

#ifndef CALIBRATE
#define NO_CAL(A) A
#define CAL(A) 
#else
#define NO_CAL(A) 
#define CAL(A) A
#endif


/**********************/

/* Basis functions are supposed to return values in the range [-0.5 .. 0.5] */

#define BASE3D(NAME,XTRA_VARS,VALUE_CALC,RETURN)\
	static double NAME(double x, double y, double z) {\
		int i;\
		double value;\
		double shift;\
		XTRA_VARS\
		\
		shift = 0;\
		for (i = 0; i < octaves; i++) {\
			VALUE_CALC;\
			x *= lacunarity;\
			y *= lacunarity;\
			z *= lacunarity;\
			shift += 37.687322;\
		}\
		return RETURN;\
	}


#define BASE4D(NAME,XTRA_VARS,VALUE_CALC,RETURN)\
	static double NAME(double x, double y, double z, double t) {\
		int i;\
		double value;\
		double shift;\
		XTRA_VARS\
		\
		shift = 0;\
		for (i = 0; i < octaves; i++) {\
			VALUE_CALC;\
			x *= lacunarity;\
			y *= lacunarity;\
			z *= lacunarity;\
			t *= lacunarity;\
			shift += 37.687322;\
		}\
		return RETURN;\
	}

#define BASE5D(NAME,XTRA_VARS,VALUE_CALC,RETURN)\
	static double NAME(double x, double y, double z, double s, double t) {\
		int i;\
		double value;\
		double shift;\
		XTRA_VARS\
		\
		shift = 0;\
		for (i = 0; i < octaves; i++) {\
			VALUE_CALC;\
			x *= lacunarity;\
			y *= lacunarity;\
			z *= lacunarity;\
			s *= lacunarity;\
			t *= lacunarity;\
			shift += 37.687322;\
		}\
		return RETURN;\
	}


#ifdef CALIBRATE
#define	OUTPUT(VALUE,MID_VALUE,SCALING) (st_min = st_min>VALUE?VALUE:st_min, st_max = st_max<VALUE?VALUE:st_max, st_sum += VALUE, (VALUE - MID_VALUE) * SCALING) 
#else
#define OUTPUT(VALUE,MID_VALUE,SCALING) (VALUE - MID_VALUE) * SCALING
#endif

/****************/

#define FUNC3D(NAME,XTRA_VARS, VALUE_CALC, CALC_FBM, CALC_MF1, CALC_MF2, MID_VALUE, SCALING) \
	BASE3D(NAME##_FBM, XTRA_VARS; value = 0;, VALUE_CALC; CALC_FBM;, OUTPUT(value,MID_VALUE,SCALING))\
	BASE3D(NAME##_MF1, XTRA_VARS; value = 1; , VALUE_CALC; CALC_MF1;,  pow( value,exponent)-0.5 )\
	BASE3D(NAME##_MF2, XTRA_VARS; value = 1; , VALUE_CALC; CALC_MF2;,  -(pow(value,exponent)-0.5) )

#define TURB3D(NAME,XTRA_VARS, VALUE_CALC, CALC_FBM, CALC_MF1, CALC_MF2, MID_VALUE, SCALING) \
	BASE3D(NAME##_FBM,  /* name */ \
		double tmp; /* extra vars */\
		XTRA_VARS; value = 0;, \
		VALUE_CALC; /* value calculation */\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_FBM;,\
		value * (SCALING * 2.0) - 0.5 /* final scaling */\
	)\
	BASE3D(NAME##_MF1, \
		double tmp;\
		XTRA_VARS; value = 1;, \
		VALUE_CALC;\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_MF1;,\
		pow(value,exponent)-0.5\
	)\
	BASE3D(NAME##_MF2, \
		double tmp;\
		XTRA_VARS; value = 1;, \
		VALUE_CALC;\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_MF2;,\
		-(pow(value,exponent)-0.5)\
	)

#define FUNC4D(NAME,XTRA_VARS, VALUE_CALC, CALC_FBM, CALC_MF1, CALC_MF2, MID_VALUE, SCALING) \
	BASE4D(NAME##_FBM, XTRA_VARS; value = 0;, VALUE_CALC; CALC_FBM;, OUTPUT(value,MID_VALUE,SCALING) )\
	BASE4D(NAME##_MF1, XTRA_VARS; value = 1;, VALUE_CALC; CALC_MF1;, pow( value,exponent)-0.5 )\
	BASE4D(NAME##_MF2, XTRA_VARS; value = 1;, VALUE_CALC; CALC_MF2;, -(pow(value,exponent)-0.5) )

#define TURB4D(NAME,XTRA_VARS, VALUE_CALC, CALC_FBM, CALC_MF1, CALC_MF2, MID_VALUE, SCALING) \
	BASE4D(NAME##_FBM,  /* name */ \
		double tmp; /* extra vars */\
		XTRA_VARS;   \
		value = 0;, \
		VALUE_CALC; /* value calculation */\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_FBM;,\
		value * (SCALING * 2.0) - 0.5 /* final scaling */\
	)\
	BASE4D(NAME##_MF1, \
		double tmp;\
		XTRA_VARS; value = 1;, \
		VALUE_CALC;\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_MF1;,\
		pow(value,exponent) - 0.5\
	)\
	BASE4D(NAME##_MF2, \
		double tmp;\
		XTRA_VARS; value = 1;, \
		VALUE_CALC;\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_MF2;,\
		-(pow(value,exponent) - 0.5)\
	)


#define FUNC5D(NAME,XTRA_VARS, VALUE_CALC, CALC_FBM, CALC_MF1, CALC_MF2, MID_VALUE, SCALING) \
	BASE5D(NAME##_FBM, XTRA_VARS; value = 0;, VALUE_CALC; CALC_FBM;, OUTPUT(value,MID_VALUE,SCALING) )\
	BASE5D(NAME##_MF1, XTRA_VARS; value = 1;, VALUE_CALC; CALC_MF1;, pow( value,exponent)-0.5 )\
	BASE5D(NAME##_MF2, XTRA_VARS; value = 1;, VALUE_CALC; CALC_MF2;, -(pow(value,exponent)-0.5) )

#define TURB5D(NAME,XTRA_VARS, VALUE_CALC, CALC_FBM, CALC_MF1, CALC_MF2, MID_VALUE, SCALING) \
	BASE5D(NAME##_FBM,  /* name */ \
		double tmp; /* extra vars */\
		XTRA_VARS;   \
		value = 0;, \
		VALUE_CALC; /* value calculation */\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_FBM;,\
		value * (SCALING * 2.0) - 0.5 /* final scaling */\
	)\
	BASE5D(NAME##_MF1, \
		double tmp;\
		XTRA_VARS; value = 1;, \
		VALUE_CALC;\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_MF1;,\
		pow(value,exponent) - 0.5\
	)\
	BASE5D(NAME##_MF2, \
		double tmp;\
		XTRA_VARS; value = 1;, \
		VALUE_CALC;\
			tmp -= MID_VALUE;\
			if (tmp < 0) tmp = - tmp;\
			CALC_MF2;,\
		-(pow(value,exponent) - 0.5)\
	)

/* debug-only functions */
/*
#define FUNC3D_PV(NAME,XTRA_VARS, VALUE_CALC, MID_VALUE, SCALING) \
	BASE3D(NAME, XTRA_VARS, VALUE_CALC, (g_printf("%lf\n",value), (value - MID_VALUE) * SCALING ))

#define FUNC4D_PV(NAME,XTRA_VARS, VALUE_CALC, MID_VALUE, SCALING) \
	BASE4D(NAME, XTRA_VARS, VALUE_CALC, (g_printf("%lf\n",value), (value - MID_VALUE) * SCALING ))

#define FUNC5D_PV(NAME,XTRA_VARS, VALUE_CALC, MID_VALUE, SCALING) \
	BASE5D(NAME, XTRA_VARS, VALUE_CALC, (g_printf("%lf\n",value), (value - MID_VALUE) * SCALING ))
*/
/******************/

#define PARAM_3D x+shift, y+shift, z+shift
#define PARAM_4D x+shift, y+shift, z+shift, t+shift
#define PARAM_5D x+shift, y+shift, z+shift, s+shift, t+shift


#define MULTI_MIX_1(VALUE, WEIGHT, MID, SCALE) ( 1.0 + (((VALUE)-(MID))*(SCALE) - 0.5)*WEIGHT)
#define MULTI_MIX_2(VALUE, WEIGHT, MID, SCALE) ( 1.0 + (((VALUE)-(MID))*-(SCALE) - 0.5)*WEIGHT)
#define TURB_MIX_1(VALUE, WEIGHT, MID, SCALE)  ( 1.0 + ( (VALUE)*(2.0 * (SCALE) ) - 1.0)*WEIGHT)
#define TURB_MIX_2(VALUE, WEIGHT, MID, SCALE)  ( 1.0 + (-(VALUE)*(2.0 * (SCALE) ))*WEIGHT)

/******************/
		
/****** Sparse noise *******/

FUNC3D(SparseNoise3D,/* no extra vars */, /* No common calculations */,
       value += SNoise3D(PARAM_3D, (SNoiseBasisCache3D *) data ) * weight[i], /* Fractal brownian motion */
       value *= MULTI_MIX_1( SNoise3D(PARAM_3D, (SNoiseBasisCache3D *) data ), weight[i], SN_3D_MID, SN_3D_FAC) ,
       value *= MULTI_MIX_2( SNoise3D(PARAM_3D, (SNoiseBasisCache3D *) data ), weight[i], SN_3D_MID, SN_3D_FAC) ,
       SN_3D_MID, SN_3D_FAC)
	
TURB3D(SparseTurb3D_1,/* no extra vars */,
       tmp = SNoise3D(PARAM_3D, (SNoiseBasisCache3D *) data ),
       value += tmp * weight[i];,
       value *= TURB_MIX_1( tmp, weight[i], SN_3D_MID, SN_3D_FAC) ,
       value *= TURB_MIX_2( tmp, weight[i], SN_3D_MID, SN_3D_FAC) ,
       SN_3D_MID, SN_3D_FAC)

/**/

FUNC4D(SparseNoise4D,/* no extra vars */, /* No common calculations */,
       value += SNoise4D(PARAM_4D, (SNoiseBasisCache4D *) data ) * weight[i],
       value *= MULTI_MIX_1( SNoise4D(PARAM_4D, (SNoiseBasisCache4D *) data ), weight[i], SN_4D_MID, SN_4D_FAC) ,
       value *= MULTI_MIX_2( SNoise4D(PARAM_4D, (SNoiseBasisCache4D *) data ), weight[i], SN_4D_MID, SN_4D_FAC) ,
       SN_4D_MID, SN_4D_FAC)
	
TURB4D(SparseTurb4D_1,/* no extra vars */,
       tmp = SNoise4D(PARAM_4D, (SNoiseBasisCache4D *) data ),
       value += tmp * weight[i];,
       value *= TURB_MIX_1( tmp, weight[i], SN_4D_MID, SN_4D_FAC) ,
       value *= TURB_MIX_2( tmp, weight[i], SN_4D_MID, SN_4D_FAC) ,
       SN_4D_MID, SN_4D_FAC)

/**/

FUNC5D(SparseNoise5D,/* no extra vars */, /* No common calculations */,
       value += SNoise5D(PARAM_5D, (SNoiseBasisCache5D *) data ) * weight[i],
       value *= MULTI_MIX_1( SNoise5D(PARAM_5D, (SNoiseBasisCache5D *) data ), weight[i], SN_5D_MID, SN_5D_FAC) ,
       value *= MULTI_MIX_2( SNoise5D(PARAM_5D, (SNoiseBasisCache5D *) data ), weight[i], SN_5D_MID, SN_5D_FAC) ,
       SN_5D_MID, SN_5D_FAC)
	
TURB5D(SparseTurb5D_1,/* no extra vars */,
       tmp = SNoise5D(PARAM_5D, (SNoiseBasisCache5D *) data ),
       value += tmp * weight[i];,
       value *= TURB_MIX_1( tmp, weight[i], SN_5D_MID, SN_5D_FAC) ,
       value *= TURB_MIX_2( tmp, weight[i], SN_5D_MID, SN_5D_FAC) ,
       SN_5D_MID, SN_5D_FAC)

/****** Lattice noise *******/


FUNC3D(LatticeNoise3D,/* no extra vars */, /* No common calculations */,
       value += LNoise3D(PARAM_3D, (guint16 *) data ) * weight[i],
       value *= MULTI_MIX_1( LNoise3D(PARAM_3D, (guint16 *) data ), weight[i], LN_3D_MID,  LN_3D_FAC),
       value *= MULTI_MIX_2( LNoise3D(PARAM_3D, (guint16 *) data ), weight[i], LN_3D_MID,  LN_3D_FAC),
       LN_3D_MID, LN_3D_FAC)
	
TURB3D(LatticeTurb3D_1,/* no extra vars */,
       tmp = LNoise3D(PARAM_3D, (guint16 *) data ),
       value += tmp * weight[i];,
       value *= TURB_MIX_1( tmp, weight[i], LN_3D_MID, LN_3D_FAC),
       value *= TURB_MIX_2( tmp, weight[i], LN_3D_MID, LN_3D_FAC),
       LN_3D_MID, LN_3D_FAC)

/**/

FUNC4D(LatticeNoise4D,/* no extra vars */, /* No common calculations */,
       value += LNoise4D(PARAM_4D, (guint16 *) data ) * weight[i],
       value *= MULTI_MIX_1( LNoise4D(PARAM_4D, (guint16 *) data ), weight[i], LN_4D_MID,  LN_4D_FAC),
       value *= MULTI_MIX_2( LNoise4D(PARAM_4D, (guint16 *) data ), weight[i], LN_4D_MID,  LN_4D_FAC),
       LN_4D_MID, LN_4D_FAC)
	
TURB4D(LatticeTurb4D_1,/* no extra vars */,
       tmp = LNoise4D(PARAM_4D, (guint16 *) data ),
       value += tmp * weight[i];,
       value *= TURB_MIX_1( tmp, weight[i], LN_4D_MID, LN_4D_FAC),
       value *= TURB_MIX_2( tmp, weight[i], LN_4D_MID, LN_4D_FAC),
       LN_4D_MID, LN_4D_FAC)

/**/

FUNC5D(LatticeNoise5D,/* no extra vars */, /* No common calculations */,
       value += LNoise5D(PARAM_5D, (guint16 *) data ) * weight[i],
       value *= MULTI_MIX_1( LNoise5D(PARAM_5D, (guint16 *) data ), weight[i], LN_5D_MID,  LN_5D_FAC),
       value *= MULTI_MIX_2( LNoise5D(PARAM_5D, (guint16 *) data ), weight[i], LN_5D_MID,  LN_5D_FAC),
       LN_5D_MID, LN_5D_FAC)
	
TURB5D(LatticeTurb5D_1,/* no extra vars */,
       tmp = LNoise5D(PARAM_5D, (guint16 *) data ),
       value += tmp * weight[i];,
       value *= MULTI_MIX_1( tmp, weight[i], LN_5D_MID, LN_5D_FAC),
       value *= MULTI_MIX_2( tmp, weight[i], LN_5D_MID, LN_5D_FAC),
       LN_5D_MID, LN_5D_FAC)

/****** CELL 1 (Skin) *******/


FUNC3D(Cell3D_1, 
       double f[3]; double delta[3][3]; int id[3];,  /* extra vars */
       Cells3D(PARAM_3D, 2, f, delta, id, (CellBasisCache3D *) data ), /* common calculation */
       value += (f[1] - f[0]) * weight[i],
       value *= MULTI_MIX_1(f[1] - f[0], weight[i], CELL1_3D_MID, CELL1_3D_FAC),
       value *= MULTI_MIX_2(f[1] - f[0], weight[i], CELL1_3D_MID, CELL1_3D_FAC),
       CELL1_3D_MID, CELL1_3D_FAC)


/**/
FUNC4D(Cell4D_1, 
       double f[3]; double delta[3][4]; int id[3];,  /* extra vars */
       Cells4D(PARAM_4D, 2, f, delta, id, (CellBasisCache4D *) data ), /* common calculation */
       value += (f[1] - f[0]) * weight[i],
       value *= MULTI_MIX_1(f[1] - f[0], weight[i], CELL1_4D_MID, CELL1_4D_FAC),
       value *= MULTI_MIX_2(f[1] - f[0], weight[i], CELL1_4D_MID, CELL1_4D_FAC),
       CELL1_4D_MID, CELL1_4D_FAC)

/**/
FUNC5D(Cell5D_1, 
       double f[3]; double delta[3][5]; int id[3];,  /* extra vars */
       Cells5D(PARAM_5D, 2, f, delta, id, (CellBasisCache5D *) data ), /* common calculation */
       value += (f[1] - f[0]) * weight[i],
       value *= MULTI_MIX_1(f[1] - f[0], weight[i], CELL1_5D_MID, CELL1_5D_FAC),
       value *= MULTI_MIX_2(f[1] - f[0], weight[i], CELL1_5D_MID, CELL1_5D_FAC),
       CELL1_5D_MID, CELL1_5D_FAC)

/****** CELL 2 (Puffy) *******/

/**/
FUNC3D(Cell3D_2, 
       double f[3]; double delta[3][3]; int id[3];,  /* extra vars */
       Cells3D(PARAM_3D, 2, f, delta, id, (CellBasisCache3D *) data ), /* common calculation */
       value += f[0] * weight[i],
       value *= MULTI_MIX_1(f[0], weight[i], CELL2_3D_MID, CELL2_3D_FAC),
       value *= MULTI_MIX_2(f[0], weight[i], CELL2_3D_MID, CELL2_3D_FAC),
       CELL2_3D_MID, CELL2_3D_FAC)

/**/
FUNC4D(Cell4D_2, 
       double f[3]; double delta[3][4]; int id[3];,  /* extra vars */
       Cells4D(PARAM_4D, 2, f, delta, id, (CellBasisCache4D *) data ), /* common calculation */
       value += f[0] * weight[i],
       value *= MULTI_MIX_1(f[0], weight[i], CELL2_4D_MID, CELL2_4D_FAC),
       value *= MULTI_MIX_2(f[0], weight[i], CELL2_4D_MID, CELL2_4D_FAC),
       CELL2_4D_MID, CELL2_4D_FAC)

/**/
FUNC5D(Cell5D_2, 
       double f[3]; double delta[3][5]; int id[3];,  /* extra vars */
       Cells5D(PARAM_5D, 2, f, delta, id, (CellBasisCache5D *) data ), /* common calculation */
       value += f[0] * weight[i],
       value *= MULTI_MIX_1(f[0], weight[i], CELL2_5D_MID, CELL2_5D_FAC),
       value *= MULTI_MIX_2(f[0], weight[i], CELL2_5D_MID, CELL2_5D_FAC),
       CELL2_5D_MID, CELL2_5D_FAC)

/******  CELL 3 (Fractured) *******/

/**/
FUNC3D(Cell3D_3, 
       double f[3]; double delta[3][3]; int id[3];,  /* extra vars */
       Cells3D(PARAM_3D, 2, f, delta, id, (CellBasisCache3D *) data ), /* common calculation */
       value += f[1] * weight[i],
       value *= MULTI_MIX_1(f[1], weight[i], CELL3_3D_MID, CELL3_3D_FAC),
       value *= MULTI_MIX_2(f[1], weight[i], CELL3_3D_MID, CELL3_3D_FAC),
       CELL3_3D_MID, CELL3_3D_FAC)

/**/
FUNC4D(Cell4D_3, 
       double f[3]; double delta[3][4]; int id[3];,  /* extra vars */
       Cells4D(PARAM_4D, 2, f, delta, id, (CellBasisCache4D *) data ), /* common calculation */
       value += f[1] * weight[i],
       value *= MULTI_MIX_1(f[1], weight[i], CELL3_4D_MID, CELL3_4D_FAC),
       value *= MULTI_MIX_2(f[1], weight[i], CELL3_4D_MID, CELL3_4D_FAC),
       CELL3_4D_MID, CELL3_4D_FAC)

/**/
FUNC5D(Cell5D_3, 
       double f[3]; double delta[3][5]; int id[3];,  /* extra vars */
       Cells5D(PARAM_5D, 2, f, delta, id, (CellBasisCache5D *) data ), /* common calculation */
       value += f[1] * weight[i],
       value *= MULTI_MIX_1(f[1], weight[i], CELL3_5D_MID, CELL3_5D_FAC),
       value *= MULTI_MIX_2(f[1], weight[i], CELL3_5D_MID, CELL3_5D_FAC),
       CELL3_5D_MID, CELL3_5D_FAC)

/****** CELL 4 (Crystals) *******/

/**/
FUNC3D(Cell3D_4, 
       double f[3]; double delta[3][3]; int id[3];,  /* extra vars */
       Cells3D(PARAM_3D, 2, f, delta, id, (CellBasisCache3D *) data ), /* common calculation */
       value += (Hash1(id[0]) * (1.0/(TABLE_SIZE-1))) * weight[i],
       value *= MULTI_MIX_1(Hash1(id[0]) * (1.0/(TABLE_SIZE-1)), weight[i], CELL4_3D_MID, CELL4_3D_FAC),
       value *= MULTI_MIX_2(Hash1(id[0]) * (1.0/(TABLE_SIZE-1)), weight[i], CELL4_3D_MID, CELL4_3D_FAC),
       CELL4_3D_MID, CELL4_3D_FAC)

/**/
FUNC4D(Cell4D_4, 
       double f[3]; double delta[3][4]; int id[3];,  /* extra vars */
       Cells4D(PARAM_4D, 2, f, delta, id, (CellBasisCache4D *) data ), /* common calculation */
       value += (Hash1(id[0]) * (1.0/(TABLE_SIZE-1))) * weight[i],
       value *= MULTI_MIX_1(Hash1(id[0]) * (1.0/(TABLE_SIZE-1)), weight[i], CELL4_4D_MID, CELL4_4D_FAC),
       value *= MULTI_MIX_2(Hash1(id[0]) * (1.0/(TABLE_SIZE-1)), weight[i], CELL4_4D_MID, CELL4_4D_FAC),
       CELL4_4D_MID, CELL4_4D_FAC)

/**/
FUNC5D(Cell5D_4, 
       double f[3]; double delta[3][5]; int id[3];,  /* extra vars */
       Cells5D(PARAM_5D, 2, f, delta, id, (CellBasisCache5D *) data ), /* common calculation */
       value += (Hash1(id[0]) * (1.0/(TABLE_SIZE-1))) * weight[i],
       value *= MULTI_MIX_1(Hash1(id[0]) * (1.0/(TABLE_SIZE-1)), weight[i], CELL4_5D_MID, CELL4_5D_FAC),
       value *= MULTI_MIX_2(Hash1(id[0]) * (1.0/(TABLE_SIZE-1)), weight[i], CELL4_5D_MID, CELL4_5D_FAC),
       CELL4_5D_MID, CELL4_5D_FAC)

/****** CELL 5 (Galvanized) *******/


/**/
FUNC3D(Cell3D_5, 
       double f[3]; double delta[3][3]; int id[3]; double v[3]; double n;,  /* extra vars */
       Cells3D(PARAM_3D, 1, f, delta, id, (CellBasisCache3D *) data );
       v[0] = (Hash1(id[0]) - ((TABLE_SIZE-1)*0.5));
       v[1] = (Hash1(id[0]+1) - ((TABLE_SIZE-1)*0.5));
       v[2] = (Hash1(id[0]+2) - ((TABLE_SIZE-1)*0.5));
       n = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
       /* we'll incorporate the factor here, and avoid a multiply later on */
       if (n < -0.001 || n > 0.001 ) n = CELL5_3D_FAC/n; 
       v[0] = ((delta[0][0] * v[0] + delta[0][1] * v[1] + delta[0][2]*v[2]) * n);		
       NO_CAL(if (v[0] < -0.5) v[0] = -0.5; if (v[0] >  0.5) v[0] = 0.5;), /* common calculation */
       value += v[0] * weight[i],
       value *= MULTI_MIX_1(v[0], weight[i], 0.0 , 1.0),
       value *= MULTI_MIX_2(v[0], weight[i], 0.0 , 1.0),
       0.0 , 1.0)


/**/
FUNC4D(Cell4D_5, 
       double f[3]; double delta[3][4]; int id[3]; double v[4]; double n;,  /* extra vars */
       Cells4D(PARAM_4D, 1, f, delta, id, (CellBasisCache4D *) data );
       v[0] = (Hash1(id[0]) - ((TABLE_SIZE-1)*0.5));
       v[1] = (Hash1(id[0]+1) - ((TABLE_SIZE-1)*0.5));
       v[2] = (Hash1(id[0]+2) - ((TABLE_SIZE-1)*0.5));
       v[3] = (Hash1(id[0]+3) - ((TABLE_SIZE-1)*0.5));
       n = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2] + v[3]*v[3]);
       /* we'll incorporate the factor here, and avoid a multiply later on */
       if (n < -0.001 || n > 0.001 ) n = CELL5_4D_FAC/n; 
       v[0] = ((delta[0][0]*v[0] + delta[0][1]*v[1] + delta[0][2]*v[2] + delta[0][3]*v[3]) * n);		
       NO_CAL(if (v[0] < -0.5) v[0] = -0.5; if (v[0] >  0.5) v[0] = 0.5;), /* common calculation */
       value += v[0] * weight[i],
       value *= MULTI_MIX_1(v[0], weight[i], 0.0 , 1.0),
       value *= MULTI_MIX_2(v[0], weight[i], 0.0 , 1.0),
       0.0 , 1.0)


/**/
FUNC5D(Cell5D_5, 
       double f[3]; double delta[3][5]; int id[3]; double v[5]; double n;,  /* extra vars */
       Cells5D(PARAM_5D, 1, f, delta, id, (CellBasisCache5D *) data );
       v[0] = (Hash1(id[0]) - ((TABLE_SIZE-1)*0.5));
       v[1] = (Hash1(id[0]+1) - ((TABLE_SIZE-1)*0.5));
       v[2] = (Hash1(id[0]+2) - ((TABLE_SIZE-1)*0.5));
       v[3] = (Hash1(id[0]+3) - ((TABLE_SIZE-1)*0.5));
       v[4] = (Hash1(id[0]+4) - ((TABLE_SIZE-1)*0.5));
       n = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2] + v[3]*v[3] + v[4]*v[4]);
       /* we'll incorporate the factor here, and avoid a multiply later on */
       if (n < -0.001 || n > 0.001 ) n = CELL5_5D_FAC/n; 
       v[0] = ((delta[0][0]*v[0] + delta[0][1]*v[1] + delta[0][2]*v[2] + delta[0][3]*v[3] + delta[0][4]*v[4]) * n);		
       NO_CAL(if (v[0] < -0.5) v[0] = -0.5; if (v[0] >  0.5) v[0] = 0.5;), /* common calculation */
       value += v[0] * weight[i],
       value *= MULTI_MIX_1(v[0], weight[i], 0.0 , 1.0),
       value *= MULTI_MIX_2(v[0], weight[i], 0.0 , 1.0),
       0.0 , 1.0)


/**********************/
#ifdef CALIBRATE
#define ITEM(INIT,DEINIT,BASIS,NAME) {(init_fn_type*)INIT, (deinit_fn_type*)DEINIT, (basis_fn_type*)BASIS, NAME }
#else
#define ITEM(INIT,DEINIT,BASIS,NAME) {(init_fn_type*)INIT, (deinit_fn_type*)DEINIT, (basis_fn_type*)BASIS }
#endif

static basis_struct basis[] =
	{
	ITEM( NULL, NULL, LatticeNoise3D_FBM, "LN_3D" ), 
	ITEM( NULL, NULL, LatticeNoise4D_FBM, "LN_4D" ), 
	ITEM( NULL, NULL, LatticeNoise5D_FBM, "LN_5D" ), 

	ITEM( NULL, NULL, LatticeNoise3D_MF1, NULL ), 
	ITEM( NULL, NULL, LatticeNoise4D_MF1, NULL ), 
	ITEM( NULL, NULL, LatticeNoise5D_MF1, NULL ), 

	ITEM( NULL, NULL, LatticeNoise3D_MF2, NULL ), 
	ITEM( NULL, NULL, LatticeNoise4D_MF2, NULL ), 
	ITEM( NULL, NULL, LatticeNoise5D_MF2, NULL ), 
	

	ITEM( NULL, NULL, LatticeTurb3D_1_FBM, NULL ),
	ITEM( NULL, NULL, LatticeTurb4D_1_FBM, NULL ),
	ITEM( NULL, NULL, LatticeTurb5D_1_FBM, NULL ),

	ITEM( NULL, NULL, LatticeTurb3D_1_MF1, NULL ),
	ITEM( NULL, NULL, LatticeTurb4D_1_MF1, NULL ),
	ITEM( NULL, NULL, LatticeTurb5D_1_MF1, NULL ),

	ITEM( NULL, NULL, LatticeTurb3D_1_MF2, NULL ),
	ITEM( NULL, NULL, LatticeTurb4D_1_MF2, NULL ),
	ITEM( NULL, NULL, LatticeTurb5D_1_MF2, NULL ),
	

	ITEM( InitSNoiseBasis3D, FinishSNoiseBasis3D, SparseNoise3D_FBM, "SN_3D" ), 
	ITEM( InitSNoiseBasis4D, FinishSNoiseBasis4D, SparseNoise4D_FBM, "SN_4D" ), 
	ITEM( InitSNoiseBasis5D, FinishSNoiseBasis5D, SparseNoise5D_FBM, "SN_5D" ), 
	                                                               
	ITEM( InitSNoiseBasis3D, FinishSNoiseBasis3D, SparseNoise3D_MF1, NULL   ), 
	ITEM( InitSNoiseBasis4D, FinishSNoiseBasis4D, SparseNoise4D_MF1, NULL   ), 
	ITEM( InitSNoiseBasis5D, FinishSNoiseBasis5D, SparseNoise5D_MF1, NULL   ), 
	
	ITEM( InitSNoiseBasis3D, FinishSNoiseBasis3D, SparseNoise3D_MF2, NULL   ), 
	ITEM( InitSNoiseBasis4D, FinishSNoiseBasis4D, SparseNoise4D_MF2, NULL   ), 
	ITEM( InitSNoiseBasis5D, FinishSNoiseBasis5D, SparseNoise5D_MF2, NULL   ), 
	

	ITEM( InitSNoiseBasis3D, FinishSNoiseBasis3D, SparseTurb3D_1_FBM, NULL  ),
	ITEM( InitSNoiseBasis4D, FinishSNoiseBasis4D, SparseTurb4D_1_FBM, NULL  ),
	ITEM( InitSNoiseBasis5D, FinishSNoiseBasis5D, SparseTurb5D_1_FBM, NULL  ),
                                                                        
	ITEM( InitSNoiseBasis3D, FinishSNoiseBasis3D, SparseTurb3D_1_MF1, NULL   ),
	ITEM( InitSNoiseBasis4D, FinishSNoiseBasis4D, SparseTurb4D_1_MF1, NULL   ),
	ITEM( InitSNoiseBasis5D, FinishSNoiseBasis5D, SparseTurb5D_1_MF1, NULL   ),
	
	ITEM( InitSNoiseBasis3D, FinishSNoiseBasis3D, SparseTurb3D_1_MF2, NULL   ),
	ITEM( InitSNoiseBasis4D, FinishSNoiseBasis4D, SparseTurb4D_1_MF2, NULL   ),
	ITEM( InitSNoiseBasis5D, FinishSNoiseBasis5D, SparseTurb5D_1_MF2, NULL   ),


	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_1_FBM, "CELL1_3D"), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_1_FBM, "CELL1_4D"), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_1_FBM, "CELL1_5D"), 
                                                                  
	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_1_MF1, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_1_MF1, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_1_MF1, NULL  ), 

	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_1_MF2, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_1_MF2, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_1_MF2, NULL  ), 
	

	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_2_FBM, "CELL2_3D"), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_2_FBM, "CELL2_4D"), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_2_FBM, "CELL2_5D"), 
                                                                  
	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_2_MF1, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_2_MF1, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_2_MF1, NULL  ), 

	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_2_MF2, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_2_MF2, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_2_MF2, NULL  ), 
	
	
	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_3_FBM, "CELL3_3D"), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_3_FBM, "CELL3_4D"), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_3_FBM, "CELL3_5D"), 
                                                                  
	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_3_MF1, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_3_MF1, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_3_MF1, NULL  ), 

	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_3_MF2, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_3_MF2, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_3_MF2, NULL  ), 
	

	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_4_FBM, "CELL4_3D"), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_4_FBM, "CELL4_4D"), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_4_FBM, "CELL4_5D"), 
                                                                  
	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_4_MF1, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_4_MF1, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_4_MF1, NULL  ), 

	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_4_MF2, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_4_MF2, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_4_MF2, NULL  ), 
	

	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_5_FBM, "CELL5_3D"), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_5_FBM, "CELL5_4D"), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_5_FBM, "CELL5_5D"), 
                                                                  
	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_5_MF1, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_5_MF1, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_5_MF1, NULL  ), 

	ITEM( InitCellBasis3D, FinishCellBasis3D,     Cell3D_5_MF2, NULL  ), 
	ITEM( InitCellBasis4D, FinishCellBasis4D,     Cell4D_5_MF2, NULL  ), 
	ITEM( InitCellBasis5D, FinishCellBasis5D,     Cell5D_5_MF2, NULL  ), 

	ITEM( NULL, NULL, NULL, NULL )
	
	};

static void SwitchBasis(int basis_fn, int dim, int multi, float p_octaves, float p_lacunarity, float p_hurst) {
	int new_data_type;
	double freq;
	double alpha;
	int i;
	static double scaling;

	new_data_type = basis_fn * 9 + (dim-3) + multi*3;

	/* (de)initialize data specific to the basis function */
	if (!data || new_data_type != data_type) {
		if (data && basis[data_type].deinit_fn)  basis[data_type].deinit_fn(data);
		data = NULL;
		if (basis[new_data_type].init_fn) data = basis[new_data_type].init_fn();
	}

	/* octaves, lacunarity, weighting coefficients.. */
	octaves = (int)floor(p_octaves);
	oct_frac = p_octaves - octaves;
	lacunarity = p_lacunarity;
	if (weight) {
		g_free(weight);
	}
	weight = g_malloc(sizeof(double) * (octaves+1));
	
	freq = 1;
	scaling = (multi!=0?1:0);
	alpha = 4.0*p_hurst; 
	for (i = 0; i <= octaves; i++) {
		weight[i] = pow(freq, -alpha);
		if (i == octaves) weight[i] *= oct_frac;
		freq *= lacunarity;
		if (multi) {
			scaling *= 1-0.5*weight[i];
		} else {
			scaling += weight[i];

		}
	}


	/* fbm coefficients must add to 1, multifractal coefficients must each be in the range 0..1 */
	if (multi) {
		exponent = log(0.5) / log(scaling);
	} else {
		for (i = 0; i <= octaves; i++) {
			weight[i] /= scaling;
		}
	}
		
	if (oct_frac > 0.0001) octaves++; 

	data_type = new_data_type;
}

void InitBasis(RenderData *rdat) {
	PluginState *state;
	int dim;


	state = rdat->p_state;
	
	InitShuffleTable(state->seed);

	/* get the index of the basis function to use */
	switch (state->mapping){ 
		case MAP_PLANAR: dim = 3; break;
		case MAP_SPHERICAL: dim = 4; break;
		case MAP_TILED: dim = 5; break;
	}
	
	if (state->ign_phase && dim > 3) dim--;
	
	SwitchBasis(state->basis, dim, state->multifractal, state->octaves, state->lacunarity, state->hurst);
	
}

void DeinitBasis() {
	if (data) {
		basis[data_type].deinit_fn(data);
	}
	FinishShuffleTable();
	if (weight) {
		g_free(weight);
	}
	weight = NULL;
	data = NULL;
}

basis_fn_type *GetBasis() {
	return basis[data_type].sample_fn;
}

#ifdef CALIBRATE

#define SAMPLES 100000 
int main(int argc, char *argv[]) {
	int i,j;
	int dim;
	basis_fn_type *fn;
	double f1,f2,f3,f4,f5;
	double mid,fac;

	InitShuffleTable(23470);

	cal_file = fopen("calibration.h","wt");
	if (!cal_file) {
		printf("Could not open cal_log for writing\n");
		return(-1);
	}

	fprintf(cal_file,"/* This file is automatically generated by 'calibrate'. Do not hand edit! */\n\n");

	for (i = 0; basis[i].sample_fn; i++) {
		if (!basis[i].name) continue;

		dim = (i % 3) + 3;

		printf ("%s\n",basis[i].name);
		SwitchBasis((i / 9), dim, (i / 3) % 3, 1.0, 1.0, 0.0);
			
		fn = GetBasis();
		
		st_sum = 0;
		st_min = 1000000000;
		st_max =-1000000000;
		
		switch (dim) {
			case 3:
				for (j = 0; j < SAMPLES; j++) {
					f1 = RandomDbl()*100;
					f2 = RandomDbl()*100;
					f3 = RandomDbl()*100;
					((basis_3d_fn*)fn)(f1,f2,f3);
				}
			break;
			case 4:
				for (j = 0; j < SAMPLES; j++) {
					f1 = RandomDbl()*100;
					f2 = RandomDbl()*100;
					f3 = RandomDbl()*100;
					f4 = RandomDbl()*100;
					((basis_4d_fn*)fn)(f1,f2,f3,f4);
				}
			break;
			case 5:
				for (j = 0; j < SAMPLES; j++) {
					f1 = RandomDbl()*100;
					f2 = RandomDbl()*100;
					f3 = RandomDbl()*100;
					f4 = RandomDbl()*100;
					f5 = RandomDbl()*100;
					((basis_5d_fn*)fn)(f1,f2,f3,f4,f5);
				}
			break;
		}
		mid = (st_max + st_min) / 2.0;
		fac = 1.0 / (st_max - st_min);
		fprintf(cal_file,"/* Min %lf  Max %lf  Range %lf  Mid %lf  Avg %lf */\n",
				st_min, st_max, st_max-st_min, mid, st_sum / SAMPLES);
		fprintf(cal_file,"#define %s_MID %lf\n",basis[i].name,mid);
		fprintf(cal_file,"#define %s_FAC %lf\n",basis[i].name,fac);
		fprintf(cal_file,"\n");
	}
	fclose(cal_file);

	printf("Done\n");
	return 0;
}

#endif /* CALIBRATE */
