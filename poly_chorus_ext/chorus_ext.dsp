import("stdfaust.lib");

voices = 6; // MUST BE EVEN
// process = _,_:ba.bypass1to2(cbp,chorus_mono(dmax,curdel,sigma,do2,voices, _));

dmax = 8192;
curdel = dmax * vslider("[0] Delay [midi:ctrl 4] [style:knob]", 0.5, 0, 1, 1) : si.smooth(0.999);

depth = vslider("[4] Depth [midi:ctrl 3] [style:knob]", 0.5, 0, 1, 0.001) : si.smooth(ba.tau2pole(depthT60/6.91));

depthT60 = 0.15661;
delayPerVoice = 0.5*curdel/voices;
sigma = delayPerVoice * vslider("[6] Deviation [midi:ctrl 58] [style:knob]",0.5,0,1,0.001) : si.smooth(0.999);

do2 = depth; 

chorus_mono(dmax,curdel,sigma,do2,voices, lfo)
	= _ <: (*(1-do2)<:_,_),(*(do2) <: par(i,voices,voice(i)) :> _,_) : ro.interleave(2,2) : +,+
    with {
        angle(i) = 2*ma.PI*(i/2)/voices + (i%2)*ma.PI/2;
        voice(i) = de.fdelay(dmax,min(dmax,del(i))) * cos(angle(i));
        del(i) = curdel*(i+1)/voices + dev(i);
        dev(i) = sigma * sin(lfo+(i*2*ma.PI/voices));
    };

chorus(lfo) = chorus_mono(dmax,curdel,sigma,do2,voices, lfo);

process = chorus;
// process = chorus_mono(dmax,curdel,sigma,do2,voices, lfo);
