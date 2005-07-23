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

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "render.h"
#include "loadsaveconf.h"

/* these are names used for configuration, (won't change when localized!!)*/
const char *mapping_names[] = {"planar","tileable","spherical",NULL};
const char *basis_names[]   = {"lattice_noise","lattice_turbulence",
				"sparse_noise","sparse_turbulence",
				"skin","puffy","fractured","crystals",
				"galvalized",NULL};
const char *color_src_names[]={"fg_bg","gradient","channels","warp",NULL};
const char *function_names[] ={"ramp","triangle","sine","half_sine",NULL};
const char *multifractal_names[]={"fbm","multifractal","inv_multifractal",NULL};
const char *color_channel_source_names[]={"1","inv_1","2","inv_2","3","inv_3","4","inv_4",
				"light","mid","dark",NULL};
const char *alpha_channel_source_names[]={"1","inv_1","2","inv_2","3","inv_3","4","inv_4",
				"solid",NULL};
const char *warp_quality_names[]={"faster","better",NULL};
const char *edge_action_names[] ={"warp","smear","black","background",NULL};


const PluginState default_state = {
                                    0, /* seed (3) */
                                    TRUE, /* random seed */
				    10, /* size x */
				    10, /* size y */
				    
				    3, /* octaves */
				    2, /* lacunarity */
				    0.5, /* hurst exponent */

				    1.0, /* frequency */
				    0.0, /* shift */

				    0, /* mapping */
				    0, /* basis */
				    0, /* color source */
				    0, /* reverse */
				    0, /* function */
				    1, /* ignore phase */
				    0, /* multifractal */ 

				    {0}, /* r,g,b,a channels */

				    10, /* warp x size */
				    10, /* warp y size */

				    0,  /* warp caustics percentage */
				    0,  /* warp rendering quality */
				    GIMP_PIXEL_FETCHER_EDGE_WRAP,  /* edge action */

				    {0}, /* gradient */

				    0.0, /* phase */

				    0.0, /* pinch */
				    0.0, /* bias */
				    0.0, /* gain */

				    1, /* linked sizes*/
				    1, /* linked warp sizes */
				    1  /* show preview */
                                };





