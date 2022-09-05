// run from /home/cc/hacktrip/ChuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/ChuckTrip/hapitrip/
//chuck --chugin-path:../ --srate:48000 test.ck
//chuck --chugin-path:../ --srate:48000 --verbose:5 test.ck

ChuckTrip ct => dac;
[128, 256] @=> int fpp[];
0 => int find;
string server;
ct.setLocalUDPaudioPort(4464);
find++;
fpp.cap()%=>find;
ct.setFPP(fpp[find]);
"jackloop"+fpp[find]+".stanford.edu" => server;
ct.connectTo(server);
<<<ct.getFPP(), server>>>;
while (true) {
6::second => now;
ct.disconnect();
<<<"disconnect">>>;
1::second => now;
find++;
fpp.cap()%=>find;
ct.setFPP(fpp[find]);
"jackloop"+fpp[find]+".stanford.edu" => server;
<<<ct.getFPP(), server>>>;
ct.connectTo(server);
}
1::hour => now;
