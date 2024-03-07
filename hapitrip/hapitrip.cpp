#include "hapitrip.h"

#include <QtEndian>
#include <math.h>
#include <iostream>
#include <ios>
#include <iomanip>

#include <QThread>

APIsettings Hapitrip::as; // declare static APIsettings instance

int Hapitrip::connectToServer([[maybe_unused]] QString server) {
    cout << "GUI params disabled, using APIsettings Hapitrip::as\n";

#ifndef AUDIO_ONLY
    as.server = server; // set the server
    mTcp = new TCP(); // temporary for handshake with server
    mUdp = new UDP(as.server); // bidirectional socket for trading audio with server
    mUdp->setPeerUdpPort(mTcp->connectToServer()); // to get the server port to send to
    delete mTcp; // done with TCP
    mUdp->setTest(as.channels); // in case of audio test points
#ifndef NO_AUDIO
    mAudio->setUdp(mUdp);
#endif
#endif
#ifndef NO_AUDIO
    mAudio->setTest(as.channels);
    mAudio->setTestPLC(as.channels,
                      as.FPP, 16, 2);
#endif
#ifndef AUDIO_ONLY
    return mUdp->getPeerUdpPort();
#else
    return 1;
#endif
}

void Hapitrip::run() { // hit this before server times out in like 10 seconds
#ifndef AUDIO_ONLY
    mUdp->start(); // bidirectional flows
#endif
#ifndef NO_AUDIO
    if (mAudio->start()) {
#ifndef AUDIO_ONLY
        mUdp->stop();
#endif
    }
#endif
}

void Hapitrip::stop() { // the whole show
#ifndef AUDIO_ONLY
    mUdp->stop();
#endif
#ifndef NO_AUDIO
    mAudio->stop();
#endif
#ifndef AUDIO_ONLY
    delete mUdp;
    mUdp = nullptr;
#endif
}

void Hapitrip::xfrBufs(float *sendBuf, float *rcvBuf) { // trigger audio rcv and send
    if (mUdp != nullptr) {
        mUdp->sendAudioData(sendBuf);
        mUdp->rcvAudioData(rcvBuf);
    }
}

int TCP::connectToServer() { // this is the TCP handshake
    QHostAddress serverHostAddress;
    if (!serverHostAddress.setAddress(Hapitrip::as.server)) { // DNS resolver
        std::cout << "\nif this is a chugin, then there's no running Qt event loop but things are ok..." << std::endl;
        QHostInfo info = QHostInfo::fromName(Hapitrip::as.server);
        std::cout << "...ignore all that\n" << std::endl;

        // the line above works but QHostInfo::fromName needs an event loop and prints
        //        QObject::connect(QObject, QThreadPool): invalid nullptr parameter
        //        QObject::connect(QObject, Unknown): invalid nullptr parameter

        if (!info.addresses().isEmpty()) {
            // use the first IP address
            serverHostAddress = info.addresses().constFirst();
        }
    }

    connectToHost(serverHostAddress, Hapitrip::as.serverTcpPort); // try to connect
    waitForConnected(Hapitrip::as.socketWaitMs); // block until connected or timeout
    int peerUdpPort = 0;
    char *port_buf = new char[sizeof(uint32_t)];
    if (state() == QTcpSocket::ConnectedState) { // connected ok
        QByteArray ba;
        qint32 tmp = Hapitrip::as.localAudioUdpPort;
        ba.setNum(tmp);
        write(ba); // send server our localAudioUdpPort
        waitForBytesWritten(1500);
        waitForReadyRead(); // wait for reply with server's audio UDP port
        read(port_buf, sizeof(uint32_t));
        peerUdpPort = qFromLittleEndian<qint32>(port_buf);
        if (Hapitrip::as.verbose) // example of using as.verbose to mask messages
            std::cout << "TCP: ephemeral port = " << peerUdpPort << std::endl;
    } else
        std::cout << "TCP: not connected to server" << std::endl;
    delete[] port_buf;
    return peerUdpPort;
}

UDP::UDP(QString server) : mServer(server) {};

