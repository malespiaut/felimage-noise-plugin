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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimpui.h>
#include <string.h>

/* FIXME: check for these in ./configure */
#include <sys/stat.h>
#include <unistd.h>


#include "main.h"
#include "render.h"
#include "poisson.h"
#include "cell.h"
#include "snoise.h"
#include "basis.h"
#include "loadsaveconf.h"

/*  Constants  */

#define SPIN_BUTTON_WIDTH  100
#define RANDOM_SEED_WIDTH  100


/* created with:  
 *
 * gdk-pixbuf-csource --raw --static --name=link_icon_data ./link_icon.png  > link_icon.h 
 *
 * */

#include "link_icon.h"


typedef struct{
	PluginState *state;
	GimpPreview *preview;
	RenderData *rdat;
	int has_alpha;
	int bpp;
} CallbackData;

typedef struct {
	CallbackData *cb_data;
	GtkWidget *group[4]; /* widget which gets (de) activated according to the color selection state*/
} ColorChangeCallbackData;


typedef struct {
	CallbackData *cb_data;
	GtkWidget *linked; 
} PhaseEnableCallbackData;


typedef struct {
	CallbackData *cb_data;
	GtkWidget *preset_combo;
	GtkWidget *preset_path;
} PresetChangeCallbackData;


GtkWidget *dlg;
GtkWidget *main_box;
GtkWidget *left_box;
GtkWidget *hbox;
GtkWidget *seed;
GtkWidget *ebox;
GtkWidget *basis;
GtkWidget *multifractal;
GtkWidget *mapping;
GtkWidget *scale;
GtkWidget *notebook;
GtkWidget *page_basis;
GtkWidget *page_colors;
GtkWidget *page_output;
GtkWidget *page_presets;
GtkObject *octaves;
GtkObject *lacuna;
GtkObject *hurst;
GtkWidget *table;
GtkWidget *gradient;
GtkWidget *col_gradient;
GtkWidget *col_channels;
GtkWidget *col_image;
GtkWidget *phase;
GtkWidget *ign_phase;
GtkObject *pinch;
GtkObject *bias;
GtkObject *gain;
GtkWidget *function;
GtkWidget *frequency;
GtkObject *shift;
GtkWidget *reverse;
GtkWidget *preview;
GdkPixbuf *link_icon_pixbuf;
GtkWidget *link_icon;
GtkWidget *channel_r;
GtkWidget *channel_g;
GtkWidget *channel_b;
GtkWidget *channel_a;
GtkWidget *color_src;
GtkWidget *warp_size;
GtkWidget *warp_quality;
GtkWidget *edge_action;
GtkObject *warp_caustics;
GtkWidget *preset_path;
GtkWidget *preset_combo;
GtkWidget *preset_save;
GtkTooltips *tooltips;

CallbackData cb_data;

static void OnSelectGradient(const gchar *gradient_name, gint width, const gdouble *grad_data, gboolean dialog_closing, gpointer user_data);
static void OnSeedChange (GtkSpinButton *spinbutton, gpointer user_data);
static void OnBasisChange (GimpIntComboBox *widget, gpointer user_data);
static void OnMultifractalChange (GimpIntComboBox *widget, gpointer user_data);
static void OnOctavesChange (GtkAdjustment *adjustment, gpointer user_data);
static void OnLacunaChange (GtkAdjustment *adjustment, gpointer user_data);
static void OnHurstChange (GtkAdjustment *adjustment, gpointer user_data);
static void OnMappingChange (GimpIntComboBox *widget, gpointer user_data);
static void OnScaleChange(GimpSizeEntry *gimpsizeentry, gpointer user_data);
static void OnPhaseChange(GtkSpinButton *spinbutton, gpointer user_data);
static void OnPhaseEnable(GtkToggleButton *togglebutton, gpointer user_data);
static void OnPinchChange(GtkAdjustment *adjustment, gpointer user_data);
static void OnBiasChange(GtkAdjustment *adjustment, gpointer user_data);
static void OnGainChange(GtkAdjustment *adjustment, gpointer user_data);
static void OnFunctionChange (GimpIntComboBox *widget, gpointer user_data);
static void OnShiftChange (GtkAdjustment *adjustment, gpointer user_data);
static void OnFrequencyChange(GtkSpinButton *spinbutton, gpointer user_data);
static void OnReverseChange(GtkToggleButton *togglebutton, gpointer user_data);
static void OnColorSrcChange(GimpIntComboBox *togglebutton, gpointer user_data);
static void OnRedChannelChange (GimpIntComboBox *widget, gpointer user_data);
static void OnGreenChannelChange (GimpIntComboBox *widget, gpointer user_data);
static void OnBlueChannelChange (GimpIntComboBox *widget, gpointer user_data);
static void OnAlphaChannelChange (GimpIntComboBox *widget, gpointer user_data);
static void OnWarpSizeChange(GimpSizeEntry *gimpsizeentry, gpointer user_data);
static void OnWarpQualityChange (GimpIntComboBox *widget, gpointer user_data);
static void OnEdgeActionChange (GimpIntComboBox *widget, gpointer user_data);
static void OnWarpCausticsChange (GtkAdjustment *adjustment, gpointer user_data);
static void OnPresetPathChange(GimpFileEntry *entry, gpointer user_data);
static void OnPresetChange (GtkComboBox *widget, gpointer user_data);
static void OnSavePreset(GtkButton *button, gpointer user_data);
/*static void OnLoadPreset(GtkButton *button, gpointer user_data);*/

static void PreviewUpdate (GimpPreview *preview, gpointer user_data);


static void NewHSeparator(GtkBox *parent) {
	GtkWidget* sep;
	sep = gtk_hseparator_new();
	gtk_box_pack_start (parent, sep, FALSE, FALSE, 0);
}

/* FIXME: Tidy this up... move these functions to another file */
static  void MakeFullPresetName(char **filename) {
	char *tmp;
	if (g_str_has_suffix(*filename,PRESET_EXTENSION)) return;
	tmp = g_malloc(strlen(*filename)+5);
	strcpy(tmp,*filename);
	strcat(tmp,PRESET_EXTENSION);
	g_free(*filename);
	*filename = tmp;
}

