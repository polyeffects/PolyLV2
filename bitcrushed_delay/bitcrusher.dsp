import("stdfaust.lib");

nbits = hslider("NBITS",8,0,16,1);
scaler = float(2^nbits-1);
round(x) = floor(x+0.5);
bitcrusher(nbits,x) = x : abs : *(scaler) : round : /(scaler) * (2*(x>0)-1.0);
# process =  _,_ : bitcrusher(max(0, min(nbits+(16*_), 16)), _);
process =  _,_ : bitcrusher(_ , _);
