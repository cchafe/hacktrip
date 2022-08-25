// run from /home/cc/hacktrip/ChuckTrip/ck
//chuck --chugin-path:../ --srate:48000 cmn9.ck 

class Server {
  ChuckTrip ct;
[ 
    ["cmn9.stanford.edu", "cmn9"]
  , ["3.231.55.190", "Baltimore"]
] @=> string servers[][];
  0 => int server;
  4464 => int baseLocalAudioPort;
  fun void go(int s) {
    0 => server;
    ct.localUDPAudioPort(s + baseLocalAudioPort);
    ct.connect(servers[server][0]);
    adc.chan(0) => ct.chan(s%2);
    ct.gain(1.0);
    <<<servers[server][1], "started">>>;
  }
  fun void stop() {
    ct.disconnect();
    ct.gain(0.0);
    <<<servers[server][1], "stopped">>>;
  }
}

1 => int nClients;
Server server[1];
for (0 => int i; i < nClients; i++) {
  spork ~poly(i);
}
1::hour => now;
fun void poly(int i) {
  i*1::second => now;
  server[0].go(i);
  while (true) {
    50::second => now;
    server[i].stop();
    2::second => now;
    server[i].go(i);
  }
}