static void RemoveExtensionPresetName(char **filename) {
	if (!g_str_has_suffix(*filename,PRESET_EXTENSION)) return;
	(*filename)[strlen(*filename)-4] = 0;
}


static int MakePath(const char *base, ...) {
	va_list arg, arg2;
	char *path, *tmp;
	char *dir;
	GSList *created, *item;
	va_start(arg, base);
	va_copy(arg2,arg);

	/* it's likely the path already exists... so check that first */
	path = g_build_filename(base,NULL);
	for(dir = va_arg(arg,char *);dir;dir = va_arg(arg,char *)) {
		tmp = path;
		path = g_build_filename(tmp,dir,NULL);
		g_free(tmp);
	}

	if (g_file_test(path,G_FILE_TEST_EXISTS) && g_file_test(path,G_FILE_TEST_IS_DIR)){
		g_free(path);
		return 0;
	}
	
	
	/* the full path doesn't exist... */
	path = g_build_filename(base,NULL);
	
	created = NULL;
	for(;;) {
		if (g_file_test(path, G_FILE_TEST_EXISTS)) {
			if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
				goto loop;		
			}
		} else {
			if (!mkdir(path,0775)){
				created = g_slist_prepend(created,g_strdup(path));	
				goto loop;
			}
		}
		/* error, couldn't jump to/go to path */
		g_free(path);
		path = NULL;
		break;
loop:
		dir = va_arg(arg2, char*);	
		if (dir == NULL) break; 
		tmp = path;
		path = g_build_filename(tmp,dir,NULL);
		g_free(tmp);
	};

	/* free the list of created directories, removing them if there was an error */
	item = created;
	while (item) {
		if (!path) {
			rmdir((char *)(item->data));
		}
		g_free(item->data);
		item = item->next;
	}
	g_slist_free (created);
	
	if (path) {
		g_free(path);
		return 0;
	} else {
		return 1;
	}
}

static char *GetPresetBasePath() {
	const char *gd;
	char *path;
	int r;

	gd = gimp_directory();
	
	r = -1;
	if (g_file_test(gd,G_FILE_TEST_IS_DIR)) {
		r = MakePath(gd,"plug-ins","felimage","noise",NULL);
		if (!r) {
			path = g_build_filename(gd,"plug-ins","felimage","noise",NULL);
		}
	}
	if (r) {
		r = MakePath(g_get_home_dir(),".felimage","gimp-plug-ins","noise",NULL);
		if (!r) {
			path = g_build_filename(g_get_home_dir(),".felimage","gimp-plug-ins","noise",NULL);
		}
	}
	if (r) {
		path = g_strdup(g_get_home_dir());
	}
	
	return path;
}

int LoadPresetsPath(const char *path, GtkWidget *combobox, const char *selected){
	GDir* dir;
	char *p_name;
	FILE *file;
	char *buffer;
	const char *name;
	int selected_idx;
	int i;
	/* we need a list to sort the items before inserting them in the combo */
	GSList *name_list, *item; 
	
	dir = g_dir_open(path, 0, NULL);

	if (!dir) {
		g_message(_("Couldn't read from path %s"),path);
		return -1;
	}
	
	gtk_list_store_clear (GTK_LIST_STORE (gtk_combo_box_get_model(GTK_COMBO_BOX(combobox))));	

	buffer = g_malloc(1024);
	name_list = NULL;

	while ((name = g_dir_read_name( dir )) != NULL) {
		if (!g_str_has_suffix(name,PRESET_EXTENSION)) continue;

		p_name = g_build_filename(path,name,NULL);
		file = fopen(p_name,"rt");
		g_free(p_name);

		if (!file) continue;
		if (!fgets(buffer, 1024, file)) {
			fclose(file);
			continue;
		}
		if (!g_str_has_prefix(buffer,PRESET_HEADER)){
			fclose(file);
			continue;
				
		}
		fclose(file);

		p_name = g_strdup(name); /* name without the extension */
		p_name[strlen(name)-4]=0;
		name_list = g_slist_insert_sorted(name_list,p_name, (GCompareFunc)strcmp);
	}
	g_free(buffer);
	g_dir_close(dir);


	/* p_name is the filename only, without path */
	p_name = NULL;
	if (selected) {
		p_name = g_path_get_basename(selected);
	}
	
	selected_idx = -1;
	for (item=name_list, i = 0; item; i++, item=item->next){
		gtk_combo_box_append_text (GTK_COMBO_BOX(combobox), (char*)(item->data));
		if (p_name && !strcmp((char*)(item->data),p_name)) {
			selected_idx = i;
		}
		g_free(item->data);
	}
	g_free(p_name);
	g_slist_free(name_list);

	if (selected_idx > -1) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox),selected_idx);
	}

	return selected_idx;
	
}

