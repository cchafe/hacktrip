#include "hapitrip.h"

#include <QtEndian>
#include <math.h>
#include <iostream>
#include <ios>
#include <iomanip>

// RegulatorThread
#include <QThread>

APIsettings Hapitrip::as; // declare static APIsettings instance
int gVerboseFlag = 0;

void errorCallback( RtAudioErrorType /*type*/, const std::string &errorText )
{
    // This example error handling function simply outputs the error message to stderr.
    if(gVerboseFlag == 2)    std::cerr << "\nerrorCallback: " << errorText << "\n\n";
}

int Hapitrip::connectToServer([[maybe_unused]] QString server) {
#ifdef AUDIO_ONLY
    mAudio.setTest(as.channels);
    return 1; // AUDIO_ONLY still needs connectToServer which needs to return non-zero
#else
    as.server = server; // set the server
    mTcp = new TCP(); // temporary for handshake with server
    mUdp = new UDP(as.server); // bidirectional socket for trading audio with server
    mUdp->setPeerUdpPort(mTcp->connectToServer()); // to get the server port to send to
    delete mTcp; // done with TCP
    mUdp->setTest(as.channels); // in case of audio test points
#ifndef NO_AUDIO
    mAudio.setUdp(mUdp);
#endif
#ifndef NO_AUDIO
    mAudio.setTest(as.channels);
#endif
    return mUdp->getPeerUdpPort();
#endif
}

void Hapitrip::run() { // hit this before server times out in like 10 seconds
#ifndef AUDIO_ONLY
    mUdp->start(); // bidirectional flows
#endif
#ifndef NO_AUDIO
    if (mAudio.start()) {
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
    mAudio.stop();
#endif
#ifndef AUDIO_ONLY
    delete mUdp;
    mUdp = nullptr;
#endif
}

#ifndef AUDIO_ONLY
// when not using an audio callback e.g., for chuck
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

    mTmpAudioBuf = new int8_t[Hapitrip::as.audioDataLen]; // for when not using an audio callback
    memset(mTmpAudioBuf, 0, Hapitrip::as.audioDataLen);

    mReg4 = new Regulator(Hapitrip::as.channels,
                          Hapitrip::as.bytesPerSample,
                          Hapitrip::as.FPP,
                          10, // qlen needs to be param
                          //                          Hapitrip::as.sampleRate,
                          //                          false, // = 4
                          Hapitrip::as.scale, Hapitrip::as.invScale,
                          Hapitrip::as.verbose,
                          Hapitrip::as.audioDataLen);
/*
    mReg3 = new Regulator(Hapitrip::as.channels,
                          Hapitrip::as.bytesPerSample,
                          Hapitrip::as.FPP,
                          0, // qlen needs to be param
                          Hapitrip::as.sampleRate,
                          true, // = 3
                          Hapitrip::as.scale, Hapitrip::as.invScale,
                          Hapitrip::as.verbose,
                          Hapitrip::as.audioDataLen);

    mReg4 = new Regulator(Hapitrip::as.channels,
                          Hapitrip::as.bytesPerSample,
                          Hapitrip::as.FPP,
                          0, // qlen needs to be param
                          Hapitrip::as.sampleRate,
                          false, // = 4
                          Hapitrip::as.scale, Hapitrip::as.invScale,
                          Hapitrip::as.verbose,
                          Hapitrip::as.audioDataLen);
*/
    // always add RegulatorThread even if unused
    // JackTrip is different, RegulatorThread only if bufstrategy 3, only settable at launch
    // HackTrip allows changes while running, so RegulatorThread needs to exist
/*    if (true) {
        mRegulatorThreadPtr = new QThread();
        mRegulatorThreadPtr->setObjectName("RegulatorThread");
        Regulator* regulatorPtr    = reinterpret_cast<Regulator*>(mReg3);
        RegulatorWorker* workerPtr = new RegulatorWorker(regulatorPtr);
        workerPtr->moveToThread(mRegulatorThreadPtr);
        QObject::connect(this, &UDP::signalReceivedNetworkPacket, workerPtr,
                         &RegulatorWorker::pullPacket, Qt::QueuedConnection);
        mRegulatorThreadPtr->start();
        mRegulatorWorkerPtr = workerPtr;
    }
*/
};

// example system commands that show udp port in use in case of trouble starting
// sudo lsof -i:4464
// sudo lsof -i -P -n
// sudo watch ss -tulpn

