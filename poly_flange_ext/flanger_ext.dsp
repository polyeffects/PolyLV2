import("stdfaust.lib");

flanger_mono(dmax,curdel,depth,fb,invert,lfo)
  = _ <: _, (-:de.fdelay(dmax,curdel(lfo))) ~ *(fb) : _,
  *(select2(invert,depth,0-depth))
  : + : *(1/(1+depth));           // ideal for dc and reinforced sinusoids (in-phase summed signals)


flanger_mono_gui(lfo) = flanger_mono(dmax,curdel,depth,fb,invert,lfo);
process = flanger_mono_gui;

curdel(lfo)   = odflange+dflange*lfo;

dmax = 2048;
odflange = 44; // ~1 ms at 44.1 kHz = min delay
dflange  = ((dmax-1)-odflange)*vslider("[1] Delay [midi:ctrl 50][style:knob]", 0.22, 0, 1, 1);

depth    = vslider("[3] Depth [midi:ctrl 3] [style:knob]", .75, 0, 1, 0.001) : si.smooth(ba.tau2pole(depthT60/6.91));

depthT60 = 0.15661;
fb       = vslider("[5] Feedback [midi:ctrl 4] [style:knob]", 0, -0.995, 0.99, 0.001) : si.smooth(ba.tau2pole(fbT60/6.91));

fbT60    = 0.15661;

invert = int(vslider("[4] Invert [midi:ctrl 49][style:knob]",0,0,1,1));