void SetWidgetsFromState(PluginState *state) {
	GtkWidget *group[4];
	char *grad_name;
	int i;

	gtk_toggle_button_set_active(GIMP_RANDOM_SEED_TOGGLE(seed),state->random_seed);
	if (!state->random_seed) {
		gtk_spin_button_set_value(GIMP_RANDOM_SEED_SPINBUTTON(seed),state->seed);
	}
	
	g_signal_handlers_block_by_func(scale, G_CALLBACK (OnScaleChange), &cb_data);
	gimp_chain_button_set_active(GIMP_COORDINATES_CHAINBUTTON(scale),state->linked_sizes);
	gimp_size_entry_set_refval(GIMP_SIZE_ENTRY(scale),1,state->size_y);
	gimp_size_entry_set_refval(GIMP_SIZE_ENTRY(scale),0,state->size_x);
	g_signal_handlers_unblock_by_func(scale, G_CALLBACK (OnScaleChange), &cb_data);

	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(basis), state->basis);
	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(multifractal), state->multifractal);
	
	gtk_spin_button_set_value(GIMP_SCALE_ENTRY_SPINBUTTON(octaves),state->octaves);
	gtk_spin_button_set_value(GIMP_SCALE_ENTRY_SPINBUTTON(lacuna),state->lacunarity);
	gtk_spin_button_set_value(GIMP_SCALE_ENTRY_SPINBUTTON(hurst),state->hurst);

	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(mapping),state->mapping);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(phase), state->phase);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ign_phase), state->ign_phase);

	gtk_widget_set_sensitive(phase, !state->ign_phase);

	gtk_spin_button_set_value(GIMP_SCALE_ENTRY_SPINBUTTON(pinch),state->pinch);
	gtk_spin_button_set_value(GIMP_SCALE_ENTRY_SPINBUTTON(bias),state->bias);
	gtk_spin_button_set_value(GIMP_SCALE_ENTRY_SPINBUTTON(gain),state->gain);

	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(function), state->function);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(frequency), state->frequency);

	gtk_spin_button_set_value(GIMP_SCALE_ENTRY_SPINBUTTON(shift),state->shift);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reverse), state->reverse);

	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(color_src), state->color_src);

	grad_name =  GetGradientName(state->gradient);
	gimp_gradient_select_widget_set (gradient, grad_name);
	g_free(grad_name);
		
	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(channel_r), state->channel[0]);
	if (channel_g && channel_b){
		gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(channel_g), state->channel[1]);
		gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(channel_b), state->channel[2]);
	}
	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(channel_a), state->channel[3]);

	g_signal_handlers_block_by_func(warp_size, G_CALLBACK (OnWarpSizeChange), &cb_data);
	gimp_chain_button_set_active(GIMP_COORDINATES_CHAINBUTTON(warp_size),state->linked_warp_sizes);
	gimp_size_entry_set_refval(GIMP_SIZE_ENTRY(warp_size),0,state->warp_x_size);
	gimp_size_entry_set_refval(GIMP_SIZE_ENTRY(warp_size),1,state->warp_y_size);
	g_signal_handlers_unblock_by_func(warp_size, G_CALLBACK (OnWarpSizeChange), &cb_data);

	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(warp_quality), state->warp_quality);
	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(edge_action), state->edge_action);

	gtk_spin_button_set_value(GIMP_SCALE_ENTRY_SPINBUTTON(warp_caustics),state->warp_caustics);

	
	group[1] = col_gradient; group[2] = col_channels; group[3] = col_image;
	for (i = 1; i < 4; i++) {
		if (!group[i]) continue;
		if (i == state->color_src) {
			gtk_widget_show_all(group[i]);
		} else {
			gtk_widget_hide_all(group[i]);
		}
	}
}