static GimpParamDef args[] =
    {
	/* Required by GIMP */
	{ GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
	{ GIMP_PDB_IMAGE, "image", "Input image" },
	{ GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
	/* Other parameters */
	{ GIMP_PDB_STRING, "parameters as string", ""},
	/*
	{ GIMP_PDB_INT32, "seed", "Seed value (used only if randomize is FALSE)" },
	{ GIMP_PDB_INT32, "randomize", "Use a random seed (TRUE, FALSE)" },
	{ GIMP_PDB_FLOAT, "size_x", "size_x" },
	{ GIMP_PDB_FLOAT, "size_y", "size_y" },

	{ GIMP_PDB_FLOAT, "octaves", "octaves" },
	{ GIMP_PDB_FLOAT, "lacunarity", "lacunarity" },
	{ GIMP_PDB_FLOAT, "hurst", "hurst exponent" },


	{ GIMP_PDB_INT8, "mapping", "" },
	{ GIMP_PDB_INT8, "color_src", "" },
 	
	{ GIMP_PDB_STRING, "gradient name", ""},
	*/
    };

static void query (void) {
	gchar *help_path;
	gchar *help_uri;

	gimp_plugin_domain_register (PLUGIN_NAME, LOCALEDIR);

	help_path = g_build_filename (DATADIR, "help", NULL);
	help_uri = g_filename_to_uri (help_path, NULL, NULL);
	g_free (help_path);

	gimp_plugin_help_register ("http://developer.gimp.org/plug-in-template/help",
	                           help_uri);

	gimp_install_procedure (PROCEDURE_NAME,
	                        "Creates several noise patterns",
	                        "Help",
	                        "Guillermo Romero <drirr_gato@users.sourceforge.net>",
	                        "Guillermo Romero",
	                        "2005",
	                        N_("Noise"),
	                        "RGB*, GRAY*",
	                        GIMP_PLUGIN,
	                        G_N_ELEMENTS (args), 0,
	                        args, NULL);

	gimp_plugin_menu_register (PROCEDURE_NAME, "<Image>/Filters/Render/Felimage/");
}


gchar *GetMD5Text(guchar *md5){
	static gchar txt[32];
	int i;
	gchar a[]="0123456789ABCDEF";

	for (i = 0; i < 16; i++) {
		txt[2*i] = a[(md5[i]>>4)&15];
		txt[2*i+1] = a[md5[i]&15];
	}
	txt[32] = 0;
	return txt;
}


void SetStateToDefaults(PluginState *state) {
	*state = default_state;
}

/* Because we can't pass pointers to dynamically allocated data between calls, we store
 * the name of the gradient as a 16 byte data (which is only the MD5 of the 
 * actual gradient name 
 */
void StoreGradientName(PluginState *state, const gchar *name){
	gchar *grad_name;

	if (name) {
		grad_name = g_utf8_strup(name,-1);
		g_assert(grad_name);
		gimp_md5_get_digest (grad_name, strlen(grad_name), state->gradient);
		g_free(grad_name);
	} else {
		memset(state->gradient,0,16);
	}
}


gchar *GetGradientName(const guchar *gradient_hash){
	gchar**    gradient;
	guchar md5[16];
	gchar *grad_name=NULL;
	gchar *final_name = NULL;
	gint tot_gradients;
	int i,j;

	if (!gradient_hash) return NULL;

	/*g_printf("\n\nLooking for gradient with hash %s\n",GetMD5Text(gradient_hash));*/

	gradient = gimp_gradients_get_list(NULL, &tot_gradients);

	g_assert(gradient);

	for (i=0; i<tot_gradients; i++) {
		
		grad_name = g_utf8_strup(gradient[i],-1);
		g_assert(grad_name);

		gimp_md5_get_digest(grad_name, strlen(grad_name), md5);

		g_free(grad_name);

		
		for (j = 0; j < 16; j++) {
			if (md5[j] != gradient_hash[j]) break;
		}
		if (j == 16) {
			final_name = g_strdup(gradient[i]);
			break;
		}
	}

	if (!final_name) {
		final_name = g_strdup(gradient[0]);
	}

	g_free(gradient);

	return final_name;
}

static void run (const gchar *name,
	     gint n_params,
	     const GimpParam *param,
	     gint *nreturn_vals,
	     GimpParam **return_vals) 
	{

	static GimpParam ret_val[1];
	static PluginState state;

	GimpDrawable *drawable;
	gint32 image_ID;
	GimpRunMode run_mode;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	*nreturn_vals = 1;
	*return_vals = ret_val;
#if 0
	/*  Initialize i18n support  */
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET

	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

	textdomain (GETTEXT_PACKAGE);
#endif

	run_mode = param[0].data.d_int32;
	image_ID = param[1].data.d_int32;
	drawable = gimp_drawable_get (param[2].data.d_drawable);

	SetStateToDefaults(&state);

	if (strcmp (name, PROCEDURE_NAME) == 0) {
		switch (run_mode) {
			case GIMP_RUN_NONINTERACTIVE:
				if (n_params != G_N_ELEMENTS (args)) {
					status = GIMP_PDB_CALLING_ERROR;
				} else {
					ParseConfig(&state, param[3].data.d_string);

					if (state.random_seed)
						state.seed = g_random_int ();
				}
				break;

			case GIMP_RUN_INTERACTIVE:
				gimp_get_data (DATA_KEY_STATE, &state);

#ifndef NOT_PLUGIN		
				if (! dialog (image_ID, drawable, &state)) {
					status = GIMP_PDB_CANCEL;
				}
#endif
				break;

			case GIMP_RUN_WITH_LAST_VALS:
				gimp_get_data (DATA_KEY_STATE, &state);

				if (state.random_seed)
					state.seed = g_random_int ();
				break;

			default:
				break;
		}
	} else {
		status = GIMP_PDB_CALLING_ERROR;
	}

	if (status == GIMP_PDB_SUCCESS) {
#ifndef NOT_PLUGIN		
		Render (image_ID, drawable, &state);
#endif
		if (run_mode != GIMP_RUN_NONINTERACTIVE)
			gimp_displays_flush ();

		if (run_mode == GIMP_RUN_INTERACTIVE) {
			gimp_set_data (DATA_KEY_STATE, &state, sizeof (state));
		}

		gimp_drawable_detach (drawable);
	}

	ret_val[0].type = GIMP_PDB_STATUS;
	ret_val[0].data.d_status = status;
}



GimpPlugInInfo PLUG_IN_INFO =
    {
        NULL,   /* init_proc  */
        NULL,   /* quit_proc  */
        query,  /* query_proc */
        run,    /* run_proc   */
    };
#ifndef NOT_PLUGIN
MAIN ()
#else
int main(int argc, char *argv[]) {
	PluginState state;

	ParseConfig(&state, "seed : 12, size_x:10\n");
	
	

	return 0;
}
	
#endif

