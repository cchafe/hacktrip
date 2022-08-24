#ifndef HAPITRIP_H
#define HAPITRIP_H

#include "hapitrip_global.h"

//#define AUDIO_ONLY
#ifdef AUDIO_ONLY
#include "qobject.h"
#include "qobjectdefs.h"
#endif

#include "../regulator/regulator.h"

#ifndef NO_AUDIO
//#include <RtAudio.h> // if built from a hapitrip.pro and it's likelt inclusion of rtaudio.pri
#include <../../rtaudio/RtAudio.h> // if not
#endif

#ifndef AUDIO_ONLY
#include <QElapsedTimer>
#include <QHostInfo>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>
#endif

const QString gVersion = "0.3-rc.1";

typedef signed short MY_TYPE; // audio interface data is 16bit ints
#define FORMAT RTAUDIO_SINT16 // which has this rtaudio name

class TestAudio { // for insertion in test points
public:
    TestAudio(int channels);
    void printSamples(MY_TYPE *buffer); // see the signal
    void sineTest(MY_TYPE *buffer); // generate a signal
private:
    std::vector<double> mPhasor; // multi-channel capable
};

#ifndef AUDIO_ONLY
struct HeaderStruct { // standard JackTrip network packet header
public:
    // watch out for alignment...
    uint64_t TimeStamp;    ///< Time Stamp
    uint16_t SeqNumber;    ///< Sequence Number
    uint16_t BufferSize;   ///< Buffer Size in Samples
    uint8_t SamplingRate;  ///< Sampling Rate in JackAudioInterface::samplingRateT
    uint8_t BitResolution; ///< Audio Bit Resolution
    uint8_t NumIncomingChannelsFromNet; ///< Number of incoming Channels from the
    ///< network
    uint8_t
    NumOutgoingChannelsToNet; ///< Number of outgoing Channels to the network
};

class UDP : public QUdpSocket  { // UDP socket for send and receive
    Q_OBJECT
public:
    UDP(QString server);
    ~UDP();
    void start(); // initialize HeaderStruct, bind socket, ready to receive
    void setPeerUdpPort(int port) { mPeerUdpPort = port; }
    int getPeerUdpPort() { return mPeerUdpPort; }
    void setTest(int channels) { mTest = new TestAudio(channels); } // for test points
    void send(int8_t *audioBuf);  // send one audio packet to peer
    void stop(); // send standard JackTrip stop packet to peer
    // for non-audio callback triggering of audio rcv and send e.g., chuck
    void rcvAudioData(float *buf); // readPendingDatagrams into ring and pull next packet from ring
    void sendAudioData(float *buf); // convert one bufferfull to short int and send out
    void ringBufferPush(int8_t *buf, int seq); // push received packet to ring
    void ringBufferPull(); // pull next packet to play out from ring put in mTmpAudioBuf
#ifndef NO_AUDIO
    int audioCallback(void *outputBuffer, void *inputBuffer,
                      unsigned int nBufferFrames, double streamTime,
                      RtAudioStreamStatus, void *bytesInfoFromStreamOpen);
#endif
private:
    void rcvElapsedTime(bool restart); // tracks elapsed time since last incoming packet
    int mWptr; // ring buffer write pointer
    int mRptr; // ring buffer read pointer
    int mRing; // ring buffer length in number of packets
    std::vector<int8_t *> mRingBuffer; // ring buffer
    QHostAddress serverHostAddress; // peer
    int mPeerUdpPort = 0; // ephemeral peer audio port given by peer
    HeaderStruct mHeader; // packet header for the outgoing packet
    QByteArray mRcvPacket; // the incoming packet
    QByteArray mSendPacket; // the outgoing packet
    int mSendSeq; // sequence number written in the header for the outgoing packet
    QElapsedTimer mRcvTimer; // for rcvElapsedTime
    TestAudio *mTest; // in case test points are needed
public slots:
    void readPendingDatagrams(); // when readyRead is signaled
private:
    int8_t *mTmpAudioBuf; // one bufferfull of audio, used for rcv and send operations
    QString mServer; // peer address
    Regulator *reg;
};

class TCP : public QTcpSocket {
public:
    int connectToServer(); // TCP handshake with server, returns ephemeral port number
};
#endif