gboolean
dialog (gint32 image_ID,
        GimpDrawable *drawable,
        PluginState *state) {

	
	int i;

	const char *channel_list1[] = {
		_("Channel 1"),_("Channel 1 inverse"),
		_("Channel 2"),_("Channel 2 inverse"),
		_("Channel 3"),_("Channel 3 inverse"),
		_("Channel 4"),_("Channel 4 inverse"),
		_("Light"),_("Mid"),_("Dark")
	};

	const char *channel_list2[] = {
		_("Channel 1"),_("Channel 1 inverse"),
		_("Channel 2"),_("Channel 2 inverse"),
		_("Channel 3"),_("Channel 3 inverse"),
		_("Channel 4"),_("Channel 4 inverse"),
		_("Solid")
	};

	gchar *grad_name;
	tooltips = gtk_tooltips_new ();

	ColorChangeCallbackData col_change_cb_data;
	PhaseEnableCallbackData phase_enable_cb_data;
	PresetChangeCallbackData preset_change_cb_data;

	char *path;
	
	gboolean run = FALSE;
	GimpUnit unit;
	gdouble xres, yres;
	RenderData rdat;
	PluginState tmp_state = *state;
	
	InitRenderData(&rdat);
	AssociateRenderToState(&rdat, &tmp_state);
	InitBasis(&rdat);
	
	preview = gimp_drawable_preview_new(drawable,&tmp_state.show_preview);
	gtk_widget_show_all (preview);

	cb_data.state = &tmp_state;
	cb_data.preview =GIMP_PREVIEW(preview);
	cb_data.rdat = &rdat;
	cb_data.bpp  = drawable->bpp;
	cb_data.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  	g_signal_connect (preview, "invalidated", G_CALLBACK (PreviewUpdate), &cb_data);

	
	gimp_ui_init (PLUGIN_NAME, TRUE);

	dlg = gimp_dialog_new ("Felimage Noise", PLUGIN_NAME,
	                       NULL, 0,
	                       gimp_standard_help_func, "plug-in-template",
	                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                       GTK_STOCK_OK, GTK_RESPONSE_OK,
	                       NULL);
	
	gtk_window_set_resizable(GTK_WINDOW(dlg),FALSE);
	main_box = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (main_box), 12);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_box);


	gtk_widget_show (main_box);

	left_box = gtk_vbox_new (FALSE,0);
	gtk_container_set_border_width (GTK_CONTAINER (left_box), 5);
	gtk_box_pack_start (GTK_BOX (main_box),left_box, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (left_box),preview, FALSE, FALSE, 12);

	gtk_box_pack_start (GTK_BOX (left_box), gtk_hseparator_new() ,TRUE ,TRUE, 12);

	hbox = gtk_hbox_new (FALSE, 0);
	
	ebox = gtk_event_box_new();
	link_icon_pixbuf = gdk_pixbuf_new_from_inline(-1, link_icon_data, FALSE, NULL);
	link_icon = gtk_image_new_from_pixbuf(link_icon_pixbuf);
	gtk_container_add(GTK_CONTAINER(ebox), link_icon);
	g_object_unref(link_icon_pixbuf);

	gtk_box_pack_start (GTK_BOX (hbox), ebox,FALSE,FALSE, 0);
	gtk_box_pack_start (GTK_BOX (left_box), hbox ,FALSE,TRUE, 0);
	gtk_widget_show_all(left_box);	

 	gtk_tooltips_set_tip(tooltips, ebox, _(" Visit www.felimage.com, the homepage of this plugin "),"");
	/* Main notebook */

	notebook = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (main_box), notebook, FALSE, FALSE, 0);

	page_basis = gtk_vbox_new(FALSE,4);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), page_basis, gtk_label_new(_("Basis")));
	gtk_container_set_border_width (GTK_CONTAINER (page_basis), 15);
	gtk_box_set_spacing(GTK_BOX(page_basis),10);

	page_colors = gtk_vbox_new(FALSE,4);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), page_colors, gtk_label_new(_("Colors")));
	gtk_container_set_border_width (GTK_CONTAINER (page_colors), 15);
	gtk_box_set_spacing(GTK_BOX(page_colors),10);
	
	page_output = gtk_vbox_new(FALSE,4);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), page_output, gtk_label_new(_("Output")));
	gtk_container_set_border_width (GTK_CONTAINER (page_output), 15);
	gtk_box_set_spacing(GTK_BOX(page_output),10);
	
	page_presets = gtk_vbox_new(FALSE,4);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), page_presets, gtk_label_new(_("Presets")));
	gtk_container_set_border_width (GTK_CONTAINER (page_presets), 15);
	gtk_box_set_spacing(GTK_BOX(page_presets),10);
	
	gtk_widget_show (notebook);
	/* PAGE: BASIS */
	
	/*  Seed  */
	seed = gimp_random_seed_new (&(tmp_state.seed), &(tmp_state.random_seed));
	gtk_widget_set_size_request (GTK_WIDGET (GIMP_RANDOM_SEED_SPINBUTTON (seed)),
	                             RANDOM_SEED_WIDTH, -1);
  	gtk_box_pack_start (GTK_BOX (page_basis), seed, FALSE, FALSE, 0);  

 	g_signal_connect (GIMP_RANDOM_SEED_SPINBUTTON(seed), "value-changed", G_CALLBACK (OnSeedChange), &cb_data);
	
	/*  Noise scale  */
  	unit = gimp_image_get_unit (image_ID);
	gimp_image_get_resolution (image_ID, &xres, &yres);

	scale = gimp_coordinates_new (unit, "%p", TRUE, TRUE, SPIN_BUTTON_WIDTH,
	                                    GIMP_SIZE_ENTRY_UPDATE_SIZE,

	                                    tmp_state.linked_sizes, TRUE,

	                                    _("Width"), tmp_state.size_x, xres,
	                                    1, GIMP_MAX_IMAGE_SIZE,
	                                    0, drawable->width,

	                                    _("Height"), tmp_state.size_y, yres,
	                                    1, GIMP_MAX_IMAGE_SIZE,
	                                    0, drawable->height);
	gtk_box_pack_start (GTK_BOX (page_basis), scale, FALSE, FALSE, 0);

 	g_signal_connect (scale, "refval-changed", G_CALLBACK (OnScaleChange), &cb_data);
 	g_signal_connect (scale, "value-changed", G_CALLBACK (OnScaleChange), &cb_data);
	
	
	/* Basis function */
	
	basis = gimp_int_combo_box_new( _("Lattice Noise"),BASIS_LNOISE,
					_("Lattice Turbulence"),BASIS_LTURB_1,
					_("Sparse Noise"),BASIS_SNOISE,
					_("Sparse Turbulence"),BASIS_STURB_1,
					_("Skin"),BASIS_CELLS_1,
					_("Puffy"),BASIS_CELLS_2,
					_("Fractured"),BASIS_CELLS_3,
					_("Crystals"),BASIS_CELLS_4,
					_("Galvalized"),BASIS_CELLS_5,
					NULL);
	gtk_box_pack_start (GTK_BOX (page_basis), basis, FALSE, FALSE, 0);
 	g_signal_connect (basis, "changed", G_CALLBACK (OnBasisChange), &cb_data);
	
	/* Multifractal option */
	
	multifractal = gimp_int_combo_box_new( _("Ordinary fBm"),0,
					_("Multifractal"),1,
					_("Inverse Multifractal"),2,
					NULL);
	gtk_box_pack_start (GTK_BOX (page_basis), multifractal, FALSE, FALSE, 0);
 	g_signal_connect (multifractal, "changed", G_CALLBACK (OnMultifractalChange), &cb_data);

	
	/* octaves, lacunarity & hurst */
	NewHSeparator(GTK_BOX(page_basis));

	table = gtk_table_new(3,2,FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 7);
	gtk_box_pack_start (GTK_BOX (page_basis), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings (GTK_TABLE(table),15);
	/* Octaves, Lacunarity, Hurst*/
	
	octaves = gimp_scale_entry_new(GTK_TABLE(table),0,0,_("Octaves"),0,0,tmp_state.octaves,
						1,15, 0.1, 1.0, 1,TRUE,0,0,NULL,0);

 	g_signal_connect (octaves, "value-changed", G_CALLBACK (OnOctavesChange), &cb_data);
	
	lacuna = gimp_scale_entry_new(GTK_TABLE(table),0,1,_("Lacunarity"),0,0,tmp_state.lacunarity,
						1,10, 0.1, 1.0, 1,TRUE,0,0,NULL,0);

 	g_signal_connect (lacuna, "value-changed", G_CALLBACK (OnLacunaChange), &cb_data);

	hurst = gimp_scale_entry_new(GTK_TABLE(table),0,2,_("Hurst exponent"),0,0,tmp_state.hurst,
						0,2, 0.01, 0.1, 2,TRUE,0,0,NULL,0);

 	g_signal_connect (hurst, "value-changed", G_CALLBACK (OnHurstChange), &cb_data);

	gtk_widget_show_all (page_basis);
	/* PAGE: OUTPUT */

	table = gtk_table_new(10,3,FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 7);
	gtk_box_pack_start (GTK_BOX (page_output), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings (GTK_TABLE(table),15);
	
	/* Mapping */
	mapping = gimp_int_combo_box_new(_("Planar"),MAP_PLANAR,_("Tileable planar"),MAP_TILED,_("Spherical"),MAP_SPHERICAL,NULL);
	
	gimp_table_attach_aligned(GTK_TABLE(table), 0, 0, _("Mapping"), 0.0, 0.5, mapping, 1, FALSE);
 	g_signal_connect (mapping, "changed", G_CALLBACK (OnMappingChange), &cb_data);

	/* phase */
	phase = gtk_spin_button_new_with_range(-1000000,1000000,0.0001);

	gimp_table_attach_aligned(GTK_TABLE(table), 0, 1, _("Phase"), 0.0, 0.5, phase, 1, FALSE);
 	g_signal_connect (phase, "value-changed", G_CALLBACK (OnPhaseChange), &cb_data);

	/* enable phase */
	phase_enable_cb_data.cb_data = &cb_data;
	phase_enable_cb_data.linked  = phase;
	
	ign_phase = gtk_check_button_new_with_label(_("Ignore"));

	gtk_table_attach_defaults(GTK_TABLE(table), ign_phase, 2,3, 1,2);
	
 	g_signal_connect (ign_phase, "toggled", G_CALLBACK (OnPhaseEnable), &phase_enable_cb_data);

	
	/* separator */
	gtk_table_attach(GTK_TABLE(table), gtk_hseparator_new(), 0,3, 2,3 , GTK_FILL, GTK_FILL, 0, 10);	
	
	/* pinch */

	pinch = gimp_scale_entry_new(GTK_TABLE(table),0,3,_("Pinch"),0,0,tmp_state.pinch,
						-1,1, 0.01, 0.1, 2,TRUE,0,0,NULL,0);
 	g_signal_connect (pinch, "value-changed", G_CALLBACK (OnPinchChange), &cb_data);
	
	/* bias */
	bias = gimp_scale_entry_new(GTK_TABLE(table),0,4,_("Bias"),0,0,tmp_state.bias,
						-1,1, 0.01, 0.1, 2,TRUE,0,0,NULL,0);

 	g_signal_connect (bias, "value-changed", G_CALLBACK (OnBiasChange), &cb_data);

	/* gain */
	gain = gimp_scale_entry_new(GTK_TABLE(table),0,5,_("Gain"),0,0,tmp_state.gain,
						-1,1, 0.01, 0.1, 2,TRUE,0,0,NULL,0);
 	g_signal_connect (gain, "value-changed", G_CALLBACK (OnGainChange), &cb_data);
	

	/* separator */
	gtk_table_attach(GTK_TABLE(table), gtk_hseparator_new(), 0,3, 6,7 , GTK_FILL, GTK_FILL, 0, 10);	
	
	/* function */
	function = gimp_int_combo_box_new(_("Ramp (saw)"),FUNC_RAMP,_("Triangle"),FUNC_TRIANGLE,_("Sine"),FUNC_SINE,_("Half-Sine"),FUNC_HALF_SINE,NULL);
	gimp_table_attach_aligned(GTK_TABLE(table), 0, 7, _("Function"), 0.0, 0.5, function, 1, FALSE);
 	g_signal_connect (function, "changed", G_CALLBACK (OnFunctionChange), &cb_data);
	
	/* frequency */
	frequency = gtk_spin_button_new_with_range(1, 100, 0.01);
	gimp_table_attach_aligned(GTK_TABLE(table), 0, 8, _("Frequency"), 0.0, 0.5, frequency, 1, FALSE);
 	g_signal_connect (frequency, "value-changed", G_CALLBACK (OnFrequencyChange), &cb_data);

	/* shift */
	shift = gimp_scale_entry_new(GTK_TABLE(table),0,9,_("Shift"),0,0,tmp_state.shift,
						0,100, 0.1, 1.0, 1,TRUE,0,0,NULL,0);

 	g_signal_connect (shift, "value-changed", G_CALLBACK (OnShiftChange), &cb_data);
	
	/* reverse */
	reverse = gtk_check_button_new();
	gimp_table_attach_aligned(GTK_TABLE(table), 0, 10, _("Reverse"), 0.0, 0.5, reverse, 1, FALSE);
 	g_signal_connect (reverse, "toggled", G_CALLBACK (OnReverseChange), &cb_data);

	gtk_widget_show_all (page_output);

	/* PAGE: COLOR*/

	table = gtk_table_new(4,2,FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 7);
	gtk_table_set_col_spacings (GTK_TABLE(table),15);
	gtk_box_pack_start (GTK_BOX (page_colors), table, FALSE, FALSE, 0);
	color_src = gimp_int_combo_box_new(_("Brush colors"),COL_FG_BG,_("Gradient"),COL_GRADIENT,_("Independent channels"),COL_CHANNELS,_("Image"),COL_WARP,NULL);
	
	gimp_table_attach_aligned(GTK_TABLE(table), 0, 0, _("Color source"), 0.0, 0.5, color_src, 1, FALSE);

	gtk_widget_show_all (table);


	/* gradient config */
	col_gradient = gtk_table_new(1,2,FALSE);
	gtk_table_set_col_spacings (GTK_TABLE(col_gradient),15);
	gtk_container_set_border_width (GTK_CONTAINER (col_gradient), 7);
	gtk_box_pack_start (GTK_BOX (page_colors), col_gradient, FALSE, FALSE, 0);

	grad_name = GetGradientName(tmp_state.gradient);
	gradient = gimp_gradient_select_widget_new (NULL,grad_name,OnSelectGradient,&cb_data);
	g_free(grad_name);

	gimp_table_attach_aligned(GTK_TABLE(col_gradient), 0, 0, _("Gradient"), 0.0, 0.5, gradient, 1, FALSE);

	NewHSeparator(GTK_BOX(page_colors));

	/* independent channels */
	col_channels = gtk_table_new(4,2,FALSE);
	gtk_table_set_col_spacings (GTK_TABLE(col_channels),15);
	gtk_container_set_border_width (GTK_CONTAINER (col_channels), 7);
	gtk_box_pack_start (GTK_BOX (page_colors), col_channels, FALSE, FALSE, 0);
	
	channel_r = gimp_int_combo_box_new_array(11, channel_list1);
	gimp_table_attach_aligned(GTK_TABLE(col_channels), 0, 1, cb_data.bpp>2?_("Red"):_("Luminosity"), 0.0, 0.5, channel_r, 1, FALSE);
 	g_signal_connect (channel_r, "changed", G_CALLBACK (OnRedChannelChange), &cb_data);

	if (cb_data.bpp>2) {
		
		channel_g = gimp_int_combo_box_new_array(11, channel_list1);
		gimp_table_attach_aligned(GTK_TABLE(col_channels), 0, 2, _("Green"), 0.0, 0.5, channel_g, 1, FALSE);
		g_signal_connect (channel_g, "changed", G_CALLBACK (OnGreenChannelChange), &cb_data);

		channel_b = gimp_int_combo_box_new_array(11, channel_list1);
		gimp_table_attach_aligned(GTK_TABLE(col_channels), 0, 3, _("Blue"), 0.0, 0.5, channel_b, 1, FALSE);
		g_signal_connect (channel_b, "changed", G_CALLBACK (OnBlueChannelChange), &cb_data);

	}

	channel_a = gimp_int_combo_box_new_array(9,  channel_list2);
	gimp_table_attach_aligned(GTK_TABLE(col_channels), 0, 4, _("Alpha"), 0.0, 0.5, channel_a, 1, FALSE);
	g_signal_connect (channel_a, "changed", G_CALLBACK (OnAlphaChannelChange), &cb_data);
	

	/* image warping */
	
	col_image = gtk_table_new(3,3,FALSE);
	gtk_table_set_col_spacings (GTK_TABLE(col_image),15);
	gtk_container_set_border_width (GTK_CONTAINER (col_image), 7);
	gtk_box_pack_start (GTK_BOX (page_colors), col_image, FALSE, FALSE, 0);	

	warp_size = gimp_coordinates_new (unit, "%p", TRUE, TRUE, SPIN_BUTTON_WIDTH,
	                                    GIMP_SIZE_ENTRY_UPDATE_SIZE,

	                                    tmp_state.linked_warp_sizes, TRUE,

	                                    _("Horizontal"), tmp_state.warp_x_size, xres,
	                                    1, GIMP_MAX_IMAGE_SIZE,
	                                    0, drawable->width,

	                                    _("Vertical"), tmp_state.warp_y_size, yres,
	                                    1, GIMP_MAX_IMAGE_SIZE,
	                                    0, drawable->height);


	gtk_table_attach_defaults(GTK_TABLE(col_image),warp_size, 0,3, 0,1);
 	g_signal_connect (warp_size, "refval-changed", G_CALLBACK (OnWarpSizeChange), &cb_data);
 	g_signal_connect (warp_size, "value-changed", G_CALLBACK (OnWarpSizeChange), &cb_data);

	
	warp_quality = gimp_int_combo_box_new(_("Faster"),0,_("Better"),1,NULL);
	gimp_table_attach_aligned(GTK_TABLE(col_image), 0, 1, _("Quality"), 0.0, 0.5, warp_quality, 1, FALSE);
 	g_signal_connect (warp_quality, "changed", G_CALLBACK (OnWarpQualityChange), &cb_data);


	edge_action = gimp_int_combo_box_new(
					_("Wrap"),GIMP_PIXEL_FETCHER_EDGE_WRAP,
					_("Smear"),GIMP_PIXEL_FETCHER_EDGE_SMEAR,
					_("Black"),GIMP_PIXEL_FETCHER_EDGE_BLACK,
					_("Background"),GIMP_PIXEL_FETCHER_EDGE_BACKGROUND,
					NULL);

	
	gimp_table_attach_aligned(GTK_TABLE(col_image), 0, 2, _("Edges"), 0.0, 0.5, edge_action, 1, FALSE);
 	g_signal_connect (edge_action, "changed", G_CALLBACK (OnEdgeActionChange), &cb_data);


	warp_caustics = gimp_scale_entry_new(GTK_TABLE(col_image),0,3,_("Caustics"),100,00,tmp_state.warp_caustics,
						-100,100,1,10,3,TRUE,0,0,NULL,0);

	g_signal_connect(warp_caustics,"value-changed", G_CALLBACK(OnWarpCausticsChange), &cb_data);
	
	
	
	/****/

	col_change_cb_data.cb_data = &cb_data;
	col_change_cb_data.group[0] = NULL;
	col_change_cb_data.group[1] = col_gradient;
	col_change_cb_data.group[2] = col_channels;
	col_change_cb_data.group[3] = col_image;

	for (i = 0; i < 4; i++) {
		if (!col_change_cb_data.group[i]) continue;
		if (i == tmp_state.color_src) {
			gtk_widget_show_all(col_change_cb_data.group[i]);
		} else {
			gtk_widget_hide_all(col_change_cb_data.group[i]);
		}
	}

 	g_signal_connect (color_src, "changed", G_CALLBACK (OnColorSrcChange), &col_change_cb_data);
	
	gtk_widget_show (page_colors);

	/* PAGE: PRESETS*/

	table = gtk_table_new(4,4,FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 7);
	gtk_box_pack_start (GTK_BOX (page_presets), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings (GTK_TABLE(table),15);

	path = GetPresetBasePath();
	
	preset_path = gimp_file_entry_new(_("Path to presets"),path , TRUE, 1);
	
	g_free(path);

	gimp_table_attach_aligned(GTK_TABLE(table), 0, 0, _("Path"), 0.0, 0.5, preset_path, 1, FALSE);

	preset_combo = gtk_combo_box_new_text();
	gimp_table_attach_aligned(GTK_TABLE(table), 0, 1, _("Preset"), 0.0, 0.5, preset_combo, 1, FALSE);

	
	preset_save = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_table_attach(GTK_TABLE(table), preset_save, 1,2, 2,3 , GTK_FILL, GTK_FILL, 0, 10);	

	preset_change_cb_data.cb_data = &cb_data;
	preset_change_cb_data.preset_combo = preset_combo;
	preset_change_cb_data.preset_path  = preset_path;

 	g_signal_connect (preset_path, "filename-changed", G_CALLBACK (OnPresetPathChange), &preset_change_cb_data);
 	g_signal_connect (preset_combo,"changed", G_CALLBACK (OnPresetChange), &preset_change_cb_data);
 	g_signal_connect (preset_save, "clicked", G_CALLBACK (OnSavePreset), &preset_change_cb_data);

	gtk_widget_show_all (page_presets);
	
	/*  Show the main containers  */

	
	SetWidgetsFromState(&tmp_state);

	gtk_widget_show (dlg);

	run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

	DeinitRenderData(&rdat);
	DeinitBasis();

	if (run) {
		*state = tmp_state;
	}

	gtk_widget_destroy (dlg);

	return run;
}


/*****************************************************************************/
/* CALLBACKS */

static void OnSelectGradient(const gchar *gradient_name, gint width, const gdouble *grad_data, gboolean dialog_closing, gpointer user_data) {
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	StoreGradientName(state, gradient_name);

	SetRenderStateDirty(rdat, DIRTY_COLOR);
	gimp_preview_invalidate(preview);
}

static void OnSeedChange (GtkSpinButton *spinbutton, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;

	
	(void) state;
	gimp_preview_invalidate(preview);
}


static void OnBasisChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->basis = tmp;

	(void) rdat;

	SetRenderStateDirty(rdat, DIRTY_COLOR);
	gimp_preview_invalidate(preview);
}


static void OnMultifractalChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;

	gimp_int_combo_box_get_active (widget, &tmp );
	state->multifractal = tmp;

	(void) rdat;

	gimp_preview_invalidate(preview);
	
}


