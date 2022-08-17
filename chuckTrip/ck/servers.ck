// run from /home/cc/hacktrip/chuckTrip/ck
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/chuckTrip/hapitrip/
//chuck --chugin-path:../chugin/ --srate:48000 servers.ck 
//chuck --chugin-path:../chugin/ --srate:48000 --verbose:5 test.ck


adc => chucktrip ct => dac;
ct => Gain g => ct;
g.gain(0.5);
dac => WvOut2 w => blackhole;
w.wavFilename("/tmp/sf.wav");
//SinOsc sin => ct;
//sin.freq(1500.0);
//sin.gain(1.0);
/////////////////////////////
Impulse imp => dj djembe => ct;
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

["35.178.52.65","54.176.100.97"] @=> string servers[];
0 => int server;
ct.connect(servers[server]);
server++;

while (true) {
5::second => now;
ct.disconnect();
ct.gain(0.0);
2::second => now;
ct.connect(servers[server]);
ct.gain(1.0);
}
1::hour => now;
