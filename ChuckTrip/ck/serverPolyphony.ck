// run from /home/cc/hacktrip/ChuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/ChuckTrip/hapitrip/
//chuck --chugin-path:../ --srate:48000 serverPolyphony.ck 
//chuck --chugin-path:../chugin/ --srate:48000 --verbose:5 test.ck

dac => WvOut2 w => blackhole;
dac.gain(5.0);
w.wavFilename("/tmp/test.wav");

class Server {
  ChuckTrip ct;
[ 
    ["13.40.164.125", "London"]
  , ["3.231.55.190", "Baltimore"]
//, ["68.66.116.192", "Boston"]
//, ["70.224.226.152", "Los Angeles"]
//, ["216.202.223.12", "Nashville"]
//, ["15.181.162.127", "New York City"]
//, ["15.220.1.14", "Portland"]
  , ["54.176.100.97", "San Francisco"]
//, ["20.125.46.74", "Seattle"]
, ["34.81.239.232", "Taipei"]
] @=> string servers[][];
  0 => int server;
  4464 => int baseLocalAudioPort;
  fun void go(int s) {
    s => server;
    if (s >= servers.cap()) <<<"bogus server index",s>>>;
    servers.cap() %=> server;
    ct.localUDPAudioPort(server + baseLocalAudioPort);
    ct.connect(servers[server][0]);
    adc.chan(0) => ct.chan(s%2);
  ct.chan(s%2) => Gain g => ct.chan(s%2);
  g.gain(0.5);
  s++;
  ct.chan(s%2) => Gain g1 => ct.chan(s%2);
  g1.gain(0.1);
    ct.chan(s%2) => dac.chan(s%2);
    ct.gain(1.0);
    <<<servers[server][1], "started">>>;
  }
  fun void stop() {
    ct.disconnect();
    ct.gain(0.0);
    <<<servers[server][1], "stopped">>>;
  }
}
4 => int nServers;
Server server[nServers];
for (0 => int i; i < nServers; i++) {
  spork ~poly(i);
}
1::hour => now;
fun void poly(int i) {
  i*1::second => now;
  server[i].go(i);
  while (true) {
    50::second => now;
    server[i].stop();
    2::second => now;
    server[i].go(i);
  }
}

