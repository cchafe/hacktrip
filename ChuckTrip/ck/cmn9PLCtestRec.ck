// run from /home/cc/hacktrip/ChuckTrip/ck
//chuck --chugin-path:../ --srate:48000 cmn9PLCtest.ck.ck 
  Gain sum => dac;

class Server {
  ChuckTrip ct;
[ 
    ["cmn9.stanford.edu", "cmn9"]
  , ["52.53.169.9", "SF"]
  , ["54.153.108.194", "SF rec"]
] @=> string serverData[][];
  0 => int which;
  4464 => int baseLocalAudioPort;
  SinOsc sin;
  sin.freq(1000);
  sin.gain(0.1);
  SndBuf snd;
  snd.read("/home/cc/f35box-wfs/Acapulco.mono.wav");
  snd.loop(true);
  snd.gain(0.3);
  fun void go(int s) {
    1 => which;
    ct.setChannels(2);
    ct.setLocalUDPaudioPort(s + baseLocalAudioPort);
    ct.connectTo(serverData[which][0]);
    1 => s;
    snd => ct.chan(s%2);
//    adc => ct.chan(s%2);
    ct => sum;
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
  spork ~servers[i].go(i);
}

ServerRec record;
spork ~record.go();

1::hour => now;

class ServerRec {
  ChuckTrip ct;
[ 
    ["cmn9.stanford.edu", "cmn9"]
  , ["52.53.169.9", "SF"]
  , ["54.153.108.194", "SF rec"]
] @=> string serverData[][];
  0 => int which;
  4564 => int baseLocalAudioPort;
  fun void go() {
    2 => which;
    ct.setChannels(2);
    ct.setLocalUDPaudioPort(baseLocalAudioPort);
    ct.connectTo(serverData[which][0]);
    sum => ct.chan(1) => blackhole;
    ct.gain(1.0);
    <<<serverData[which][1], "started">>>;
  }
  fun void stop() {
    ct.disconnect();
    ct.gain(0.0);
    <<<serverData[which][1], "stopped">>>;
  }
}

