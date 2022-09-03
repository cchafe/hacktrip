//chuck --chugin-path:../ --srate:48000 stereoTest.ck 

dac => WvOut2 w => blackhole;
dac.gain(5.0);
w.wavFilename("/tmp/test1.wav");
ChuckTrip ct;
ct.connect("54.238.170.179");
adc.chan(0) =>  dac.chan(0);
adc.chan(0) => ct.chan(1);
ct.chan(0) => dac.chan(0);
ct.chan(1) => dac.chan(1);
1::hour => now;

