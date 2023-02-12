// run from /home/cc/hacktrip/ChuckTrip/ck
//chuck --chugin-path:../ --srate:48000 meshDemo.ck

dac => WvOut2 w => blackhole;
w.wavFilename("/tmp/test1.wav");
Gain scatter;
scatter.gain(0.35);
class Server {
  ChuckTrip ct;
[ 
   ["13.246.2.11", "cape town"]
,   ["54.65.182.125", "tokyo"]
,   ["68.66.116.88", "boston"]
,    ["cmn9.stanford.edu", "cmn9"]
  , ["3.231.55.190", "Baltimore"]
] @=> string serverData[][];
  0 => int which;
  4464 => int baseLocalAudioPort;
  Gain fb[2];
  for (0 => int i; i < 2; i++)  fb[i].gain(0.5);
  dac.gain(4.0);
  fun void wireMic (int in, int out) {
    2 %=> out;
//    adc.chan(in) =>  dac.chan(out);
    adc.chan(in) => fb[out] => ct.chan(out);
    scatter => fb[out];
    ct.chan(out) => dac.chan(0);
    ct.chan(out) => dac.chan(1);
    ct.chan(out) => fb[out];
    ct.chan(out) => scatter;
  }
  fun void wireCelletto (int in, int out) {
    2 %=> out;
    adc.chan(in) => fb[out];
  }
  fun void go(int which) {
    ct.setChannels(1);
    ct.setLocalUDPaudioPort(which + baseLocalAudioPort);
    ct.connectTo(serverData[which][0]);
    wireMic(0, which);
    for (0 => int i; i < 2; i++) wireCelletto(i, which);
//    ct => dac;
    <<<serverData[which][1], "started", which>>>;
  }
  fun void stop() {
    ct.disconnect();
    <<<serverData[which][1], "stopped">>>;
  }
}

3 => int nClients;
Server servers[nClients];

for (0 => int i; i < nClients; i++) {
  spork ~poly(i);
}

1::hour => now;

fun void poly(int i) {
//  i*1::second => now;
  servers[i].go(i);
  while (true) {
    500::second => now;
    servers[i].stop();
    2::second => now;
    servers[i].go(i);
  }
}

