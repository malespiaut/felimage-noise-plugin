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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "main.h"

#include "basis.h"

#include "render.h"


#define GRADIENT_SAMPLES 512

#define EPSILON (1.0/1024.0)
						  
#define REVERSE_NO 0
#define REVERSE_YES 1

/* this is how much non-linear the caustics adjustment is:
 * 0.5 -> linear (just like the gain function) 
 */
#define WARP_GAIN 0.8

#define FN_MODE(FUNCTION,REVERSE) ((FUNCTION<<1)+REVERSE)


GimpPixelFetcher *GetPixelFetcher(PluginState *state, GimpDrawable *drawable) {
	GimpPixelFetcher *fetcher;
	GimpRGB bg_color;
	
	fetcher = gimp_pixel_fetcher_new(drawable, FALSE);
	
	if (state->edge_action == GIMP_PIXEL_FETCHER_EDGE_BACKGROUND) {
		gimp_context_get_background(&bg_color);
		gimp_pixel_fetcher_set_bg_color (fetcher, &bg_color);
	}

	gimp_pixel_fetcher_set_edge_mode(fetcher, state->edge_action);

	return fetcher;
}


static int FillRegionPlane(RenderData *rdat, float value);

static void PrecalcRenderStuff(RenderData *rdat) {
	PluginState *state;
	guint dirty;
	int failed;
	double gain, pinch, bias;
	GimpRGB col_fg_bg,col_bg;
	char *grad_name;
	double tmp, tmp2;
	int tot_samples;
	int i,j;
	gdouble *fp_gradient;
	double s;

	dirty = rdat->dirty;

	/*g_message("Dirty: %i",dirty);*/

	state = rdat->p_state;

	if (dirty & DIRTY_FEATURE_SIZE) {
		rdat->dx = 0.5 / state->size_x;
		rdat->dy = 0.5 / state->size_y;
	}
	if (dirty & (DIRTY_FEATURE_SIZE | DIRTY_REGION_PARAMS | DIRTY_BUFFER_TYPE)) {
		rdat->px = rdat->dx * (rdat->region_x-rdat->x_offs);
		rdat->py = rdat->dy * (rdat->region_y-rdat->y_offs);

		tot_samples = (4* rdat->region_height * rdat->region_width * sizeof(float));
		
		if (rdat->buf_alloc < tot_samples) {
			/*g_message("Reallocating buffer to %i bytes",tot_samples);*/
			if (rdat->buf_alloc) g_free(rdat->buffer);
			rdat->buffer = g_malloc(tot_samples);
			rdat->buf_alloc = tot_samples;
		}
		
	}

	if (dirty & (DIRTY_MAPPING | DIRTY_BUFFER_TYPE | DIRTY_REGION_PARAMS )) {
		rdat->polar = 0;
		if (state->mapping == MAP_TILED || state->mapping == MAP_SPHERICAL) {
			rdat->rad1 = rdat->buffer_height * rdat->dy / (2.0 * M_PI); 
			rdat->rad2 = rdat->buffer_width  * rdat->dx / (2.0 * M_PI);
			rdat->ang2 = ((rdat->region_x - rdat->x_offs)* M_PI * 2.0) / rdat->buffer_width;

			if (state->mapping == MAP_SPHERICAL) {
				rdat->dang1= (M_PI) / rdat->buffer_height; /* just half the turn */
				rdat->ang1 = ((rdat->region_y - rdat->y_offs)* M_PI) / rdat->buffer_height;
			} else {
				rdat->dang1= (M_PI * 2.0) / rdat->buffer_height; /* a whole turn */
				rdat->ang1 = ((rdat->region_y - rdat->y_offs)* M_PI * 2.0) / rdat->buffer_height;
			}
			rdat->dang2= (M_PI * 2.0) / rdat->buffer_width;

			rdat->polar = 1;
		}
	}
	if (dirty & DIRTY_OUTPUT_FUNCTION) {
		if (state->function == FUNC_SINE || state->function == FUNC_HALF_SINE) {
			rdat->frequency = state->frequency * M_PI * 2;
		} else {
			rdat->frequency = state->frequency;
		}

		rdat->shift = fmod(state->shift / 100.0, 1.0);
		rdat->function_mode = FN_MODE(state->function, state->reverse);
		/*g_message("Adjust shift to %lf",rdat->shift);*/
	}

	if (dirty & DIRTY_GAIN_PINCH_BIAS) {
		gain = state->gain;
		pinch= state->pinch;
		bias = state->bias;
		
		if (gain>1) gain=1;
		if (gain<-1) gain=-1;

		if (bias>1) bias=1;
		if (bias<-1) bias=-1;

		if (pinch>1) pinch=1;
		if (pinch<-1) pinch=-1;
		
		gain = pow(6,gain);
		bias = (bias*0.499)+0.5;
		pinch= (pinch*0.499)+0.5;

		tmp = (1.0 / bias) - 2.0;
		
		rdat->bias_coef[0] = (tmp + 1.0);
		rdat->bias_coef[1] = (- tmp);

		tmp = (1.0 / (1.0-pinch)) - 2.0;
		tmp2= 2*tmp + 1;

		rdat->pinch_coef[0] = (tmp + 1.0);
		rdat->pinch_coef[1] = (-2.0 * tmp);
		
		rdat->pinch_coef[2] = - tmp / tmp2;
		rdat->pinch_coef[3] = ((1.0-tmp) / tmp2);
		rdat->pinch_coef[4] = (2.0 * tmp  / tmp2);

		rdat->gain = gain;


		/* calculate the average (ie.. where does a value of 0.5 ends) */
		tmp = (0.5) / (rdat->bias_coef[0] + rdat->bias_coef[1] * 0.5);

		if (tmp < 0.5) {
			tmp = (tmp) / (rdat->pinch_coef[0] + rdat->pinch_coef[1] * tmp);
		} else {
			tmp = (rdat->pinch_coef[2] + tmp) / (rdat->pinch_coef[3] + rdat->pinch_coef[4] * tmp);
		}

		rdat->average = tmp;

	}

	if (dirty & (DIRTY_BASIS | DIRTY_FEATURE_SIZE | DIRTY_WARP)) {

		tmp = (state->warp_caustics + 100) / 200; /* the range is -100 .. 100 */
		if (tmp > 1) tmp = 1;
		if (tmp < 0) tmp = 0;

		/* same formulas as used for the 'gain' */
		tmp2 = (1.0 / WARP_GAIN - 2.0)*(1.0 - 2.0 * tmp);
		
		if (tmp < 0.5) {
			tmp = tmp / (tmp2 + 1 ); 
		} else {
			tmp = (tmp2 - tmp) / (tmp2 - 1); 
		}

		tmp = -(tmp-0.5)*2;
	
		/* the caustics are based on derivatives, so we multiply the coefficients so
		 * the caustics intensity doesn't change with scale */
		rdat->caustic_coef_x = tmp * (state->size_x / state->warp_x_size);
		rdat->caustic_coef_y = tmp * (state->size_y / state->warp_y_size);
	}

	if (dirty & (DIRTY_COLOR | DIRTY_BUFFER_TYPE)) {
		if (rdat->gradient) {
			g_free(rdat->gradient);
			rdat->gradient = NULL;
		}
		
		failed = 0;
		if (state->color_src == COL_GRADIENT) {
			grad_name = GetGradientName(state->gradient);
			if (grad_name && gimp_gradient_get_uniform_samples(
							grad_name,
							GRADIENT_SAMPLES,
							0,
							&tot_samples,
							&fp_gradient)
			) {
				rdat->gradient = fp_gradient;
				if (rdat->write_mode != MODE_COLOR) {
					for (i = j = 0; j < tot_samples; i+=2, j+=4) {
						fp_gradient[i+0] = fp_gradient[j+0]*0.30+fp_gradient[j+1]*0.59+fp_gradient[j+2]*0.11;
						fp_gradient[i+1] = fp_gradient[j+3];
					}
				}
			} else {
				failed = 1;
			}
			g_free(grad_name);
		}

			
		if (failed || state->color_src == COL_FG_BG) {
				gimp_context_get_foreground(& col_fg_bg);
				gimp_context_get_background(& col_bg);
				
				col_fg_bg.r -= col_bg.r;
				col_fg_bg.g -= col_bg.b;
				col_fg_bg.b -= col_bg.g;
				col_fg_bg.a -= col_bg.a;

				
				if (rdat->write_mode == MODE_COLOR) {
					rdat->gradient = g_malloc(GRADIENT_SAMPLES * sizeof(double) * 4);
					for (i = 0; i < GRADIENT_SAMPLES*4; i+=4) {
						s = (double)i / (double)(GRADIENT_SAMPLES*4-4);
						rdat->gradient[i+0] = SCALE_TO_BUFFER( col_fg_bg.r*s + col_bg.r );
						rdat->gradient[i+1] = SCALE_TO_BUFFER( col_fg_bg.g*s + col_bg.g );
						rdat->gradient[i+2] = SCALE_TO_BUFFER( col_fg_bg.b*s + col_bg.b );
						rdat->gradient[i+3] = VALUE_MAX;
					}
				} else {
					rdat->gradient = g_malloc(GRADIENT_SAMPLES*sizeof(double) * 2);
					col_fg_bg.r = 0.30*col_fg_bg.r + 0.59*col_fg_bg.g + 0.11*col_fg_bg.b;
					col_bg.r    = 0.30*col_bg.r    + 0.59*col_bg.g    + 0.11*col_bg.b;
					for (i = 0; i < GRADIENT_SAMPLES*2; i+=2) {
						s = (double)i / (double)(GRADIENT_SAMPLES*2-2);
						rdat->gradient[i+0] = SCALE_TO_BUFFER( col_fg_bg.r*s + col_bg.r );
						rdat->gradient[i+1] = VALUE_MAX;
					}
				}
		}
	}

	rdat->dirty = 0;
	rdat->plane = 0.0; 
}


