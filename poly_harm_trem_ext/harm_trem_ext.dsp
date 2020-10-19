import("stdfaust.lib");

// fastpow2 = ffunction(float fastpow2(float), "fast_pow2.h", "");

cross = vslider("[1] CrossoverFreq [midi:ctrl 64] [style:knob]", 800.0, 20.00, 10000, 1) : si.smooth(0.999);
depth = hslider("[2] depth",0,0,1,0.01) : si.smooth(0.999);

multiply_a(lfo) = 1 - depth*(lfo*0.5 + 0.5);
multiply_b(lfo) = 1 - depth*((1-lfo)*0.5 + 0.5);


tremolo(a,b,lfo) = a*multiply_a(lfo), b*multiply_b(lfo) :> _;

process = fi.filterbank(3,(cross)),_ : tremolo;


