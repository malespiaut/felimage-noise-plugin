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

#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"


static int GetByName(const char *value, int def, const char **names) {
	int i;

	i = 0;
	while (*names) {
		if (!strcasecmp(*names, value)) {
			return i;
		}
		names++;
		i++;
	}

	return def;
}


#define GET_INT(VALUE, MIN, MAX) (tmp_i = (int)atoi(VALUE), tmp_i<MIN?MIN:(tmp_i>MAX?MAX:tmp_i))
#define GET_FLOAT(VALUE, MIN, MAX) (tmp_f = (float)atof(VALUE), tmp_f<MIN?MIN:(tmp_f>MAX?MAX:tmp_f))
#define GET_UNBOUND_INT(VALUE) ((int)atoi(VALUE))
#define GET_UNBOUND_FLOAT(VALUE) ((float)atof(VALUE))
#define GET_BOOL(VALUE) (strchr("yYtT1",VALUE[0]) != NULL)

const char *keywords[]={
	"seed", "size_x", "size_y", "octaves", "lacunarity", "hurst",
	"frequency", "shift", "mapping", "basis", "color_src", 
	"reverse", "function", "phase", "multifractal", 
	"channel_r", "channel_g", "channel_b", "channel_a",
	"warp_size_x", "warp_size_y", "warp_caustics", "warp_quality",
	"edge_action", "gradient", "pinch", "bias", "gain",NULL
};

static void SetValue(const char *key, const char *value, PluginState *state) {
	float tmp_f;
	int v;

	v = GetByName(key, -1, keywords);
	
	switch(v) {

		case 0: state->seed = GET_UNBOUND_INT(value);
			state->random_seed = 0;
			break;
		case 1: state->size_x = GET_FLOAT(value,1,1000000);
			break;
		case 2: state->size_y = GET_FLOAT(value,1,1000000);
			break;
		case 3: state->octaves = GET_FLOAT(value,1,15);
			break;
		case 4: state->lacunarity = GET_FLOAT(value,1,10);
			break;
		case 5: state->hurst = GET_FLOAT(value,0,2);
			break;
		case 6: state->frequency = GET_FLOAT(value,0.001, 1000);
			break;
		case 7: state->shift = GET_FLOAT(value, 0, 100);
			break;
		case 8: state->mapping = GetByName(value, state->mapping, mapping_names); 
			break;
		case 9: state->basis = GetByName(value, state->basis, basis_names);
			break;
		case 10:state->color_src = GetByName(value, state->color_src, color_src_names);
			break;
		case 11:state->reverse = GET_BOOL(value);
			break;
		case 12:state->function=GetByName(value,state->function, function_names);
			break;
		case 13:state->phase = GET_UNBOUND_FLOAT(value);
			state->ign_phase = 0;
			break;
		case 14:state->multifractal=GetByName(value,state->multifractal, multifractal_names);
			break;
		case 15:state->channel[0] = GetByName(value,state->channel[0], color_channel_source_names);
			state->color_src = COL_CHANNELS;
			break;
		case 16:state->channel[1] = GetByName(value,state->channel[1], color_channel_source_names);
			state->color_src = COL_CHANNELS;
			break;
		case 17:state->channel[2] = GetByName(value,state->channel[2], color_channel_source_names);
			state->color_src = COL_CHANNELS;
			break;
		case 18:state->channel[3] = GetByName(value,state->channel[3], alpha_channel_source_names);
			state->color_src = COL_CHANNELS;
			break;
		case 19:state->warp_x_size = GET_FLOAT(value,1,1000000); 
			state->color_src = COL_WARP;
			break;
		case 20:state->warp_y_size = GET_FLOAT(value,1,1000000); 
			state->color_src = COL_WARP;
			break;
		case 21:state->warp_caustics = GET_FLOAT(value,-100,100);
			state->color_src = COL_WARP;
			break;
		case 22:state->warp_quality = GetByName(value,state->warp_quality, warp_quality_names);
			state->color_src = COL_WARP;
			break;
		case 23:state->edge_action = GetByName(value,state->edge_action, edge_action_names);
			break;
		case 24:StoreGradientName(state, value);
			state->color_src = COL_GRADIENT;
			break;
		case 25:state->pinch = GET_FLOAT(value,-1,1);
			break;		
		case 26:state->bias = GET_FLOAT(value,-1,1);
			break;		
		case 27:state->gain = GET_FLOAT(value,-1,1);
			break;		
			
	}
}

/* may modify "l" */
static void ParseIncremental(PluginState *state, char *l) {
	char **tokens;
	g_strchug(l);


	if (!l[0] || l[0] == '#') return;

	g_strchomp(l);

	tokens = g_strsplit_set(l, ":", 2);
	
	g_strstrip(tokens[0]);
	g_strstrip(tokens[1]);

	SetValue(tokens[0], tokens[1], state);

	g_strfreev(tokens);
}

static void PostParse(PluginState *state) {
	state->linked_sizes = (state->size_x == state->size_y)?1:0;
	state->linked_warp_sizes = (state->warp_x_size == state->warp_y_size)?1:0;
}



void ParseConfig(PluginState *state, const char *cfg) {
	char **lines;
	int n;
	char *loc;

	lines = g_strsplit_set(cfg, ",\n", -1);

	SetStateToDefaults(state);
	
	loc = setlocale(LC_NUMERIC,NULL);
	setlocale(LC_NUMERIC,"C");

	for (n = 0; lines[n]; n++) {
		ParseIncremental(state, lines[n]);
	}

	setlocale(LC_NUMERIC,loc);
	PostParse(state);

	g_strfreev(lines);
}


int LoadConfig(const char *filename, PluginState *state){
	FILE *file;
	char *buffer;
	char *loc;

	file = fopen(filename,"rt");
	if (!file) return -1;

	SetStateToDefaults(state);

	buffer = g_malloc(1024);
	
	loc = setlocale(LC_NUMERIC,NULL);
	setlocale(LC_NUMERIC,"C");

	while (fgets(buffer, 1024, file)) {
		ParseIncremental(state, buffer);
	}
	
	setlocale(LC_NUMERIC,loc);
	PostParse(state);

	g_free(buffer);

	fclose(file);

	return 0;
}
