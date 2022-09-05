// run from /home/cc/hacktrip/ChuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/ChuckTrip/hapitrip/
//chuck --chugin-path:../ --srate:48000 test.ck
//chuck --chugin-path:../ --srate:48000 --verbose:5 test.ck

ChuckTrip ct => dac;
[128, 256, 64, 32] @=> int fpp[];
[1, 2, 1] @=> int nChans[];
0 => int find;
0 => int cind;
0 => int pind;
string server;
pind++;
ct.setLocalUDPaudioPort(4464+pind);
find++;
fpp.cap()%=>find;
ct.setFPP(fpp[find]);
cind++;
nChans.cap()%=>cind;
ct.setChannels(nChans[cind]);
"jackloop"+fpp[find]+".stanford.edu" => server;
ct.connectTo(server);
<<<ct.getFPP(), server, ct.getChannels()>>>;
while (true) {
6::second => now;
ct.disconnect();
<<<"disconnect">>>;
1::second => now;
pind++;
ct.setLocalUDPaudioPort(4464+pind);
find++;
fpp.cap()%=>find;
ct.setFPP(fpp[find]);
cind++;
nChans.cap()%=>cind;
ct.setChannels(nChans[cind]);
"jackloop"+fpp[find]+".stanford.edu" => server;
<<<ct.getFPP(), server, ct.getChannels()>>>;
ct.connectTo(server);
}
1::hour => now;