void UDP::start() {
    // init the JackTrip header struct
    mHeader.TimeStamp = (uint64_t)0;
    mHeader.SeqNumber = (uint16_t)0;
    mHeader.BufferSize = (uint16_t)Hapitrip::as.FPP;
    mHeader.SamplingRate = (uint8_t)3; // hardwired 48000 for now
    mHeader.BitResolution = (uint8_t)sizeof(MY_TYPE) * 8; // checked this in jacktrip
    mHeader.NumIncomingChannelsFromNet = Hapitrip::as.channels;
    mHeader.NumOutgoingChannelsToNet = Hapitrip::as.channels;

    // packets for rcv and send
    int packetDataLen = sizeof(HeaderStruct) + Hapitrip::as.audioDataLen;
    mRcvPacket.resize(packetDataLen);
    mRcvPacket.fill(0, packetDataLen);
    memcpy(mRcvPacket.data(), &mHeader, sizeof(HeaderStruct));
    mSendPacket.resize(packetDataLen);
    mSendPacket.fill(0, packetDataLen);
    memcpy(mSendPacket.data(), &mHeader, sizeof(HeaderStruct));

    // setup ring buffer
    mRing = Hapitrip::as.ringBufferLength;
    mWptr = mRing / 2;
    mRptr = mWptr - 2;
    for (int i = 0; i < mRing; i++) {
        int8_t *tmp = new int8_t[Hapitrip::as.audioDataLen];
        for (int j = 0; j < Hapitrip::as.audioDataLen; j++)
            tmp[j] = 0;
        mRingBuffer.push_back(tmp);
    }

    if (!serverHostAddress.setAddress(mServer)) { // looks a lot like TCP's DNS
        QHostInfo info = QHostInfo::fromName(mServer);
        if (!info.addresses().isEmpty()) {
            // use the first IP address
            serverHostAddress = info.addresses().constFirst();
        }
    }
    int ret = 0;
    ret = bind(Hapitrip::as.localAudioUdpPort); // start listening
    connect(this, &QUdpSocket::readyRead, this, &UDP::readPendingDatagrams); // dispatch arriving packet
    if (Hapitrip::as.verbose)
        std::cout << "UDP: start listening = " << ret << " "
                  << serverHostAddress.toString().toLocal8Bit().data() << std::endl;
    mSendSeq = 0;
    mRcvSeq = 0;
    mLastRcvSeq = 0;

    mByteTmpAudioBuf = new int8_t[Hapitrip::as.audioDataLen]; // for when not using an audio callback
    memset(mByteTmpAudioBuf, 0, Hapitrip::as.audioDataLen);

    mTmpAudioBuf = new int8_t[Hapitrip::as.audioDataLen]; // for when not using an audio callback
    memset(mTmpAudioBuf, 0, Hapitrip::as.audioDataLen);


    // always addd RegulatorThread even if unused
    // JackTrip is different, RegulatorThread only if bufstrategy 3, only settable at launch
    // HackTrip allows changes while running, so RegulatorThread needs to exist
};
// example system commands that show udp port in use in case of trouble starting
// sudo lsof -i:4464
// sudo lsof -i -P -n
// sudo watch ss -tulpn

UDP::~UDP() {
    for (int i = 0; i < mRing; i++) delete[] mRingBuffer[i];
    delete[] mTmpAudioBuf;
    //    delete reg; // don't delete if unused?
}

void UDP::rcvElapsedTime(bool restart) { // measure inter-packet interval
    double elapsed = (double)mRcvTimer.nsecsElapsed() / 1000000.0;
    double delta = (double)elapsed-(double)Hapitrip::as.packetPeriodMS;
    if (restart) mRcvTimer.start();
    else if (Hapitrip::as.verbose && (delta > 0.0)) {
        std::cout.setf(std::ios::showpoint);
        std::cout   << std::setprecision(4) << std::setw(4);
        std::cout   << elapsed
                    << " (ms)   nominal = "
                    << Hapitrip::as.packetPeriodMS
                    << ""
                    << "   delta = "
                    << delta
                    << std::endl;
    }
}

void UDP::byteRingBufferPush(int8_t *buf, [[maybe_unused]] int seq) { // push received packet to ring
    // force sine
    // mTest->sineTest((MY_TYPE *)buf);
    //    mTest->printSamples((MY_TYPE *)buf);

    {
        mRcvSeq = seq;
        memcpy(mByteTmpAudioBuf, buf, Hapitrip::as.audioDataLen); // put in ring

    }
}

// JackTrip mBufferStrategy 1,2,3,4
// virtual void receiveNetworkPacket(int8_t* ptrToReadSlot)
// translates to ringBufferPull()

bool UDP::byteRingBufferPull() { // pull next packet to play out from regulator or ring
    // std::cout << "byteRingBufferPull ";
    bool glitch = (mLastRcvSeq == mRcvSeq);
    mLastRcvSeq = mRcvSeq;
    return glitch;
}

// when not using an audio callback e.g., for chuck these are called from its tick loop
///////////////////////////////////////////////////
// chuck is interleaved, so convert what it wants to send to be non-interleaved
void UDP::sendAudioData(float *buf) { // buf from chuck is interleaved
    MY_TYPE * tmp = (MY_TYPE *)mTmpAudioBuf; // make outgoing packet non-interleaved
    for (int i = 0; i < Hapitrip::as.channels*Hapitrip::as.FPP; i++) {
        int nilFr = i / Hapitrip::as.channels;
        int nilCh = i % Hapitrip::as.channels;
        tmp[nilCh*Hapitrip::as.FPP + nilFr] = (MY_TYPE)(buf[i] * Hapitrip::as.scale); // non-interleaved = interleaved
    }
    send(mTmpAudioBuf);
}

