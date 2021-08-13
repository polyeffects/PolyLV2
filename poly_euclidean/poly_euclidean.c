/*
  A euclidean euclidean LV2 plugin 
  Copyright 2021 Loki Davison 
  Poly Effects

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
#include <stdbool.h>
#include <stdlib.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "math_func.h"
#include "common.h"

#define TRIGGER_LENGTH 48 // 1 ms at 48kHz

/*
 *
 * steps beats shift for 4 steps
 * is enabled x 4
 * cv out: current step out x 4
 * cv in: step forward / step backwards
 * cv out:  is beat 1 x 4
 * cv reset
 * control reset
 *
 * */

#define EUCLIDEAN_NUM_TRACKS 4
#define EUCLIDEAN_MAX_STEPS 32

#define EUCLIDEAN_BACK_TRIGGER      0
#define EUCLIDEAN_TRIGGER           1
#define EUCLIDEAN_RESET             2
#define EUCLIDEAN_STEPS1             3
#define EUCLIDEAN_STEPS2             4
#define EUCLIDEAN_STEPS3             5
#define EUCLIDEAN_STEPS4             6
#define EUCLIDEAN_BEATS1             7
#define EUCLIDEAN_BEATS2             8
#define EUCLIDEAN_BEATS3             9
#define EUCLIDEAN_BEATS4             10
#define EUCLIDEAN_SHIFT1             11
#define EUCLIDEAN_SHIFT2             12
#define EUCLIDEAN_SHIFT3             13
#define EUCLIDEAN_SHIFT4             14
#define EUCLIDEAN_IS_ENABLED1         15
#define EUCLIDEAN_IS_ENABLED2             16
#define EUCLIDEAN_IS_ENABLED3             17
#define EUCLIDEAN_IS_ENABLED4             18
#define EUCLIDEAN_OUT1         19
#define EUCLIDEAN_OUT2             20
#define EUCLIDEAN_OUT3             21
#define EUCLIDEAN_OUT4             22
#define EUCLIDEAN_IS_THE_ONE1         23
#define EUCLIDEAN_IS_THE_ONE2         24 
#define EUCLIDEAN_IS_THE_ONE3         25
#define EUCLIDEAN_IS_THE_ONE4        26
#define EUCLIDEAN_CURRENT_STEP_OUT1    27 
#define EUCLIDEAN_CURRENT_STEP_OUT2    28
#define EUCLIDEAN_CURRENT_STEP_OUT3   29
#define EUCLIDEAN_CURRENT_STEP_OUT4  30


