// run from /home/cc/hacktrip/ChuckTrip/ck
// chuck -s template.ck genChug.ck 

<<<"replacing ../chucktrip.cpp">>>;
FileIO fout;
fout.open("../chucktrip.cpp",FileIO.WRITE);

fout <= "// this file was automatically generated with \n // chuck -s chuck -s template.ck genChug.ck \n";
Template t;

"
CK_DLL_MFUN(ChuckTrip_sayHi);
" +=> t.STATIC_DECLARATIONS;

"
    t_CKINT sayHi() {
        fprintf(stderr,\"ChuckTrip Hi \\n\");
        return 0;
    }
" +=> t.CLASS_PUBLIC_METHODS;

"
    QUERY->add_mfun(QUERY, ChuckTrip_sayHi, \"int\", \"hi\");
    QUERY->doc_func(QUERY, \"Oscillator frequency [Hz]. \");
" +=> t.QUERY_CLASS;

"
CK_DLL_MFUN(ChuckTrip_sayHi)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_int = bcdata->sayHi();
}
" +=> t.STATIC_IMPLEMENTATIONS;









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
