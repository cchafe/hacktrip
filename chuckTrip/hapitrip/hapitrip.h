#ifndef HAPITRIP_H
#define HAPITRIP_H

#include "hapitrip_global.h"

////#define AUDIO_ONLY

#define FAKE_STREAMS
#define FAKE_STREAMS_TIMER
#define NO_AUDIO
#ifndef NO_AUDIO
#include <RtAudio.h>
#endif

#ifndef AUDIO_ONLY
#include <QElapsedTimer>
#include <QHostInfo>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

const QString gVersion = "clientV2";
//const QString gServer = "3.101.24.143"; // cc EC2 new IP 15-Aug-2022
//const QString gServer = "54.153.79.243"; // ccTest
//const QString gServer = "54.176.100.97"; // SF
//const QString gServer = "35.178.52.65"; // London

//const QString gServer = "jackloop256.stanford.edu";
// const QString gServer = "cmn55.stanford.edu";
// const QString gServer = "cmn9.stanford.edu";
// const QString gServer = "171.64.197.158";
// const QString gServer = "127.0.0.2"; // don't use "loopback", doesn't resolve
// const QString gServer = "localhost";
#endif

typedef signed short MY_TYPE; // audio interface data is 16bit ints
#define FORMAT RTAUDIO_SINT16 // which has this rtaudio name

class TestAudio {
public:
  TestAudio(int channels);
  void printSamples(MY_TYPE *buffer);
  void sineTest(MY_TYPE *buffer);

private:
  std::vector<double> mPhasor;
};

#ifndef AUDIO_ONLY
struct HeaderStruct {
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

class UDP : public QUdpSocket  {
Q_OBJECT
public:
    ~UDP();
  void start();
  void setPeer(QString peer) { mServer = peer; }
  void setPeerUdpPort(int port) { mPeerUdpPort = port; }
  void setTest(int channels) { mTest = new TestAudio(channels); }
  void stop();
  void send(int8_t *audioBuf);
#ifndef NO_AUDIO
  int audioCallback(void *outputBuffer, void *inputBuffer,
                    unsigned int nBufferFrames, double streamTime,
                    RtAudioStreamStatus, void *bytesInfoFromStreamOpen);
#endif
private:
  //    QMutex mMutex;                     ///< Mutex to protect read and write
  //    operations
  int mWptr;
  int mRptr;
  int mRing;
  std::vector<int8_t *> mRingBuffer;
  QHostAddress serverHostAddress;
  HeaderStruct mHeader;
  QHostAddress mPeerAddr;
  int mPeerUdpPort;
  QByteArray mBufSend;
  QByteArray mBufRcv;
  int mSendSeq;
  QElapsedTimer mRcvTmer;
  QElapsedTimer mRcvTimeout;
  TestAudio *mTest;
public slots:
  void readPendingDatagrams();
  void rcvTimeout(bool restart);
#ifdef FAKE_STREAMS
  void sendDummyData(float *buf);
  void rcvDummyData(float *buf);
private:
  QTimer mSendTmer;
  int8_t *mTmpAudioBuf;
  QString mServer;
#endif
};

class TCP : public QTcpSocket {
public:
  int connectToServer();
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

    static const int dSampleRate = 48000;
    static const int dFPP = 128;
    static const int dChannels = 1;
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
    int sampleRate = dSampleRate;
    int FPP = dFPP;
    int channels = dChannels;
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

    QString server = NULL;
    friend class TCP;
    friend class UDP;
#endif
    friend class Audio;
    friend class TestAudio;
    friend class Hapitrip;
};


class HAPITRIP_EXPORT Hapitrip : public QObject {
    Q_OBJECT
public:
  void connectToServer(QString server);
  void setLocalUDPaudioPort(int port) { as.localAudioUdpPort = port; };
  void run();
  void stop();
  int getFPP() { return as.FPP; }
  void xfrBufs(float *sendBuf, float *rcvBuf);

private:
  static APIsettings as;
  friend class Audio;
  friend class TestAudio;
#ifndef AUDIO_ONLY
  friend class TCP;
  friend class UDP;
  TCP * mTcp;
  UDP * mUdp;
#endif
#ifndef NO_AUDIO
  Audio mAudio;
#endif
};

#endif // HAPITRIP_H