// chuck is interleaved, so non-interleaved arriving packets are converted
void UDP::rcvAudioData(float *buf) { // buf to chuck is interleaved
    readPendingDatagrams(); // and put in ring buffer
    // ringBufferPull();
    //    mTest->sineTest((MY_TYPE *)mTmpAudioBuf); // non-interleaved test signal
    MY_TYPE * tmp1 = (MY_TYPE *)mTmpAudioBuf; // non-interleaved
    float * tmp2 = (float *)buf; // interleaved
    for (int i = 0; i < Hapitrip::as.channels*Hapitrip::as.FPP; i++) {
        int nilFr = i / Hapitrip::as.channels;
        int nilCh = i % Hapitrip::as.channels;
        tmp2[i] = tmp1[nilCh*Hapitrip::as.FPP + nilFr] * Hapitrip::as.invScale; // interleaved = non-interleaved
    }
}
///////////////////////////////////////////////////

void UDP::readPendingDatagrams() { // incoming is triggered from readyRead signal or from rcvAudioData
    // read datagrams in a loop to make sure that all received datagrams are processed
    // because readyRead is emitted for a datagram only when all
    // previous datagrams are read
    if (!hasPendingDatagrams()) rcvElapsedTime(false);
    while (hasPendingDatagrams()) {
        rcvElapsedTime(true);
        int size = pendingDatagramSize();
        if (size == Hapitrip::as.exitPacketSize)
            stop();
        QHostAddress sender;
        quint16 senderPort;
        readDatagram(mRcvPacket.data(), mRcvPacket.size(), &sender, &senderPort);
        memcpy(&mHeader, mRcvPacket.data(), sizeof(HeaderStruct));
        int rcvSeq = mHeader.SeqNumber;
        if (rcvSeq % Hapitrip::as.reportAfterPackets == 0)
            if (Hapitrip::as.verbose) std::cout << "UDP rcv: seq = " << rcvSeq << std::endl;
        int8_t *audioBuf = (int8_t *)(mRcvPacket.data() + sizeof(HeaderStruct));
                   mTest->sineTest((MY_TYPE *)audioBuf); // output sines
        //            mTest->printSamples((MY_TYPE *)audioBuf); // print audio signal
        byteRingBufferPush(audioBuf, rcvSeq);
    }
}

void UDP::send(int8_t *audioBuf) { // outgoing
    mHeader.SeqNumber = (uint16_t)mSendSeq;
    memcpy(mSendPacket.data(), &mHeader, sizeof(HeaderStruct));
    memcpy(mSendPacket.data() + sizeof(HeaderStruct), audioBuf, Hapitrip::as.audioDataLen);
    writeDatagram(mSendPacket, serverHostAddress, mPeerUdpPort);
    if (mSendSeq % Hapitrip::as.reportAfterPackets == 0)
        if (Hapitrip::as.verbose)    std::cout << "UDP send: packet = " << mSendSeq << std::endl;
    mSendSeq++;
    mSendSeq %= 65536;
}

void UDP::stop() { // the connection and close the socket
    disconnect(this, &QUdpSocket::readyRead, this, &UDP::readPendingDatagrams);
    // Send exit packet (with 1 redundant packet).
    if (Hapitrip::as.verbose) std::cout << "sending exit packet" << std::endl;
    QByteArray stopBuf;
    stopBuf.resize(Hapitrip::as.exitPacketSize);
    stopBuf.fill(0xff, Hapitrip::as.exitPacketSize);
    writeDatagram(stopBuf, serverHostAddress, mPeerUdpPort);
    writeDatagram(stopBuf, serverHostAddress, mPeerUdpPort);
    waitForBytesWritten();
    close(); // stop rcv
}

#ifndef NO_AUDIO
int Audio::wrapperProcessCallback(void *outputBuffer, void *inputBuffer, // shim to format above callback method

                                  unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status, void *arg) {
    return static_cast<Audio *>(arg)->audioCallback(
        outputBuffer, inputBuffer, nBufferFrames, streamTime, status, arg);
}

