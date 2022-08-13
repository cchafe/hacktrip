/*----------------------------------------------------------------------------
 ChucK to chucktrip
 -----------------------------------------------------------------------------*/

#include "chuck_dl.h"
#include "chuck_def.h"

#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "hapitrip.h"
#include <iostream>

CK_DLL_CTOR(chucktrip_ctor);
CK_DLL_DTOR(chucktrip_dtor);

CK_DLL_MFUN(chucktrip_setFreq);
CK_DLL_MFUN(chucktrip_getFreq);

CK_DLL_TICK(chucktrip_tick);
CK_DLL_MFUN(chucktrip_connect);
CK_DLL_MFUN(chucktrip_disconnect);

CK_DLL_MFUN(chucktrip_getFPP);

t_CKINT chucktrip_data_offset = 0;

class chucktrip
{
public:
    
    chucktrip(float fs)
    {
        m_fs = fs;
        setFreq(440); // internal magic sine
        m_x = 1;
        m_y = 0;
        m_FPP = ht->getFPP();
        m_sendBuffer = new float[m_FPP]; // garnered from pitchtrack chugin
        m_rcvBuffer = new float[m_FPP];
        for (int i = 0; i < m_FPP; i++)
        {
            m_sendBuffer[i] = 0.0;
            m_rcvBuffer[i] = 0.0;
        }
        m_sampleCount = 0;
    }

    ~chucktrip ()
    {
        fprintf(stderr,"chucktrip dtor reached!! \n");
        std::cout << "chucktrip: I'd be surprised if this ever prints" << std::endl;
        free(m_sendBuffer);
        free(m_rcvBuffer);
    }

    t_CKFLOAT connect()
    {
        ht = new Hapitrip();
        ht->connectToServer();
        ht->run();
         return(0.0);
    }

    t_CKFLOAT disconnect()
    {
        ht->stop();
        return(0.0);
    }

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

CK_DLL_QUERY(chucktrip)
{
    QUERY->setname(QUERY, "chucktrip");
    
    QUERY->begin_class(QUERY, "chucktrip", "UGen");
    QUERY->doc_class(QUERY, "Fast, recursive sine wave generator using the so-called &quot;magic circle&quot; algorithm (see <a href=\"https://ccrma.stanford.edu/~jos/pasp/Digital_Sinusoid_Generators.html\">https://ccrma.stanford.edu/~jos/pasp/Digital_Sinusoid_Generators.html</a>). "
                            "Can be 30-40% faster than regular SinOsc. "
                            "Frequency modulation will negate this performance benefit; most useful when pure sine tones are desired or for additive synthesis. ");
    
    QUERY->add_ctor(QUERY, chucktrip_ctor);
    QUERY->add_dtor(QUERY, chucktrip_dtor);
    
    QUERY->add_ugen_func(QUERY, chucktrip_tick, NULL, 1, 1);
    
    QUERY->add_mfun(QUERY, chucktrip_setFreq, "float", "freq");
    QUERY->add_arg(QUERY, "float", "arg");
    QUERY->doc_func(QUERY, "Oscillator frequency [Hz]. ");

    QUERY->add_mfun(QUERY, chucktrip_getFreq, "float", "freq");
    QUERY->doc_func(QUERY, "Oscillator frequency [Hz]. ");

    QUERY->add_mfun(QUERY, chucktrip_connect, "void", "connect");
    QUERY->add_mfun(QUERY, chucktrip_disconnect, "void", "disconnect");

    QUERY->add_mfun(QUERY, chucktrip_getFPP, "int", "fpp");
    QUERY->doc_func(QUERY, "Oscillator frequency [Hz]. ");

    chucktrip_data_offset = QUERY->add_mvar(QUERY, "int", "@chucktrip_data", false);
    
    QUERY->end_class(QUERY);

    return TRUE;
}


CK_DLL_CTOR(chucktrip_ctor)
{
    OBJ_MEMBER_INT(SELF, chucktrip_data_offset) = 0;
    
    chucktrip * bcdata = new chucktrip(API->vm->get_srate(API, SHRED));
    
    OBJ_MEMBER_INT(SELF, chucktrip_data_offset) = (t_CKINT) bcdata;
}

CK_DLL_DTOR(chucktrip_dtor)
{
    std::cout << "chucktrip: !!" << std::endl;
    chucktrip * bcdata = (chucktrip *) OBJ_MEMBER_INT(SELF, chucktrip_data_offset);
    if(bcdata)
    {
        delete bcdata;
        OBJ_MEMBER_INT(SELF, chucktrip_data_offset) = 0;
        bcdata = NULL;
    }
}

CK_DLL_TICK(chucktrip_tick)
{
    chucktrip * c = (chucktrip *) OBJ_MEMBER_INT(SELF, chucktrip_data_offset);
    
    if(c) *out = c->tick(in);

    return TRUE;
}

CK_DLL_MFUN(chucktrip_setFreq)
{
    chucktrip * bcdata = (chucktrip *) OBJ_MEMBER_INT(SELF, chucktrip_data_offset);
    // TODO: sanity check
    RETURN->v_float = bcdata->setFreq(GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(chucktrip_getFreq)
{
    chucktrip * bcdata = (chucktrip *) OBJ_MEMBER_INT(SELF, chucktrip_data_offset);
    RETURN->v_float = bcdata->getFreq();
}

CK_DLL_MFUN(chucktrip_connect)
{
    chucktrip * bcdata = (chucktrip *) OBJ_MEMBER_INT(SELF, chucktrip_data_offset);
    RETURN->v_float = bcdata->connect();
}

CK_DLL_MFUN(chucktrip_disconnect)
{
    chucktrip * bcdata = (chucktrip *) OBJ_MEMBER_INT(SELF, chucktrip_data_offset);
    RETURN->v_float = bcdata->disconnect();
}

CK_DLL_MFUN(chucktrip_getFPP)
{
    chucktrip * bcdata = (chucktrip *) OBJ_MEMBER_INT(SELF, chucktrip_data_offset);
    RETURN->v_int = bcdata->getFPP();
}
