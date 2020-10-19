/*
  An LV2 plugin representing a simple mono amplifier.
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

#include <stdlib.h>
#include <math.h>
// #include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <string.h>
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define AMP_GAIN   0
#define AMP_INPUT  1
#define AMP_OUTPUT 2

typedef struct {
	const float* gain;
	const float* input;
	float*       output;
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
	case AMP_GAIN:
		plugin->gain = (const float*)data;
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

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Amp* plugin = (Amp*)instance;

	/* Gain (dB) */
	const float* gain = plugin->gain;

	/* Input */
	const float* input = plugin->input;

	/* Output */
	float* output = plugin->output;

	for (uint32_t s = 0; s < sample_count; ++s) {
		const float gn    = gain[s];
		const float scale = (float)expf((fmin(gn, 1) - 1) * 9.21f);

		output[s] = scale * input[s];
	}
}


static const LV2_Descriptor descriptor = {
	"http://polyeffects.com/lv2/basic_modular#amp",
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
