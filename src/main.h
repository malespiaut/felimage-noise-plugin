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

/*
 *  This file was based on the gimp plugin template by 
 *  Michael Natterer, with the original copyight as follows:
 */

/* GIMP Plug-in Template
 * Copyright (C) 2000-2004  Michael Natterer <mitch@gimp.org> (the "Author").
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Author of the
 * Software shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the Author.
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#define PROCEDURE_NAME   "plug_in_fimg_noise"
#define DATA_KEY_STATE   "fimg_noise_data"
#define PLUGIN_DESCR	 "Felimage Noise Plugin"
#define PRESET_HEADER    "# Felimage Noise Plugin"
#define PRESET_EXTENSION ".fnp"

#ifndef AS_TEST

#define _(String) (String)
#define N_(String) (String)


enum {COL_FG_BG, COL_GRADIENT, COL_CHANNELS, COL_WARP};
enum {BASIS_LNOISE, BASIS_LTURB_1, BASIS_SNOISE, BASIS_STURB_1, 
	BASIS_CELLS_1, BASIS_CELLS_2, BASIS_CELLS_3, BASIS_CELLS_4, BASIS_CELLS_5 };
enum {FUNC_RAMP, FUNC_TRIANGLE, FUNC_SINE, FUNC_HALF_SINE};
enum {MAP_PLANAR,MAP_TILED,MAP_SPHERICAL,MAP_RADIAL};
enum {CHAN_1, CHAN_1_INV, CHAN_2, CHAN_2_INV, CHAN_3, CHAN_3_INV, CHAN_4, CHAN_4_INV, CHAN_MAX, CHAN_MED, CHAN_MIN};

extern const char *mapping_names[];
extern const char *basis_names[];
extern const char *color_src_names[];
extern const char *function_names[];
extern const char *multifractal_names[];
extern const char *color_channel_source_names[];
extern const char *alpha_channel_source_names[];
extern const char *warp_quality_names[];
extern const char *edge_action_names[];

typedef struct {

	/* seed is the parameter # 4 (index 3) */
	guint seed;
	gboolean random_seed;

	gfloat size_x; /* size in pixels */
	gfloat size_y;

	gfloat octaves; /* default = 3 */
	gfloat lacunarity; /* default = 2 */
	gfloat hurst; /* default = 0.5 */
	
	gfloat frequency; /* default = 1 */
	gfloat shift;  /* default = 0 (percentage) */ 

	gint8   mapping;   /* default = 0 (planar) */
	gint8   basis;     /* default = 0 (noise) */
	gint8   color_src; /* default = 0 (use brush colors) */
	gint8   reverse;   /* default = 0 */
	gint8   function;  /* default = 0 (ramp) */
	gint8   ign_phase; /* default = 1 */
	gint8   multifractal; /* default = 0 */

	gint8	channel[4];

	gfloat warp_x_size;
	gfloat warp_y_size;

	gfloat warp_caustics;  /* default = 0 (percentage) */

	gint8 warp_quality;
	gint8 edge_action;

	gint8 gradient[16]; /* gradient name hash */

	gfloat phase;


	gfloat pinch;
	gfloat bias;
	gfloat gain;

	/* UI state */

	gint8 linked_sizes;
	gint8 linked_warp_sizes;
	gboolean show_preview;

} PluginState;

void SetStateToDefaults(PluginState *state);

void StoreGradientName(PluginState *state, const gchar *name);

gchar *GetGradientName(const guchar *gradient_hash);

/* we hide this, are we're not including the GIMP headers for calibration */
#ifndef CALIBRATE
gboolean dialog (gint32 image_ID,
            GimpDrawable *drawable,
            PluginState *state);
#endif /* CALIBRATE */

#endif
#endif /* __MAIN_H__ */
