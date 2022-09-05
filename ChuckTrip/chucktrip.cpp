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

CK_DLL_MFUN(ChuckTrip_setChannels);

CK_DLL_MFUN(ChuckTrip_setFPP);

CK_DLL_MFUN(ChuckTrip_setLocalUDPaudioPort);

CK_DLL_MFUN(ChuckTrip_connectTo);

CK_DLL_MFUN(ChuckTrip_getChannels);

CK_DLL_MFUN(ChuckTrip_getFPP);

CK_DLL_MFUN(ChuckTrip_getLocalUDPaudioPort);

CK_DLL_MFUN(ChuckTrip_disconnect);

class ChuckTrip
{
public:
    
    ChuckTrip(float fs)
    {
        m_fs = fs;
        m_sampleCount = 0;
        setFPP(0); // use ht default
        setChannels(0); // use ht default

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
        if (!m_channels || (m_channels>2)) fprintf(stderr,"ChuckTrip m_channels = %d!! \n",m_channels);
        m_sampleCount %= m_FPP;
        memset(out, 0, sizeof(SAMPLE)*m_channels*nframes);
        for (int i=0; i < nframes; i+=m_channels)
        {
            m_sendBuffer[m_sampleCount*m_channels] = in[i];
            m_sendBuffer[m_sampleCount*m_channels+1] = in[i+1];
            out[i] = m_rcvBuffer[m_sampleCount*m_channels];
            out[i+1] = m_rcvBuffer[m_sampleCount*m_channels+1];
        }
        m_sampleCount++;
        if (m_sampleCount==m_FPP) {
            ht->xfrBufs(m_sendBuffer, m_rcvBuffer);
        }
    }
    

    void setChannels(int nChans) {
        if (nChans) ht->setChannels(nChans);
        m_channels = ht->getChannels();
        setBuffers();

       // void function
    }

    void setFPP(int FPP) {
        if (FPP) ht->setFPP(FPP);
        m_FPP = ht->getFPP();
        setBuffers();

       // void function
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

    t_CKINT getChannels( ) {
        int rtnInt =  m_channels; 
       return rtnInt;
    }

    t_CKINT getFPP( ) {
        int rtnInt =  m_FPP; 
       return rtnInt;
    }

    t_CKINT getLocalUDPaudioPort( ) {
        int rtnInt =  ht->getLocalUDPaudioPort(); 
       return rtnInt;
    }

    void disconnect( ) {
        ht->stop();
       // void function
    }

public:
    char * m_server;

private:
    void setBuffers() {
        m_sendBuffer = new float[m_FPP * m_channels]; // garnered from pitchtrack chugin
        m_rcvBuffer = new float[m_FPP * m_channels];
        for (int i = 0; i < m_FPP * m_channels; i++) {
          m_sendBuffer[i] = 0.0;
          m_rcvBuffer[i] = 0.0;
        }
    };

private:
    t_CKFLOAT m_fs;
    t_CKINT m_channels;
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

    QUERY->add_mfun(QUERY, ChuckTrip_setChannels, "void", "setChannels");
    QUERY->doc_func(QUERY, "setChannels: sets number of channels");
    QUERY->add_arg(QUERY, "int", "arg");

    QUERY->add_mfun(QUERY, ChuckTrip_setFPP, "void", "setFPP");
    QUERY->doc_func(QUERY, "setFPP: sets FPP");
    QUERY->add_arg(QUERY, "int", "arg");

    QUERY->add_mfun(QUERY, ChuckTrip_setLocalUDPaudioPort, "void", "setLocalUDPaudioPort");
    QUERY->doc_func(QUERY, "setLocalUDPaudioPort: sets local UDP port for incoming stream");
    QUERY->add_arg(QUERY, "int", "arg");

    QUERY->add_mfun(QUERY, ChuckTrip_connectTo, "void", "connectTo");
    QUERY->doc_func(QUERY, "connectTo: connects to hub server and runs");
    QUERY->add_arg(QUERY, "string", "arg");

    QUERY->add_mfun(QUERY, ChuckTrip_getChannels, "int", "getChannels");
    QUERY->doc_func(QUERY, "getChannels: returns internal number of channels");

    QUERY->add_mfun(QUERY, ChuckTrip_getFPP, "int", "getFPP");
    QUERY->doc_func(QUERY, "getFPP: returns internal FPP");

    QUERY->add_mfun(QUERY, ChuckTrip_getLocalUDPaudioPort, "int", "getLocalUDPaudioPort");
    QUERY->doc_func(QUERY, "getLocalUDPaudioPort: returns localUDPaudioPort");

    QUERY->add_mfun(QUERY, ChuckTrip_disconnect, "void", "disconnect");
    QUERY->doc_func(QUERY, "disconnect: disconnects from hub server");

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

CK_DLL_MFUN(ChuckTrip_setChannels)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    bcdata->setChannels(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(ChuckTrip_setFPP)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    bcdata->setFPP(GET_NEXT_INT(ARGS));
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

CK_DLL_MFUN(ChuckTrip_getChannels)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_int = bcdata->getChannels();
}

CK_DLL_MFUN(ChuckTrip_getFPP)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_int = bcdata->getFPP();
}

CK_DLL_MFUN(ChuckTrip_getLocalUDPaudioPort)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_int = bcdata->getLocalUDPaudioPort();
}

CK_DLL_MFUN(ChuckTrip_disconnect)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    bcdata->disconnect();
}
