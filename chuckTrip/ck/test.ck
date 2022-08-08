//chuck --chugin-path:../qt/ssr/ --srate:48000 ssr.ck 

ssr s => dac;
dac.gain(0.05);
s.connect();
1::second => now;

s.freq(100.0);
<<<s.freq()>>>;

while (true) {
  10::ms => now;
  <<<s.checkConnection()>>>;
}
1::hour => now;
