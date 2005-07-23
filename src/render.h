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


#define CAST_TO_INT(A)  (int)(A)

#define SCALE_TO_BUFFER(A) (A)
#define VALUE_MAX	1.0
typedef float buftype;

#define DIRTY_FEATURE_SIZE     1
#define DIRTY_REGION_PARAMS    2
#define DIRTY_MAPPING          4 
#define DIRTY_OUTPUT_FUNCTION  8 /* frequency ramp, sine, triangle, half sine, reverse... */
#define DIRTY_GAIN_PINCH_BIAS  16
#define DIRTY_BUFFER_TYPE      32
#define DIRTY_COLOR            64
#define DIRTY_BASIS            128
#define DIRTY_WARP             256

#define DIRTY_ALL	       511


#define MODE_RAW            0
#define MODE_COLOR          1 
#define MODE_GRAYSCALE      2


typedef struct RenderDataStr {
	PluginState	*p_state;
	
	double *gradient;

	float *buffer;

	int buf_alloc;
		
	int x_offs, y_offs;
	int buffer_height, buffer_width;
	int region_height, region_width;
	int region_x, region_y;
	int pixel_stride;

	int polar;

	double rad1, rad2;
	double ang1, ang2;
	double dang1, dang2;

	double pinch_coef[5];
	double bias_coef[2];
	double gain; /* this one is normalized */
	double dx, dy;
	double px, py;
	double frequency;
	double shift;

	double plane; /* this is like a "seed" */
	double average;

	double caustic_coef_x;
	double caustic_coef_y;
	
	int function_mode;
	int write_mode;

	guint dirty; 
} RenderData;

/* we hide these, are we're not including the GIMP headers for calibration */

#ifndef CALIBRATE

GimpPixelFetcher *GetPixelFetcher(PluginState *state, GimpDrawable *drawable);

void Render (gint32 image_ID,
		GimpDrawable *drawable,
		PluginState *state);

int RenderChannels(RenderData *rdat);
int RenderWarp(RenderData *rdat, int overscan);
int RenderLow(RenderData *rdat, int plane);
void Blend(RenderData *rdat, guchar *bg, guchar *dest, int row_stride, int bytes_pp);
void Warp(RenderData *rdat, GimpPixelFetcher *fetcher, guchar *dest, int row_stride, int bytes_pp, int overscan);

void InitRenderData(RenderData *rdat);
void DeinitRenderData(RenderData *rdat);

void AssociateRenderToState(RenderData *rdat, PluginState *state);
void SetRenderStateDirty(RenderData *rdat, guint dirty);

void SetRenderRegion(RenderData *rdat, int width, int height, int region_x, int region_y);

void SetRenderBuffer(RenderData *rdat, int width, int height, int offset_x, int offset_y, int mode, int pixel_stride);
void SetRenderBufferForDrawable(RenderData *rdat, GimpDrawable *drawable);
void SetRenderBufferMode(RenderData *rdat, int mode, int pixel_stride);

#endif