int Audio::audioCallback(void *outputBuffer, void *inputBuffer,
                         unsigned int /* nBufferFrames */,
                         double /* streamTime */,
                         RtAudioStreamStatus /* status */,
                         void * /* data */) // last arg is used for "this"
{ // xxx
    mUdp->send((int8_t *)inputBuffer); // send one packet to server with contents from the audio input source
    // cout << mTestPLC->channels << "\n";
    mTestPLC->zeroTmpFloatBuf();
    bool glitch = mUdp->byteRingBufferPull();
    glitch = false;
    mTest->sineTest((MY_TYPE *)mUdp->mByteTmpAudioBuf);

    mTestPLC->toFloatBuf((MY_TYPE *)mUdp->mByteTmpAudioBuf);
    // mTestPLC->burg( glitch );
    mTestPLC->fromFloatBuf((MY_TYPE *)outputBuffer);
    // mTest->sineTest((MY_TYPE *)outputBuffer);
    mTestPLC->mPcnt++;

    // memcpy(outputBuffer, inputBuffer, Hapitrip::as.audioDataLen); // test straight wire
    // mTest->sineTest((MY_TYPE *)outputBuffer); // output sines
    // mTest->printSamples((MY_TYPE *)outputBuffer); // print audio signal

    return 0;
}
#endif

#ifndef NO_AUDIO
bool Audio::start() {
    m_streamTimePrintIncrement = 1.0; // seconds -- (unused) from RtAudio examples/duplex
    m_streamTimePrintTime = 1.0;      // seconds -- (unused) from RtAudio examples/duplex
    // setup borrowed from RtAudio examples/duplex
    m_channels = Hapitrip::as.channels;
    m_fs = Hapitrip::as.sampleRate;

    // various RtAudio API's
#ifdef USEBETA
    m_adac = new RtAudio(RtAudio::Api(Hapitrip::as.rtAudioAPI)); // reference by enum
#else
    m_adac = new RtAudio(); // test with win10
#endif
    m_iOffset = 0;
    m_oOffset = 0; // channel 0 is first channel
#ifdef USEBETA
    std::vector<unsigned int> deviceIds = m_adac->getDeviceIds(); // list audio devices
    if (deviceIds.size() < 1) {
        std::cout << "\nNo audio devices found!\n";
        exit(1);
    }
    if (m_adac->getDeviceCount() < 1) { // something found but double check with getDeviceCount
        std::cout << "\nNo audio devices found!\n";
        exit(1);
    } else {
        std::cout << "\naudio devices found =\n"
                  << m_adac->getDeviceCount() << "\n";
    }
    m_iDevice = m_oDevice = 0; // unused
    m_iParams.nChannels = m_channels;
    m_iParams.firstChannel = m_iOffset;
    m_oParams.nChannels = m_channels;
    m_oParams.firstChannel = m_oOffset;
    m_iParams.deviceId = m_adac->getDefaultInputDevice();
    m_oParams.deviceId = m_adac->getDefaultOutputDevice();
#else
    m_iParams.deviceId = 0;
    m_iParams.nChannels = m_channels;
    m_iParams.firstChannel = m_iOffset;
    m_oParams.deviceId = 0;
    m_oParams.nChannels = m_channels;
    m_oParams.firstChannel = m_oOffset;
    if ( m_iParams.deviceId == 0 )
        m_iParams.deviceId = m_adac->getDefaultInputDevice();
    if ( m_oParams.deviceId == 0 )
        m_oParams.deviceId = m_adac->getDefaultOutputDevice();
#endif

    m_adac->showWarnings(true);

    options.flags = RTAUDIO_NONINTERLEAVED | RTAUDIO_SCHEDULE_REALTIME; // non-interleaved
    options.numberOfBuffers =
            Hapitrip::as.numberOfBuffersSuggestionToRtAudio;
    std::cout << "\niParams.deviceId = " << m_iParams.deviceId << std::endl;
    std::cout << "\noParams.deviceId = " << m_oParams.deviceId << std::endl;

    // Windows DirectSound, Linux OSS, and Linux Alsa APIs only.
    // value set above is replaced during execution of the
    // RtAudio::openStream() function by the value actually used by the system

    std::cout << "using default audio interface device\n"; // brag about being set correctly
    std::cout << m_adac->getDeviceInfo(m_iParams.deviceId).name
              << "\tfor input and output\n";
    std::cout << "\tIf another is needed, either change your settings\n";
    std::cout << "\tor the choice in the code\n";
    unsigned int bufferFrames = Hapitrip::as.FPP;
#ifndef AUDIO_ONLY
#ifdef USEBETA
    if (m_adac->openStream(&m_oParams, &m_iParams, FORMAT,
                           Hapitrip::as.sampleRate,
                           &bufferFrames, &Audio::wrapperProcessCallback,
                           (void *)this, // was mUdp class callback instead
                           &options)) // specify Audio class callback
        std::cout << "\nCouldn't open audio device streams!\n";
#else
    m_adac->openStream( &m_oParams, &m_iParams, FORMAT,
                        Hapitrip::as.sampleRate, &bufferFrames,
                        &Audio::wrapperProcessCallback,
                        (void *)mUdp,
                        &options );
#endif
#else
    // from RtAudio examples/duplex
    if (m_adac->openStream(&m_oParams, &m_iParams, FORMAT, Hapitrip::as.sampleRate,
                           &bufferFrames, &Audio::wrapperProcessCallback,
                           (void *)this, &options)) // specify Audio class callback
        std::cout << "\nCouldn't open audio device streams!\n";

#endif
    bool fail = false;
    if (m_adac->isStreamOpen() == false) {
        std::cout << "\nCouldn't open audio device streams!\n";
        exit(1);
    } else {
        std::cout << "\trunning "
                  << m_adac->getApiDisplayName(m_adac->getCurrentApi()) << "\n";
        std::cout << "\nStream latency = " << m_adac->getStreamLatency()
                  << " frames" << std::endl;
    }
    if (Hapitrip::as.FPP != (int)bufferFrames)
        std::cout << "selected FPP conflicts with audio backend bufferFrames "
                  << Hapitrip::as.FPP << " != "
                  << bufferFrames
                  << "\n-- make them the same then connect & run again\n";
    else {

#ifdef USEBETA
        std::cout << "\nAudio stream starting" << std::endl; // phew...
        if (m_adac->startStream()) {
            std::cout << "\nCouldn't start streams!\n";
            fail = true;
        }
        else
            std::cout << "\nAudio stream started" << std::endl; // phew...
#else
        try {
            m_adac->startStream();
        }
        catch ( RtAudioError& e ) {
            std::cout << '\n' << e.getMessage() << '\n' << std::endl;
            fail = true;
        }
#endif
    }
    return fail;
}

