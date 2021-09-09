// based on  Mindi by Spencer Jackson
//cv_to_cc.c
#include "cv_to_cc.h"
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
#include<math.h>
#include<lv2.h>
#include<lv2/lv2plug.in/ns/ext/urid/urid.h>
#include<lv2/lv2plug.in/ns/ext/midi/midi.h>
#include<lv2/lv2plug.in/ns/ext/atom/util.h>
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

//midi stuff
#define MIDI_NOTE_OFF        0x80
#define MIDI_NOTE_ON         0x90
#define MIDI_KEYPRESSURE     0xA0
#define MIDI_CONTROL_CHANGE  0xB0
#define MIDI_PROGRAMCHANGE   0xC0
#define MIDI_CHANNELPRESSURE 0xD0
#define MIDI_PITCHBEND       0xE0
#define MIDI_SYSTEMMSG       0xF0

#define MIDI_TYPE_MASK       0xF0
#define MIDI_CHANNEL_MASK    0x0F
#define MIDI_DATA_MASK       0x7F
#define MIDI_PITCH_CENTER    0x2000
#define MIDI_SUSTAIN_PEDAL   0x40
#define MIDI_ALL_NOTES_OFF   123
#define MIDI_STOP            252

typedef struct _CV_TO_CC
{
    //midi
    LV2_URID_Map* urid_map;
    LV2_URID midi_ev_urid;
    LV2_Atom_Forge forge;
    LV2_Atom_Forge_Frame frame;

    //ports
    LV2_Atom_Sequence* midi_out_p;
    const float* cv_in_p;
    const float* resolution_p;
    const float* chan_p; 
    const float* cc_num_p;

	// 
    float prev_cv;

} CV_TO_CC;

//main functions
LV2_Handle init_cv_to_cc(const LV2_Descriptor *descriptor,double sample_rate, const char *bundle_path,const LV2_Feature * const* host_features)
{
    CV_TO_CC* plug = (CV_TO_CC*)calloc(1, sizeof(CV_TO_CC));
    plug->prev_cv = 0.0;

    //get urid map value for midi events
    for (int i = 0; host_features[i]; i++)
    {
        if (strcmp(host_features[i]->URI, LV2_URID__map) == 0)
        {
            plug->urid_map = (LV2_URID_Map *) host_features[i]->data;
            if (plug->urid_map)
            {
                plug->midi_ev_urid = plug->urid_map->map(plug->urid_map->handle, LV2_MIDI__MidiEvent);
                break;
            }
        }
    }

    lv2_atom_forge_init(&plug->forge,plug->urid_map);

    return plug;
}


void connect_cv_to_cc_ports(LV2_Handle handle, uint32_t port, void *data)
{
    CV_TO_CC* plug = (CV_TO_CC*)handle;
    if(port == MIDI_OUT)    plug->midi_out_p = (LV2_Atom_Sequence*)data;
    else if(port == CV_IN)plug->cv_in_p = (float*)data;
    else if(port == RESOLUTION)plug->resolution_p = (float*)data;
    else if(port == CHAN)   plug->chan_p = (float*)data;
    else if(port == CC_NUM)  plug->cc_num_p = (float*)data;
}

void run_cv_to_cc( LV2_Handle handle, uint32_t n_samples)
{
    CV_TO_CC* plug = (CV_TO_CC*)handle;
    /* uint8_t msg[3]; */
	//
	// Struct for a 3 byte MIDI event, used for writing CC
	typedef struct {
		LV2_Atom_Event event;
		uint8_t        msg[3];
	} MIDICCEvent;


	const uint32_t capacity = plug->midi_out_p->atom.size;
	lv2_atom_sequence_clear(plug->midi_out_p);
	plug->midi_out_p->atom.type = plug->midi_ev_urid;
	/* lv2_atom_forge_set_buffer(&plug->forge,(uint8_t*)plug->midi_out_p, capacity); */
	/* lv2_atom_forge_sequence_head(&plug->forge, &plug->frame, 0); */
	
	const uint8_t msg_type = (uint8_t)176 + ((uint8_t)(*plug->chan_p - 1) % 16);

	for (uint32_t i = 0; (i < n_samples); i = (i + 1)) {
	// if difference is greater than resolution
		if(fabs(plug->cv_in_p[i] - plug->prev_cv) > *plug->resolution_p)  
		{
			MIDICCEvent midiatom;
			//get midi port ready
			/* midiatom.type = plug->midi_ev_urid; */
			/* midiatom.size = 3;//midi CC */

			//make event
			midiatom.msg[0] = msg_type;

			plug->prev_cv = *plug->cv_in_p;

			midiatom.msg[1] = MIDI_DATA_MASK & (uint8_t)*plug->cc_num_p;
			midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t)(plug->cv_in_p[i] * 127.0f);
			midiatom.event.body.type = plug->midi_ev_urid;
			midiatom.event.body.size = 3;
			midiatom.event.time.frames = i;

			/* lv2_atom_forge_frame_time(&plug->forge,0); */
			/* lv2_atom_forge_raw(&plug->forge,&midiatom,sizeof(LV2_Atom)); */
			/* lv2_atom_forge_raw(&plug->forge,msg,midiatom.size); */
			/* lv2_atom_forge_pad(&plug->forge,midiatom.size+sizeof(LV2_Atom)); */

			lv2_atom_sequence_append_event(plug->midi_out_p, capacity, &midiatom.event);

			plug->prev_cv = plug->cv_in_p[i];
		}
	}
}

void cleanup_cv_to_cc(LV2_Handle handle)
{
    CV_TO_CC* plug = (CV_TO_CC*) handle;
    free(plug);
}


//lv2 stuff
static const LV2_Descriptor cv_to_cc_descriptor=
{
    CV_TO_CC_URI,
    init_cv_to_cc,
    connect_cv_to_cc_ports,
    NULL,//activate
    run_cv_to_cc,
    NULL,//deactivate
    cleanup_cv_to_cc,
    NULL//extension
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    switch (index)
    {
    case 0:
        return &cv_to_cc_descriptor;
    default:
        return NULL;
    }
}