UDP::~UDP() {
    for (int i = 0; i < mRing; i++) delete mRingBuffer[i];
    delete mTmpAudioBuf;
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

void UDP::ringBufferPush(int8_t *buf, [[maybe_unused]] int seq) { // push received packet to ring

    // force sine
       // mTest->sineTest((MY_TYPE *)buf);
    //    mTest->printSamples((MY_TYPE *)buf);


    if (Hapitrip::as.usePLC) {
        if (Hapitrip::as.usePLCthread) mReg3->shimFPP(buf, Hapitrip::as.audioDataLen, seq); // where datalen should be incoming for shimmng
        else mReg4->shimFPP(buf, Hapitrip::as.audioDataLen, seq); // where datalen should be incoming for shimmng
    } else {
        memcpy(mRingBuffer[mWptr], buf, Hapitrip::as.audioDataLen); // put in ring
        mWptr++;
        mWptr %= mRing;
    }
}

// JackTrip mBufferStrategy 1,2,3,4
// virtual void receiveNetworkPacket(int8_t* ptrToReadSlot)
// translates to ringBufferPull()

void UDP::ringBufferPull() { // pull next packet to play out from regulator or ring
//    std::cout << "ringBufferPull ";
    if (Hapitrip::as.usePLC) {
        if (Hapitrip::as.usePLCthread) emit signalReceivedNetworkPacket();
        else mReg4->readSlotNonBlocking(mTmpAudioBuf);
//        if (Hapitrip::as.usePLCthread) std::cout << " == 3 "; else std::cout << " == 4 ";
    } else  { // simple version of mBufferStrategy 1,2
//        std::cout << " == 1 ";
        if (mRptr == mWptr) mRptr = mWptr - 2; // if there's an incoming packet stream underrun
        if (mRptr<0) mRptr += mRing;
        mRptr %= mRing;
        memcpy(mTmpAudioBuf, mRingBuffer[mRptr], Hapitrip::as.audioDataLen); // audio output of next ring buffer slot
        mRptr++; // advance to the next slot
    }
//    std::cout << "\n";
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
    ringBufferPull();
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
        //            mTest->sineTest((MY_TYPE *)audioBuf); // output sines
        //            mTest->printSamples((MY_TYPE *)audioBuf); // print audio signal
        ringBufferPush(audioBuf, rcvSeq);
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
bool UDP::exceedsCallbackInterval( double msPadding ) {
    bool exceeds = false;
    if (lastCallbackTime == 0.0) mCallbackTimer.start();
    else {
        double now = (double)mCallbackTimer.nsecsElapsed() / 1000000.0;
        double delta = now - lastCallbackTime;
        std::cout.setf(std::ios::showpoint);
        std::cout   << std::setprecision(4) << std::setw(4);
        std::cout   << delta
                  << " (ms)   nominal = "
                  << Hapitrip::as.packetPeriodMS
                  << std::endl;
        lastCallbackTime = now;
        if ((delta - Hapitrip::as.packetPeriodMS) > msPadding) exceeds = true;
    }
    return exceeds;
}

int UDP::audioCallback(void *outputBuffer, void *inputBuffer, // called by audio driver for audio transfers
                       unsigned int /* nBufferFrames */,
                       double /* streamTime */,
                       RtAudioStreamStatus /* status */,
                       void * /* data */) // last arg is used for "this"
{
    // if (exceedsCallbackInterval(0.25)) {
    //     std::cout << "    that's how bad the previous callback exceeded \n";
    // }
    // else {
    send((int8_t *)inputBuffer); // send one packet to server with contents from the audio input source
    ringBufferPull();
    memcpy(outputBuffer, mTmpAudioBuf, Hapitrip::as.audioDataLen);
    // audio diagnostics, modify or print output and input buffers
    //    memcpy(outputBuffer, inputBuffer, Hapitrip::mAudioDataLen); // test
    //    straight wire mTest->sineTest((MY_TYPE *)outputBuffer); // output sines
    //    mTest->printSamples((MY_TYPE *)outputBuffer); // print audio signal
    // }
    return 0;
}

int Audio::wrapperProcessCallback(void *outputBuffer, void *inputBuffer, // shim to format UDP callback method
                                  unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status, void *arg) {
    return static_cast<UDP *>(arg)->audioCallback( // callback method
                                                   outputBuffer, inputBuffer, nBufferFrames, streamTime, status, arg);
}

#endif

// comment out this directive so testPLC can take over callback
// #else // test with the straight wire example in RtAudio examples/duplex

int Audio::audioCallback(void *outputBuffer, void *inputBuffer,
                         unsigned int /* nBufferFrames */,
                         double /* streamTime */,
                         RtAudioStreamStatus /* status */,
                         void * /* data */) // last arg is used for "this"
{
    // audio diagnostics, modify or print output and input buffers
    memcpy(outputBuffer, inputBuffer,
           Hapitrip::as.audioDataLen); // test straight wire
    // mTest->sineTest((MY_TYPE *)outputBuffer); // output sines
           // mTest->printSamples((MY_TYPE *)outputBuffer); // print audio signal

    return 0;
}

int Audio::wrapperProcessCallback(void *outputBuffer, void *inputBuffer, // shim to format above callback method

                                  unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status, void *arg) {
    return static_cast<Audio *>(arg)->audioCallback(
                outputBuffer, inputBuffer, nBufferFrames, streamTime, status, arg);
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
    m_adac = new RtAudio(RtAudio::Api(Hapitrip::as.rtAudioAPI), &errorCallback ); // reference by enum

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
                           (void *)mUdp,
                           &options)) // specify UDP class callback
        std::cout << "\nCouldn't open audio device streams!\n";
#else
    m_adac->openStream( &m_oParams, &m_iParams, FORMAT,
                        Hapitrip::as.sampleRate, &bufferFrames,
                        &Audio::wrapperProcessCallback,
                        (void *)mUdp,
                        &options );
#endif
// comment out this directive to use openStream with mTestPLC below
// #else
    // from RtAudio examples/duplex
    if (m_adac->openStream(&m_oParams, &m_iParams, FORMAT, Hapitrip::as.sampleRate,
                           &bufferFrames, &Audio::wrapperProcessCallback,
                           (void *)this, &options)) // specify Audio class callback
        std::cout << "\nCouldn't open audio device streams!\n";

#endif
    m_adac->openStream( &m_oParams, &m_iParams, FORMAT,
                       Hapitrip::as.sampleRate, &bufferFrames,
                       &Audio::wrapperProcessCallback,
                       (void *)mTestPLC,
                       &options );
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

void TestAudio::sineTest(MY_TYPE *buffer) { // generate next bufferfull and convert to short int
    for (int ch = 0; ch < Hapitrip::as.channels; ch++) {
        for (int i = 0; i < Hapitrip::as.FPP; i++) {
            double tmp = sin(mPhasor[ch]);
            tmp *= 0.1;
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

int Audio::wrapperProcessCallback(void *outputBuffer, void *inputBuffer, // shim to format UDP callback method
                                  unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status, void *arg) {
    // static_cast<TestPLC *>(arg)->mTmpAudioBufIn[0] = 9.99;

    return static_cast<TestPLC *>(arg)->audioCallback( // callback method
        outputBuffer, inputBuffer, nBufferFrames, streamTime, status, arg);
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

int TestPLC::audioCallback(void *outputBuffer, void *inputBuffer, // called by audio driver for audio transfers
                           unsigned int /* nBufferFrames */,
                           double /* streamTime */,
                           RtAudioStreamStatus /* status */,
                           void * /* data */) // last arg is used for "this"
{
    // send((int8_t *)inputBuffer); // send one packet to server with contents from the audio input source
    // ringBufferPull();
    // memcpy(outputBuffer, inputBuffer, Hapitrip::as.audioDataLen);
    sineTest((MY_TYPE *)inputBuffer); // output sines
    // straightWire((MY_TYPE *)outputBuffer,(MY_TYPE *)inputBuffer,(!(pCnt%80)));
    burg((MY_TYPE *)outputBuffer,(MY_TYPE *)inputBuffer,(!(pCnt%80)));
    pCnt++;
    return 0;
}

TestPLC::TestPLC(int channels) : TestAudio (channels) {
    pCnt = 0;
    //////////////////////////////////////
    fpp = Hapitrip::as.FPP;
    packetsInThePast = 2;
#define FROMTHEPAST ((pCnt - packetsInThePast) * fpp) // incrementing time
#define NOW (pCnt * fpp) // incrementing time
    upToNow = packetsInThePast * fpp; // duration
    beyondNow = (packetsInThePast + 1) * fpp; // duration
    mFadeUp.resize( fpp );
    mFadeDown.resize( fpp );
    for (int i = 0; i < fpp; i++) {
        mFadeUp[i]   = (double)i / (double)fpp;
        mFadeDown[i] = 1.0 - mFadeUp[i];
    }
#define PACKETSAMP ( int s = 0; s < fpp; s++ )
    predictedNowPacket.resize( fpp );
    realNowPacket.resize( fpp );
    outputNowPacket.resize( fpp );
    futurePredictedPacket.resize( fpp );

    realPast.resize( upToNow );
    for (int i = 0; i < packetsInThePast; i++) {
        vector<float> tmp(fpp);
        for (int j = 0; j < fpp; j++) tmp[j] = 0.0;
        predictedPast.push_back(tmp);
    }
    lastWasGlitch = false;
    mTmpAudioBufIn.resize( fpp );
    mTmpAudioBufOut.resize( fpp );
}

void TestPLC::burg(MY_TYPE *out, MY_TYPE *in, bool glitch) { // generate next bufferfull and convert to short int
    for (int ch = 0; ch < Hapitrip::as.channels; ch++) {
        //////////////////////////////////////
        for (int i = 0; i < Hapitrip::as.FPP; i++) {
            double tmpIn = ((MY_TYPE)*in++) * Hapitrip::as.invScale;
            mTmpAudioBufIn[i] = tmpIn;
        }

        for (int i = 0; i < Hapitrip::as.FPP; i++) {
            ringBufferPush(mTmpAudioBufIn);
            mTmpAudioBufOut[i] = mTmpAudioBufIn[i];
        }

        for (int i = 0; i < Hapitrip::as.FPP; i++) {
            double tmpOut = mTmpAudioBufOut[i];
            if (tmpOut > 1.0) tmpOut = 1.0;
            if (tmpOut < -1.0) tmpOut = -1.0;
            *out++ = (MY_TYPE)(tmpOut * Hapitrip::as.scale);
        }
        //////////////////////////////////////

    }
}
