/*
  An LV2 plugin to simulate an analogue style step sequencer.
  Copyright 2020 Loki Davison 
  Copyright 2011 David Robillard
  Copyright 2002 Mike Rawes

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

#include <stdio.h>
#include <stdlib.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "math_func.h"
#include "common.h"
#include <stdbool.h>

#define SEQUENCER_MAX_INPUTS 16

#define SEQUENCER_BACK_TRIGGER      0
#define SEQUENCER_TRIGGER           1
#define SEQUENCER_LOOP_POINT        2
#define SEQUENCER_RESET             3
#define SEQUENCER_GLIDE				4
#define SEQUENCER_VALUE_START       5
#define SEQUENCER_OUTPUT            (SEQUENCER_MAX_INPUTS + 5)
#define SEQUENCER_CURRENT_STEP_OUT      (SEQUENCER_MAX_INPUTS + 6) // so we can show step in UI

typedef struct {
	const float* back_trigger;
	const float* trigger;
	const float* loop_steps;
	const float* reset;
	const float* glide;
	const float* values[SEQUENCER_MAX_INPUTS];
	float*       output;
	float*       current_step_out;
	float        srate;
	float        inv_srate;
	float        last_back_trigger;
	float        last_trigger;
	float        last_reset;
	float        last_value1;
	float        last_value2;
	unsigned int step_index;

	bool        prev_back_trigger;
	bool        prev_trigger;
} Sequencer;

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
	Sequencer* plugin = (Sequencer*)instance;


	switch (port) {
	case SEQUENCER_BACK_TRIGGER:
		plugin->back_trigger = (const float*)data;
		break;
	case SEQUENCER_TRIGGER:
		plugin->trigger = (const float*)data;
		break;
	case SEQUENCER_LOOP_POINT:
		plugin->loop_steps = (const float*)data;
		break;
	case SEQUENCER_OUTPUT:
		plugin->output = (float*)data;
		break;
	case SEQUENCER_CURRENT_STEP_OUT:
		plugin->current_step_out = (float*)data;
		break;
	case SEQUENCER_RESET:
		plugin->reset = (const float*)data;
		break;
	case SEQUENCER_GLIDE:
		plugin->glide = (const float*)data;
		break;
	default:
		if (port >= SEQUENCER_VALUE_START && port < SEQUENCER_OUTPUT) {
			plugin->values[port - SEQUENCER_VALUE_START] = (const float*)data;
		}
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Sequencer* plugin = (Sequencer*)malloc(sizeof(Sequencer));
	if (!plugin) {
		return NULL;
	}

	plugin->srate     = (float)sample_rate;
	plugin->inv_srate = 1.0f / plugin->srate;

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	Sequencer* plugin = (Sequencer*)instance;

	plugin->last_back_trigger    = 0.0f;
	plugin->last_trigger = 0.0f;
	plugin->last_reset = 0.0f;
	plugin->last_value1   = 0.0f;
	plugin->last_value2   = 0.0f;
	plugin->step_index   = 0;
	plugin->prev_back_trigger    = false;
	plugin->prev_trigger = false;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Sequencer* plugin = (Sequencer*)instance;

	/* back_trigger */
	const float* back_trigger = plugin->back_trigger;

	/* Step Trigger */
	const float* trigger = plugin->trigger;

	/* Reset trigger */
	const float* reset = plugin->reset;

	/* Loop Steps */
	const float loop_steps = *(plugin->loop_steps);


	/* Glide, filter coefficient */
	float glide = *(plugin->glide);

	/* Step Values */
	float values[SEQUENCER_MAX_INPUTS];

	/* Output */
	float* output = plugin->output;
	float* current_step_out = plugin->current_step_out;

	float last_back_trigger    = plugin->last_back_trigger;
	float last_trigger = plugin->last_trigger;
	float last_reset   = plugin->last_reset;
	float last_value1   = plugin->last_value1;
	float last_value2   = plugin->last_value2;

	bool prev_back_trigger    = plugin->prev_back_trigger;
	bool prev_trigger = plugin->prev_trigger;

	unsigned int  step_index = plugin->step_index;
	unsigned int  loop_index = LRINTF(loop_steps);
	int           i;

	loop_index = loop_index == 0 ?  1 : loop_index;
	loop_index = (loop_index > SEQUENCER_MAX_INPUTS)
		? SEQUENCER_MAX_INPUTS
		: loop_index;

	for (i = 0; i < SEQUENCER_MAX_INPUTS; i++) {
		values[i] = *(plugin->values[i]);
	}

   float fSlow0 = expf((0.0f - (plugin->inv_srate / glide)));
   float fSlow1 = (1.0f - fSlow0);

	for (uint32_t s = 0; s < sample_count; ++s) {
		if (reset[s] > 0.0f) {
			step_index = 0;
		} else {
			if (trigger[s] > 0.4f && !(prev_trigger)) {
				prev_trigger = true;
				step_index++;
				if (step_index >= loop_index) {
					step_index = 0;
				}
			}
			else if (trigger[s] <= 0.08){ // 0.25 volts in Hector
				prev_trigger = false;
			}

			if (back_trigger[s] > 0.4f && !(prev_back_trigger)) {
				prev_back_trigger = true;
				if (step_index == 0){
					step_index = loop_index-1;
				}
				else {
					step_index--;
				}
			} else if (back_trigger[s] <= 0.08){ // 0.25 volts in Hector
				prev_back_trigger = false;
			}
		}
		last_back_trigger    = back_trigger[s];
		last_trigger = trigger[s];
		last_reset = reset[s];

		last_value1 = ((fSlow0 * last_value2) + (fSlow1 * values[step_index]));
		last_value2 = last_value1;
		output[s] = last_value1;
	}

	plugin->last_back_trigger    = last_back_trigger;
	plugin->last_trigger = last_trigger;
	plugin->last_reset = last_reset;
	plugin->last_value1   = last_value1;
	plugin->last_value2   = last_value2;
	plugin->step_index   = step_index;
	plugin->prev_back_trigger    = prev_back_trigger;
	plugin->prev_trigger = prev_trigger;
	current_step_out[0] = (float) step_index;
}

static const LV2_Descriptor descriptor = {
	URI_PREFIX "/step_sequencer",
	instantiate,
	connect_port,
	activate,
	run,
	NULL,
	cleanup,
	NULL
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