static void OnOctavesChange (GtkAdjustment *adjustment, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;

	state->octaves = gtk_adjustment_get_value(adjustment);
	
	gimp_preview_invalidate(preview);
}

static void OnLacunaChange (GtkAdjustment *adjustment, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;


	state->lacunarity= gtk_adjustment_get_value(adjustment);
	
	gimp_preview_invalidate(preview);
}

static void OnHurstChange (GtkAdjustment *adjustment, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;


	state->hurst= gtk_adjustment_get_value(adjustment);
	
	gimp_preview_invalidate(preview);
}


static void OnMappingChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->mapping = tmp;

	SetRenderStateDirty(rdat, DIRTY_MAPPING);
	gimp_preview_invalidate(preview);
}

static void OnScaleChange(GimpSizeEntry *gimpsizeentry, gpointer user_data) {
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->size_x = gimp_size_entry_get_refval (gimpsizeentry,0);
	state->size_y = gimp_size_entry_get_refval (gimpsizeentry,1);
	state->linked_sizes = 
	    gimp_chain_button_get_active (GIMP_COORDINATES_CHAINBUTTON (gimpsizeentry));

	SetRenderStateDirty(rdat, DIRTY_FEATURE_SIZE);
	gimp_preview_invalidate(preview);
}

static void OnPhaseChange(GtkSpinButton *spinbutton, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->phase = gtk_spin_button_get_value(spinbutton);

	(void) rdat;
	
	gimp_preview_invalidate(preview);
}

