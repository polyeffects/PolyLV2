/*
  An LV2 plugin for calibrating v/oct pitch
  Copyright 2021 loki Davison for Poly Effects

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
#include <stdbool.h>
#include <math.h>
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <string.h>
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define CAL_OFFSET   0
#define CAL_SCALE   1
#define CAL_INPUT   2
#define CAL_OUTPUT 3
#define CAL_MEASURE   4 // control rate
#define CAL_MIN   5
#define CAL_MAX   6

typedef struct {

	const float* offset;
	const float* scale;
	const float* measure;
	const float* input;
	float* min_level;
	float* max_level;
	float* output;

	float prev_min_level;
	float prev_max_level;

	bool prev_measure;
} Cal;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Cal* plugin = (Cal*)instance;

	switch (port) {
		case CAL_OFFSET:
			plugin->offset = (const float*)data;
			break;
		case CAL_SCALE:
			plugin->scale = (const float*)data;
			break;
		case CAL_INPUT:
			plugin->input = (const float*)data;
			break;
		case CAL_OUTPUT:
			plugin->output = (float*)data;
			break;
		case CAL_MEASURE:
			plugin->measure = (const float*)data; // control rate input
			break;
		case CAL_MIN:
			plugin->min_level = (float*)data;
			break;
		case CAL_MAX:
			plugin->max_level = (float*)data;
			break;
	}
}


static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Cal* plugin = (Cal*)malloc(sizeof(Cal));
	if (!plugin) {
		return NULL;
	}

    plugin->prev_min_level        = 0.0;
    plugin->prev_max_level        = 0.0;
    plugin->prev_measure        = false;

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Cal* plugin = (Cal*)instance;

	const float offset = *(plugin->offset);
	const float scale = *(plugin->scale);
	const float measure = *(plugin->measure);
	const float* const input = plugin->input;
	float* const min_level = plugin->min_level;
	float* const max_level = plugin->max_level;
	float* const output = plugin->output;

	bool prev_measure = plugin->prev_measure;
	float prev_min_level = plugin->prev_min_level;
	float prev_max_level = plugin->prev_max_level;


	for (uint32_t s = 0; s < sample_count; ++s) {
		if (measure > 0.4){
			if (!prev_measure){
				prev_min_level = input[s];
				prev_max_level = input[s];
				prev_measure = true;
			} else {
				if (input[s] > prev_max_level){
					prev_max_level = input[s];
				}
				if (input[s] < prev_min_level){
					prev_min_level = input[s];
				}
			}
		} else {
			prev_measure = false;
		}
		output[s] = offset + (scale * input[s]);
	}

	min_level[0] = prev_min_level;
	max_level[0] = prev_max_level;
	plugin->prev_measure = prev_measure;
	plugin->prev_min_level = prev_min_level;
	plugin->prev_max_level = prev_max_level;
}

static void
run_out(LV2_Handle instance,
    uint32_t   sample_count)
{
	Cal* plugin = (Cal*)instance;

	const float offset = *(plugin->offset) * 0.2f;
	const float scale = *(plugin->scale);
	const float* const input = plugin->input;
	float* const output = plugin->output;

	for (uint32_t s = 0; s < sample_count; ++s) {
		output[s] = offset + (scale * input[s]);
	}
}



static const LV2_Descriptor descriptor0 = {
	"http://polyeffects.com/lv2/pitch_cal#in",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor1 = {
	"http://polyeffects.com/lv2/pitch_cal#out",
	instantiate,
	connect_port,
	NULL,
	run_out,
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
		case 1:
			return &descriptor1;
		default: return NULL;
	}
}
