// run from /home/cc/hacktrip/ChuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/ChuckTrip/hapitrip/
//chuck --chugin-path:../chugin/ --srate:48000 test.ck 
//chuck --chugin-path:../chugin/ --srate:48000 --verbose:5 test.ck

chucktrip ct => dac;
// => dac;
SinOsc sin => ct;
sin.freq(1500.0);
sin.gain(0.02);
dac.gain(10.0);
ct.connect();
ct.freq(2000.0);

while (true) {
1::second => now;
ct.disconnect();
ct.freq(2000.0);
1::second => now;
ct.connect();
ct.freq(2000.0);
}
1::hour => now;