void Audio::stop() { // graceful audio shutdown
    if (m_adac != nullptr)
        if (m_adac->isStreamRunning()) {
            std::cout << "\nAudio stream stop" << std::endl;
            m_adac->stopStream();
            if (m_adac->isStreamOpen()) {
                m_adac->closeStream();
                std::cout << "Audio stream closed" << std::endl;
            }
        }
    while (m_adac->isStreamRunning()) {
        std::cout << "\nAudio stream stopped? " << m_adac->isStreamRunning() << std::endl;
        QThread::msleep(500);
    }
    std::cout << "\nAudio stream stopped? " << m_adac->isStreamRunning() << std::endl;
    delete m_adac;
    m_adac = nullptr;
}
#endif

TestAudio::TestAudio(int channels) { mPhasor.resize(channels, 0.0); }

void TestAudio::sineTest(MY_TYPE *buffer) { // generate next bufferfull and convert to short int
    for (int ch = 0; ch < Hapitrip::as.channels; ch++) {
        for (int i = 0; i < Hapitrip::as.FPP; i++) {
            double tmp = sin(mPhasor[ch]);
            *buffer++ = (MY_TYPE)(tmp * Hapitrip::as.scale);
            mPhasor[ch] += ((ch) ? 0.20 : 0.22);
        }
    }
}

void TestAudio::printSamples(MY_TYPE *buffer) { // get next bufferfull, convert to double and print
    for (int ch = 0; ch < Hapitrip::as.channels; ch++) {
        for (int i = 0; i < Hapitrip::as.FPP; i++) {
            double tmp = ((MY_TYPE)*buffer++) * Hapitrip::as.invScale;
            std::cout << "\t" << tmp << std::endl;
        }
    }
}

void TestPLC::straightWire(MY_TYPE *out, MY_TYPE *in, bool glitch) { // generate next bufferfull and convert to short int
    for (int ch = 0; ch < Hapitrip::as.channels; ch++) {
        for (int i = 0; i < Hapitrip::as.FPP; i++) {
            double tmpIn = ((MY_TYPE)*in++) * Hapitrip::as.invScale;
            double tmpOut = (glitch) ? 0.0 : tmpIn;
            *out++ = (MY_TYPE)(tmpOut * Hapitrip::as.scale);
        }
    }
}

