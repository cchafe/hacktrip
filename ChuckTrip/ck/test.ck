// run from /home/cc/hacktrip/ChuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/ChuckTrip/hapitrip/
//chuck --chugin-path:../ --srate:48000 test.ck
//chuck --chugin-path:../ --srate:48000 --verbose:5 test.ck

ChuckTrip ct => dac;
"jackloop256.stanford.edu" => string server;

ct.connectTo(server);
<<<ct.htFPP()>>>;
while (true) {
6::second => now;
ct.disconnect();
<<<"disconnect">>>;
1::second => now;
ct.connectTo(server);
}
1::hour => now;
