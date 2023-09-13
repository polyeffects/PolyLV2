//envfollower.h
#ifndef CV_TO_CC_H
#define CV_TO_CC_H

#define STRUM_URI "http://polyeffects.com/lv2/strum" 

enum cv_to_cc_ports
{
    MIDI_OUT = 0,
	V_OCT_OUT,
	GATE_OUT,
    CHORD_ROOT,
    CHORD_TYPE,
    LEAD_MODE,
	TOUCHED_NOTES, 
    OCTAVE,
    VELOCITY,
};

enum chord_types
{
    MAJOR = 0,
	MINOR,
	MINOR_7,
    DOM_7,
};

#endif