void InitRenderData(RenderData *rdat){ 
	rdat->gradient = NULL;
	rdat->buffer   = NULL;
	rdat->dirty = ~0;
	rdat->buf_alloc = 0;
}

void AssociateRenderToState(RenderData *rdat, PluginState *state){
	rdat->p_state = state;
	rdat->dirty = ~0;
}

void SetRenderStateDirty(RenderData *rdat, guint dirty) {
	rdat->dirty |= dirty;
}

void SetRenderBuffer(RenderData *rdat, int width, int height, int offset_x, int offset_y, int mode, int pixel_stride){
	rdat->buffer_width = width;
	rdat->buffer_height= height;
	rdat->x_offs = offset_x;
	rdat->y_offs = offset_y;

	SetRenderBufferMode(rdat, mode, pixel_stride);

}

void SetRenderBufferMode(RenderData *rdat, int mode, int pixel_stride){
	switch (mode) {
		case MODE_RAW:       rdat->pixel_stride = pixel_stride; break;
		case MODE_COLOR:     rdat->pixel_stride = 4; break;
		case MODE_GRAYSCALE: rdat->pixel_stride = 2; break;
	}				 

	rdat->write_mode   = mode;
	rdat->dirty |= DIRTY_BUFFER_TYPE;
}

void SetRenderBufferForDrawable(RenderData *rdat, GimpDrawable *drawable){
	int x1,y1,x2,y2;
	
	gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
	
	rdat->buffer_width = x2-x1;
	rdat->buffer_height= y2-y1;
	rdat->x_offs = x1;
	rdat->y_offs = y1;

	switch (drawable->bpp) {
		case 1: case 2: 
			rdat->write_mode = MODE_GRAYSCALE;
			rdat->pixel_stride = 2;
			break;
		case 3: case 4: 
			rdat->write_mode = MODE_COLOR;
			rdat->pixel_stride = 4;
			break;
	}


	rdat->dirty |= DIRTY_BUFFER_TYPE;
}