typedef struct {
	// ports
	const float* back_trigger;
	const float* trigger;
	const float* reset;

	const float* steps[EUCLIDEAN_NUM_TRACKS];
	const float* beats[EUCLIDEAN_NUM_TRACKS];
	const float* shift[EUCLIDEAN_NUM_TRACKS];
	const float* is_enabled[EUCLIDEAN_NUM_TRACKS];

	float*	     out_trigger[EUCLIDEAN_NUM_TRACKS];
	float*       is_the_one[EUCLIDEAN_NUM_TRACKS];
	float*       current_step_out[EUCLIDEAN_NUM_TRACKS];

	// internal
	bool        prev_back_trigger;
	bool        prev_trigger;
	unsigned int step_index[EUCLIDEAN_NUM_TRACKS];

	unsigned int trigger_count_down[EUCLIDEAN_NUM_TRACKS];
	unsigned int is_the_one_count_down[EUCLIDEAN_NUM_TRACKS];

	bool		 beats_array[EUCLIDEAN_NUM_TRACKS][EUCLIDEAN_MAX_STEPS];
	bool		 shifted_array[EUCLIDEAN_NUM_TRACKS][EUCLIDEAN_MAX_STEPS];
} Euclidean;

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
	Euclidean* plugin = (Euclidean*)instance;

	switch (port) {
	case EUCLIDEAN_BACK_TRIGGER:
		plugin->back_trigger = (const float*)data;
		break;
	case EUCLIDEAN_TRIGGER:
		plugin->trigger = (const float*)data;
		break;
	case EUCLIDEAN_RESET:
		plugin->reset = (const float*)data;
		break;
	case EUCLIDEAN_STEPS1:          
		plugin->steps[0] = (const float*)data;
		break;
	case EUCLIDEAN_STEPS2:          
		plugin->steps[1] = (const float*)data;
		break;
	case EUCLIDEAN_STEPS3:          
		plugin->steps[2] = (const float*)data;
		break;
	case EUCLIDEAN_STEPS4:          
		plugin->steps[3] = (const float*)data;
		break;
	case EUCLIDEAN_BEATS1:          
		plugin->beats[0] = (const float*)data;
		break;
	case EUCLIDEAN_BEATS2:          
		plugin->beats[1] = (const float*)data;
		break;
	case EUCLIDEAN_BEATS3:          
		plugin->beats[2] = (const float*)data;
		break;
	case EUCLIDEAN_BEATS4:          
		plugin->beats[3] = (const float*)data;
		break;
	case EUCLIDEAN_SHIFT1:          
		plugin->shift[0] = (const float*)data;
		break;
	case EUCLIDEAN_SHIFT2:          
		plugin->shift[1] = (const float*)data;
		break;
	case EUCLIDEAN_SHIFT3:          
		plugin->shift[2] = (const float*)data;
		break;
	case EUCLIDEAN_SHIFT4:          
		plugin->shift[3] = (const float*)data;
		break;
	case EUCLIDEAN_IS_ENABLED1:     
		plugin->is_enabled[0] = (const float*)data;
		break;
	case EUCLIDEAN_IS_ENABLED2:     
		plugin->is_enabled[1] = (const float*)data;
		break;
	case EUCLIDEAN_IS_ENABLED3:     
		plugin->is_enabled[2] = (const float*)data;
		break;
	case EUCLIDEAN_IS_ENABLED4:     
		plugin->is_enabled[3] = (const float*)data;
		break;

		// Outputs
	case EUCLIDEAN_OUT1:         
		plugin->out_trigger[0] = (float*)data;
		break;
	case EUCLIDEAN_OUT2:          
		plugin->out_trigger[1] = (float*)data;
		break;
	case EUCLIDEAN_OUT3:          
		plugin->out_trigger[2] = (float*)data;
		break;
	case EUCLIDEAN_OUT4:          
		plugin->out_trigger[3] = (float*)data;
		break;
	case EUCLIDEAN_IS_THE_ONE1:   
		plugin->is_the_one[0] = (float*)data;
		break;
	case EUCLIDEAN_IS_THE_ONE2:    
		plugin->is_the_one[1] = (float*)data;
		break;
	case EUCLIDEAN_IS_THE_ONE3:   
		plugin->is_the_one[2] = (float*)data;
		break;
	case EUCLIDEAN_IS_THE_ONE4:   
		plugin->is_the_one[3] = (float*)data;
		break;
	case EUCLIDEAN_CURRENT_STEP_OUT1:    
		plugin->current_step_out[0] = (float*)data;
		break;
	case EUCLIDEAN_CURRENT_STEP_OUT2:   
		plugin->current_step_out[1] = (float*)data;
		break;
	case EUCLIDEAN_CURRENT_STEP_OUT3:  
		plugin->current_step_out[2] = (float*)data;
		break;
	case EUCLIDEAN_CURRENT_STEP_OUT4: 
		plugin->current_step_out[3] = (float*)data;
		break;
	default:
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Euclidean* plugin = (Euclidean*)malloc(sizeof(Euclidean));
	if (!plugin) {
		return NULL;
	}

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	Euclidean* plugin = (Euclidean*)instance;

	plugin->prev_back_trigger    = false;
	plugin->prev_trigger = false;
	for (int i = 0; i < EUCLIDEAN_NUM_TRACKS; i++) {
		plugin->step_index[i]   = 0;
		plugin->trigger_count_down[i]   = 0;
		plugin->is_the_one_count_down[i]   = 0;
	}
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Euclidean* plugin = (Euclidean*)instance;
	/*
	 * calculate beats for each track
	 *
	 *  if trigger, find if current beat is a beat
	 *
	 *  if it is a beat, we set the output trigger to 1.0, else 0.0
	 *
	 */

	/* back_trigger */
	const float* back_trigger = plugin->back_trigger;

	/* Step Trigger */
	const float* trigger = plugin->trigger;

	/* Reset trigger */
	const float* reset = plugin->reset;

	/* Values */
	int beats[EUCLIDEAN_NUM_TRACKS];
	int steps[EUCLIDEAN_NUM_TRACKS];
	int shift[EUCLIDEAN_NUM_TRACKS];
	bool is_enabled[EUCLIDEAN_NUM_TRACKS];

	/* /1* Output *1/ */
	/* float* out_trigger[EUCLIDEAN_NUM_TRACKS] = plugin->out_trigger; */
	/* float* is_the_one[EUCLIDEAN_NUM_TRACKS] = plugin->is_the_one; */
	/* float* current_step_out[EUCLIDEAN_NUM_TRACKS] = plugin->current_step_out; */

	bool prev_back_trigger    = plugin->prev_back_trigger;
	bool prev_trigger = plugin->prev_trigger;

	unsigned int  step_index[EUCLIDEAN_NUM_TRACKS]; 
	unsigned int  trigger_count_down[EUCLIDEAN_NUM_TRACKS]; 
	unsigned int  is_the_one_count_down[EUCLIDEAN_NUM_TRACKS]; 

	for (int i = 0; i < EUCLIDEAN_NUM_TRACKS; i++) {
		beats[i] = (int) *(plugin->beats[i]);
		steps[i] = (int) *(plugin->steps[i]);
		shift[i] = (int) *(plugin->shift[i]);
		is_enabled[i] = *(plugin->is_enabled[i]) > 0.9;
		step_index[i] = plugin->step_index[i]; // this is dumb but I can't remember how to do pointers properly today...
		trigger_count_down[i] = plugin->trigger_count_down[i]; // this is dumb but I can't remember how to do pointers properly today...
		is_the_one_count_down[i] = plugin->is_the_one_count_down[i]; // this is dumb but I can't remember how to do pointers properly today...
	}

	int bucket;

	for (int t = 0; t < EUCLIDEAN_NUM_TRACKS; t++){
		bucket = 0;
	
		for (int i = 0; i < steps[t]; i++) {
			bucket += beats[t];
			if (bucket >= steps[t]){
				bucket -= steps[t];
				plugin->beats_array[t][steps[t]-1-i] = true;
			} else {
				plugin->beats_array[t][steps[t]-1-i] = false;
			}
		}

		for (int i = 0; i < steps[t]; i++) {
			plugin->shifted_array[t][(i+shift[t]) % steps[t]] = plugin->beats_array[t][i];
		}
	}

	for (uint32_t s = 0; s < sample_count; ++s) {

		if (reset[s] > 0.4) {
			for (int i = 0; i < EUCLIDEAN_NUM_TRACKS; i++) {
				step_index[i] = 0;
				prev_trigger = false;
			}
		} else {
			if (trigger[s] > 0.4f && !(prev_trigger)) {
				prev_trigger = true;
				for (int i = 0; i < EUCLIDEAN_NUM_TRACKS; i++) {
					if (is_enabled[i] > 0.9f){
						step_index[i]++;
						if (step_index[i] >= steps[i]) {
							step_index[i] = 0;
						}
						if (plugin->shifted_array[i][step_index[i]]){
							trigger_count_down[i] = TRIGGER_LENGTH;
						}
						if (step_index[i] == 0){
							is_the_one_count_down[i] = TRIGGER_LENGTH;
						}
					}
				}
			} 
			else if (trigger[s] <= 0.08){ // 0.4 volts in Hector
				prev_trigger = false;
			}

			if (back_trigger[s] > 0.4f && !(prev_back_trigger)) {
				prev_back_trigger = true;
				for (int i = 0; i < EUCLIDEAN_NUM_TRACKS; i++) {
					if (is_enabled[i] > 0.9f){
						if (step_index[i] == 0){
							step_index[i] = steps[i]-1;
						}
						else {
							step_index[i]--;
						}
						if (plugin->shifted_array[i][step_index[i]]){
							trigger_count_down[i] = TRIGGER_LENGTH;
						}
						if (step_index[i] == 0){
							is_the_one_count_down[i] = TRIGGER_LENGTH;
						}
					}
				}
			} else if (back_trigger[s] <= 0.08){ // 0.25 volts in Hector
				prev_back_trigger = false;
			}
		}
		for (int i = 0; i < EUCLIDEAN_NUM_TRACKS; i++) {
			if (trigger_count_down[i] > 0){
				trigger_count_down[i] = trigger_count_down[i] - 1;
				plugin->out_trigger[i][s] = 1.0f;
			}
			else {
				plugin->out_trigger[i][s] = 0.0f;
			}

			if (is_the_one_count_down[i] > 0){
				is_the_one_count_down[i] = is_the_one_count_down[i] - 1;
				plugin->is_the_one[i][s] = 1.0f;
			}
			else {
				plugin->is_the_one[i][s] = 0.0f;
			}

		}
	}

	plugin->prev_back_trigger    = prev_back_trigger;
	plugin->prev_trigger = prev_trigger;
	
	for (int i = 0; i < EUCLIDEAN_NUM_TRACKS; i++) {
		plugin->step_index[i]   = step_index[i];
		plugin->current_step_out[i][0] = (float) step_index[i];
		plugin->trigger_count_down[i]   = trigger_count_down[i];
		plugin->is_the_one_count_down[i]   = is_the_one_count_down[i];
	}
}

static const LV2_Descriptor descriptor = {
	URI_PREFIX "/euclidean",
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
