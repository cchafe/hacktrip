// run from /home/cc/hacktrip/ChuckTrip/ck
//chuck --chugin-path:../ --srate:48000 tokyoEchoDemo.ck

dac => WvOut2 w => blackhole;
w.wavFilename("/tmp/test1.wav");

class Server {
  ChuckTrip ct;
[ 
   ["54.65.182.125", "tokyo"]
,   ["35.178.207.30", "london"]
,    ["cmn9.stanford.edu", "cmn9"]
  , ["3.231.55.190", "Baltimore"]
] @=> string serverData[][];
  0 => int which;
  4464 => int baseLocalAudioPort;
  fun void wire (int ch) {
//    adc.chan(ch) =>  dac.chan(0);
    adc.chan(ch) => ct.chan(0);
//    adc.chan(ch) =>  dac.chan(1);
    adc.chan(ch) => ct.chan(1);
  }
  fun void go(int s) {
    0 => which;
    ct.setChannels(1);
    ct.setLocalUDPaudioPort(s + baseLocalAudioPort);
    ct.connectTo(serverData[which][0]);
    wire(0);
    for (0 => int i; i < 1; i++) wire(i);
    ct.chan(0) => dac.chan(0);
    ct.chan(0) => dac.chan(1);
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

