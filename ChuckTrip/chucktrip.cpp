/*----------------------------------------------------------------------------
 ChucK to ChuckTrip
 -----------------------------------------------------------------------------*/

#include "chuck_dl.h"
#include "chuck_def.h"

#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "hapitrip.h"
#include <iostream>

CK_DLL_CTOR(ChuckTrip_ctor);
CK_DLL_DTOR(ChuckTrip_dtor);

CK_DLL_MFUN(ChuckTrip_setFreq);
CK_DLL_MFUN(ChuckTrip_getFreq);

// CK_DLL_TICK(ChuckTrip_tick);
CK_DLL_TICKF(ChuckTrip_tick);
CK_DLL_MFUN(ChuckTrip_connect);
CK_DLL_MFUN(ChuckTrip_disconnect);

CK_DLL_MFUN(ChuckTrip_setLocalUDPaudioPort);

CK_DLL_MFUN(ChuckTrip_getFPP);

t_CKINT ChuckTrip_data_offset = 0;

class ChuckTrip
{
public:
    
    ChuckTrip(float fs)
    {
        m_fs = fs;
        setFreq(440); // internal magic sine
        m_x = 1;
        m_y = 0;
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

    t_CKFLOAT connect(char *filename)
    {
        //        fprintf(stderr,"%s",filename);
        ht = new Hapitrip();
        ht->connectToServer(filename);
        ht->run();
        return(0.0);
    }

    t_CKFLOAT disconnect()
    {
        ht->stop();
        return(0.0);
    }

    t_CKFLOAT setLocalUDPaudioPort(int port)
    {
        ht->setLocalUDPaudioPort(port);
        return(0.0);
    }

    /*
mono tick sample version
    SAMPLE tick(SAMPLE in)
    {
        m_sampleCount %= m_FPP;
        m_sendBuffer[m_sampleCount] = in;
        t_CKFLOAT out = m_rcvBuffer[m_sampleCount];
        m_x = m_x + m_epsilon*m_y;
        m_y = -m_epsilon*m_x + m_y;
        //        m_sendBuffer[m_sampleCount] = m_y; // internal magic sine
        m_sampleCount++;
        if (m_sampleCount==m_FPP) {
            ht->xfrBufs(m_sendBuffer, m_rcvBuffer);
        }
        return out;
    }
*/

    void tick( SAMPLE * in, SAMPLE * out, int nframes ) // nframes = 1
    {
        // needs work if more than stereo
        int nChans = 1; // need to add a method to set from Hapitrip::as.channels
        m_x = m_x + m_epsilon*m_y;
        m_y = -m_epsilon*m_x + m_y;
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
    
    t_CKFLOAT setFreq(t_CKFLOAT f)
    {
        m_freq = f;
        m_epsilon = 2.0*sin(2.0*ONE_PI*(m_freq/m_fs)/2.0);
        return m_freq;
    }

    t_CKFLOAT getFreq() {
        return m_freq;
    }

    t_CKINT getFPP() {
        return m_FPP;
    }

    char * m_server;
private:
    SAMPLE m_x, m_y;
    t_CKFLOAT m_fs;
    t_CKFLOAT m_freq;
    t_CKFLOAT m_epsilon;
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
    QUERY->doc_class(QUERY, "Fast, recursive sine wave generator using the so-called &quot;magic circle&quot; algorithm (see <a href=\"https://ccrma.stanford.edu/~jos/pasp/Digital_Sinusoid_Generators.html\">https://ccrma.stanford.edu/~jos/pasp/Digital_Sinusoid_Generators.html</a>). "
                            "Can be 30-40% faster than regular SinOsc. "
                            "Frequency modulation will negate this performance benefit; most useful when pure sine tones are desired or for additive synthesis. ");
    
    QUERY->add_ctor(QUERY, ChuckTrip_ctor);
    QUERY->add_dtor(QUERY, ChuckTrip_dtor);
    
    // QUERY->add_ugen_func(QUERY, ChuckTrip_tick, NULL, 1, 1);
    QUERY->add_ugen_funcf(QUERY, ChuckTrip_tick, NULL, 2, 2);

    QUERY->add_mfun(QUERY, ChuckTrip_setFreq, "float", "freq");
    QUERY->add_arg(QUERY, "float", "arg");
    QUERY->doc_func(QUERY, "Oscillator frequency [Hz]. ");

    QUERY->add_mfun(QUERY, ChuckTrip_getFreq, "float", "freq");
    QUERY->doc_func(QUERY, "Oscillator frequency [Hz]. ");

    QUERY->add_mfun(QUERY, ChuckTrip_connect, "void", "connect");
    QUERY->add_arg(QUERY, "string", "name" );
    QUERY->doc_func(QUERY, "Server name. ");

    QUERY->add_mfun(QUERY, ChuckTrip_disconnect, "void", "disconnect");

    QUERY->add_mfun(QUERY, ChuckTrip_setLocalUDPaudioPort, "int", "localUDPAudioPort");
    QUERY->add_arg(QUERY, "int", "arg");
    QUERY->doc_func(QUERY, "LocalUDPaudioPort. ");

    QUERY->add_mfun(QUERY, ChuckTrip_getFPP, "int", "fpp");
    QUERY->doc_func(QUERY, "Oscillator frequency [Hz]. ");

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
    std::cout << "ChuckTrip: !!" << std::endl;
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    if(bcdata)
    {
        delete bcdata;
        OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset) = 0;
        bcdata = NULL;
    }
}

/*
CK_DLL_TICK(ChuckTrip_tick)
{
    ChuckTrip * c = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    
    if(c) *out = c->tick(in);

    return TRUE;
}
*/
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

CK_DLL_MFUN(ChuckTrip_setFreq)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    // TODO: sanity check
    RETURN->v_float = bcdata->setFreq(GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(ChuckTrip_getFreq)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_float = bcdata->getFreq();
}

CK_DLL_MFUN(ChuckTrip_connect)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    bcdata->m_server = (char *)GET_NEXT_STRING(ARGS)->c_str();
    RETURN->v_float = bcdata->connect(bcdata->m_server);
    //    RETURN->v_float = bcdata->connect((char *)GET_NEXT_STRING(ARGS)->c_str());
}

CK_DLL_MFUN(ChuckTrip_disconnect)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_float = bcdata->disconnect();
}

CK_DLL_MFUN(ChuckTrip_setLocalUDPaudioPort)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_float = bcdata->setLocalUDPaudioPort(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(ChuckTrip_getFPP)
{
    ChuckTrip * bcdata = (ChuckTrip *) OBJ_MEMBER_INT(SELF, ChuckTrip_data_offset);
    RETURN->v_int = bcdata->getFPP();
}
