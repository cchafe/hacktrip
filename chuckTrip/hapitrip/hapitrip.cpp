#include "hapitrip.h"

#include <QtEndian>
#include <asm-generic/socket.h>
#include <math.h>
#include <iostream>
#include <sys/socket.h>

void Hapitrip::connect() {
#ifndef AUDIO_ONLY
    mTcp = new TCP();
    mUdp = new UDP();
    mUdp->setPeerUdpPort(mTcp->connectToServer());
    delete mTcp;
    mUdp->setTest(Hapitrip::mChannels);
#ifndef NO_AUDIO
    mAudio.setUdp(mUdp);
#endif
#endif
#ifndef NO_AUDIO
    mAudio.setTest(Hapitrip::mChannels);
#endif
}

void Hapitrip::run() {
#ifndef AUDIO_ONLY
    mUdp->start();
#endif
#ifndef NO_AUDIO
    mAudio.start();
#endif
}

void Hapitrip::xfrBufs(float *sendBuf, float *rcvBuf) {
#ifdef FAKE_STREAMS
    if (mUdp != nullptr) {
        mUdp->sendDummyData(sendBuf);
        mUdp->rcvDummyData(rcvBuf);
    }
#endif
}

void Hapitrip::stop() {
#ifndef AUDIO_ONLY
    mUdp->stop();
#endif
#ifndef NO_AUDIO
    mAudio.stop();
#endif
    delete mUdp;
    mUdp = nullptr;
}

#ifndef AUDIO_ONLY
int TCP::connectToServer() {
    QHostAddress serverHostAddress;

    if (!serverHostAddress.setAddress(gServer)) {
        std::cout << "\nthings are ok (despite qt bug related to QHostInfo::fromName)..." << std::endl;
        QHostInfo info = QHostInfo::fromName(gServer);
        std::cout << "...ignore all that\n" << std::endl;

        // the line above works but has a bug in the chugin port where it prints
        //        QObject::connect(QObject, QThreadPool): invalid nullptr parameter
        //        QObject::connect(QObject, Unknown): invalid nullptr parameter
        // no obvious workaround

        if (!info.addresses().isEmpty()) {
            // use the first IP address
            serverHostAddress = info.addresses().constFirst();
        }
    }

//    std::cout << "TCP: serverHostAddress = "
//              << serverHostAddress.toString().toStdString()
//              << std::endl;
    connectToHost(serverHostAddress, Hapitrip::mServerTcpPort);
    waitForConnected(Hapitrip::mSocketWaitMs);
    int peerUdpPort = 0;
    char *port_buf = new char[sizeof(uint32_t)];
    if (state() == QTcpSocket::ConnectedState) {
        QByteArray ba;
        qint32 tmp = Hapitrip::mLocalAudioUdpPort;
        ba.setNum(tmp);
        write(ba);
        waitForBytesWritten(1500);
        waitForReadyRead();
        read(port_buf, sizeof(uint32_t));
        peerUdpPort = qFromLittleEndian<qint32>(port_buf);
        std::cout << "TCP: ephemeral port = " << peerUdpPort << std::endl;
    } else
        std::cout << "TCP: not connected to server" << std::endl;
    delete[] port_buf;
    return peerUdpPort;
}

