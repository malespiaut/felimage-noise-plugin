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

/* human-readable labels for the choice arguments, parallel to the tables above */
static const char *mapping_labels[] = {"Planar","Tileable","Spherical",NULL};
static const char *basis_labels[]   = {"Lattice Noise","Lattice Turbulence",
				"Sparse Noise","Sparse Turbulence",
				"Skin","Puffy","Fractured","Crystals",
				"Galvanized",NULL};
static const char *color_src_labels[]={"FG to BG","Gradient","Channels","Image warp",NULL};
static const char *function_labels[] ={"Ramp","Triangle","Sine","Half sine",NULL};
static const char *multifractal_labels[]={"Ordinary fBm","Multifractal","Inverse Multifractal",NULL};
static const char *color_channel_source_labels[]={"Channel 1","Inverted channel 1",
				"Channel 2","Inverted channel 2",
				"Channel 3","Inverted channel 3",
				"Channel 4","Inverted channel 4",
				"Lightest","Middle","Darkest",NULL};
static const char *alpha_channel_source_labels[]={"Channel 1","Inverted channel 1",
				"Channel 2","Inverted channel 2",
				"Channel 3","Inverted channel 3",
				"Channel 4","Inverted channel 4",
				"Solid",NULL};
static const char *warp_quality_labels[]={"Faster","Better",NULL};
static const char *edge_action_labels[] ={"Wrap","Smear","Black","Background color",NULL};


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
				    EDGE_WRAP,  /* edge action */

				    {0}, /* gradient */

				    0.0, /* phase */

				    0.0, /* pinch */
				    0.0, /* bias */
				    0.0, /* gain */

				    1, /* linked sizes*/
				    1, /* linked warp sizes */
				    1  /* show preview */
                                };


void SetStateToDefaults(PluginState *state) {
	*state = default_state;
}

/* The gradient is stored by name; these helpers are kept so callers don't
 * have to deal with the fixed-size buffer directly.
 */
void StoreGradientName(PluginState *state, const gchar *name){
	if (name) {
		g_strlcpy(state->gradient, name, sizeof(state->gradient));
	} else {
		memset(state->gradient, 0, sizeof(state->gradient));
	}
}


gchar *GetGradientName(const gchar *gradient_name){
	if (!gradient_name || !gradient_name[0]) return NULL;

	return g_strdup(gradient_name);
}


/* ------------------------------------------------------------------------ */
/* GimpPlugIn subclass                                                      */
/* ------------------------------------------------------------------------ */

typedef struct _Felimage      Felimage;
typedef struct _FelimageClass FelimageClass;

struct _Felimage {
	GimpPlugIn parent_instance;
};

struct _FelimageClass {
	GimpPlugInClass parent_class;
};

#define FELIMAGE_TYPE (felimage_get_type ())
#define FELIMAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FELIMAGE_TYPE, Felimage))

GType felimage_get_type (void) G_GNUC_CONST;

static GList          *felimage_query_procedures (GimpPlugIn *plug_in);
static GimpProcedure  *felimage_create_procedure (GimpPlugIn *plug_in,
                                                  const gchar *name);
static gboolean        felimage_set_i18n         (GimpPlugIn *plug_in,
                                                  const gchar *procedure_name,
                                                  gchar **gettext_domain,
                                                  gchar **catalog_dir);

static GimpValueArray *noise_run (GimpProcedure       *procedure,
                                  GimpRunMode          run_mode,
                                  GimpImage           *image,
                                  GimpDrawable       **drawables,
                                  GimpProcedureConfig *config,
                                  gpointer             run_data);

G_DEFINE_TYPE (Felimage, felimage, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FELIMAGE_TYPE)


static void felimage_class_init (FelimageClass *klass) {
	GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

	plug_in_class->query_procedures = felimage_query_procedures;
	plug_in_class->create_procedure = felimage_create_procedure;
	plug_in_class->set_i18n         = felimage_set_i18n;
}

static void felimage_init (Felimage *felimage) {
}

