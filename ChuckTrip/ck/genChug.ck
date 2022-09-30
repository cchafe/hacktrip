// run from /home/cc/hacktrip/ChuckTrip/ck
// chuck -s template.ck genChug.ck 
// chuck -s newTemplate.ck genChug.ck 

<<<"replacing ../chucktrip.cpp">>>;
FileIO fout;
fout.open("../chucktrip.cpp",FileIO.WRITE);

fout <= "// this file was automatically generated with \n // chuck -s newTemplate.ck genChug.ck \n";
Template t;

["",""] @=> string args[];

//t.newFun("hi", "void", args, "  fprintf(stderr,\"ChuckTrip Hi \\n\");  ", "prints --hi-- msg");
//t.newFun("bye", "void", args, "  fprintf(stderr,\"ChuckTrip Bye \\n\");  ", "prints --bye-- msg");
//t.newFun("htFPP", "int", args, "  ht->getFPP();  ", "prints --htFPP--");
//[ "QString","word"] @=> args;
//t.newFun("printSomeString", "void", args, "  fprintf(stderr,\"ChuckTrip %s \\n\", 
//  word.toStdString().c_str());  ", "prints --bye-- msg");

["int","nChans"] @=> args;
t.newFun("setChannels", "void", args, "if (nChans) ht->setChannels(nChans);
        m_channels = ht->getChannels();
        setBuffers();
", "sets number of channels");

["int","FPP"] @=> args;
t.newFun("setFPP", "void", args, "if (FPP) ht->setFPP(FPP);
        m_FPP = ht->getFPP();
        setBuffers();
", "sets FPP");

["int","use"] @=> args;
t.newFun("setUsePLC", "void", args, "if (use) ht->setUsePLC(use);
        m_PLC = ht->getUsePLC();
", "sets PLC");

["int","udpPort"] @=> args;
t.newFun("setLocalUDPaudioPort", "void", args, "ht->setLocalUDPaudioPort(udpPort);", "sets local UDP port for incoming stream");

["QString","server"] @=> args;
t.newFun("connectTo", "void", args, "ht = new Hapitrip();
        ht->connectToServer(server);
        ht->run(); ", "connects to hub server and runs");

["",""] @=> args;
t.newFun("getChannels", "int", args, " m_channels; ", "returns internal number of channels");

t.newFun("getFPP", "int", args, " m_FPP; ", "returns internal FPP");

t.newFun("getUsePLC", "int", args, " m_PLC; ", "returns internal PLC state");

t.newFun("getLocalUDPaudioPort", "int", args, " ht->getLocalUDPaudioPort(); ", "returns localUDPaudioPort");

t.newFun("disconnect", "void", args, "ht->stop();", "disconnects from hub server");

fout <= t.INCLUDES;
fout <= t.STATIC_DECLARATIONS;
fout <= t.START_CLASS;
fout <= t.CLASS_CONSTRUCTOR + "\n}";
fout <= t.CLASS_DESTRUCTOR;
fout <= t.CLASS_PUBLIC_METHODS;
fout <= t.CLASS_PUBLIC_MEMBERS;
fout <= t.CLASS_PRIVATE_METHODS;
fout <= t.CLASS_PRIVATE_MEMBERS;
fout <= t.FINISH_CLASS;
fout <= t.QUERY_CLASS;
fout <= t.FINISH_QUERY_CLASS;
fout <= t.STATIC_IMPLEMENTATIONS;

fout.close();