// show udp port in use
// sudo lsof -i:4464
// sudo lsof -i -P -n
// sudo watch ss -tulpn
void UDP::start() {
    mHeader.TimeStamp = (uint64_t)0;
    mHeader.SeqNumber = (uint16_t)0;
    mHeader.BufferSize = (uint16_t)Hapitrip::mFPP;
    mHeader.SamplingRate = (uint8_t)3;
    mHeader.BitResolution = (uint8_t)sizeof(MY_TYPE) * 8; // checked in jacktrip
    mHeader.NumIncomingChannelsFromNet = Hapitrip::mChannels;
    mHeader.NumOutgoingChannelsToNet = Hapitrip::mChannels;
    int packetDataLen = sizeof(HeaderStruct) + Hapitrip::mAudioDataLen;
    mBufSend.resize(packetDataLen);
    mBufSend.fill(0, packetDataLen);
    memcpy(mBufSend.data(), &mHeader, sizeof(HeaderStruct));
    mBufRcv.resize(packetDataLen);
    mBufRcv.fill(0, packetDataLen);
    memcpy(mBufRcv.data(), &mHeader, sizeof(HeaderStruct));

    if (!serverHostAddress.setAddress(gServer)) {
        QHostInfo info = QHostInfo::fromName(gServer);
        if (!info.addresses().isEmpty()) {
            // use the first IP address
            serverHostAddress = info.addresses().constFirst();
        }
    }

    /*
     * https://forum.qt.io/topic/90687/how-to-set-so_reuseport-option-for-qudpsocket/3
     * So I did it using Berkeley sockets. I'm using Linux, the same code might work with WinSock, if not something trivially similar will. The following code creates a QUdpSocket, provides it with a low-level socket descriptor that has been set for SO_REUSEPORT, then binds it to port 34567:
QUdpSocket *sock = new QUdpSocket;
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
int optval = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
              (void *) &optval, sizeof(optval));
sock->setSocketDescriptor(sockfd, QUdpSocket::UnconnectedState);
sock->bind(34567, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
     */
    int ret = 0;
    ret = bind(Hapitrip::mLocalAudioUdpPort);
//    ret = bind();
    std::cout << "UDP: start send = " << ret << " "
              << serverHostAddress.toString().toLocal8Bit().data() << std::endl;
    connect(this, &QUdpSocket::readyRead, this, &UDP::readPendingDatagrams);
    std::cout << this->state()  << " readPendingDatagrams = "<< QUdpSocket::BoundState << std::endl;
    connect(&mRcvTimeout, &QTimer::timeout, this, &UDP::rcvTimeout);
    mRing = Hapitrip::mRingBufferLength;
    mWptr = mRing / 2;
    mRptr = 0;
    for (int i = 0; i < mRing; i++) {
        int8_t *tmp = new int8_t[Hapitrip::mAudioDataLen];
        for (int j = 0; j < Hapitrip::mAudioDataLen; j++)
            tmp[j] = 0;
        mRingBuffer.push_back(tmp);
    }
    mSendSeq = 0;
    mRcvTmer.start();
#ifdef FAKE_STREAMS
    mTmpAudioBuf = new int8_t[Hapitrip::mAudioDataLen];
    memset(mTmpAudioBuf, 0, Hapitrip::mAudioDataLen);
#endif
#ifdef FAKE_STREAMS_TIMER
    connect(&mSendTmer, &QTimer::timeout, this, &UDP::sendDummyData);
    mSendTmer.start(Hapitrip::mPacketPeriodMS);
#endif
};

UDP::~UDP() {
    for (int i = 0; i < mRing; i++) delete mRingBuffer[i];
#ifdef FAKE_STREAMS
    delete mTmpAudioBuf;
#endif
}

void UDP::rcvTimeout() {
    std::cout << "rcv: ms since last packet = "
              << (double)mRcvTmer.nsecsElapsed() / 1000000.0 << std::endl;
    mRcvTimeout.start();
}

#ifdef FAKE_STREAMS
void UDP::sendDummyData(float *buf) {
    MY_TYPE * tmp = (MY_TYPE *)mTmpAudioBuf;
    for (int ch = 0; ch < 1; ch++) {
        for (int i = 0; i < Hapitrip::mFPP; i++) {
            *tmp++ = (MY_TYPE)(buf[i] * Hapitrip::mScale);
        }
    }
    send(mTmpAudioBuf);
}

void UDP::rcvDummyData(float *buf) {
    readPendingDatagrams();
    if (mRptr == mWptr)
        mRptr = mRing / 2;
    mRptr %= mRing;
    memcpy(mTmpAudioBuf, mRingBuffer[mRptr], Hapitrip::mAudioDataLen);
    mRptr++;

    MY_TYPE * tmp1 = (MY_TYPE *)mTmpAudioBuf;
    float * tmp2 = (float *)buf;

    for (int ch = 0; ch < 1; ch++) {
        for (int i = 0; i < Hapitrip::mFPP; i++) {
            tmp2[i] = tmp1[i] * Hapitrip::mInvScale;
        }
    }
}

#endif

void UDP::send(int8_t *audioBuf) {
    mHeader.SeqNumber = (uint16_t)mSendSeq;
    memcpy(mBufSend.data(), &mHeader, sizeof(HeaderStruct));
    memcpy(mBufSend.data() + sizeof(HeaderStruct), audioBuf, Hapitrip::mAudioDataLen);
    writeDatagram(mBufSend, serverHostAddress, mPeerUdpPort);
    if (mSendSeq % Hapitrip::mReportAfterPackets == 0)
        std::cout << "UDP send: packet = " << mSendSeq << std::endl;
    mSendSeq++;
    mSendSeq %= 65536;
//    std::cout << "\nsineTest " << mSendSeq << std::endl;
}