#define NOW (pCnt * fpp) // incrementing time
Channel::Channel ( int fpp, int upToNow, int packetsInThePast ) {
    predictedNowPacket.resize( fpp );
    realNowPacket.resize( fpp );
    outputNowPacket.resize( fpp );
    futurePredictedPacket.resize( fpp );
    mTmpFloatBuf.resize( fpp );
    mZeros.resize( fpp );
    for (int i = 0; i < fpp; i++)
        predictedNowPacket[i] = realNowPacket[i] =
            outputNowPacket[i] = futurePredictedPacket[i] =
            mTmpFloatBuf[i] = mZeros[i] = 0.0;

    realPast.resize( upToNow );
    for (int i = 0; i < upToNow; i++) realPast[i] = 0.0;
    zeroPast.resize( upToNow );
    for (int i = 0; i < upToNow; i++) zeroPast[i] = 1.0;

    for (int i = 0; i < packetsInThePast; i++) { // don't resize, using push_back
        vector<float> tmp(fpp);
        for (int j = 0; j < fpp; j++) tmp[j] = 0.0;
        predictedPast.push_back(tmp);
    }

    coeffs.resize( upToNow - 1 );
    for (int i = 0; i < upToNow - 1; i++) {
        coeffs[i] = 0.0;
    }

    prediction.resize( upToNow + fpp * 2 );
    for (int i = 0; i < upToNow + fpp * 2; i++) {
        prediction[i] = 0.0;
    }

    // setup ring buffer
    mRing = packetsInThePast;
    mWptr = mRing / 2;
    // mRptr = mWptr - 2;
    for (int i = 0; i < mRing; i++) { // don't resize, using push_back
        vector<float> tmp(fpp);
        for (int j = 0; j < fpp; j++) tmp[j] = 0.0;
        mPacketRing.push_back(tmp);
    }

    fakeNow.resize( fpp );
    fakeNowPhasor = 0.0;
    fakeNowPhasorInc = 0.22;
    for (int i = 0; i < fpp; i++) {
        double tmp = sin(fakeNowPhasor);
        tmp *= 0.1;
        fakeNow[i] = tmp;
        fakeNowPhasor += fakeNowPhasorInc;
    }
    lastWasGlitch = false;
}

TestPLC::TestPLC(int chans, int fpp, int bps, int packetsInThePast)
    : channels(chans), fpp(fpp), bps(bps), packetsInThePast(packetsInThePast)
{
    mPcnt = 0;
    mTime = new Time();
    mTime->start();
    if (bps == 16) {
        scale = 32767.0;
        invScale = 1.0 / 32767.0;
    } else cout << "bps != 16 -- add code\n";
    //////////////////////////////////////
    upToNow = packetsInThePast * fpp; // duration
    beyondNow = (packetsInThePast + 1) * fpp; // duration

    mChanData.resize( channels );
    for (int ch = 0; ch < channels; ch++) {
        mChanData[ch] = new Channel ( fpp, upToNow, packetsInThePast );
        mChanData[ch]->fakeNowPhasorInc = 0.11 + 0.03 * ch;
    }

    mFadeUp.resize( fpp );
    mFadeDown.resize( fpp );
    for (int i = 0; i < fpp; i++) {
        mFadeUp[i]   = (double)i / (double)fpp;
        mFadeDown[i] = 1.0 - mFadeUp[i];
    }

    ba = new BurgAlgorithm( upToNow );

    // late = vector<int>{ 42, 44, 46, 48  };
    // lateMod = 45;
    // big gap
    late = vector<int>{ 39, 65, 66, 67, 80, 82, 87, 89, 94, 103, 104, 105, 124, 134,
                       143, 144, 145, 146, 147, 148, 149, 150, 166, 180, 181, 182, 184, 218, 219, 220, 222, 239, 244, 250, 255, 256, 257, 258, 260, 264, 269, 276, 280, 294, 295, 296, 297, 299, 311, 321, 326, 331, 334, 335, 336, 337, 345, 347, 351, 357, 362, 365, 367, 371, 372, 373, 374, 377, 383, 387, 390, 392, 397, 402, 405, 409, 410, 411, 412, 416, 419, 421, 423, 425, 433, 448, 449, 450, 451, 487, 488, 489, 490, 499, 513, 516, 518, 524, 525, 529, 531, 532, 534, 535, 537, 538, 551, 564, 565, 566, 567, 602, 603, 604, 627, 640, 641, 642, 643, 653, 662, 673, 679, 680, 681, 684, 685, 686, 692, 693, 694, 704, 717, 718, 719, 720, 722, 730, 750, 756, 757, 758, 760, 768, 794, 796, 797, 805, 811, 820, 826, 833, 834, 835, 871, 872, 873, 874, 875, 876, 877, 909, 910, 911, 913, 941, 948, 949, 950, 952, 986, 987, 988, 1024, 1025, 1026, 1027, 1028, 1063, 1064, 1065, 1067, 1096, 1101, 1102, 1103, 1105, 1140, 1141, 1142, 1178, 1179, 1180, 1181, 1207, 1212, 1216, 1217, 1218, 1219, 1253, 1255, 1256, 1257, 1258, 1293, 1294, 1295, 1296, 1298, 1299, 1316, 1331, 1332, 1333, 1334, 1344, 1370, 1371, 1372, 1374, 1375, 1380, 1382, 1386, 1395, 1408, 1409, 1410, 1411, 1413, 1414, 1441, 1442, 1447, 1448, 1449, 1451, 1461, 1485, 1487, 1488, 1522, 1523, 1524, 1525, 1526, 1528, 1562, 1563, 1564, 1565, 1588, 1600, 1601, 1602, 1603, 1612, 1639, 1640, 1641, 1642, 1643, 1675, 1677, 1678, 1679, 1680, 1716, 1717, 1718, 1720, 1728, 1754, 1755, 1756, 1757, 1776, 1792, 1793, 1794, 1795, 1818, 1831, 1832, 1833, 1835, 1843, 1869, 1870, 1871, 1872, 1908, 1909, 1910, 1912, 1934, 1946, 1947, 1948, 1949, 1970, 1982, 1984, 1985, 1986, 1987, 1989};
    lateMod = late.size();
    // late = vector<int>{ 39, 65, 66, 67, 80, 82, 87, 89, 94, 103, 104, 105, 124, 134,
    //                    143, 144, 150, 166, 180, 181, 182, 184, 218, 219, 220, 222, 239, 244, 250, 255, 256, 257, 258, 260, 264, 269, 276, 280, 294, 295, 296, 297, 299, 311, 321, 326, 331, 334, 335, 336, 337, 345, 347, 351, 357, 362, 365, 367, 371, 372, 373, 374, 377, 383, 387, 390, 392, 397, 402, 405, 409, 410, 411, 412, 416, 419, 421, 423, 425, 433, 448, 449, 450, 451, 487, 488, 489, 490, 499, 513, 516, 518, 524, 525, 529, 531, 532, 534, 535, 537, 538, 551, 564, 565, 566, 567, 602, 603, 604, 627, 640, 641, 642, 643, 653, 662, 673, 679, 680, 681, 684, 685, 686, 692, 693, 694, 704, 717, 718, 719, 720, 722, 730, 750, 756, 757, 758, 760, 768, 794, 796, 797, 805, 811, 820, 826, 833, 834, 835, 871, 872, 873, 874, 875, 876, 877, 909, 910, 911, 913, 941, 948, 949, 950, 952, 986, 987, 988, 1024, 1025, 1026, 1027, 1028, 1063, 1064, 1065, 1067, 1096, 1101, 1102, 1103, 1105, 1140, 1141, 1142, 1178, 1179, 1180, 1181, 1207, 1212, 1216, 1217, 1218, 1219, 1253, 1255, 1256, 1257, 1258, 1293, 1294, 1295, 1296, 1298, 1299, 1316, 1331, 1332, 1333, 1334, 1344, 1370, 1371, 1372, 1374, 1375, 1380, 1382, 1386, 1395, 1408, 1409, 1410, 1411, 1413, 1414, 1441, 1442, 1447, 1448, 1449, 1451, 1461, 1485, 1487, 1488, 1522, 1523, 1524, 1525, 1526, 1528, 1562, 1563, 1564, 1565, 1588, 1600, 1601, 1602, 1603, 1612, 1639, 1640, 1641, 1642, 1643, 1675, 1677, 1678, 1679, 1680, 1716, 1717, 1718, 1720, 1728, 1754, 1755, 1756, 1757, 1776, 1792, 1793, 1794, 1795, 1818, 1831, 1832, 1833, 1835, 1843, 1869, 1870, 1871, 1872, 1908, 1909, 1910, 1912, 1934, 1946, 1947, 1948, 1949, 1970, 1982, 1984, 1985, 1986, 1987, 1989};
    // lateMod = late.size();
    latePtr = 0;
    mNotTrained = 0;
}