static void OnPhaseEnable(GtkToggleButton *togglebutton, gpointer user_data) {
	CallbackData *cbd  = ((PhaseEnableCallbackData*)user_data)->cb_data;
	PluginState *state= cbd->state;
	GimpPreview *preview = cbd->preview;
	RenderData *rdat= cbd->rdat;

	state->ign_phase = gtk_toggle_button_get_active(togglebutton);
	
	gtk_widget_set_sensitive(((PhaseEnableCallbackData*)user_data)->linked, !(state->ign_phase));
	
	(void) rdat;

	gimp_preview_invalidate(preview);
	
}

static void OnPinchChange(GtkAdjustment *adjustment, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->pinch = gtk_adjustment_get_value(adjustment);
	
	SetRenderStateDirty(rdat, DIRTY_GAIN_PINCH_BIAS);
	gimp_preview_invalidate(preview);
}

	
static void OnBiasChange(GtkAdjustment *adjustment, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->bias = gtk_adjustment_get_value(adjustment);
	
	SetRenderStateDirty(rdat, DIRTY_GAIN_PINCH_BIAS);
	gimp_preview_invalidate(preview);
}

	
static void OnGainChange(GtkAdjustment *adjustment, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->gain = gtk_adjustment_get_value(adjustment);
	
	SetRenderStateDirty(rdat, DIRTY_GAIN_PINCH_BIAS);
	gimp_preview_invalidate(preview);
}