void UDP::stop() {
#ifdef FAKE_STREAMS_TIMER
    disconnect(&mSendTmer, &QTimer::timeout, this, &UDP::sendDummyData);
    mSendTmer.stop();
#endif
    disconnect(&mRcvTimeout, &QTimer::timeout, this, &UDP::rcvTimeout);
    mRcvTimeout.stop();
    disconnect(this, &QUdpSocket::readyRead, this, &UDP::readPendingDatagrams);
    // Send exit packet (with 1 redundant packet).
    std::cout << "sending exit packet" << std::endl;
    QByteArray stopBuf;
    stopBuf.resize(Hapitrip::mExitPacketSize);
    stopBuf.fill(0xff, Hapitrip::mExitPacketSize);
    writeDatagram(stopBuf, serverHostAddress, mPeerUdpPort);
    writeDatagram(stopBuf, serverHostAddress, mPeerUdpPort);
    waitForBytesWritten();
    close(); // stop rcv
}

void UDP::readPendingDatagrams() {
    // read datagrams in a loop to make sure that all received datagrams are
    // processed since readyRead() is emitted for a datagram only when all
    // previous datagrams are read
    //     QMutexLocker locker(&mMutex);

//    mRcvTmer.start();
//    mRcvTimeout.start(Hapitrip::mTimeoutMS);
    while (hasPendingDatagrams()) {
        int size = pendingDatagramSize();
        if (size == Hapitrip::mExitPacketSize)
            stop();
        QHostAddress sender;
        quint16 senderPort;
        readDatagram(mBufRcv.data(), mBufRcv.size(), &sender, &senderPort);
        //        std::cout << sender.toIPv4Address() << " " << senderPort <<
        //        std::endl;
        memcpy(&mHeader, mBufRcv.data(), sizeof(HeaderStruct));
        int rcvSeq = mHeader.SeqNumber;
        if (rcvSeq % Hapitrip::mReportAfterPackets == 0)
            std::cout << "UDP rcv: seq = " << rcvSeq << std::endl;
        int8_t *audioBuf = (int8_t *)(mBufRcv.data() + sizeof(HeaderStruct));
        //            mTest->sineTest((MY_TYPE *)audioBuf); // output sines
        //            mTest->printSamples((MY_TYPE *)audioBuf); // print audio
        //            signal
        memcpy(mRingBuffer[mWptr], audioBuf, Hapitrip::mAudioDataLen);
        mWptr++;
        mWptr %= mRing;
    }
}

#ifndef NO_AUDIO
int UDP::audioCallback(void *outputBuffer, void *inputBuffer,
                       unsigned int /* nBufferFrames */,
                       double /* streamTime */,
                       RtAudioStreamStatus /* status */,
                       void * /* data */) // last arg is used for "this"
{
    send((int8_t *)inputBuffer);
    //        QMutexLocker locker(&mMutex);
    if (mRptr == mWptr)
        mRptr = mRing / 2;
    //    if (mRptr < 0) mRptr = 0;
    mRptr %= mRing;
    memcpy(outputBuffer, mRingBuffer[mRptr], Hapitrip::mAudioDataLen);
    mRptr++;

    // audio diagnostics, modify or print output and input buffers
    //    memcpy(outputBuffer, inputBuffer, Hapitrip::mAudioDataLen); // test
    //    straight wire mTest->sineTest((MY_TYPE *)outputBuffer); // output sines
    //    mTest->printSamples((MY_TYPE *)outputBuffer); // print audio signal

    return 0;
}

int Audio::wrapperProcessCallback(void *outputBuffer, void *inputBuffer,
                                  unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status, void *arg) {
    return static_cast<UDP *>(arg)->audioCallback(
                outputBuffer, inputBuffer, nBufferFrames, streamTime, status, arg);
}
#endif
#else

int Audio::audioCallback(void *outputBuffer, void *inputBuffer,
                         unsigned int /* nBufferFrames */,
                         double /* streamTime */,
                         RtAudioStreamStatus /* status */,
                         void * /* data */) // last arg is used for "this"
{
    // audio diagnostics, modify or print output and input buffers
    memcpy(outputBuffer, inputBuffer,
           Hapitrip::mAudioDataLen); // test straight wire
    //        mTest->sineTest((MY_TYPE *)outputBuffer); // output sines
    //        mTest->printSamples((MY_TYPE *)outputBuffer); // print audio signal

    return 0;
}

int Audio::wrapperProcessCallback(void *outputBuffer, void *inputBuffer,
                                  unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status, void *arg) {
    return static_cast<Audio *>(arg)->audioCallback(
                outputBuffer, inputBuffer, nBufferFrames, streamTime, status, arg);
}
#endif

