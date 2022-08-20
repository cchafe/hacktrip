// run from /home/cc/hacktrip/ChuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/ChuckTrip/hapitrip/
//chuck --chugin-path:../chugin/ --srate:48000 servers.ck 
//chuck --chugin-path:../chugin/ --srate:48000 --verbose:5 test.ck

dac => WvOut2 w => blackhole;
w.wavFilename("/tmp/sf.wav");
//SinOsc sin => ct;
//sin.freq(1500.0);
//sin.gain(1.0);
/////////////////////////////
Impulse imp => dj djembe; // => ct;
fun void dj () {
  while (true) {
    imp.next(1.0);
      djembe.freq(Std.mtof( 1 + (1) * 35));
      djembe.gain( Std.dbtorms(80 + 1 * 20) ); // 80 20
      djembe.sharp(2.0 * Std.dbtorms(40 + 1 * 60)); // 40 50
      djembe.pos(1);
    0.5::second => now;
  }
}
//spork ~dj();
//["54.153.79.243", "ccTest"]
//["jackloop128.stanford.edu", "jackloop128"]
//["cmn9.stanford.edu", "cmn9"]
//["localhost", "localhost"]

class Server {
  adc => chucktrip ct => dac;
  ct => Gain g => ct;
  g.gain(0.5);
[ ["35.178.52.65", "London"]
, ["3.239.229.107", "Baltimore"]
, ["68.66.116.192", "Boston"]
, ["70.224.226.152", "Los Angeles"]
, ["216.202.223.12", "Nashville"]
, ["15.181.162.127", "New York City"]
, ["15.220.1.14", "Portland"]
, ["54.176.100.97", "San Francisco"]
, ["20.125.46.74", "Seattle"]
, ["35.185.142.249", "Taipei"]
] @=> string servers[][];
  0 => int server;
  4464 => int baseLocalAudioPort;
  fun void go() {
    ct.localUDPAudioPort(server + baseLocalAudioPort);
    ct.connect(servers[server][0]);
    ct.gain(1.0);
    <<<servers[server][1], "started">>>;
  }
  fun void stop() {
    ct.disconnect();
    ct.gain(0.0);
    <<<servers[server][1], "stopped">>>;
    server++;
    servers.cap() %=> server;
  }
}
Server server;
server.go();
while (true) {
  5::second => now;
  server.stop();
  2::second => now;
  server.go();
}
1::hour => now;