void SetRenderRegion(RenderData *rdat, int width, int height, int region_x, int region_y) {
	rdat->region_width = width;
	rdat->region_height= height;
	rdat->region_x = region_x;
	rdat->region_y = region_y;
	rdat->dirty |= DIRTY_REGION_PARAMS;
}

void DeinitRenderData(RenderData *rdat) {
	if (rdat->gradient) {
		g_free(rdat->gradient);
		rdat->gradient = NULL;
	}
	if (rdat->buffer){
		g_free(rdat->buffer);
		rdat->buffer = NULL;
	}
	rdat->buf_alloc = 0;
}

/*****************************************************************************/

void Render (gint32 image_ID,
            GimpDrawable *drawable,
            PluginState *state){
	
	GimpPixelRgn dst_rgn;
	GimpPixelRgn src_rgn;

	gint    progress, max_progress;
	gint    has_alpha, alpha;
	gint    x1, y1, x2, y2;
	gpointer pr;
	GimpPixelFetcher* fetcher;
	RenderData rdat;
	

	gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
	has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

	alpha =  drawable->bpp - 1;

	progress = 0;
	max_progress = (x2 - x1) * (y2 - y1);
	
	gimp_pixel_rgn_init(&dst_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
	gimp_pixel_rgn_init(&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);

	
	InitRenderData(&rdat);
	AssociateRenderToState(&rdat, state);
	InitBasis(&rdat);
	
	gimp_progress_init("Rendering noise...");
	SetRenderBufferForDrawable(&rdat, drawable);

	switch (state->color_src) {
		case COL_CHANNELS:
			SetRenderBufferMode(&rdat, MODE_RAW, (drawable->bpp<=2) ? 2 : 4);
			for (pr = gimp_pixel_rgns_register(2, &dst_rgn, &src_rgn); pr!=NULL; 
			     pr = gimp_pixel_rgns_process(pr)) {

				SetRenderRegion(&rdat, dst_rgn.w, dst_rgn.h, dst_rgn.x, dst_rgn.y);
				
				RenderChannels(&rdat);
				Blend(&rdat, src_rgn.data, dst_rgn.data, src_rgn.rowstride, src_rgn.bpp);

				progress += dst_rgn.w * dst_rgn.h;
				gimp_progress_update((double) progress / max_progress);
			}
			break;
		case COL_WARP:
			SetRenderBufferMode(&rdat, MODE_RAW, 1); 
			fetcher = GetPixelFetcher(state, drawable);
			gimp_pixel_fetcher_set_edge_mode(fetcher, state->edge_action);
			for (pr = gimp_pixel_rgns_register(1, &dst_rgn); pr!=NULL; 
			     pr = gimp_pixel_rgns_process(pr)) {

				SetRenderRegion(&rdat, dst_rgn.w, dst_rgn.h, dst_rgn.x, dst_rgn.y);
				
				RenderWarp(&rdat,2);
				Warp(&rdat, fetcher, dst_rgn.data, dst_rgn.rowstride, dst_rgn.bpp,2);
				
				progress += dst_rgn.w * dst_rgn.h;
				gimp_progress_update((double) progress / max_progress);
			}

			gimp_pixel_fetcher_destroy(fetcher);
			break;
			
		default:
			for (pr = gimp_pixel_rgns_register(2, &dst_rgn, &src_rgn); pr!=NULL; 
			     pr = gimp_pixel_rgns_process(pr)) {
				
				SetRenderRegion(&rdat, dst_rgn.w, dst_rgn.h, dst_rgn.x, dst_rgn.y);
				
				RenderLow(&rdat,0);
				Blend(&rdat, src_rgn.data, dst_rgn.data, dst_rgn.rowstride, dst_rgn.bpp);
				
				progress += dst_rgn.w * dst_rgn.h;
				gimp_progress_update((double) progress / max_progress);
			}
			break;
	}

	DeinitRenderData(&rdat);
	
	DeinitBasis();

	gimp_drawable_flush (drawable);
	gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
	gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
	
}

/* FIXME: This function is not threadsafe, it modifies (temporarily) the 'p_state' in rdat */
int RenderChannels(RenderData *rdat) {
	int cnum, rev;
	int chan;
	int alpha_channel;
	PluginState *state;
	
	state = rdat->p_state;
	
	rev =  state->reverse?1:0;


	/* FIXME: this can be optimized so that it doesn't recalculate the same channel twice (reverse or not) */
	/* render each channel independently */
	alpha_channel = (rdat->pixel_stride <= 2)? 1: 3;
	g_assert(rdat->pixel_stride <= 4);
	PrecalcRenderStuff(rdat);
	for (cnum = 0; cnum < rdat->pixel_stride ; cnum++) {
		chan = rdat->p_state->channel[cnum];
		switch (chan) {
			case CHAN_MAX: FillRegionPlane(rdat, (rev && cnum != alpha_channel)?0.0:1.0); break;
			case CHAN_MIN: FillRegionPlane(rdat, (rev && cnum != alpha_channel)?1.0:0.0); break;
			case CHAN_MED: FillRegionPlane(rdat, 0.5); break;
			default: 
				state->reverse = rev ^ (chan & 1); 
				rdat->dirty |= DIRTY_OUTPUT_FUNCTION;
				PrecalcRenderStuff(rdat);
				RenderLow(rdat, chan);
				break;
		}
		rdat->buffer++;
	}
	rdat->buffer-= rdat->pixel_stride;
	state->reverse = rev;
	rdat->dirty |= DIRTY_OUTPUT_FUNCTION;
	return 0;
}

int RenderWarp(RenderData *rdat, int overscan) {
	int cnum;
	
	rdat->region_width += overscan;
	rdat->region_height+= overscan;
	
	rdat->dirty |= DIRTY_REGION_PARAMS;

	for (cnum = 0; cnum < rdat->pixel_stride ; cnum++) { 
		RenderLow(rdat,cnum);
		rdat->buffer++;
	}

	rdat->region_width -=overscan;
	rdat->region_height-=overscan;
	
	rdat->dirty |= DIRTY_REGION_PARAMS;

	rdat->buffer-= rdat->pixel_stride;
	return 0;

}

static int FillRegionPlane(RenderData *rdat, float value) {
	float *p;
	int stride;
	int i;
	
	stride = rdat->pixel_stride;
	p = rdat->buffer;
	i = rdat->region_width * rdat->region_height/* * stride*/;
	g_assert(stride<=4);

	for (; i>0; p+= stride,i--) {
		*p = value;
	}

	return 0;
}


int RenderLow(RenderData *rdat, int plane){

	PluginState *state;
	basis_fn_type *basis_fn;	
	float *p;
	gint x,y;
	double value;
	int write_mode;
	
	double rad1, rad2;
	double ang1, ang2;
	double dang1, dang2;

	double alpha, beta;

	double *gradient;
	double bias_coef[3];
	double pinch_coef[5];
	int i;
	int polar;
	int vp;
	
	double x_orig;
	int function_mode;
	int mapping_mode;
	double px,py;
	double dx,dy;
	double frequency;
	int height, width;
	double c1,s1,c2,s2;
	double phase;
	double gain;
	int    pixel_stride;
	double shift;
	double plane1, plane2;



	if (rdat->dirty) {
		PrecalcRenderStuff(rdat);
	}

	/* make local copies */
	
	state         = rdat->p_state;
	
	mapping_mode  = state->mapping;
	phase         = state->ign_phase?0:state ->phase;

	frequency     = rdat->frequency;
	shift         = rdat->shift;
	
	gradient      = rdat->gradient;
	write_mode    = rdat->write_mode;
	function_mode = rdat->function_mode;
	p             = rdat->buffer;
	height        = rdat->region_height;
	width         = rdat->region_width;
	gain          = rdat->gain;
	x_orig        = rdat->px;
	py            = rdat->py;
	dx            = rdat->dx;
	dy            = rdat->dy;
	polar         = rdat->polar;
	pixel_stride  = rdat->pixel_stride;

	plane1        = plane + 78479.20945239; /* just any large value */
	plane2        = plane + 11824.19784571; /* ditto */

	if (polar) {
		rad1 = rdat->rad1;  rad2 = rdat->rad2;
		ang1 = rdat->ang1;  ang2 = rdat->ang2;
		dang1= rdat->dang1; dang2= rdat->dang2;
	}


	for (i = 0; i < 2; i++) {
		bias_coef[i] = rdat->bias_coef[i];
		pinch_coef[i] = rdat->pinch_coef[i];
	}
	for (;i<5; i++) {
		pinch_coef[i] = rdat->pinch_coef[i];
	}

	basis_fn = GetBasis();
	
	for (y = 0; y < height; y++) {
			px = x_orig;
			if (polar) {
				alpha = ang1 + y * dang1;
				beta  = ang2;
				c1 = cos(alpha)*rad1;
				s1 = sin(alpha); /* we need this 'times rad1' only in MAP_TILED */
			}
			for (x = 0; x < width; x++) {
				switch (mapping_mode) {
					case MAP_PLANAR:
						value = ((basis_3d_fn*)basis_fn)(0.957826*px+0.287348*phase+plane1,
								  0.957826*py+0.287348*phase+plane2,
								  0.917431*phase-0.275229*(px+py)
								);
						break;

					case MAP_TILED: /* 4D torus, moving in a 5D space */
						c2 = cos(beta)*rad2;
						s2 = sin(beta)*rad2;
						beta += dang2;
						value = ((basis_5d_fn*)basis_fn)(c1+plane1, s1*rad1, c2+plane2, s2, phase);
						break;
						
					case MAP_SPHERICAL: /* 3D sphere, moving in a 4D space */
						c2 = cos(beta)*rad2;
						s2 = sin(beta)*rad2;
						beta += dang2;
						value = ((basis_4d_fn*)basis_fn)(c2*s1, s2*s1+plane1, c1+plane2, phase);
						break;

					case MAP_RADIAL:
						value = 0.5;
						break;
				}

				value = (value*gain)+0.5;

				if (value > (1.0-EPSILON)) 
					value = (1.0-EPSILON);
				else if (value < EPSILON) 
					value = EPSILON;
				else {
					value = (value) / (bias_coef[0] + bias_coef[1] * value);

					if (value < 0.5) {
						value = (value ) / (pinch_coef[0] + pinch_coef[1] * value);
					} else {
						value = (pinch_coef[2] + value) / (pinch_coef[3] + pinch_coef[4] * value);
					}
				}

				switch (function_mode) {
					case FN_MODE( FUNC_RAMP, REVERSE_NO ):
						value = fmod(value*frequency,1.0);
						break;
					case FN_MODE( FUNC_TRIANGLE, REVERSE_NO ):
						value = fmod(value*frequency,1.0)*2;
						if (value > 1) value = 2.0-value;
						break;
					case FN_MODE( FUNC_SINE, REVERSE_NO ):
						value = (1 - cos(value*frequency))*0.5;
						break;
					case FN_MODE( FUNC_HALF_SINE, REVERSE_NO ):
						value = cos(value*frequency);
						if (value<0.0) value = -value;
						break;
						
					case FN_MODE( FUNC_RAMP, REVERSE_YES ):
						value = 1-fmod(value*frequency,1.0);
						break;
					case FN_MODE( FUNC_TRIANGLE, REVERSE_YES ):
						value = fmod(value*frequency,1.0)*2;
						if (value > 1) value = 2.0-value;
						value = 1-value;
						break;
					case FN_MODE( FUNC_SINE, REVERSE_YES ):
						value = (1 + cos(value*frequency))*0.5;
						break;
					case FN_MODE( FUNC_HALF_SINE, REVERSE_YES ):
						value = cos(value*frequency);
						if (value<0.0) value = -value;
						value = 1-value;
						break;

					default: return -1;
				}
				
				value += shift;
				if (value > 1.0) value -= 1.0;

				switch (write_mode) {
					case MODE_RAW: /* write the value as-is */
							p[0] = value;
							p+=pixel_stride;
							break;
					case MODE_COLOR: /* write the value as RGBA */
							vp = 4*CAST_TO_INT((GRADIENT_SAMPLES-1) * value);
							p[0] = gradient[vp];
							p[1] = gradient[vp+1];
							p[2] = gradient[vp+2];
							p[3] = gradient[vp+3];
							p+=4; /* pixel_stride */
							break;
					case MODE_GRAYSCALE: /* write the value as GRAY-A*/
							vp = 2*CAST_TO_INT((GRADIENT_SAMPLES-1) * value);
							p[0] = gradient[vp];
							p[1] = gradient[vp+1];
							p+=2; /* pixel_stride */
							break; 

					default: return -1;						 
				} /*switch*/
				px += dx;
			} /* for x */
			py += dy;
	} /* for y */

	return 0;
}




void Blend(RenderData *rdat, guchar *bg, guchar *dest, int row_stride, int bytes_pp) {
	int x,y;
	int width, height;
	float *fg;
	float gamma, bg_alpha, fg_alpha;
	float fg_comp[3], bg_comp[3];

	fg = rdat->buffer;
	width = rdat->region_width;
	height= rdat->region_height;

	
	row_stride -= (width * bytes_pp);
	for (y = 0; y < height; y++) {
		for (x = 0 ; x < width; x++) {
			switch (bytes_pp) {
				case 4: /* destination is: rgb + alpha */
					/* if the foreground is too opaque or the background is transparent...*/
					if (bg[3]==0 || fg[3]>0.996){
						dest[0] = fg[0] * 255.0 + 0.5;
						dest[1] = fg[1] * 255.0 + 0.5;
						dest[2] = fg[2] * 255.0 + 0.5;
						dest[3] = fg[3] * 255.0 + 0.5;
						fg += 4;
						break;
					}
					bg_alpha = bg[3]*(1.0/255.0); /* bg[n] is [0..255] */
					fg_alpha = 1.0-fg[3];
					
					gamma = fg_alpha * bg_alpha + fg[3]; /* this is [0..1] */

					/* premultiply alphas... */
					fg_comp[0] = fg[0]*fg[3];
					fg_comp[1] = fg[1]*fg[3];
					fg_comp[2] = fg[2]*fg[3];

					bg_alpha *= (1.0 / 255.0);

					bg_comp[0] = bg[0]*bg_alpha;
					bg_comp[1] = bg[1]*bg_alpha;
					bg_comp[2] = bg[2]*bg_alpha;

					dest[3] = gamma*255.0 +0.5;
					
					gamma = 255.0 / gamma;

					dest[0] = (bg_comp[0]*fg_alpha + fg_comp[0]) * gamma + 0.5;
					dest[1] = (bg_comp[1]*fg_alpha + fg_comp[1]) * gamma + 0.5;
					dest[2] = (bg_comp[2]*fg_alpha + fg_comp[2]) * gamma + 0.5;
					fg += 4;
					break;
					
				case 3: /* rgb */
					/* if the foreground is too opaque...*/

					if (fg[3]>0.996){
						dest[0] = fg[0] * 255.0 + 0.5;
						dest[1] = fg[1] * 255.0 + 0.5;
						dest[2] = fg[2] * 255.0 + 0.5;
						fg += 4;
						break;
					}
					dest[0] = (int)((fg[0]*255.0-bg[0])*fg[3]+0.5)+bg[0];
					dest[1] = (int)((fg[1]*255.0-bg[1])*fg[3]+0.5)+bg[1];
					dest[2] = (int)((fg[2]*255.0-bg[2])*fg[3]+0.5)+bg[2];
					
					fg += 4;
					break;

				case 2: /* grayscale + alpha */
					/* if the foreground is too opaque or the background is transparent...*/
					if (bg[1]==0 || fg[1]>0.996){
						dest[0] = fg[0]*255.0 + 0.5;
						dest[1] = fg[1]*255.0 + 0.5;
						fg += 2;
						break;
					}

					bg_alpha = bg[1]*(1.0/255.0); /* bg[n] is [0..255] */
					fg_alpha = 1.0-fg[1];
					
					gamma = fg_alpha * bg_alpha + fg[1]; /* this is [0..1] */

					/* premultiply alphas... */
					fg_comp[0] = fg[0]*fg[1];
					bg_comp[0] = bg[0]*bg_alpha*(1.0/255.0);

					dest[1] = gamma*255.0 + 0.5;
					
					dest[0] = (bg_comp[0]*fg_alpha + fg_comp[0]) * 255.0 / gamma + 0.5;

					fg += 2;
					break;

				case 1: /* grayscale */
					/* if the foreground is too opaque or the background is transparent...*/
					if (fg[1]>0.996){
						dest[0] = fg[0]*255.0;
						fg += 2;
						break;
					}
					dest[0] = (int)((fg[0]*255.0-bg[0])*fg[1]+0.5)+bg[0];
					fg += 2;
					
					break;
			}
			dest += bytes_pp;
			bg   += bytes_pp;
		}
		dest += row_stride;
		bg   += row_stride;
	}	
}


void Warp(RenderData *rdat, GimpPixelFetcher *fetcher, guchar *dest, int row_stride, int bytes_pp, int overscan){
	int x,y;
	int width, height;
	int src_x, src_y;
	int px,py;
	double fpx,fpy;
	double dx,dy;
	float *fg;
	int shift;
	int row1, row2;
	double dx1,dy1,dx2,dy2;
	double scale_x;
	double scale_y;
	double area;
	double area_inv;
	double v[4];
	double sum[4];
	double average;
	int x_samples, y_samples;
	double caustics_x,caustics_y;
	double f1,f2;
	int sampling;
	int col_channels;
	int alpha_channel;
	double tmp1, tmp2;
	int i;

	PluginState *state = rdat->p_state;
	
	guchar pixel[4];
	int ix,iy;
	
	width = rdat->region_width;
	height= rdat->region_height;


	row_stride -= (width * bytes_pp);
	src_y = rdat->region_y;
	fg = rdat->buffer;
	shift = overscan * rdat->pixel_stride;

	row1 = rdat->pixel_stride * (width + overscan);
	row2 = row1+row1;

	scale_x = state->warp_x_size * state->size_x;
	scale_y = state->warp_y_size * state->size_y;
	caustics_x = rdat->caustic_coef_x;
	caustics_y = rdat->caustic_coef_y;
	sampling = state->warp_quality;
	average = rdat->average;

	
	switch (bytes_pp) {
		case 1: col_channels = 1; alpha_channel = 0; break; /* 0 means no alpha channel */
		case 2: col_channels = 1; alpha_channel = 1; break;
		case 3: col_channels = 3; alpha_channel = 0; break;
		case 4: col_channels = 3; alpha_channel = 3; break;
	}
				

	for (y = 0; y < height; y++) {
		src_x = rdat->region_x;
		for (x = 0 ; x < width; x++) {
	
			switch (sampling) {

				case 0: /* single point sampling */	

					dx1 = (fg[1+row1] - fg[  row1]) * scale_x;
					dx2 = (fg[2+row1] - fg[1+row1]) * scale_x;

					dy1 = (fg[row1+1] - fg[     1]) * scale_y;
					dy2 = (fg[row2+1] - fg[row1+1]) * scale_y;

					tmp1 = (dx1-dx2)*caustics_x+1;
					if (tmp1<0) tmp1=0;

					tmp2 = (dy1-dy2)*caustics_y+1;
					if (tmp2<0) tmp2=0;
					
					area = tmp1*tmp2;

					if (area < 0.001) area = 0.001;
					area_inv = 1.0 / area;

					px = src_x - (int)((dx1+dx2)*0.5);
					py = src_y - (int)((dy1+dy2)*0.5);
					gimp_pixel_fetcher_get_pixel(fetcher, px, py, dest);

					if (area_inv > 1.0) {
						for (i = 0; i < col_channels; i++) {
							v[i] = dest[i] * area_inv;
							if (v[i] > 255.1) v[i] = 255.1;
							dest[i] = v[i];
						}
					} else {
						for (i = 0; i < col_channels; i++) {
							dest[i] = dest[i] * area_inv;
						}
					}
					break;

				case 1: /* multipoint sampling */
					dx1 = (
						(fg[1     ] - fg[0     ])*0.25+
						(fg[1+row1] - fg[0+row1])*0.50+
						(fg[1+row2] - fg[0+row2])*0.25
						) * scale_x;
					dx2 = (
						(fg[2     ] - fg[1     ])*0.25+
						(fg[2+row1] - fg[1+row1])*0.50+
						(fg[2+row2] - fg[1+row2])*0.25
						) * scale_x;

					dy1 = ( 
						(fg[row1  ] - fg[0  ])*0.25+
						(fg[row1+1] - fg[0+1])*0.5+
						(fg[row1+2] - fg[0+2])*0.25
						)* scale_y;

					dy2 = ( 
						(fg[row2  ] - fg[row1  ])*0.25+ 
						(fg[row2+1] - fg[row1+1])*0.5+
						(fg[row2+2] - fg[row1+2])*0.25
						)* scale_y;


					tmp1 = (dx1-dx2)*caustics_x+1;
					if (tmp1<0) tmp1=0;

					tmp2 = (dy1-dy2)*caustics_y+1;
					if (tmp2<0) tmp2=0;
					
					area = fabs(tmp1*tmp2);

					if (area < 0.001) area = 0.001;
					area_inv = 1.0 / area;
					
					x_samples = ceil(fabs(dx2 - dx1))+1;
					y_samples = ceil(fabs(dy2 - dy1))+1;

					dx = (dx2 - dx1) / (x_samples-1);
					dy = (dy2 - dy1) / (y_samples-1);
					if (x_samples > 6) x_samples = 6;
					if (y_samples > 6) y_samples = 6;
					
					sum[0] = sum[1] = sum[2] = sum[3] = 0;
					fpy = src_y - dy1;
					for (iy = 0; iy < y_samples; iy ++) {
						fpx = src_x - dx1;
						for (ix = 0; ix < x_samples; ix ++) {
							/* FIXME: we could add some jitter, and some gaussian weighting here... 
							 * plus correct sample averaging considering the alpha */
							gimp_pixel_fetcher_get_pixel(fetcher, fpx, fpy, pixel);
							for (i = 0; i < bytes_pp; i++) {
								sum[i] += pixel[i];
							}
							fpx += dx;
						}
						fpy+=dy;
					}
					f1 = 1.0 / (x_samples * y_samples);
					f2 = area_inv * f1;
					if (area_inv > 1.0) {
						for (i = 0; i < col_channels; i++) {
							sum[i] *= f2;
							if (sum[i] > 255.1) sum[i] = 255.1;
							dest[i] = sum[i];
						}
					} else {
						for (i = 0; i < col_channels; i++) {
							dest[i] = sum[i] * f2;
						}
					}
					if (alpha_channel) {
						dest[alpha_channel] = sum[alpha_channel] * f1;
					}
					break;
			}

			dest += bytes_pp;
			fg += rdat->pixel_stride;
			src_x++;
		}
		fg += shift;
		src_y++;
		dest += row_stride;
	}	

}

