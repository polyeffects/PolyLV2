import("stdfaust.lib");

vibrato2_mono(sections,fb,width,frqmin,fratio,frqmax, lfo) =
 (+ : seq(i,sections,ap2p(R,th(i)))) ~ *(fb)
with {
     //tf2 = component("filters.lib").tf2;
     // second-order resonant digital allpass given pole radius and angle:
     ap2p(R,th) = fi.tf2(a2,a1,1,a1,a2) with {
       a2 = R^2;
       a1 = -2*R*cos(th);
     };
     R = exp(-pi*width/ma.SR);
     pi = 4*atan(1);
     thmin = 2*pi*frqmin/ma.SR;
     thmax = 2*pi*frqmax/ma.SR;
     th1 = thmin + (thmax-thmin)*lfo;
     th(i) = (fratio^(i+1))*th1;
};

phaser2_mono(Notches,width,frqmin,fratio,frqmax,depth,fb,invert, lfo) =
      _ <: *(g1) + g2mi*vibrato2_mono(Notches, fb,width,frqmin,fratio,frqmax, lfo)
with {               // depth=0 => direct-signal only
     g1 = 1-depth/2; // depth=1 => phaser mode (equal sum of direct and allpass-chain)
     g2 = depth/2;   // depth=2 => vibrato mode (allpass-chain signal only)
     g2mi = select2(invert,g2,-g2); // inversion negates the allpass-chain signal
};

process = phaser2_demo;

phaser2_demo(lfo) = phaser2_mono(Notches, width,frqmin,fratio,frqmax, mdepth,fb,invert, lfo);
phaser2_group(x) = vgroup("PHASER2 [tooltip: Reference:
	https://ccrma.stanford.edu/~jos/pasp/Flanging.html]", x);
meter_group(x) = phaser2_group(hgroup("[0]", x));
ctl_group(x)  = phaser2_group(hgroup("[1]", x));
nch_group(x)  = phaser2_group(hgroup("[2]", x));
lvl_group(x)  = phaser2_group(hgroup("[3]", x));

invert = meter_group(checkbox("[1] Invert Internal Phaser Sum"));
vibr = meter_group(checkbox("[2] Vibrato Mode")); // In this mode you can hear any "Doppler"

Notches = 6; // Compile-time parameter: 4 is typical for analog phaser stomp-boxes

depth  = ctl_group(hslider("[2] Notch Depth (Intensity) [style:knob]", 1, 0, 1, 0.001));
fb     = ctl_group(hslider("[3] Feedback Gain [style:knob]", 0, -0.999, 0.999, 0.001));
width  = nch_group(hslider("[1] Notch width [unit:Hz] [style:knob] [scale:log]",
	1000, 10, 5000, 1));
frqmin = nch_group(hslider("[2] Min Notch1 Freq [unit:Hz] [style:knob] [scale:log]",
	100, 20, 5000, 1));
frqmax = nch_group(hslider("[3] Max Notch1 Freq [unit:Hz] [style:knob] [scale:log]",
	800, 20, 10000, 1)) : max(frqmin);
fratio = nch_group(hslider("[4] Notch Freq Ratio [style:knob]",
	1.5, 1.1, 4, 0.001));
mdepth = select2(vibr,depth,2); // Improve "ease of use"
