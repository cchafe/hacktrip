/*----------------------------------------------------------------------------
 ChucK to ChuckTrip
 -----------------------------------------------------------------------------*/

public class Template {
"
#include \"chuck_dl.h\"
#include \"chuck_def.h\"

#include <stdio.h>
#include <limits.h>
#include <math.h>

#include \"hapitrip.h\"
#include <iostream>
" => string INCLUDES; ////////////////////////////////////////////

"
CK_DLL_CTOR(ChuckTrip_ctor);
CK_DLL_DTOR(ChuckTrip_dtor);
CK_DLL_TICKF(ChuckTrip_tick);

t_CKINT ChuckTrip_data_offset = 0;
" => string STATIC_DECLARATIONS; ////////////////////////////////////////////

"
class ChuckTrip
{
public:
" => string START_CLASS; ////////////////////////////////////////////

"    
    ChuckTrip(float fs)
    {
        m_fs = fs;
        m_sampleCount = 0;
        setFPP(0); // use ht default
        setUsePLC(0); // use ht default
" => string CLASS_CONSTRUCTOR; ////////////////////////////////////////////

"
    ~ChuckTrip ()
    {
        fprintf(stderr,\"ChuckTrip dtor reached!! \\n\");
        std::cout << \"ChuckTrip: I'd be surprised if this ever prints\" << std::endl;
        free(m_sendBuffer);
        free(m_rcvBuffer);
    }
" => string CLASS_DESTRUCTOR; ////////////////////////////////////////////

"

    void tick( SAMPLE * in, SAMPLE * out, int nframes ) // nframes = 1
    {
        // needs work if more than stereo
        if (!m_channels || (m_channels>2)) fprintf(stderr,\"ChuckTrip m_channels = %d!! \\n\",m_channels);
        m_sampleCount %= m_FPP;
        memset(out, 0, sizeof(SAMPLE)*m_channels*nframes);
        for (int i=0; i < nframes; i+=m_channels)
        {
          for (int j=0; j < m_channels; j++) {
              m_sendBuffer[m_sampleCount*m_channels] = in[i+j];
              out[i+j] = m_rcvBuffer[m_sampleCount*m_channels+j];
          }
        }
        m_sampleCount++;
        if (m_sampleCount==m_FPP) {
            ht->xfrBufs(m_sendBuffer, m_rcvBuffer);
        }
    }
    
" => string CLASS_PUBLIC_METHODS; ////////////////////////////////////////////

"
public:
    char * m_server;
" => string CLASS_PUBLIC_MEMBERS; ////////////////////////////////////////////

"
private:
    void setBuffers() {
        m_sendBuffer = new float[m_FPP * m_channels]; // garnered from pitchtrack chugin
        m_rcvBuffer = new float[m_FPP * m_channels];
        for (int i = 0; i < m_FPP * m_channels; i++) {
          m_sendBuffer[i] = 0.0;
          m_rcvBuffer[i] = 0.0;
        }
    };
" => string CLASS_PRIVATE_METHODS; ////////////////////////////////////////////

"
private:
    t_CKFLOAT m_fs;
    t_CKINT m_channels;
    t_CKINT m_FPP;
    t_CKINT m_PLC;
    Hapitrip *ht;
    t_CKINT m_sampleCount;
    float *m_sendBuffer;
    float *m_rcvBuffer;
" => string CLASS_PRIVATE_MEMBERS; ////////////////////////////////////////////

"
};
" => string FINISH_CLASS; ////////////////////////////////////////////

"
CK_DLL_QUERY(ChuckTrip)
{
    QUERY->setname(QUERY, \"ChuckTrip\");
    
    QUERY->begin_class(QUERY, \"ChuckTrip\", \"UGen\");
    QUERY->doc_class(QUERY, \"Fast, recursive sine wave generator using the so-called &quot;magic circle&quot; algorithm \");
    
    QUERY->add_ctor(QUERY, ChuckTrip_ctor);
    QUERY->add_dtor(QUERY, ChuckTrip_dtor);
    
    // QUERY->add_ugen_func(QUERY, ChuckTrip_tick, NULL, 1, 1);
    QUERY->add_ugen_funcf(QUERY, ChuckTrip_tick, NULL, 2, 2);
" => string QUERY_CLASS; ////////////////////////////////////////////

"
    ChuckTrip_data_offset = QUERY->add_mvar(QUERY, \"int\", \"@ChuckTrip_data\", false);
    
    QUERY->end_class(QUERY);

    return TRUE;
}
" => string FINISH_QUERY_CLASS; ////////////////////////////////////////////


"
CK_DLL_CTOR(ChuckTrip_ctor)
{
    OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset) = 0;
    
    ChuckTrip * bcdata = new ChuckTrip(API->vm->get_srate(API, SHRED));
    
    OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset) = (t_CKINT) bcdata;
}

CK_DLL_DTOR(ChuckTrip_dtor)
{
    std::cout << \"ChuckTrip: destructor !!\" << std::endl;
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    if(bcdata)
    {
        delete bcdata;
        OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset) = 0;
        bcdata = NULL;
    }
}

// implementation for tick function
CK_DLL_TICKF(ChuckTrip_tick)
{
    // get our c++ class pointer
    ChuckTrip * c = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);

    // invoke our tick function; store in the magical out variable
    if(c) c->tick(in,out, nframes);

    // yes
    return TRUE;
}
" => string STATIC_IMPLEMENTATIONS; ////////////////////////////////////////////

/////////////////////////////
fun void newFun (string name, string returnType, string args[], string body, string docText) {

  "" => string methodArgType;
  "" => string methodArgVar;
  "" => string queryArgType;
  "" => string staticArgType;
  args.cap()/2 => int argCnt; // wired for only 1 arg at the moment
  args[0] => methodArgType; 
  args[1] => methodArgVar; 
  if (args[0]=="QString") {
      "string" => queryArgType;
      "GET_NEXT_STRING(ARGS)->c_str()" => staticArgType;
  }
  else if (args[0]=="int") {
      "int" => queryArgType;
      "GET_NEXT_INT(ARGS)" => staticArgType;
  }
  else 0 => argCnt;
 
  name => string callWith; // callWith is the eventual chuck call and could be set differently
  "void" => string methodReturnType;
  "// void function" => string methodReturnCode;
  "" => string methodReturnVar;
  "" => string staticReturnCode;
  if (returnType=="int") {
    "t_CKINT" => methodReturnType;
    "int rtnInt = "  => methodReturnVar; // reserved var rtnInt for magic
    "return rtnInt;" => methodReturnCode; // reserved var rtnInt for magic
    "RETURN->v_int = " => staticReturnCode; // chuck magic 
  }
  if (returnType=="float") {
    "t_CKFLOAT" => methodReturnType;
    "float rtnFloat = "  => methodReturnVar; // reserved var rtnFloat for magic
    "return rtnFloat;" => methodReturnCode; // reserved var rtnFloat for magic
    "RETURN->v_float = " => staticReturnCode; // chuck magic 
  }
"
CK_DLL_MFUN(ChuckTrip_"+name+");
" +=> STATIC_DECLARATIONS;

"
    "+methodReturnType+" "+name+"("+methodArgType+" "+methodArgVar+") {
        "+methodReturnVar+body+"
       "+methodReturnCode+"
    }
" +=> CLASS_PUBLIC_METHODS;

"
    QUERY->add_mfun(QUERY, ChuckTrip_"+name+", \""+returnType+"\", \""+callWith+"\");
    QUERY->doc_func(QUERY, \""+name+": "+docText+"\");
" +=> QUERY_CLASS;

for (0 => int arg; arg < argCnt; arg++) {  // but wired for only 1 arg at the moment
"    QUERY->add_arg(QUERY, \""+queryArgType+"\", \"arg\");
" +=> QUERY_CLASS;
}

"
CK_DLL_MFUN(ChuckTrip_"+name+")
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    "+staticReturnCode+"bcdata->"+name+"("+staticArgType+");
}
" +=> STATIC_IMPLEMENTATIONS;
}
/////////////////////////////
}

