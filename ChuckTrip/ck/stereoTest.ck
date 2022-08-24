dac => WvOut2 w => blackhole;
dac.gain(5.0);
w.wavFilename("/tmp/test1.wav");
ChuckTrip ct;
ct.connect("13.40.164.125");
adc.chan(0) => ct.chan(1);
ct => dac;
1::hour => now;