void TestPLC::burg(bool glitch) { // generate next bufferfull and convert to short int
    bool primed = mPcnt > packetsInThePast;
    for (int ch = 0; ch < channels; ch++) {
        Channel * c = mChanData[ch];
        //////////////////////////////////////
        if (glitch) mTime->trigger();

        for (int i = 0; i < fpp; i++) {
            double tmp = sin( c->fakeNowPhasor );
            tmp *= 0.1;
            c->fakeNow[i] = tmp;
            c->fakeNowPhasor += c->fakeNowPhasorInc;
        }

        for ( int s = 0; s < fpp; s++ ) c->realNowPacket[s] = (!glitch) ? c->mTmpFloatBuf[s] : 0.0;
        // for ( int s = 0; s < fpp; s++ ) c->realNowPacket[s] = (!glitch) ? c->fakeNow[s] : 0.0;
        // keep history of generated signal
        if (!glitch) {
            for ( int s = 0; s < fpp; s++ ) c->mTmpFloatBuf[s] = c->realNowPacket[s];
            c->ringBufferPush();
        }

        if (primed) {
            int offset = 0;
            for ( int i = 0; i < packetsInThePast; i++ ) {
                c->ringBufferPull( packetsInThePast - i );
                for ( int s = 0; s < fpp; s++ )  c->realPast[s + offset] = c->mTmpFloatBuf[s];
                offset += fpp;
            }
        }

        if (glitch) {
            for ( int s = 0; s < upToNow; s++ ) c->prediction[s] =
                    c->predictedPast[s/fpp][s%fpp];
            // for ( int s = 0; s < upToNow; s++ ) c->prediction[s] = (s%fpp) ?
            //                            c->predictedPast[s/fpp][s%fpp]
            //                            : 0.5;
            // if (!(mNotTrained%100))
            {
                ba->train( c->coeffs,
                          // c->realPast
                          c->prediction
                          // (c->lastWasGlitch) ? c->prediction : c->realPast
                          , upToNow );
                cout << "\ncoeffs ";
            }
            // if (mNotTrained < 2) c->coeffs[0] = 0.9;
            mNotTrained++;

            ba->predict( c->coeffs, c->prediction );
            // if (pCnt < 200) for ( int s = 0; s < 3; s++ )
            //         cout << pCnt << "\t" << s << "---"
            //              << prediction[s+upToNow] << " \t"
            //              << coeffs[s] << " \n";
            for ( int s = 0; s < fpp; s++ ) c->predictedNowPacket[s] =
                    c->prediction[upToNow + s];
        }

        for ( int s = 0; s < fpp; s++ ) c->mTmpFloatBuf[s] = c->outputNowPacket[s] =
                ((glitch) ?
                     ( (primed) ? c->predictedNowPacket[s] : 0.0 )
                          :
                     ( (c->lastWasGlitch) ?
                          ( mFadeDown[s] * c->futurePredictedPacket[s] + mFadeUp[s] * c->realNowPacket[s] )
                                         : c->realNowPacket[s] ));

        for ( int s = 0; s < fpp; s++ ) c->mTmpFloatBuf[s] = c->outputNowPacket[s];
        //         (c->lastWasGlitch) ? c->prediction[s] : c->realPast[s];
        // for ( int s = 0; s < fpp; s++ ) c->mTmpFloatBuf[s] = c->coeffs[s + 0*fpp];
        // for ( int s = 0; s < fpp; s++ ) c->mTmpFloatBuf[s] = c->prediction[upToNow + s];

        c->lastWasGlitch = glitch;

        for ( int i = 0; i < packetsInThePast - 1; i++ ) {
            for ( int s = 0; s < fpp; s++ ) c->predictedPast[i][s] =
                    c->predictedPast[i + 1][s];
        }
        for ( int s = 0; s < fpp; s++ ) c->predictedPast[packetsInThePast - 1][s] =
                c->outputNowPacket[s];

        if (false) for ( int i = 0; i < packetsInThePast - 1; i++ ) {
                for ( int s = 0; s < fpp; s++ ) c->predictedPast[i][s] =
                        c->prediction[(i + 1) * fpp + s];
            }

        for ( int s = 0; s < fpp; s++ ) {
            c->futurePredictedPacket[s] =
                c->prediction[beyondNow + s - 0];
            // earlier bug was heap overflow because of smaller coeffs size, so -1 was ok, now prediction is larger
        }
        //////////////////////////////////////

        if (glitch) mTime->collect();
    }
    if (Hapitrip::as.dVerbose)
        if (!(mPcnt%300)) std::cout << "avg " << mTime->avg() << " \n";
}

