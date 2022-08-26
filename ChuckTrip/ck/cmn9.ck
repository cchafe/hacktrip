// run from /home/cc/hacktrip/ChuckTrip/ck
//chuck --chugin-path:../ --srate:48000 cmn9.ck 

class Server {
  ChuckTrip ct;
[ 
    ["cmn9.stanford.edu", "cmn9"]
  , ["3.231.55.190", "Baltimore"]
] @=> string serverData[][];
  0 => int which;
  4464 => int baseLocalAudioPort;
  fun void go(int s) {
    0 => which;
    ct.localUDPAudioPort(s + baseLocalAudioPort);
    ct.connect(serverData[which][0]);
    adc.chan(0) => ct => dac.chan(s%2);
    ct.gain(1.0);
    <<<serverData[which][1], "started", s>>>;
  }
  fun void stop() {
    ct.disconnect();
    ct.gain(0.0);
    <<<serverData[which][1], "stopped">>>;
  }
}

1 => int nClients;
Server servers[nClients];

for (0 => int i; i < nClients; i++) {
  spork ~poly(i);
}

1::hour => now;

fun void poly(int i) {
  i*1::second => now;
  servers[i].go(i);
  while (true) {
    50::second => now;
    servers[i].stop();
    2::second => now;
    servers[i].go(i);
  }
}

