// run from /home/cc/hacktrip/ChuckTrip/ck
// chuck -s template.ck genChug.ck 

<<<"replacing ../chucktrip.cpp">>>;
FileIO fout;
fout.open("../chucktrip.cpp",FileIO.WRITE);

fout <= "// this file was automatically generated with \n // chuck -s chuck -s template.ck genChug.ck \n";
Template t;

t.newFun("sayHi", "hi", "  fprintf(stderr,\"ChuckTrip Hi \\n\");  ");
t.newFun("sayBye", "bye", "  fprintf(stderr,\"ChuckTrip Bye \\n\");  ");








fout <= t.INCLUDES;
fout <= t.STATIC_DECLARATIONS;
fout <= t.START_CLASS;
fout <= t.CLASS_CONSTRUCTOR + "\n}";
fout <= t.CLASS_DESTRUCTOR;
fout <= t.CLASS_PUBLIC_METHODS;
fout <= t.CLASS_PUBLIC_MEMBERS;
fout <= t.CLASS_PRIVATE_MEMBERS;
fout <= t.FINISH_CLASS;
fout <= t.QUERY_CLASS;
fout <= t.FINISH_QUERY_CLASS;
fout <= t.STATIC_IMPLEMENTATIONS;

fout.close();