TestPLC::~TestPLC(){
    delete mTime;
    for (int ch = 0; ch < channels; ch++) delete mChanData[ch];
    delete ba;
}

void TestPLC::zeroTmpFloatBuf() {
    for (int ch = 0; ch < channels; ch++)
        mChanData[ch]->mTmpFloatBuf = mChanData[ch]->mZeros;
}

void TestPLC::toFloatBuf(MY_TYPE *in) {
    for (int ch = 0; ch < channels; ch++)
        for (int i = 0; i < fpp; i++) {
            double tmpIn = ((MY_TYPE)*in++) * invScale;
            mChanData[ch]->mTmpFloatBuf[i] = tmpIn;
        }
}

void TestPLC::fromFloatBuf(MY_TYPE *out) {
    for (int ch = 0; ch < channels; ch++)
        for (int i = 0; i < fpp; i++) {
            double tmpOut = mChanData[ch]->mTmpFloatBuf[i];
            if (tmpOut > 1.0) tmpOut = 1.0;
            if (tmpOut < -1.0) tmpOut = -1.0;
            *out++ = (MY_TYPE)(tmpOut * scale);
        }
}

void Channel::ringBufferPush() { // push received packet to ring
    mPacketRing[mWptr] = mTmpFloatBuf;
    mWptr++;
    mWptr %= mRing;
}

void Channel::ringBufferPull(int past) { // pull numbered packet from ring
    // bool priming = ((mPcnt - past) < 0); checked outside
    // if (!priming) {
    int pastPtr = mWptr - past;
    if (pastPtr < 0) pastPtr += mRing;
    mTmpFloatBuf = mPacketRing[pastPtr];
    // } else cout << "ring buffer not primed\n";
}