#ifndef NO_AUDIO
class Audio {
public:
    void start();
    void stop();
    int audioCallback(void *outputBuffer, void *inputBuffer,
                      unsigned int nBufferFrames, double streamTime,
                      RtAudioStreamStatus, void *bytesInfoFromStreamOpen);
    static int wrapperProcessCallback(void *outputBuffer, void *inputBuffer,
                                      unsigned int nBufferFrames,
                                      double streamTime,
                                      RtAudioStreamStatus status, void *arg);
#ifndef AUDIO_ONLY
    void setUdp(UDP *udp) { mUdp = udp; }
#endif
    void setTest(int channels) { mTest = new TestAudio(channels); }

private:
    // these are identical to the rtaudio/tests/Duplex.cpp example
    // except with m_ prepended
    RtAudio *m_adac;
    double m_streamTimePrintIncrement;
    double m_streamTimePrintTime;
    unsigned int m_channels;
    unsigned int m_fs;
    unsigned int m_oDevice;
    unsigned int m_iDevice;
    unsigned int m_iOffset;
    unsigned int m_oOffset;
    RtAudio::StreamParameters m_iParams;
    RtAudio::StreamParameters m_oParams;
    RtAudio::StreamOptions options;
    TestAudio *mTest;
#ifndef AUDIO_ONLY
    UDP *mUdp;
#endif
};
#endif

class APIsettings {
    // default values
    static const int dRtAudioAPI = 3;
    static const int dSampleRate = 48000;
    static const int dFPP = 128;
    static const int dChannels = 2;
    static const int dBytesPerSample = sizeof(MY_TYPE);
    static const int dAudioDataLen = dFPP * dChannels * dBytesPerSample;
    constexpr static const double dScale = 32767.0;
    constexpr static const double dInvScale = 1.0 / 32767.0;
    static const int dNumberOfBuffersSuggestionToRtAudio = 2;
#ifndef AUDIO_ONLY
    static const int dServerTcpPort = 4464;
    static const int dLocalAudioUdpPort = 4464;
    static const int dSocketWaitMs = 1500;
    static const int dBufferQueueLength =
            3; // queue not used for localhost testing
    static const int dExitPacketSize = 63;
    static const int dTimeoutMS = 1000;
    constexpr static const double dPacketPeriodMS =
            (1000.0 / (double)(dSampleRate / dFPP));
    static const int dRingBufferLength = 50;
    static const int dReportAfterPackets = 500;
    static const bool dVerbose = 0;
#endif

private:
    // variable values initialized to defaults
    int rtAudioAPI = dRtAudioAPI;
    int FPP = dFPP;
    int channels = dChannels;
    int sampleRate = dSampleRate;
    int bytesPerSample = dBytesPerSample;
    int audioDataLen = dAudioDataLen;
    double scale = dScale;
    double invScale = dInvScale;
    int numberOfBuffersSuggestionToRtAudio = dNumberOfBuffersSuggestionToRtAudio;
#ifndef AUDIO_ONLY
    int serverTcpPort = dServerTcpPort;
    int localAudioUdpPort = dLocalAudioUdpPort;
    int socketWaitMs = dSocketWaitMs;
    int bufferQueueLength = dBufferQueueLength;
    int exitPacketSize = dExitPacketSize;
    int timeoutMS = dTimeoutMS;
    double packetPeriodMS = dPacketPeriodMS;
    int ringBufferLength = dRingBufferLength;
    int reportAfterPackets = dReportAfterPackets;
    bool verbose = dVerbose;

    QString server = NULL; // the server name or IP address
    friend class TCP;
    friend class UDP;
    friend class Regulator;
    friend class StdDev;
#endif
    friend class Audio;
    friend class TestAudio;
    friend class Hapitrip;
};


class HAPITRIP_EXPORT Hapitrip : public QObject {
    Q_OBJECT
public:
    int connectToServer(QString server); // initiate handshake and start listening for UDP incoming
    void run(); // initiate bidirectional flows, sending UDP outgoing to server starts it sending
    void stop(); // stop the works
    void xfrBufs(float *sendBuf, float *rcvBuf); // when not using an audio callback e.g., for chuck

    // getters of current API parameter values and setters to override their initial default settings
    void setRtAudioAPI(int api) { as.rtAudioAPI = api; }

    int getFPP() { return as.FPP; }
    void setFPP(int fpp) {
        as.FPP = fpp;
        as.audioDataLen = as.FPP * as.channels * as.bytesPerSample;
    }

    int getChannels() { return as.channels; }
    void setChannels(int nChans) {
        as.channels = nChans;
        as.audioDataLen = as.FPP * as.channels * as.bytesPerSample;
    }

    int getSampleRate() { return as.sampleRate; }
    void setSampleRate(int srate) { as.sampleRate = srate; }

#ifndef AUDIO_ONLY
    int getLocalUDPaudioPort() { return as.localAudioUdpPort; };
    void setLocalUDPaudioPort(int port) { as.localAudioUdpPort = port; };
#endif

private:
    static APIsettings as; // all API parameters
    friend class Audio;
    friend class TestAudio;
#ifndef AUDIO_ONLY
    friend class TCP;
    friend class UDP;
    friend class Regulator;
    friend class StdDev;
    TCP * mTcp;
    UDP * mUdp;
#endif
#ifndef NO_AUDIO
    Audio mAudio;
#endif
};

#endif // HAPITRIP_H
