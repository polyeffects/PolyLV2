// based on  Mindi by Spencer Jackson
//cv_to_note.c
#include "cv_to_note.h"
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

typedef struct _CV_TO_NOTE
{
    //midi
    LV2_URID_Map* urid_map;
    LV2_URID midi_ev_urid;
    LV2_Atom_Forge forge;
    LV2_Atom_Forge_Frame frame;

    //ports
    LV2_Atom_Sequence* midi_out;
    const float* pitch;
    const float* gate;
    const float* velocity;
    const float* chan; 
    const float* note_offset;

	// 
    float note_number;
	bool note_is_on;

} CV_TO_NOTE;

//main functions
LV2_Handle init_cv_to_note(const LV2_Descriptor *descriptor,double sample_rate, const char *bundle_path,const LV2_Feature * const* host_features)
{
    CV_TO_NOTE* plug = (CV_TO_NOTE*)calloc(1, sizeof(CV_TO_NOTE));
    plug->note_number = 60;
    plug->note_is_on = false;

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


void connect_cv_to_note_ports(LV2_Handle handle, uint32_t port, void *data)
{
    CV_TO_NOTE* plug = (CV_TO_NOTE*)handle;
    if(port == MIDI_OUT)    plug->midi_out = (LV2_Atom_Sequence*)data;
    else if(port == PITCH_IN)plug->pitch = (float*)data;
    else if(port == GATE_IN)plug->gate = (float*)data;
    else if(port == VELOCITY_IN)plug->velocity = (float*)data;
    else if(port == CHAN)   plug->chan = (float*)data;
    else if(port == NOTE_OFFSET)  plug->note_offset = (float*)data;
}

void run_cv_to_note( LV2_Handle handle, uint32_t n_samples)
{
    CV_TO_NOTE* plug = (CV_TO_NOTE*)handle;
    /* uint8_t msg[3]; */
	//
	// Struct for a 3 byte MIDI event, used for writing CC
	typedef struct {
		LV2_Atom_Event event;
		uint8_t        msg[3];
	} MIDICCEvent;


	const uint32_t capacity = plug->midi_out->atom.size;
	lv2_atom_sequence_clear(plug->midi_out);
	plug->midi_out->atom.type = plug->midi_ev_urid;
	
    const uint8_t chan = (uint8_t) *(plug->chan);
	const uint8_t note_on = (uint8_t)0x90 + ((uint8_t)(chan - 1) % 16);
	const uint8_t note_off = (uint8_t)0x80 + ((uint8_t)(chan - 1) % 16);

    const float* pitch = plug->pitch;
    const float* gate = plug->gate;
    const float* velocity = plug->velocity;

    const uint8_t note_offset = (uint8_t) *(plug->note_offset);

    uint8_t note_number = plug->note_number;

	for (uint32_t s = 0; s < n_samples; ++s) {

		if (gate[s] >= 0.4) { // 2 volt in Hector
			if (!(plug->note_is_on)){
				plug->note_is_on = true;

				MIDICCEvent midiatom;
				//make event
				midiatom.msg[0] = note_on;
				note_number = ((60 * (pitch[s] +1)) + note_offset);
				midiatom.msg[1] = MIDI_DATA_MASK & (uint8_t) note_number;
				if (velocity[s] > -40.0f){
					midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t) (velocity[s] * 127.0f);
				} else {
					midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t) (64);
				}

				midiatom.event.body.type = plug->midi_ev_urid;
				midiatom.event.body.size = 3;
				midiatom.event.time.frames = s;

				lv2_atom_sequence_append_event(plug->midi_out, capacity, &midiatom.event);
			}
		} else if (gate[s] <= 0.05){ // 0.25 volts in Hector
			if (plug->note_is_on){
				plug->note_is_on = false;
				MIDICCEvent midiatom;
				//make event
				midiatom.msg[0] = note_off;
				midiatom.msg[1] = MIDI_DATA_MASK & (uint8_t) note_number;
				if (velocity[s] > -40.0f){
					midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t) (velocity[s] * 127.0f);
				} else {
					midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t) (64);
				}

				midiatom.event.body.type = plug->midi_ev_urid;
				midiatom.event.body.size = 3;
				midiatom.event.time.frames = s;

				lv2_atom_sequence_append_event(plug->midi_out, capacity, &midiatom.event);
			}
		}
	}
	plug->note_number = note_number;
}

void cleanup_cv_to_note(LV2_Handle handle)
{
    CV_TO_NOTE* plug = (CV_TO_NOTE*) handle;
    free(plug);
}


//lv2 stuff
static const LV2_Descriptor cv_to_note_descriptor=
{
    CV_TO_NOTE_URI,
    init_cv_to_note,
    connect_cv_to_note_ports,
    NULL,//activate
    run_cv_to_note,
    NULL,//deactivate
    cleanup_cv_to_note,
    NULL//extension
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    switch (index)
    {
    case 0:
        return &cv_to_note_descriptor;
    default:
        return NULL;
    }
}
