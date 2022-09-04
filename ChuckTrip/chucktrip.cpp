// this file was automatically generated with 
 // chuck -s chuck -s template.ck genChug.ck 

#include "chuck_dl.h"
#include "chuck_def.h"

#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "hapitrip.h"
#include <iostream>

CK_DLL_CTOR(ChuckTrip_ctor);
CK_DLL_DTOR(ChuckTrip_dtor);
CK_DLL_TICKF(ChuckTrip_tick);

t_CKINT ChuckTrip_data_offset = 0;

CK_DLL_MFUN(ChuckTrip_setLocalUDPaudioPort);

CK_DLL_MFUN(ChuckTrip_connectTo);

CK_DLL_MFUN(ChuckTrip_disconnect);

CK_DLL_MFUN(ChuckTrip_getFPP);

class ChuckTrip
{
public:
    
    ChuckTrip(float fs)
    {
        m_fs = fs;
        m_FPP = ht->getFPP();
        m_sendBuffer = new float[m_FPP * 2]; // garnered from pitchtrack chugin
        m_rcvBuffer = new float[m_FPP * 2];
        for (int i = 0; i < m_FPP * 2; i++)
        {
            m_sendBuffer[i] = 0.0;
            m_rcvBuffer[i] = 0.0;
        }
        m_sampleCount = 0;

}
    ~ChuckTrip ()
    {
        fprintf(stderr,"ChuckTrip dtor reached!! \n");
        std::cout << "ChuckTrip: I'd be surprised if this ever prints" << std::endl;
        free(m_sendBuffer);
        free(m_rcvBuffer);
    }


    void tick( SAMPLE * in, SAMPLE * out, int nframes ) // nframes = 1
    {
        // needs work if more than stereo
        int nChans = 2; // need to add a method to set from Hapitrip::as.channels
        m_sampleCount %= m_FPP;
        memset(out, 0, sizeof(SAMPLE)*nChans*nframes);
        for (int i=0; i < nframes; i+=nChans)
        {
            m_sendBuffer[m_sampleCount*nChans] = in[i];
            m_sendBuffer[m_sampleCount*nChans+1] = in[i+1];
            out[i] = m_rcvBuffer[m_sampleCount*nChans];
            out[i+1] = m_rcvBuffer[m_sampleCount*nChans+1];
        }
        m_sampleCount++;
        if (m_sampleCount==m_FPP) {
            ht->xfrBufs(m_sendBuffer, m_rcvBuffer);
        }
    }
    

    void setLocalUDPaudioPort(int udpPort) {
        ht->setLocalUDPaudioPort(udpPort);
       // void function
    }

    void connectTo(QString server) {
        ht = new Hapitrip();
        ht->connectToServer(server);
        ht->run(); 
       // void function
    }

    void disconnect( ) {
        ht->stop();
       // void function
    }

    t_CKINT getFPP( ) {
        int rtnInt =  m_FPP; 
       return rtnInt;
    }

public:
    char * m_server;

private:
    t_CKFLOAT m_fs;
    t_CKINT m_FPP;
    Hapitrip *ht;
    t_CKINT m_sampleCount;
    float *m_sendBuffer;
    float *m_rcvBuffer;

};

CK_DLL_QUERY(ChuckTrip)
{
    QUERY->setname(QUERY, "ChuckTrip");
    
    QUERY->begin_class(QUERY, "ChuckTrip", "UGen");
    QUERY->doc_class(QUERY, "Fast, recursive sine wave generator using the so-called &quot;magic circle&quot; algorithm ");
    
    QUERY->add_ctor(QUERY, ChuckTrip_ctor);
    QUERY->add_dtor(QUERY, ChuckTrip_dtor);
    
    // QUERY->add_ugen_func(QUERY, ChuckTrip_tick, NULL, 1, 1);
    QUERY->add_ugen_funcf(QUERY, ChuckTrip_tick, NULL, 2, 2);

    QUERY->add_mfun(QUERY, ChuckTrip_setLocalUDPaudioPort, "void", "setLocalUDPaudioPort");
    QUERY->doc_func(QUERY, "setLocalUDPaudioPort: sets local UDP port for incoming stream");
    QUERY->add_arg(QUERY, "int", "arg");

    QUERY->add_mfun(QUERY, ChuckTrip_connectTo, "void", "connectTo");
    QUERY->doc_func(QUERY, "connectTo: connects to hub server and runs");
    QUERY->add_arg(QUERY, "string", "arg");

    QUERY->add_mfun(QUERY, ChuckTrip_disconnect, "void", "disconnect");
    QUERY->doc_func(QUERY, "disconnect: disconnects from hub server");

    QUERY->add_mfun(QUERY, ChuckTrip_getFPP, "int", "getFPP");
    QUERY->doc_func(QUERY, "getFPP: returns innternal FPP");

    ChuckTrip_data_offset = QUERY->add_mvar(QUERY, "int", "@ChuckTrip_data", false);
    
    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(ChuckTrip_ctor)
{
    OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset) = 0;
    
    ChuckTrip * bcdata = new ChuckTrip(API->vm->get_srate(API, SHRED));
    
    OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset) = (t_CKINT) bcdata;
}

CK_DLL_DTOR(ChuckTrip_dtor)
{
    std::cout << "ChuckTrip: destructor !!" << std::endl;
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

CK_DLL_MFUN(ChuckTrip_setLocalUDPaudioPort)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    bcdata->setLocalUDPaudioPort(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(ChuckTrip_connectTo)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    bcdata->connectTo(GET_NEXT_STRING(ARGS)->c_str());
}

CK_DLL_MFUN(ChuckTrip_disconnect)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    bcdata->disconnect();
}

CK_DLL_MFUN(ChuckTrip_getFPP)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_int = bcdata->getFPP();
}
