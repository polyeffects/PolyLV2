import("stdfaust.lib");

echo_group(x) = x;
   knobs_group(x) = vgroup("[0] Knobs",x);
   switches_group(x)  = vgroup("[1] Switches",x); 

dmax = 1048576; // 21 seconds
dmaxs = float(dmax)/48000.0;


nbits = hslider("NBITS",8,0,16,1);
scaler = float(2^nbits-1);
round(x) = floor(x+0.5);
bitcrusher(nbits,x) = x : abs : *(scaler) : round : /(scaler) * (2*(x>0)-1.0);
//process = os.oscrs(300) : bitcrusher(nbits);

Nnines = 1.8; // Increase until you get the desired maximum amount of smoothing when fbs==1
fastpow2 = ffunction(float fastpow2(float), "fast_pow2.h", "");
fbspr(fbs) = 1.0 - fastpow2(-3.33219*Nnines*fbs); // pole radius of feedback smoother
inputSelect(gi) = _,0 : select2(gi);
echo_mono(dmax,curdel,tapdel,fb,fbspr,gi) = inputSelect(gi) : (+:si.smooth(fbspr)
	<: (de.fdelay(dmax,curdel):bitcrusher(nbits)),
				      (de.fdelay(dmax,tapdel):bitcrusher(nbits)))
				      ~(*(fb),!) : !,_;

tau2pole(tau) = ba.if(tau>0, exp(-1.0/(tau*ma.SR)), 0.0);
t60smoother(dEchoT60) = si.smooth(tau2pole(dEchoT60/6.91));

bpmT60 = 0.15661;
dBPM = knobs_group(vslider("[0] BPM [midi:ctrl 63] [style:knob]", 120.0, 30.00, 300, 0.1));

dEchoT60 = knobs_group(vslider("[1] DelayT60 [midi:ctrl 60] [style:knob]", 0.5, 0, 100, 0.001));
dEchoBeats = knobs_group(vslider("[0] Delay [midi:ctrl 61] [style:knob]", 0.5, 0.001, 64.0, 0.001));
dEchoSamplesRaw = (60.0 / dBPM) * dEchoBeats * ma.SR; // beats to sample time
dEchoSamples = dEchoSamplesRaw : t60smoother(dEchoT60);
warpRaw = knobs_group(vslider("[0] Warp [midi:ctrl 62] [style:knob]", 0, -1.0, 1.0, 0.001));

scrubAmpRaw = 0;

scrubPhaseRaw = 0;
fb = knobs_group(vslider("[2] Feedback [midi:ctrl 2] [style:knob]", .3, 0.0, 1.0, 0.0001));
amp = knobs_group(vslider("[3] Amp [midi:ctrl 75] [style:knob]", .5, 0, 1, 0.001)) : si.smooth(ba.tau2pole(ampT60/6.91));

ampT60 = 0.15661;
fbs = knobs_group(vslider("[5] [midi:ctrl 76] FeedbackSm [style:knob]", 0, 0, 1, 0.00001));
gi = switches_group(1-vslider("[7] [midi:ctrl 105] EnableEcho[style:knob]",0,0,1,1)); // "ground input" switches input to zeros

// Warp and Scrubber stuff:
enableEcho = (scrubAmpRaw > 0.00001);
triggerScrubOn = (enableEcho - enableEcho') > 0;  // enableEcho went 0 to 1
triggerScrubOff = (enableEcho - enableEcho') < 0; // enableEcho went 1 to 0
// Ramps up only during scrub "hold" time and is otherwise zero:
counter = (enableEcho * (triggerScrubOn : + ~ +(1) * enableEcho : -(2))) & (dmax-1);
// implementation that continues scrubbing where it left off:

scrubPhase = scrubPhaseRaw : t60smoother(dEchoT60*(1-triggerScrubOff));
scrubAmp = scrubAmpRaw : t60smoother(dEchoT60*(1-triggerScrubOff));
warp = warpRaw : t60smoother(dEchoT60);
dTapSamplesRaw = dEchoSamplesRaw * (1.0 + warp + scrubPhase * scrubAmp) + float(counter);
dTapSamples = dTapSamplesRaw : t60smoother(dEchoT60*(1-triggerScrubOff));

// process = _ <: _, amp * echo_mono(dmax,dEchoSamples,dTapSamples,fb,fbspr(fbs),gi) : +;
process = _ : amp * echo_mono(dmax,dEchoSamples,dTapSamples,fb,fbspr(fbs),gi);
