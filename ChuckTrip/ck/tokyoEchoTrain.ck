// run from /home/cc/hacktrip/ChuckTrip/ck
//chuck --chugin-path:../ --srate:48000 --channels:6 tokyoEchoTrain.ck

dac => WvOut2 w => blackhole;
w.wavFilename("/tmp/test1.wav");

class Server {
  ChuckTrip ct;
[ 
   ["54.238.170.179", "tokyo"]
,   ["35.178.207.30", "london"]
,    ["cmn9.stanford.edu", "cmn9"]
  , ["3.231.55.190", "Baltimore"]
] @=> string serverData[][];
  0 => int which;
  4464 => int baseLocalAudioPort;
  Gain fb[2];
  dac.gain(2.0);
  fun void wire (int ch) {
    adc.chan(ch) =>  dac.chan(0);
    adc.chan(ch) => fb[0] => ct.chan(0);
    adc.chan(ch) =>  dac.chan(1);
    adc.chan(ch) => fb[1] => ct.chan(1);
  }
  fun void go(int s) {
    0 => which;
    ct.localUDPAudioPort(s + baseLocalAudioPort);
    ct.connect(serverData[which][0]);
    wire(0);
    for (2 => int i; i < 6; i++) wire(i);
    ct => dac;
    for (0 => int i; i < 2; i++) {
      ct.chan(i) => fb[i];
      fb[i].gain(0.1);
    }
    <<<serverData[which][1], "started", s>>>;
  }
  fun void stop() {
    ct.disconnect();
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
    500::second => now;
    servers[i].stop();
    2::second => now;
    servers[i].go(i);
  }
}

