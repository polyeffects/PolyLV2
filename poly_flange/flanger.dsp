import("stdfaust.lib");

flanger_mono(dmax,curdel,depth,fb,invert,lfoshape)
  = _ <: _, (-:de.fdelay(dmax,curdel)) ~ *(fb) : _,
  *(select2(invert,depth,0-depth))
  : + : *(1/(1+depth));           // ideal for dc and reinforced sinusoids (in-phase summed signals)


process = flanger_mono_gui;

flangeview = lfo(freq);
flanger_mono_gui = attach(flangeview) : flanger_mono(dmax,curdel,depth,fb,invert,lfoshape);

sinlfo(freq) = (1 + os.oscrs(freq))/2;
trilfo(freq) = 1.0-abs(os.saw1(freq));
lfo(f) = (lfoshape * trilfo(f)) + ((1-lfoshape) * sinlfo(f));

dmax = 2048;
odflange = 44; // ~1 ms at 44.1 kHz = min delay
dflange  = ((dmax-1)-odflange)*vslider("[1] Delay [midi:ctrl 50][style:knob]", 0.22, 0, 1, 1);

bpm = vslider("[2] BPM [midi:ctrl 2] [style:knob]", 30, 5, 300, 0.01);
freq = bpm / 60 : si.smooth(ba.tau2pole(freqT60/6.91));

freqT60  = 0.15661;
depth    = vslider("[3] Depth [midi:ctrl 3] [style:knob]", .75, 0, 1, 0.001) : si.smooth(ba.tau2pole(depthT60/6.91));

depthT60 = 0.15661;
fb       = vslider("[5] Feedback [midi:ctrl 4] [style:knob]", 0, -0.995, 0.99, 0.001) : si.smooth(ba.tau2pole(fbT60/6.91));

fbT60    = 0.15661;
lfoshape = vslider("[6] Waveshape [midi:ctrl 54] [style:knob]", 0, 0, 1, 0.001);
curdel   = odflange+dflange*lfo(freq);

invert = int(vslider("[4] Invert [midi:ctrl 49][style:knob]",0,0,1,1));