static void OnFunctionChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->function = tmp;

	SetRenderStateDirty(rdat, DIRTY_OUTPUT_FUNCTION);
	gimp_preview_invalidate(preview);
}


static void OnShiftChange (GtkAdjustment *adjustment, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->shift = gtk_adjustment_get_value(adjustment);
	
	SetRenderStateDirty(rdat, DIRTY_OUTPUT_FUNCTION);
	gimp_preview_invalidate(preview);
}

static void OnFrequencyChange(GtkSpinButton *spinbutton, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->frequency = gtk_spin_button_get_value(spinbutton);
	
	SetRenderStateDirty(rdat, DIRTY_OUTPUT_FUNCTION);
	gimp_preview_invalidate(preview);
}

static void OnReverseChange(GtkToggleButton *togglebutton, gpointer user_data) {
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->reverse = gtk_toggle_button_get_active(togglebutton);
	
	SetRenderStateDirty(rdat, DIRTY_OUTPUT_FUNCTION);
	gimp_preview_invalidate(preview);
	
}

static void OnColorSrcChange(GimpIntComboBox *widget, gpointer user_data) {
	ColorChangeCallbackData *data = (ColorChangeCallbackData*)user_data;
	CallbackData *cb_data = data->cb_data;
	gint col_src;
	int i;
	
	gimp_int_combo_box_get_active (widget, &col_src );
	cb_data->state->color_src = col_src;

	for (i = 0; i < 4; i++) {
		if (!data->group[i]) continue;
		if (i == col_src) {
			gtk_widget_show_all(data->group[i]);
		} else {
			gtk_widget_hide_all(data->group[i]);
		}
	}


	SetRenderStateDirty(cb_data->rdat, DIRTY_COLOR);
	gimp_preview_invalidate(cb_data->preview);
}

static void OnRedChannelChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->channel[0] = tmp;

	(void)rdat;

	gimp_preview_invalidate(preview);
}

static void OnGreenChannelChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->channel[1] = tmp;

	(void)rdat;

	gimp_preview_invalidate(preview);
}

static void OnBlueChannelChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->channel[2] = tmp;

	(void)rdat;

	gimp_preview_invalidate(preview);
}

static void OnAlphaChannelChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->channel[(((CallbackData*)user_data)->bpp<=2)?1:3] = tmp;

	(void)rdat;

	gimp_preview_invalidate(preview);
}

static void OnWarpSizeChange(GimpSizeEntry *gimpsizeentry, gpointer user_data) {
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

	state->warp_x_size = gimp_size_entry_get_refval (gimpsizeentry,0);
	state->warp_y_size = gimp_size_entry_get_refval (gimpsizeentry,1);
	state->linked_warp_sizes = 
	    gimp_chain_button_get_active (GIMP_COORDINATES_CHAINBUTTON (gimpsizeentry));

	SetRenderStateDirty(rdat, DIRTY_WARP);
	gimp_preview_invalidate(preview);
}

static void OnWarpQualityChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->warp_quality = tmp;

	(void) rdat;

	SetRenderStateDirty(rdat, DIRTY_WARP);
	gimp_preview_invalidate(preview);
}


