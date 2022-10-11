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

#define MATH_URI "http://polyeffects.com/lv2/basic_modular#"

#define TRIGGER_LENGTH 48 // 1 ms at 48kHz

#define DIFFERENCE_MINUEND    0
#define DIFFERENCE_SUBTRAHEND 1
#define DIFFERENCE_DIFFERENCE 2
#define DIFFERENCE_MINUEND_CV    3
#define DIFFERENCE_SUBTRAHEND_CV 4

#define TOGGLE_TRIGGER    0
#define TOGGLE_OUTPUT 1

typedef struct {
	const float* minuend;
	const float* subtrahend;
	const float* minuend_cv;
	const float* subtrahend_cv;
	float*       difference;

	int operator;	
	int trigger_countdown;	
	float srate;
	bool prev_trigger;	
	bool on_state;	
} Difference;

typedef enum {
	DIFFERENCE = 0,
	SUM,
	RATIO,
	PRODUCT,
	MAX,
	MIN,
	ABS,
} Operators;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*     data)
{
	Difference* plugin = (Difference*)instance;

	switch (port) {
	case DIFFERENCE_MINUEND:
		plugin->minuend = (const float*)data;
		break;
	case DIFFERENCE_SUBTRAHEND:
		plugin->subtrahend = (const float*)data;
		break;
	case DIFFERENCE_MINUEND_CV:
		plugin->minuend_cv = (const float*)data;
		break;
	case DIFFERENCE_SUBTRAHEND_CV:
		plugin->subtrahend_cv = (const float*)data;
		break;
	case DIFFERENCE_DIFFERENCE:
		plugin->difference = (float*)data;
		break;
	}
}

