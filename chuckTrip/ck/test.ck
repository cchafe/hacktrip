// run from /home/cc/hacktrip/chuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/chuckTrip/hapitrip/
//chuck --chugin-path:../chugin/ --srate:48000 test.ck 
//chuck --chugin-path:../chugin/ --srate:48000 --verbose:5 test.ck

chucktrip ct => dac;
dac.gain(0.05);
ct.connect();
1::second => now;

ct.freq(100.0);
<<<ct.fpp()>>>;

while (true) {
  10::ms => now;
}
1::hour => now;
