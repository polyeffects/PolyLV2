//envfollower.h
#ifndef CV_TO_CC_H
#define CV_TO_CC_H

#define CV_TO_NOTE_URI "http://polyeffects.com/lv2/cv_to_note" 

enum cv_to_cc_ports
{
    MIDI_OUT = 0,
    PITCH_IN,
    GATE_IN,
	VELOCITY_IN, // default if not connected
    CHAN,
    NOTE_OFFSET,
};

#endif
