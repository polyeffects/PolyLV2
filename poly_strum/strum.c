// based on  Mindi by Spencer Jackson
//strum.c
#include "strum.h"
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

typedef struct _STRUM
{
    //midi
    LV2_URID_Map* urid_map;
    LV2_URID midi_ev_urid;
    LV2_Atom_Forge forge;
    LV2_Atom_Forge_Frame frame;

    //ports
    LV2_Atom_Sequence* midi_out;
    float* v_oct;
    float* gate; 
	// inputs
    const float* chord_root;
    const float* chord_type;
    const float* lead_mode;
    const float* touched_notes;
    const float* octave;
    const float* velocity; // x pos

	// 
    float note_number;
    float prev_touched_notes;
	bool note_is_on;

} STRUM;

//main functions
LV2_Handle init_strum(const LV2_Descriptor *descriptor,double sample_rate, const char *bundle_path,const LV2_Feature * const* host_features)
{
    STRUM* plug = (STRUM*)calloc(1, sizeof(STRUM));
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


void connect_strum_ports(LV2_Handle handle, uint32_t port, void *data)
{
    STRUM* plug = (STRUM*)handle;
    if(port == MIDI_OUT)    plug->midi_out = (LV2_Atom_Sequence*)data;
    else if(port == V_OCT_OUT)plug->v_oct = (float*)data;
    else if(port == GATE_OUT)plug->gate = (float*)data;
    else if(port == CHORD_ROOT)plug->chord_root = (float*)data;
    else if(port == CHORD_TYPE)plug->chord_type = (float*)data;
    else if(port == LEAD_MODE)plug->lead_mode = (float*)data;
    else if(port == TOUCHED_NOTES)plug->touched_notes = (float*)data;
    else if(port == OCTAVE)plug->octave = (float*)data;
    else if(port == VELOCITY)   plug->velocity = (float*)data;
}

void run_strum( LV2_Handle handle, uint32_t n_samples)
{
    STRUM* plug = (STRUM*)handle;
    /* uint8_t msg[3]; */
	//
	// Struct for a 3 byte MIDI event, used for writing CC
	typedef struct {
		LV2_Atom_Event event;
		uint8_t        msg[3];
	} MIDICCEvent;


	const uint32_t capacity = 32760;//plug->midi_out->atom.size; // XXX dodgy
	lv2_atom_sequence_clear(plug->midi_out);
	plug->midi_out->atom.type = plug->midi_ev_urid;
	
    const uint8_t chan = (uint8_t) 1;
	const uint8_t note_on = (uint8_t)0x90 + ((uint8_t)(chan - 1) % 16);
	const uint8_t note_off = (uint8_t)0x80 + ((uint8_t)(chan - 1) % 16);

    float* v_oct = plug->v_oct;
    float* gate = plug->gate; 
	// inputs
    const uint8_t chord_root = (uint8_t) *(plug->chord_root);
    const uint8_t chord_type = (uint8_t) *(plug->chord_type);
    const uint8_t lead_mode = (uint8_t) *(plug->lead_mode);
    const int8_t touched_notes = (int8_t) *(plug->touched_notes);
    const int8_t octave = (int8_t) *(plug->octave);
    const float* velocity = plug->velocity; // x pos

    /* const uint8_t note_offset = (uint8_t) *(plug->note_offset); */

    uint8_t note_number = plug->note_number;
    int8_t prev_touched_notes = plug->prev_touched_notes;
    bool note_is_on = plug->note_is_on;
    uint8_t n = 0;
    int8_t note_offset = -1;
    int8_t octave_offset = 0;
	float v_oct_c;
	uint32_t on_time = n_samples / 8;
	uint32_t off_time = n_samples / 16;

	if (touched_notes != prev_touched_notes){ 
		prev_touched_notes = touched_notes;
		if (note_is_on){
			note_is_on = false;
			MIDICCEvent midiatom;
			void *r_v = NULL;
			//
			//make event
			midiatom.msg[0] = note_off;
			midiatom.msg[1] = MIDI_DATA_MASK & (uint8_t) note_number;
			/* midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t) (velocity[0] * 127.0f); */
			midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t) (127.0f);

			midiatom.event.body.type = plug->midi_ev_urid;
			midiatom.event.body.size = 3;
			midiatom.event.time.frames = off_time;

			r_v = lv2_atom_sequence_append_event(plug->midi_out, capacity, &midiatom.event);
			/* printf("appended note off %d %d %d %p capacity %lu \n", midiatom.msg[0], midiatom.msg[1], midiatom.msg[2], r_v, (unsigned long)capacity); */
		}
		if (touched_notes >= 0) { // got any touches
			if (!(note_is_on)){
				note_is_on = true;

				MIDICCEvent midiatom;
				// make event
				// need to find what note corrosponds to the current touch
				// 4 octaves of 4 notes
				n = touched_notes % 4;
				note_offset = -1;
				if (n == 0){
					// 0 is root note, octave 0
					note_offset = 0;
				} else if (n == 1){
				// 1 is 3rd, 
					if (chord_type == MAJOR || chord_type == DOM_7){
						note_offset = 4;
					}  else {
						note_offset = 3;
					}
				} else if (n == 2){
				//2 is 5th, 
					note_offset = 7;
				
				} else if (n == 3){
				//3 is 7th
					if (chord_type == DOM_7 || chord_type == MINOR_7){
						note_offset = 10;
					}
				}

				if (note_offset > -1){ // don't do anything if we've touched a 7th and we're not a 7
					octave_offset = ((uint8_t) (touched_notes / 4) * 12) + (octave * 12);
					
					void *r_v = NULL;
					midiatom.msg[0] = note_on;
					note_number = (60 + chord_root + note_offset + octave_offset);
					midiatom.msg[1] = MIDI_DATA_MASK & (uint8_t) note_number;
					/* midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t) (velocity[0] * 127.0f); */
					midiatom.msg[2] = MIDI_DATA_MASK & (uint8_t) (127.0f);

					midiatom.event.body.type = plug->midi_ev_urid;
					midiatom.event.body.size = 3;
					midiatom.event.time.frames = on_time;

					r_v = lv2_atom_sequence_append_event(plug->midi_out, capacity, &midiatom.event);
					/* printf("appended note on %d %d %d %p capacity %lu \n", midiatom.msg[0], midiatom.msg[1], midiatom.msg[2], r_v, (unsigned long)capacity); */
				} 
				/* else { */
				/* 	gate[s] = 0.0f; */
				/* } */
			}
		}
	}
	v_oct_c = ((float)((float)(note_number-60) * 1/12.0f)) * 0.2; // below 60 are negative vol / oct

	for (uint32_t s = 0; s < n_samples; ++s) {

		if (note_is_on) {
			gate[s] = 1.0f;
		} else {
			gate[s] = 0.0f;
		}
		v_oct[s] = v_oct_c; 
	}
	plug->note_number = note_number;
	plug->prev_touched_notes = prev_touched_notes;
	plug->note_is_on = note_is_on;
}

void cleanup_strum(LV2_Handle handle)
{
    STRUM* plug = (STRUM*) handle;
    free(plug);
}


//lv2 stuff
static const LV2_Descriptor strum_descriptor=
{
    STRUM_URI,
    init_strum,
    connect_strum_ports,
    NULL,//activate
    run_strum,
    NULL,//deactivate
    cleanup_strum,
    NULL//extension
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    switch (index)
    {
    case 0:
        return &strum_descriptor;
    default:
        return NULL;
    }
}
