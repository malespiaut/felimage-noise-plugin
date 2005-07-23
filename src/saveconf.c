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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"


int SaveConfig(const char *filename, PluginState *state) {
	FILE *file;
	char *gradient;

	file = fopen(filename,"wt");
	if (!file) return -1;

	fprintf(file,PRESET_HEADER " v" PACKAGE_VERSION " for the GIMP\n");
	fprintf(file,"# Preset file\n\n");
	if (!state->random_seed) {
		fprintf(file,"seed:          %i\n",state->seed);
	}
	fprintf(file,"size_x:        %f\n",state->size_x);
	fprintf(file,"size_y:        %f\n",state->size_y);
	fprintf(file,"octaves:       %f\n",state->octaves);
	fprintf(file,"lacunarity:    %f\n",state->lacunarity);
	fprintf(file,"hurst:         %f\n",state->hurst);
	fprintf(file,"frequency:     %f\n",state->frequency);
	fprintf(file,"shift:         %f\n",state->shift);
	fprintf(file,"mapping:       %s\n",mapping_names[state->mapping]);
	fprintf(file,"basis:         %s\n",basis_names[state->basis]);
	fprintf(file,"reverse:       %s\n",state->reverse?"YES":"NO");
	fprintf(file,"function:      %s\n",function_names[state->function]);
	if (!state->ign_phase) {
		fprintf(file,"phase:         %f\n",state->phase);
	}
	fprintf(file,"multifractal:  %s\n",multifractal_names[state->multifractal]);
	fprintf(file,"edge_action:   %s\n",edge_action_names[state->edge_action]);

	fprintf(file,"pinch:         %f\n",state->pinch);
	fprintf(file,"bias:          %f\n",state->bias);
	fprintf(file,"gain:          %f\n",state->gain);

	fprintf(file,"color_src:     %s\n",color_src_names[state->color_src]);
	switch (state->color_src) {
		case COL_FG_BG: break;
		case COL_GRADIENT:
				if (state->gradient) {
					gradient = GetGradientName(state->gradient);
					fprintf(file,"gradient:      %s\n",gradient);
					g_free(gradient);
				}
				break;
		case COL_CHANNELS: 
				fprintf(file,"channel_r:     %s\n",color_channel_source_names[state->channel[0]]);
				fprintf(file,"channel_g:     %s\n",color_channel_source_names[state->channel[1]]);
				fprintf(file,"channel_b:     %s\n",color_channel_source_names[state->channel[2]]);
				fprintf(file,"channel_a:     %s\n",alpha_channel_source_names[state->channel[3]]);
				break;
		case COL_WARP:
				fprintf(file,"warp_size_x:   %f\n",state->warp_x_size);
				fprintf(file,"warp_size_y:   %f\n",state->warp_y_size);
				fprintf(file,"warp_caustics: %f\n",state->warp_caustics);
				fprintf(file,"warp_quality:  %s\n",warp_quality_names[state->warp_quality]);
				break;
	}

	
	fprintf(file,"\n");

	fclose(file);

	return 0;
}