#ifndef NO_AUDIO
void Audio::start() {
    m_streamTimePrintIncrement = 1.0; // seconds
    m_streamTimePrintTime = 1.0;      // seconds
    //    m_adac = new RtAudio(RtAudio::UNIX_JACK);
    //    m_adac = new RtAudio();
    m_adac = new RtAudio(RtAudio::LINUX_PULSE);

    std::vector<unsigned int> deviceIds = m_adac->getDeviceIds();
    if (deviceIds.size() < 1) {
        std::cout << "\nNo audio devices found!\n";
        exit(1);
    }
    if (m_adac->getDeviceCount() < 1) {
        std::cout << "\nNo audio devices found!\n";
        exit(1);
    } else {
        std::cout << "\naudio devices found =\n"
                  << m_adac->getDeviceCount() << "\n";
    }
    m_channels = Hapitrip::mChannels;
    m_fs = Hapitrip::mSampleRate;
    m_iDevice = m_oDevice = 0;
    m_iOffset = m_oOffset = 0; // first channel
    m_adac->showWarnings(true);
    // copy all setup into all stream info
    m_iParams.nChannels = m_channels;
    m_iParams.firstChannel = m_iOffset;
    m_oParams.nChannels = m_channels;
    m_oParams.firstChannel = m_oOffset;
    m_iParams.deviceId = m_adac->getDefaultInputDevice();
    m_oParams.deviceId = m_adac->getDefaultOutputDevice();
    options.flags = RTAUDIO_NONINTERLEAVED | RTAUDIO_SCHEDULE_REALTIME;
    options.numberOfBuffers =
            Hapitrip::mNumberOfBuffersSuggestionToRtAudio; // Windows DirectSound,
    // Linux OSS, and Linux
    // Alsa APIs only.
    // value set by the user is replaced during execution of the
    // RtAudio::openStream() function by the value actually used by the system

    std::cout << "using default audio interface device\n";
    std::cout << m_adac->getDeviceInfo(m_iParams.deviceId).name
              << "\tfor input and output\n";
    std::cout << "\tIf another is needed, either change your settings\n";
    std::cout << "\tor the choice in the code\n";
    unsigned int bufferFrames = Hapitrip::mFPP;
#ifndef AUDIO_ONLY
    if (m_adac->openStream(&m_oParams, &m_iParams, FORMAT, Hapitrip::mSampleRate,
                           &bufferFrames, &Audio::wrapperProcessCallback,
                           (void *)mUdp, &options))
        std::cout << "\nCouldn't open audio device streams!\n";
#else
    if (m_adac->openStream(&m_oParams, &m_iParams, FORMAT, Hapitrip::mSampleRate,
                           &bufferFrames, &Audio::wrapperProcessCallback,
                           (void *)this, &options))
        std::cout << "\nCouldn't open audio device streams!\n";

#endif
    if (m_adac->isStreamOpen() == false) {
        std::cout << "\nCouldn't open audio device streams!\n";
        exit(1);
    } else {
        std::cout << "\trunning "
                  << m_adac->getApiDisplayName(m_adac->getCurrentApi()) << "\n";
        std::cout << "\nStream latency = " << m_adac->getStreamLatency()
                  << " frames" << std::endl;
    }
    std::cout << "\nAudio stream start" << std::endl;
    if (m_adac->startStream())
        std::cout << "\nCouldn't start streams!\n";
    std::cout << "\nAudio stream started" << std::endl;
}

void Audio::stop() {
    if (m_adac)
        if (m_adac->isStreamRunning()) {
            std::cout << "\nAudio stream stop" << std::endl;
            m_adac->stopStream();
            if (m_adac->isStreamOpen()) {
                m_adac->closeStream();
                std::cout << "Audio stream closed" << std::endl;
            }
        }
}
#endif

TestAudio::TestAudio(int channels) { mPhasor.resize(channels, 0.0); }

void TestAudio::sineTest(MY_TYPE *buffer) {
    for (int ch = 0; ch < Hapitrip::mChannels; ch++) {
        for (int i = 0; i < Hapitrip::mFPP; i++) {
            double tmp = sin(mPhasor[ch]);
            *buffer++ = (MY_TYPE)(tmp * Hapitrip::mScale);
            mPhasor[ch] += ((ch) ? 0.20 : 0.22);
        }
    }
}

void TestAudio::printSamples(MY_TYPE *buffer) {
    for (int ch = 0; ch < Hapitrip::mChannels; ch++) {
        for (int i = 0; i < Hapitrip::mFPP; i++) {
            double tmp = ((MY_TYPE)*buffer++) * Hapitrip::mInvScale;
            std::cout << "\t" << tmp << std::endl;
        }
    }
}
