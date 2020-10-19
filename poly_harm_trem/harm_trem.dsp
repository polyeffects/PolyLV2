import("stdfaust.lib");

// fastpow2 = ffunction(float fastpow2(float), "fast_pow2.h", "");

bpm = vslider("[0] BPM [midi:ctrl 63] [style:knob]", 120.0, 15.00, 300, 0.1) : si.smooth(0.999);
cross = vslider("[1] CrossoverFreq [midi:ctrl 64] [style:knob]", 800.0, 20.00, 10000, 1) : si.smooth(0.999);
depth = hslider("[2] depth",0,0,1,0.01) : si.smooth(0.999);
freq = bpm / 60;

lfo = os.osc(freq);
multiply_a = 1 - depth*(lfo*0.5 + 0.5);
multiply_b = 1 - depth*((1-lfo)*0.5 + 0.5);


tremolo(a,b) = a*multiply_a, b*multiply_b :> _;

process = fi.filterbank(3,(cross)) : tremolo;


