// run from /home/cc/hacktrip/ChuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/ChuckTrip/hapitrip/
//chuck --chugin-path:../ --srate:48000 test.ck
//chuck --chugin-path:../ --srate:48000 --verbose:5 test.ck

ChuckTrip ct => dac;
ct.connect("jackloop256.stanford.edu");
ct.hi();
<<<ct.htFPP()>>>;
while (true) {
6::second => now;
ct.disconnect();
<<<"disconnect">>>;
ct.bye();
1::second => now;
ct.connect("jackloop256.stanford.edu");
}
1::hour => now;
