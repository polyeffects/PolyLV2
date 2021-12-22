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

#define SEQUENCER_MAX_INPUTS 16

#define SEQUENCER_BACK_GATE      0
#define SEQUENCER_PLAY           1
#define SEQUENCER_LOOP_POINT        2
#define SEQUENCER_RESET             3
#define SEQUENCER_OCTAVE				4
#define SEQUENCER_VALUE_START       5
#define SEQUENCER_OUTPUT            (SEQUENCER_MAX_INPUTS + 5)
#define SEQUENCER_CURRENT_STEP_OUT      (SEQUENCER_MAX_INPUTS + 6) // so we can show step in UI
#define SEQUENCER_BPM				(SEQUENCER_MAX_INPUTS + 7)
#define SEQUENCER_NOTE_LENGTH		(SEQUENCER_MAX_INPUTS + 8)

#define clamp(x, lower, upper) (fmax(lower, fmin(x, upper)))

typedef struct {
	const float* back_gate;
	const float* play;
	const float* loop_steps;
	const float* reset;
	const float* octave;
	const float* bpm;
	const float* note_length;
	const float* values[SEQUENCER_MAX_INPUTS];
	float*       output;
	float*       current_step_out;
	float        srate;
	float        inv_srate;
	float        last_back_gate;
	float        last_trigger;
	unsigned int step_index;
	int samples_elapsed;
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
	case SEQUENCER_BACK_GATE:
		plugin->back_gate = (const float*)data;
		break;
	case SEQUENCER_PLAY:
		plugin->play = (const float*)data;
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
	case SEQUENCER_OCTAVE:
		plugin->octave = (const float*)data;
		break;
	case SEQUENCER_BPM:
		plugin->bpm = (const float*)data;
		break;
	case SEQUENCER_NOTE_LENGTH:
		plugin->note_length = (const float*)data;
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

	plugin->samples_elapsed    = 0;
	plugin->step_index   = 0;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Sequencer* plugin = (Sequencer*)instance;

	/* back_gate */
	const float* back_gate = plugin->back_gate;

	/* Step play */
	const float* play = plugin->play;

	/* Reset trigger */
	const float* reset = plugin->reset;

	/* Loop Steps */
	const float loop_steps = *(plugin->loop_steps);


	/* OCTAVE, filter coefficient */
	const float octave = *(plugin->octave);
	float bpm = *(plugin->bpm);
	float note_length = *(plugin->note_length);

	// given current note length and bpm, calculate samples per step
	// 48000 per second / 120 bpm = 0.5 seconds per beat, 48k * 0.5 samples per beat at 1/4 notes  (1/4 / 1/8 == 2)
	note_length = clamp(note_length, 0.03125, 1.0);
	bpm = clamp(bpm, 10, 320);
   
	const int samples_per_step = (int) (((60.0 / bpm) * plugin->srate) / (0.25 / note_length));
	/* const int samples_per_step = 24000; */

	/* Step Values */
	float values[SEQUENCER_MAX_INPUTS];

	/* Output */
	float* output = plugin->output;
	float* current_step_out = plugin->current_step_out;

	unsigned int  step_index = plugin->step_index;
	unsigned int  loop_index = LRINTF(loop_steps);
	int  samples_elapsed = plugin->samples_elapsed;
	int           i;

	loop_index = loop_index == 0 ?  1 : loop_index;
	loop_index = (loop_index > SEQUENCER_MAX_INPUTS)
		? SEQUENCER_MAX_INPUTS
		: loop_index;

	for (i = 0; i < SEQUENCER_MAX_INPUTS; i++) {
		values[i] = *(plugin->values[i]);
	}

	for (uint32_t s = 0; s < sample_count; ++s) {
		samples_elapsed++;
		if (reset[s] >= 0.4f) {
			step_index = 0;
			samples_elapsed = 0;
		}
		else {
			if (play[s] >= 0.4f || back_gate[s] >= 0.4f) {
				if (samples_elapsed > samples_per_step){
					if (play[s] >= 0.40f) {
						step_index++;
						if (step_index >= loop_index) {
							step_index = 0;
						}
					}
					if (back_gate[s] >= 0.4f) {
						if (step_index == 0){
							step_index = loop_index-1;
						}
						else {
							step_index--;
						}
					}

					samples_elapsed = 0;
				}
			}
		}	

		output[s] = ((values[step_index] / 12.0) + octave) * 0.2 ; // -1 to 1 instead of  -5 to 5
	}

	plugin->step_index   = step_index;
	plugin->samples_elapsed = samples_elapsed; 
	current_step_out[0] = (float) step_index;
}

static const LV2_Descriptor descriptor = {
	URI_PREFIX "/note_sequencer_bpm",
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
