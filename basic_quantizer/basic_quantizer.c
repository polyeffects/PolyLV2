/*
 * Copyright 2020 Loki Davison Poly Effects 
 * originally based on BLOP
  Copyright 2011 David Robillard
  Copyright 2004 Mike Rawes

  This is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdbool.h>
// #include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <string.h>
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define MATH_URI "http://polyeffects.com/lv2/basic_quantizer"

#define QUANTIZER_INPUT    0
#define QUANTIZER_START_NOTES 1
#define QUANTIZER_END_NOTES 12
#define QUANTIZER_OUTPUT 13
#define QUANTIZER_CHANGED    14

typedef struct {
	const float* input;
	const float* notes_enabled[12];
	float*       output;
	float*       changed;

	float prev_out;	
} Quantizer;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

int mod_euclidean(int a, int b) {
  int m = a % b;
  if (m < 0) {
    m = (b < 0) ? m - b : m + b;
  }
  return m;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*     data)
{
	Quantizer* plugin = (Quantizer*)instance;

	switch (port) {
	case QUANTIZER_INPUT:
		plugin->input = (const float*)data;
		break;
	case QUANTIZER_OUTPUT:
		plugin->output = (float*)data;
		break;
	case QUANTIZER_CHANGED:
		plugin->changed = (float*)data;
		break;
	default:
		if (port >= QUANTIZER_START_NOTES && port <= QUANTIZER_END_NOTES){
			plugin->notes_enabled[port - QUANTIZER_START_NOTES] = (const float*)data;
		}
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Quantizer* plugin = (Quantizer*)malloc(sizeof(Quantizer));
	if (!plugin){
		return NULL;
	}
	plugin->prev_out = 0.0f;

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Quantizer* const plugin     = (Quantizer*)instance;
	const float* const     input    = plugin->input;
	/* const float* const      minuend_cv    = plugin->minuend_cv; */
	float* const            output = plugin->output;
	float* const            changed = plugin->changed;

	bool notes_enabled[12];
	// multiply by 12, then % 12 to convert to semitones from v/oct
	// search out from current semitone to find nearest enabled note
	//

	for (int i = 0; i < 12; i++){
		notes_enabled[i] = plugin->notes_enabled[i][0] >= 1.0f;
	}

	int in_note = 0;
	int cur_note = 0;
	int octave = 0;
	float prev_out = plugin->prev_out;


	for (uint32_t s = 0; s < sample_count; ++s) { 
		in_note = (int) fabs(roundf (input[s] * 60.0)) % 12; 	
		octave = truncf(input[s] * 5.0f);
		for (int i = 0; i < 12; i++){

			cur_note = mod_euclidean(in_note - i, 12);
			if (notes_enabled[cur_note]){
				break;
			}
			cur_note = mod_euclidean(in_note + i, 12);
			if (notes_enabled[cur_note]){
				break;
			}
		}
		if (input[s] > 0){
			output[s] = (octave + (cur_note / 12.0)) * 0.2 ;
		} else {
			output[s] = (octave - (cur_note / 12.0)) * 0.2;
		}
		if (output[s] == prev_out){
			changed[s] = 0.0f;
		} else {
			changed[s] = 1.0f;
		}
		prev_out = output[s];
	} 

	plugin->prev_out = prev_out;	
}



static const LV2_Descriptor descriptor0 = {
	MATH_URI,
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};


LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
		case 0:
			return &descriptor0;
		default:
			return NULL;
	}
}
