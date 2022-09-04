// run from /home/cc/hacktrip/ChuckTrip/ck
// chuck -s template.ck genChug.ck 

<<<"replacing ../chucktrip.cpp">>>;
FileIO fout;
fout.open("../chucktrip.cpp",FileIO.WRITE);

fout <= "// this file was automatically generated with \n // chuck -s chuck -s template.ck genChug.ck \n";
Template t;

["",""] @=> string args[];

t.newFun("hi", "void", args, "  fprintf(stderr,\"ChuckTrip Hi \\n\");  ", "prints --hi-- msg");
t.newFun("bye", "void", args, "  fprintf(stderr,\"ChuckTrip Bye \\n\");  ", "prints --bye-- msg");
t.newFun("htFPP", "int", args, "  = ht->getFPP();  ", "prints --htFPP--");
[ "QString","word"] @=> args;
t.newFun("printSomeString", "void", [ "QString","word"], "  fprintf(stderr,\"ChuckTrip %s \\n\", 
  word.toStdString().c_str());  ", "prints --bye-- msg");
t.newFun("connectTo", "void", [ "QString","server"], "ht = new Hapitrip();
        ht->connectToServer(server);
        ht->run(); ", "connects to hub server and runs");

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