static void
connect_port_toggle(LV2_Handle instance,
             uint32_t   port,
             void*     data)
{
	Difference* plugin = (Difference*)instance;

	switch (port) {
	case TOGGLE_TRIGGER:
		plugin->minuend_cv = (const float*)data;
		break;
	case TOGGLE_OUTPUT:
		plugin->difference = (float*)data;
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Difference* plugin = (Difference*)malloc(sizeof(Difference));
	if (!plugin) {
		return NULL;
	}

	if (!strcmp (descriptor->URI, MATH_URI "difference")) {
		plugin->operator = DIFFERENCE;
	} else if (!strcmp (descriptor->URI, MATH_URI "sum")) {
		plugin->operator = SUM;
	} else if (!strcmp (descriptor->URI, MATH_URI "ratio")) {
		plugin->operator = RATIO;
	} else if (!strcmp (descriptor->URI, MATH_URI "product")) {
		plugin->operator = PRODUCT;
	} else if (!strcmp (descriptor->URI, MATH_URI "max")) {
		plugin->operator = MAX;
	} else if (!strcmp (descriptor->URI, MATH_URI "min")) {
		plugin->operator = MIN;
	} else if (!strcmp (descriptor->URI, MATH_URI "abs")) {
		plugin->operator = ABS;
    }
	plugin->prev_trigger = false;	
	plugin->on_state = false;	
	plugin->trigger_countdown = 0;	
	plugin->srate  = (float)sample_rate;

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	const Difference* const plugin     = (Difference*)instance;
	const float      minuend    = *(plugin->minuend);
	const float      subtrahend = *(plugin->subtrahend);
	const float* const      minuend_cv    = plugin->minuend_cv;
	const float* const      subtrahend_cv = plugin->subtrahend_cv;
	float* const            difference = plugin->difference;
	const int     operator = plugin->operator;

	if (operator == DIFFERENCE){
		for (uint32_t s = 0; s < sample_count; ++s) { 
			difference[s] = (minuend_cv[s]+minuend) - (subtrahend_cv[s]+subtrahend); 
		} 
	} else if (operator == SUM){
		for (uint32_t s = 0; s < sample_count; ++s) { 
			difference[s] = (minuend_cv[s]+minuend) + (subtrahend_cv[s]+subtrahend); 
		} 
	} else if (operator == RATIO){
		for (uint32_t s = 0; s < sample_count; ++s) { 
			difference[s] = (minuend_cv[s]+minuend) / (FLT_EPSILON+subtrahend_cv[s]+subtrahend); 
		} 
	} else if (operator == PRODUCT){
		for (uint32_t s = 0; s < sample_count; ++s) { 
			difference[s] = (minuend_cv[s]+minuend) * (subtrahend_cv[s]+subtrahend); 
		} 
	} else if (operator == MAX){
		for (uint32_t s = 0; s < sample_count; ++s) { 
			difference[s] = fmax(minuend_cv[s]+minuend, subtrahend_cv[s]+subtrahend); 
		} 
	} else if (operator == MIN){
		for (uint32_t s = 0; s < sample_count; ++s) { 
			difference[s] = fmin(minuend_cv[s]+minuend, subtrahend_cv[s]+subtrahend); 
		} 
	} else if (operator == ABS){
		for (uint32_t s = 0; s < sample_count; ++s) { 
			difference[s] = fabs(minuend_cv[s]+minuend); 
		} 
	}
}

static void
run_toggle(LV2_Handle instance,
    uint32_t   sample_count)
{
	Difference* const plugin     = (Difference*)instance;
	const float* const      trigger    = plugin->minuend_cv;
	float* const            out = plugin->difference;

	for (uint32_t s = 0; s < sample_count; ++s) { 
		if (trigger[s] >= 0.4) {
			if (!(plugin->prev_trigger)){
				plugin->on_state = !(plugin->on_state);
				plugin->prev_trigger = true;
			}
		} else if (trigger[s] <= 0.05){ // 0.25 volts in Hector
			plugin->prev_trigger = false;
		}
		out[s] = (float) (plugin->on_state);

	} 
}

static void
run_schmitt(LV2_Handle instance,
    uint32_t   sample_count)
{
	Difference* const plugin     = (Difference*)instance;
	const float* const      trigger    = plugin->minuend_cv;
	float* const            out = plugin->difference;

	for (uint32_t s = 0; s < sample_count; ++s) { 
		if (trigger[s] >= 0.4) { // 2 volt in Hector
			if (!(plugin->prev_trigger)){
				plugin->prev_trigger = true;
				plugin->trigger_countdown = TRIGGER_LENGTH;
			}
		} else if (trigger[s] <= 0.05){ // 0.25 volts in Hector
			plugin->prev_trigger = false;
		}
		if (plugin->trigger_countdown > 0){
			plugin->trigger_countdown--;
			out[s] = 1.0f;
		}
		else {
			out[s] = 0.0f;
		}
	} 
}

static void
run_tempo_ratio(LV2_Handle instance,
    uint32_t   sample_count)
{
	const Difference* const plugin     = (Difference*)instance;
	const float       a    = *(plugin->minuend);
	const float       b = *(plugin->subtrahend);
	const float       tempo_in    = *(plugin->minuend_cv);
	float* const      tempo_out = plugin->difference;

	tempo_out[0] = tempo_in / (a / b);

}

static void
run_trigger_to_gate(LV2_Handle instance,
    uint32_t   sample_count)
{
	Difference* const plugin     = (Difference*)instance;
	const float* const      trigger    = plugin->minuend_cv;
	float* const            out = plugin->difference;

	const float      tempo    = *(plugin->minuend);
	const float      gate_length = *(plugin->subtrahend);
	const float* const      extra_time = plugin->subtrahend_cv;

	for (uint32_t s = 0; s < sample_count; ++s) { 
		if (trigger[s] >= 0.4) { // 2 volt in Hector
			if (!(plugin->prev_trigger)){
				plugin->prev_trigger = true;
				plugin->trigger_countdown = (int) (((60.0 / tempo) * plugin->srate) / (0.25 / (gate_length+extra_time[s])));
			}
		} else if (trigger[s] <= 0.05){ // 0.25 volts in Hector
			plugin->prev_trigger = false;
		}
		if (plugin->trigger_countdown > 0){
			plugin->trigger_countdown--;
			out[s] = 1.0f;
		}
		else {
			out[s] = 0.0f;
		}
	} 
}


static const LV2_Descriptor descriptor0 = {
	MATH_URI "difference",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor1 = {
	MATH_URI "sum",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor2 = {
	MATH_URI "ratio",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor3 = {
	MATH_URI "product",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor4 = {
	MATH_URI "max",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor5 = {
	MATH_URI "min",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor6 = {
	MATH_URI "abs",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor7 = {
	MATH_URI "tempo_ratio",
	instantiate,
	connect_port,
	NULL,
	run_tempo_ratio,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor8 = {
	MATH_URI "toggle",
	instantiate,
	connect_port_toggle,
	NULL,
	run_toggle,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor9 = {
	MATH_URI "schmitt",
	instantiate,
	connect_port_toggle,
	NULL,
	run_schmitt,
	NULL,
	cleanup,
	NULL,
};

static const LV2_Descriptor descriptor10 = {
	MATH_URI "trigger_to_gate",
	instantiate,
	connect_port,
	NULL,
	run_trigger_to_gate,
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
		case 2:
			return &descriptor2;
		case 3:
			return &descriptor3;
		case 4:
			return &descriptor4;
		case 5:
			return &descriptor5;
		case 6:
			return &descriptor6;
		case 7:
			return &descriptor7;
		case 8:
			return &descriptor8;
		case 9:
			return &descriptor9;
		case 10:
			return &descriptor10;
		default:
			return NULL;
	}
}
