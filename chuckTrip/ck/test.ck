// run from /home/cc/hacktrip/chuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/chuckTrip/hapitrip/
//chuck --chugin-path:../chugin/ --srate:48000 test.ck 
//chuck --chugin-path:../chugin/ --srate:48000 --verbose:5 test.ck

chucktrip ct => dac;
SinOsc sin => ct;
sin.freq(1000.0);
sin.gain(0.05);
dac.gain(0.05);
ct.connect();
ct.freq(1000.0);

while (true) {
1::second => now;
ct.disconnect();
ct.freq(100.0);
1::second => now;
ct.connect();
ct.freq(1000.0);
}
1::hour => now;
