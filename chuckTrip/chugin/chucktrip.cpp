/*----------------------------------------------------------------------------
 ChucK to ssr
 -----------------------------------------------------------------------------*/

#include "chuck_dl.h"
#include "chuck_def.h"

#include <stdio.h>
#include <limits.h>
#include <math.h>
#include "tcp.h"

CK_DLL_CTOR(ssr_ctor);
CK_DLL_DTOR(ssr_dtor);

CK_DLL_MFUN(ssr_setFreq);
CK_DLL_MFUN(ssr_addSrc);
CK_DLL_MFUN(ssr_mvSrc);
CK_DLL_MFUN(ssr_mvSrcV);
CK_DLL_MFUN(ssr_getFreq);
CK_DLL_MFUN(ssr_checkConnection);

CK_DLL_TICK(ssr_tick);
CK_DLL_MFUN(ssr_connect);

t_CKINT ssr_data_offset = 0;


class ssr
{
public:
    
    ssr(float fs)
    {
        m_fs = fs;
        setFreq(1440);
//        fprintf(stderr,"xxxxxxxxxxxxxxxxxx  %g\n",fs);
        m_x = 1;
        m_y = 0;
        tcp = new TCP();
    }

    t_CKFLOAT connect()
    {
      tcp->connectToHost();
      if (tcp->socket->waitForConnected(1000))
        fprintf(stderr,"Connected to server\n");
      return(0.0);
    }

    SAMPLE tick(SAMPLE in)
    {
        m_x = m_x + m_epsilon*m_y;
        m_y = -m_epsilon*m_x + m_y;
        return m_y;
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

    t_CKFLOAT checkConnection() {
//        if (tcp->socket->state() == QTcpSocket::ConnectedState)
//            tcp->connected();
//        else
//            tcp->disconnected();
        double x = m_x - 1.0;
        x *= 10000000.0;
        tcp->sendToHost(x, 2.0);

//        return tcp->socket->state();
        return x;
    }

    t_CKFLOAT addSrc(t_CKFLOAT i) {
        tcp->addSrc((int)i,0.0, 2.0);
        return 0.0;
    }

    t_CKFLOAT mvSrc(t_CKFLOAT i) {
        double x = m_x - 1.0;
        x *= 10000000.0;
        tcp->mvSrc((int)i, x, 2.0, 2.0);
        return 0.0;
    }

    t_CKFLOAT mvSrcV(t_CKVEC4 v) {
//        v.x = m_x - 1.0;
//        v.x *= 10000000.0;
        int i = (int)v.w;
        tcp->mvSrc(i, v.x, v.y, v.z);
        return 0.0;
    }

private:
TCP * tcp;
    SAMPLE m_x, m_y;
    t_CKFLOAT m_fs;
    t_CKFLOAT m_freq;
    t_CKFLOAT m_epsilon;
};

CK_DLL_QUERY(ssr)
{
    QUERY->setname(QUERY, "ssr");
    
    QUERY->begin_class(QUERY, "ssr", "UGen");
    QUERY->doc_class(QUERY, "Fast, recursive sine wave generator using the so-called &quot;magic circle&quot; algorithm (see <a href=\"https://ccrma.stanford.edu/~jos/pasp/Digital_Sinusoid_Generators.html\">https://ccrma.stanford.edu/~jos/pasp/Digital_Sinusoid_Generators.html</a>). "
        "Can be 30-40% faster than regular SinOsc. "
        "Frequency modulation will negate this performance benefit; most useful when pure sine tones are desired or for additive synthesis. ");
    
    QUERY->add_ctor(QUERY, ssr_ctor);
    QUERY->add_dtor(QUERY, ssr_dtor);
    
    QUERY->add_ugen_func(QUERY, ssr_tick, NULL, 1, 1);
    
    QUERY->add_mfun(QUERY, ssr_setFreq, "float", "freq");
    QUERY->add_arg(QUERY, "float", "arg");
    QUERY->doc_func(QUERY, "Oscillator frequency [Hz]. ");

    QUERY->add_mfun(QUERY, ssr_addSrc, "float", "addSrc");
    QUERY->add_arg(QUERY, "float", "arg");
    QUERY->doc_func(QUERY, "Oscillator frequency again[Hz]. ");

    QUERY->add_mfun(QUERY, ssr_mvSrc, "float", "mvSrc");
    QUERY->add_arg(QUERY, "float", "arg");
    QUERY->doc_func(QUERY, "Oscillator frequency again[Hz]. ");

    QUERY->add_mfun(QUERY, ssr_mvSrcV, "float", "mvSrcV");
    QUERY->add_arg(QUERY, "vec4", "arg");
    QUERY->doc_func(QUERY, "Oscillator frequency again[Hz]. ");

    QUERY->add_mfun(QUERY, ssr_getFreq, "float", "freq");
    QUERY->doc_func(QUERY, "Oscillator frequency [Hz]. ");

    QUERY->add_mfun(QUERY, ssr_checkConnection, "float", "checkConnection");
    QUERY->doc_func(QUERY, "Oscillator frequency again[Hz]. ");

    QUERY->add_mfun(QUERY, ssr_connect, "void", "connect");

    ssr_data_offset = QUERY->add_mvar(QUERY, "int", "@ssr_data", false);
    
    QUERY->end_class(QUERY);

    return TRUE;
}


CK_DLL_CTOR(ssr_ctor)
{
    OBJ_MEMBER_INT(SELF, ssr_data_offset) = 0;
    
    ssr * bcdata = new ssr(API->vm->get_srate(API, SHRED));
    
    OBJ_MEMBER_INT(SELF, ssr_data_offset) = (t_CKINT) bcdata;
}

CK_DLL_DTOR(ssr_dtor)
{
    ssr * bcdata = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    if(bcdata)
    {
        delete bcdata;
        OBJ_MEMBER_INT(SELF, ssr_data_offset) = 0;
        bcdata = NULL;
    }
}

CK_DLL_TICK(ssr_tick)
{
    ssr * c = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    
    if(c) *out = c->tick(in);

    return TRUE;
}

CK_DLL_MFUN(ssr_setFreq)
{
    ssr * bcdata = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    // TODO: sanity check
    RETURN->v_float = bcdata->setFreq(GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(ssr_addSrc)
{
    ssr * bcdata = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    RETURN->v_float = bcdata->addSrc(GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(ssr_mvSrc)
{
    ssr * bcdata = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    RETURN->v_float = bcdata->mvSrc(GET_NEXT_FLOAT(ARGS));
}

//#define GET_NEXT_FLOAT(ptr)    (*((t_CKFLOAT *&)ptr)++)
CK_DLL_MFUN(ssr_mvSrcV)
{
    ssr * bcdata = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    RETURN->v_float = bcdata->mvSrcV(GET_NEXT_VEC4(ARGS));
}

CK_DLL_MFUN(ssr_getFreq)
{
    ssr * bcdata = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    RETURN->v_float = bcdata->getFreq();
}

CK_DLL_MFUN(ssr_checkConnection)
{    ssr * bcdata = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    RETURN->v_float = bcdata->checkConnection();
}


CK_DLL_MFUN(ssr_connect)
{
    ssr * bcdata = (ssr *) OBJ_MEMBER_INT(SELF, ssr_data_offset);
    RETURN->v_float = bcdata->connect();
}
