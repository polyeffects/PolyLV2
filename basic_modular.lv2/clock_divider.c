/*
 * Copyright 2022 Loki Davison Poly Effects 

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
#include <stdbool.h>
// #include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <string.h>
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define AMP_DIVIDER_PARAM   0
#define AMP_INPUT  1
#define AMP_OUTPUT 2

typedef struct {
	const float* divider;
	const float* input;
	float*       output;

	bool prev_trigger;	
	bool on_state;	
	int pulse_count;
} Amp;

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
	Amp* plugin = (Amp*)instance;

	switch (port) {
	case AMP_DIVIDER_PARAM:
		plugin->divider = (const float*)data;
		break;
	case AMP_INPUT:
		plugin->input = (const float*)data;
		break;
	case AMP_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Amp* plugin = (Amp*)malloc(sizeof(Amp));
	if (!plugin) {
		return NULL;
	}
	plugin->prev_trigger = false;	
	plugin->on_state = false;	
	plugin->pulse_count = 0;

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Amp* plugin = (Amp*)instance;

	const float divider = *(plugin->divider);
	/* Input */
	const float* trigger = plugin->input;

	/* Output */
	float* output = plugin->output;
	int pulse_count = plugin->pulse_count;

	for (uint32_t s = 0; s < sample_count; ++s) { 
		if (trigger[s] >= 0.4) {
			if (!(plugin->prev_trigger)){
				plugin->on_state = true;
				plugin->prev_trigger = true;
				pulse_count++;
				if (pulse_count >= divider){
					// this pulse goes through
					pulse_count = 0;
					output[s] = trigger[s];
				} else {
					output[s] = 0.0f;
				};
			} else {
				output[s] = trigger[s];
			}
		} else if (trigger[s] <= 0.05){ // 0.25 volts in Hector
			plugin->prev_trigger = false;
			plugin->on_state = false;
			output[s] = 0.0f;
		} else {
			output[s] = 0.0f;
		}
	} 
	plugin->pulse_count = pulse_count;
}


static const LV2_Descriptor descriptor = {
	"http://polyeffects.com/lv2/basic_modular#clock_divider",
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
	case 0:  return &descriptor;
	default: return NULL;
	}
}