static void OnEdgeActionChange (GimpIntComboBox *widget, gpointer user_data){
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;
	gint tmp;
	
	gimp_int_combo_box_get_active (widget, &tmp );
	state->edge_action = tmp;

	(void) rdat;

	SetRenderStateDirty(rdat, DIRTY_WARP);
	gimp_preview_invalidate(preview);
}


static void OnWarpCausticsChange (GtkAdjustment *adjustment, gpointer user_data) {
	PluginState *state= ((CallbackData*)user_data)->state;
	GimpPreview *preview = ((CallbackData*)user_data)->preview;
	RenderData *rdat= ((CallbackData*)user_data)->rdat;

  	state->warp_caustics = gtk_adjustment_get_value(adjustment);

	(void) rdat;

	SetRenderStateDirty(rdat, DIRTY_WARP);
	gimp_preview_invalidate(preview);

}

static void OnPresetPathChange(GimpFileEntry *entry, gpointer user_data) {
	char *path;
	path = gimp_file_entry_get_filename(entry);
	LoadPresetsPath(path, ((PresetChangeCallbackData*)user_data)->preset_combo, NULL);
	g_free(path);

}

static void OnPresetChange (GtkComboBox *widget, gpointer user_data){
	CallbackData *cb_data = ((PresetChangeCallbackData*)user_data)->cb_data;
	char *path;
	char *file;
	char *full_path;
	int r;
	
	path = gimp_file_entry_get_filename(GIMP_FILE_ENTRY(((PresetChangeCallbackData*)user_data)->preset_path));
	file = gtk_combo_box_get_active_text (GTK_COMBO_BOX(widget));

	if (path && file) {
		MakeFullPresetName(&file);
		full_path = g_build_filename(path,file,NULL);

		r = LoadConfig(full_path,cb_data->state);
		if (r) {
			g_message(_("Could not load file %s"),full_path);
		} else {
			SetRenderStateDirty(cb_data->rdat, DIRTY_ALL);
			SetWidgetsFromState(cb_data->state);
			gimp_preview_invalidate(cb_data->preview);
		}
	}
	g_free(path);
	g_free(file);


}

static void OnSavePreset(GtkButton *button, gpointer user_data){
	CallbackData *cb_data = ((PresetChangeCallbackData*)user_data)->cb_data;
	GtkWidget *dialog;
	GtkFileFilter *filter;
	char *path;
	char *file;
	char *full_path;
	char *filename;
	int  r;

	path = gimp_file_entry_get_filename(GIMP_FILE_ENTRY(((PresetChangeCallbackData*)user_data)->preset_path));
	file = gtk_combo_box_get_active_text(GTK_COMBO_BOX(((PresetChangeCallbackData*)user_data)->preset_combo));
	full_path = NULL;

	dialog = gtk_file_chooser_dialog_new (_("Save preset"),
						  NULL,
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						  NULL);

	if (file) {
		MakeFullPresetName(&file);
		full_path = g_build_filename(path,file,NULL);
		if (g_file_test(full_path,G_FILE_TEST_EXISTS)) {
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),full_path);
		} else {
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),path);
		}
	} else {
		full_path = g_build_filename(path,NULL);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),path);
	}


	g_free(file);

	if (full_path) {
		g_free(full_path);
	}

	filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter,PRESET_EXTENSION);
	
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		MakeFullPresetName(&filename);
	
		r = SaveConfig(filename,cb_data->state);
		if (r) {
			g_message(_("There was an error saving preset %s"),filename);
		} else {
			RemoveExtensionPresetName(&filename);
			LoadPresetsPath(path, ((PresetChangeCallbackData*)user_data)->preset_combo, filename);
		}
			
		/* save the file */
		g_free (filename);
	  }

	gtk_widget_destroy (dialog);

  
	g_free(path);
	
}


/*****************************************************************************/

static void PreviewUpdate (GimpPreview *preview, gpointer user_data) {
	GimpDrawable  *drawable;
	gint		 bpp;
	gint           rgn_x, rgn_y;
	guchar        *buffer;  
	gint           rgn_w;   
	gint           rgn_h;   

	GimpPixelRgn   srcPR;   
	GimpPixelFetcher* fetcher;
	gint           stride;
	RenderData      *rdat = ((CallbackData*)user_data)->rdat;
	PluginState     *state=((CallbackData*)user_data)->state;
	
	InitBasis(rdat);
	
	/* Get drawable info */
	drawable = gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview));
	bpp = drawable->bpp;

	gimp_preview_get_position (preview, &rgn_x, &rgn_y);
	gimp_preview_get_size (preview, &rgn_w, &rgn_h);

	gimp_pixel_rgn_init (&srcPR, drawable, rgn_x, rgn_y, rgn_w, rgn_h, FALSE, FALSE);

	stride = rgn_w * bpp;
	
	buffer = g_new (guchar, stride*rgn_h);

	gimp_pixel_rgn_get_rect (&srcPR, buffer, rgn_x, rgn_y, rgn_w, rgn_h);

	SetRenderBufferForDrawable(rdat, drawable);

	switch (state->color_src) {
		case COL_CHANNELS: 
			SetRenderBufferMode(rdat, MODE_RAW, (drawable->bpp<=2) ? 2 : 4);
			SetRenderRegion(rdat, rgn_w, rgn_h, rgn_x, rgn_y);
			RenderChannels(rdat);
			Blend(rdat, buffer, buffer, stride, bpp);
			break;
		case COL_WARP:
			SetRenderBufferMode(rdat, MODE_RAW, 1); 
			SetRenderRegion(rdat, rgn_w, rgn_h, rgn_x, rgn_y);
			RenderWarp(rdat,2);
	
			fetcher = GetPixelFetcher(state, drawable);
			Warp(rdat, fetcher, buffer, stride, bpp, 2);
			gimp_pixel_fetcher_destroy(fetcher);

			break;
		default:
			SetRenderRegion(rdat, rgn_w, rgn_h, rgn_x, rgn_y);
			RenderLow(rdat,0);
			Blend(rdat, buffer, buffer, stride, bpp);
			break;
	}

	 
	gimp_preview_draw_buffer (preview, buffer, stride);

	g_free (buffer);
}