/* Localization was never wired up (no message catalogs exist); disable it
 * explicitly so GIMP doesn't go looking for one next to the executable.
 */
static gboolean felimage_set_i18n (GimpPlugIn *plug_in,
                                   const gchar *procedure_name,
                                   gchar **gettext_domain,
                                   gchar **catalog_dir) {
	return FALSE;
}

static GList *felimage_query_procedures (GimpPlugIn *plug_in) {
	return g_list_append (NULL, g_strdup (PROCEDURE_NAME));
}

static GimpChoice *MakeChoice(const char **nicks, const char **labels) {
	GimpChoice *choice;
	int i;

	choice = gimp_choice_new ();
	for (i = 0; nicks[i]; i++) {
		gimp_choice_add (choice, nicks[i], i, labels[i], NULL);
	}

	return choice;
}

static GimpProcedure *felimage_create_procedure (GimpPlugIn *plug_in,
                                                 const gchar *name) {
	GimpProcedure *procedure = NULL;

	if (strcmp (name, PROCEDURE_NAME) != 0) return NULL;

	procedure = gimp_image_procedure_new (plug_in, name,
	                                      GIMP_PDB_PROC_TYPE_PLUGIN,
	                                      noise_run, NULL, NULL);

	gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
	gimp_procedure_set_sensitivity_mask (procedure,
	                                     GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

	gimp_procedure_set_menu_label (procedure, _("Noise"));
	gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Render/Felimage/");

	gimp_procedure_set_documentation (procedure,
	                                  "Creates several noise patterns",
	                                  "Renders procedural noise (lattice, "
	                                  "sparse and cellular basis functions, "
	                                  "combined as fBm or multifractals) "
	                                  "into the drawable, colored from the "
	                                  "context colors, a gradient, per-channel "
	                                  "sources, or by warping the image.",
	                                  PROCEDURE_NAME);
	gimp_procedure_set_attribution (procedure,
	                                "Guillermo Romero <drirr_gato@users.sourceforge.net>",
	                                "Guillermo Romero",
	                                "2005");

	gimp_procedure_add_uint_argument (procedure, "seed",
	                                  "Seed",
	                                  "Random seed (used when random-seed is FALSE)",
	                                  0, G_MAXUINT32, 0,
	                                  G_PARAM_READWRITE);
	gimp_procedure_add_boolean_argument (procedure, "random-seed",
	                                     "Random seed",
	                                     "Use a new random seed on every run",
	                                     TRUE,
	                                     G_PARAM_READWRITE);

	gimp_procedure_add_double_argument (procedure, "size-x",
	                                    "Feature width",
	                                    "Horizontal feature size in pixels",
	                                    1, GIMP_MAX_IMAGE_SIZE, 10,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "size-y",
	                                    "Feature height",
	                                    "Vertical feature size in pixels",
	                                    1, GIMP_MAX_IMAGE_SIZE, 10,
	                                    G_PARAM_READWRITE);

	gimp_procedure_add_double_argument (procedure, "octaves",
	                                    "Octaves",
	                                    "Number of octaves",
	                                    1, 15, 3,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "lacunarity",
	                                    "Lacunarity",
	                                    "Frequency multiplier between octaves",
	                                    1, 10, 2,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "hurst",
	                                    "Hurst exponent",
	                                    "Roughness of the fractal",
	                                    0, 2, 0.5,
	                                    G_PARAM_READWRITE);

	gimp_procedure_add_double_argument (procedure, "frequency",
	                                    "Frequency",
	                                    "Frequency of the output function",
	                                    1, 100, 1,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "shift",
	                                    "Shift",
	                                    "Phase shift of the output function (percent)",
	                                    0, 100, 0,
	                                    G_PARAM_READWRITE);

	gimp_procedure_add_choice_argument (procedure, "mapping",
	                                    "Mapping",
	                                    "How the noise is projected onto the image",
	                                    MakeChoice (mapping_names, mapping_labels),
	                                    mapping_names[0],
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "basis",
	                                    "Basis",
	                                    "Noise basis function",
	                                    MakeChoice (basis_names, basis_labels),
	                                    basis_names[0],
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "color-src",
	                                    "Color source",
	                                    "How the noise is colored",
	                                    MakeChoice (color_src_names, color_src_labels),
	                                    color_src_names[0],
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_boolean_argument (procedure, "reverse",
	                                     "Reverse",
	                                     "Reverse the output function",
	                                     FALSE,
	                                     G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "function",
	                                    "Function",
	                                    "Output shaping function",
	                                    MakeChoice (function_names, function_labels),
	                                    function_names[0],
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_boolean_argument (procedure, "ign-phase",
	                                     "Ignore phase",
	                                     "Ignore the phase value",
	                                     TRUE,
	                                     G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "phase",
	                                    "Phase",
	                                    "Phase of the noise (used when ign-phase is FALSE)",
	                                    -1000000, 1000000, 0,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "multifractal",
	                                    "Multifractal",
	                                    "How the octaves are combined",
	                                    MakeChoice (multifractal_names, multifractal_labels),
	                                    multifractal_names[0],
	                                    G_PARAM_READWRITE);

	gimp_procedure_add_choice_argument (procedure, "channel-r",
	                                    "Red channel source",
	                                    "Noise channel written to the red channel",
	                                    MakeChoice (color_channel_source_names,
	                                                color_channel_source_labels),
	                                    color_channel_source_names[0],
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "channel-g",
	                                    "Green channel source",
	                                    "Noise channel written to the green channel",
	                                    MakeChoice (color_channel_source_names,
	                                                color_channel_source_labels),
	                                    color_channel_source_names[0],
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "channel-b",
	                                    "Blue channel source",
	                                    "Noise channel written to the blue channel",
	                                    MakeChoice (color_channel_source_names,
	                                                color_channel_source_labels),
	                                    color_channel_source_names[0],
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "channel-a",
	                                    "Alpha channel source",
	                                    "Noise channel written to the alpha channel",
	                                    MakeChoice (alpha_channel_source_names,
	                                                alpha_channel_source_labels),
	                                    alpha_channel_source_names[0],
	                                    G_PARAM_READWRITE);

	gimp_procedure_add_double_argument (procedure, "warp-size-x",
	                                    "Warp width",
	                                    "Horizontal warp displacement in pixels",
	                                    1, GIMP_MAX_IMAGE_SIZE, 10,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "warp-size-y",
	                                    "Warp height",
	                                    "Vertical warp displacement in pixels",
	                                    1, GIMP_MAX_IMAGE_SIZE, 10,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "warp-caustics",
	                                    "Caustics",
	                                    "Caustics intensity (percent)",
	                                    -100, 100, 0,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "warp-quality",
	                                    "Warp quality",
	                                    "Rendering quality of the warp",
	                                    MakeChoice (warp_quality_names, warp_quality_labels),
	                                    warp_quality_names[0],
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_choice_argument (procedure, "edge-action",
	                                    "Edges",
	                                    "How pixels outside the image are sampled when warping",
	                                    MakeChoice (edge_action_names, edge_action_labels),
	                                    edge_action_names[0],
	                                    G_PARAM_READWRITE);

	gimp_procedure_add_gradient_argument (procedure, "gradient",
	                                      "Gradient",
	                                      "Gradient used when color-src is 'gradient'",
	                                      TRUE, NULL, TRUE,
	                                      G_PARAM_READWRITE);

	gimp_procedure_add_double_argument (procedure, "pinch",
	                                    "Pinch",
	                                    "Pinch of the output function",
	                                    -1, 1, 0,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "bias",
	                                    "Bias",
	                                    "Bias of the output function",
	                                    -1, 1, 0,
	                                    G_PARAM_READWRITE);
	gimp_procedure_add_double_argument (procedure, "gain",
	                                    "Gain",
	                                    "Gain of the output function",
	                                    -1, 1, 0,
	                                    G_PARAM_READWRITE);

	/* dialog-only state, persisted between runs but not PDB arguments */
	gimp_procedure_add_boolean_aux_argument (procedure, "linked-sizes",
	                                         "Linked sizes",
	                                         "Feature width and height are linked",
	                                         TRUE,
	                                         G_PARAM_READWRITE);
	gimp_procedure_add_boolean_aux_argument (procedure, "linked-warp-sizes",
	                                         "Linked warp sizes",
	                                         "Warp width and height are linked",
	                                         TRUE,
	                                         G_PARAM_READWRITE);
	gimp_procedure_add_boolean_aux_argument (procedure, "show-preview",
	                                         "Show preview",
	                                         "Update the dialog preview automatically",
	                                         TRUE,
	                                         G_PARAM_READWRITE);

	return procedure;
}


/* ------------------------------------------------------------------------ */
/* GimpProcedureConfig <-> PluginState                                      */
/* ------------------------------------------------------------------------ */

static void state_from_config (GimpProcedureConfig *config, PluginState *state) {
	guint seed;
	gboolean random_seed, reverse, ign_phase;
	gboolean linked_sizes, linked_warp_sizes, show_preview;
	gdouble size_x, size_y, octaves, lacunarity, hurst;
	gdouble frequency, shift, phase, pinch, bias, gain;
	gdouble warp_size_x, warp_size_y, warp_caustics;
	GimpGradient *gradient;

	g_object_get (config,
	              "seed",              &seed,
	              "random-seed",       &random_seed,
	              "size-x",            &size_x,
	              "size-y",            &size_y,
	              "octaves",           &octaves,
	              "lacunarity",        &lacunarity,
	              "hurst",             &hurst,
	              "frequency",         &frequency,
	              "shift",             &shift,
	              "reverse",           &reverse,
	              "ign-phase",         &ign_phase,
	              "phase",             &phase,
	              "warp-size-x",       &warp_size_x,
	              "warp-size-y",       &warp_size_y,
	              "warp-caustics",     &warp_caustics,
	              "gradient",          &gradient,
	              "pinch",             &pinch,
	              "bias",              &bias,
	              "gain",              &gain,
	              "linked-sizes",      &linked_sizes,
	              "linked-warp-sizes", &linked_warp_sizes,
	              "show-preview",      &show_preview,
	              NULL);

	state->seed          = seed;
	state->random_seed   = random_seed;
	state->size_x        = size_x;
	state->size_y        = size_y;
	state->octaves       = octaves;
	state->lacunarity    = lacunarity;
	state->hurst         = hurst;
	state->frequency     = frequency;
	state->shift         = shift;
	state->reverse       = reverse;
	state->ign_phase     = ign_phase;
	state->phase         = phase;
	state->warp_x_size   = warp_size_x;
	state->warp_y_size   = warp_size_y;
	state->warp_caustics = warp_caustics;
	state->pinch         = pinch;
	state->bias          = bias;
	state->gain          = gain;

	state->linked_sizes      = linked_sizes;
	state->linked_warp_sizes = linked_warp_sizes;
	state->show_preview      = show_preview;

	state->mapping      = gimp_procedure_config_get_choice_id (config, "mapping");
	state->basis        = gimp_procedure_config_get_choice_id (config, "basis");
	state->color_src    = gimp_procedure_config_get_choice_id (config, "color-src");
	state->function     = gimp_procedure_config_get_choice_id (config, "function");
	state->multifractal = gimp_procedure_config_get_choice_id (config, "multifractal");
	state->channel[0]   = gimp_procedure_config_get_choice_id (config, "channel-r");
	state->channel[1]   = gimp_procedure_config_get_choice_id (config, "channel-g");
	state->channel[2]   = gimp_procedure_config_get_choice_id (config, "channel-b");
	state->channel[3]   = gimp_procedure_config_get_choice_id (config, "channel-a");
	state->warp_quality = gimp_procedure_config_get_choice_id (config, "warp-quality");
	state->edge_action  = gimp_procedure_config_get_choice_id (config, "edge-action");

	if (gradient) {
		gchar *name = gimp_resource_get_name (GIMP_RESOURCE (gradient));

		StoreGradientName (state, name);
		g_free (name);
		g_object_unref (gradient);
	} else {
		StoreGradientName (state, NULL);
	}
}

static void state_to_config (PluginState *state, GimpProcedureConfig *config) {
	GimpGradient *gradient = NULL;

	if (state->gradient[0]) {
		gradient = gimp_gradient_get_by_name (state->gradient);
	}

	g_object_set (config,
	              "seed",              (guint) state->seed,
	              "random-seed",       (gboolean) state->random_seed,
	              "size-x",            (gdouble) state->size_x,
	              "size-y",            (gdouble) state->size_y,
	              "octaves",           (gdouble) state->octaves,
	              "lacunarity",        (gdouble) state->lacunarity,
	              "hurst",             (gdouble) state->hurst,
	              "frequency",         (gdouble) state->frequency,
	              "shift",             (gdouble) state->shift,
	              "reverse",           (gboolean) state->reverse,
	              "ign-phase",         (gboolean) state->ign_phase,
	              "phase",             (gdouble) state->phase,
	              "warp-size-x",       (gdouble) state->warp_x_size,
	              "warp-size-y",       (gdouble) state->warp_y_size,
	              "warp-caustics",     (gdouble) state->warp_caustics,
	              "gradient",          gradient,
	              "pinch",             (gdouble) state->pinch,
	              "bias",              (gdouble) state->bias,
	              "gain",              (gdouble) state->gain,
	              "mapping",           mapping_names[(int) state->mapping],
	              "basis",             basis_names[(int) state->basis],
	              "color-src",         color_src_names[(int) state->color_src],
	              "function",          function_names[(int) state->function],
	              "multifractal",      multifractal_names[(int) state->multifractal],
	              "channel-r",         color_channel_source_names[(int) state->channel[0]],
	              "channel-g",         color_channel_source_names[(int) state->channel[1]],
	              "channel-b",         color_channel_source_names[(int) state->channel[2]],
	              "channel-a",         alpha_channel_source_names[(int) state->channel[3]],
	              "warp-quality",      warp_quality_names[(int) state->warp_quality],
	              "edge-action",       edge_action_names[(int) state->edge_action],
	              "linked-sizes",      (gboolean) state->linked_sizes,
	              "linked-warp-sizes", (gboolean) state->linked_warp_sizes,
	              "show-preview",      (gboolean) state->show_preview,
	              NULL);
}


/* ------------------------------------------------------------------------ */
/* Run                                                                      */
/* ------------------------------------------------------------------------ */

static GimpValueArray *noise_run (GimpProcedure       *procedure,
                                  GimpRunMode          run_mode,
                                  GimpImage           *image,
                                  GimpDrawable       **drawables,
                                  GimpProcedureConfig *config,
                                  gpointer             run_data) {
	PluginState state;
	GimpDrawable *drawable;

	if (!drawables || !drawables[0] || drawables[1]) {
		GError *error = g_error_new (GIMP_PLUG_IN_ERROR, 0,
		                             "%s works with exactly one drawable.",
		                             PROCEDURE_NAME);

		return gimp_procedure_new_return_values (procedure,
		                                         GIMP_PDB_CALLING_ERROR,
		                                         error);
	}

	drawable = drawables[0];

	SetStateToDefaults (&state);
	state_from_config (config, &state);

	switch (run_mode) {
		case GIMP_RUN_NONINTERACTIVE:
		case GIMP_RUN_WITH_LAST_VALS:
			if (state.random_seed)
				state.seed = g_random_int ();
			break;

		case GIMP_RUN_INTERACTIVE:
			gimp_ui_init (PLUGIN_NAME);

			if (! dialog (image, drawable, &state)) {
				return gimp_procedure_new_return_values (procedure,
				                                         GIMP_PDB_CANCEL,
				                                         NULL);
			}

			state_to_config (&state, config);
			break;

		default:
			break;
	}

	Render (image, drawable, &state);

	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush ();

	return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
