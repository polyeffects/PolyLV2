/*
  An LV2 plugin to calculate the sum of two signals.
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
#include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "uris.h"
#include "vector_op.h"

#define SUM_INPUT1 0
#define SUM_INPUT2 1
#define SUM_OUTPUT 2

typedef struct {
	const float* input1;
	const float* input2;
	float*       output;
	uint32_t     input1_is_cv;
	uint32_t     input2_is_cv;
	uint32_t     output_is_cv;
	URIs         uris;
} Sum;

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
	Sum* plugin = (Sum*)instance;

	switch (port) {
	case SUM_INPUT1:
		plugin->input1 = (const float*)data;
		break;
	case SUM_INPUT2:
		plugin->input2 = (const float*)data;
		break;
	case SUM_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}
static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	Sum*     plugin = (Sum*)instance;
	uint32_t ret    = 0;
	for (const LV2_Options_Option* o = options; o->key; ++o) {
		if (o->context != LV2_OPTIONS_PORT) {
			ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
		} else if (o->key != plugin->uris.morph_currentType) {
			ret |= LV2_OPTIONS_ERR_BAD_KEY;
		} else if (o->type != plugin->uris.atom_URID) {
			ret |= LV2_OPTIONS_ERR_BAD_VALUE;
		} else {
			LV2_URID port_type = *(const LV2_URID*)(o->value);
			if (port_type != plugin->uris.lv2_ControlPort &&
			    port_type != plugin->uris.lv2_CVPort) {
				ret |= LV2_OPTIONS_ERR_BAD_VALUE;
				continue;
			}
			switch (o->subject) {
			case SUM_INPUT1:
				plugin->input1_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case SUM_INPUT2:
				plugin->input2_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			default:
				ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
			}
		}
	}
	plugin->output_is_cv = plugin->input1_is_cv || plugin->input2_is_cv;
	return ret;
}

static uint32_t
options_get(LV2_Handle          instance,
            LV2_Options_Option* options)
{
	const Sum* plugin = (const Sum*)instance;
	uint32_t   ret    = 0;
	for (LV2_Options_Option* o = options; o->key; ++o) {
		if (o->context != LV2_OPTIONS_PORT || o->subject != SUM_OUTPUT) {
			ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
		} else if (o->key != plugin->uris.morph_currentType) {
			ret |= LV2_OPTIONS_ERR_BAD_KEY;
		} else {
			o->size  = sizeof(LV2_URID);
			o->type  = plugin->uris.atom_URID;
			o->value = (plugin->output_is_cv
			            ? &plugin->uris.lv2_CVPort
			            : &plugin->uris.lv2_ControlPort);
		}
	}
	return ret;
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Sum* plugin = (Sum*)malloc(sizeof(Sum));

	if (plugin) {
		plugin->input1_is_cv = 0;
		plugin->input2_is_cv = 0;
		plugin->output_is_cv = 0;

		map_uris(&plugin->uris, features);
	}

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	const Sum* const   plugin = (Sum*)instance;
	const float* const input1 = plugin->input1;
	const float* const input2 = plugin->input2;
	float* const       output = plugin->output;

	VECTOR_OP(+, output,
	          input1, plugin->input1_is_cv,
	          input2, plugin->input2_is_cv);
}

static const void*
extension_data(const char* uri)
{
	static const LV2_Options_Interface options = { options_get, options_set };
	if (!strcmp(uri, LV2_OPTIONS__interface)) {
		return &options;
	}
	return NULL;
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/blop/sum",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
