// run from /home/cc/hacktrip/ChuckTrip/ck
//chuck --chugin-path:../ --srate:48000 london.ck 

dac => WvOut2 w => blackhole;
adc.gain(1.0);
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
  fun void go(int s) {
    0 => which;
    ct.localUDPAudioPort(s + baseLocalAudioPort);
    ct.connect(serverData[which][0]);
    adc.chan(0) => ct.chan(0) => dac.chan(s%2);
    adc.chan(0) => ct.chan(1) => dac.chan((s+1)%2);
    adc.chan(0) =>  dac.chan(s%2);
    adc.chan(0) =>  dac.chan((s+1)%2);

    adc.chan(2) => ct.chan(0) => dac.chan(s%2);
    adc.chan(2) => ct.chan(1) => dac.chan((s+1)%2);
    adc.chan(2) =>  dac.chan(s%2);
    adc.chan(2) =>  dac.chan((s+1)%2);

    adc.chan(3) => ct.chan(0) => dac.chan(s%2);
    adc.chan(3) => ct.chan(1) => dac.chan((s+1)%2);
    adc.chan(3) =>  dac.chan(s%2);
    adc.chan(3) =>  dac.chan((s+1)%2);

    adc.chan(4) => ct.chan(0) => dac.chan(s%2);
    adc.chan(4) => ct.chan(1) => dac.chan((s+1)%2);
    adc.chan(4) =>  dac.chan(s%2);
    adc.chan(4) =>  dac.chan((s+1)%2);

    adc.chan(5) => ct.chan(0) => dac.chan(s%2);
    adc.chan(5) => ct.chan(1) => dac.chan((s+1)%2);
    adc.chan(5) =>  dac.chan(s%2);
    adc.chan(5) =>  dac.chan((s+1)%2);

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
    500::second => now;
    servers[i].stop();
    2::second => now;
    servers[i].go(i);
  }
}

