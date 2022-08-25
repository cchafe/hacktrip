#include "hapitrip.h"

#include <QtEndian>
#include <math.h>
#include <iostream>
#include <ios>
#include <iomanip>

APIsettings Hapitrip::as; // declare static APIsettings instance

int Hapitrip::connectToServer([[maybe_unused]] QString server) {
#ifndef AUDIO_ONLY
    as.server = server; // set the server
    mTcp = new TCP(); // temporary for handshake with server
    mUdp = new UDP(as.server); // bidirectional socket for trading audio with server
    mUdp->setPeerUdpPort(mTcp->connectToServer()); // to get the server port to send to
    delete mTcp; // done with TCP
    mUdp->setTest(as.channels); // in case of audio test points
#ifndef NO_AUDIO
    mAudio.setUdp(mUdp);
#endif
#endif
#ifndef NO_AUDIO
    mAudio.setTest(as.channels);
#endif
    return mUdp->getPeerUdpPort();
}

void Hapitrip::run() { // hit this before server times out in like 10 seconds
#ifndef AUDIO_ONLY
    mUdp->start(); // bidirectional flows
#endif
#ifndef NO_AUDIO
    mAudio.start();
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
    reg = new Regulator(Hapitrip::as.channels,
                        Hapitrip::as.bytesPerSample,
                        Hapitrip::as.FPP,
                        35, // qlen needs to be param
                        Hapitrip::as.scale, Hapitrip::as.invScale,
                        Hapitrip::as.verbose,
                        Hapitrip::as.audioDataLen);
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

    //    mTest->sineTest((MY_TYPE *)buf);
    //    mTest->printSamples((MY_TYPE *)buf);


    if (Hapitrip::as.usePLC)
        reg->shimFPP(buf, Hapitrip::as.audioDataLen, seq); // where datalen should be incoming for shimmng
    else {
        memcpy(mRingBuffer[mWptr], buf, Hapitrip::as.audioDataLen); // put in ring
        mWptr++;
        mWptr %= mRing;
    }
}

void UDP::ringBufferPull() { // pull next packet to play out from ring
    if (Hapitrip::as.usePLC)
        reg->pullPacket(mTmpAudioBuf);
    //    mTest->sineTest((MY_TYPE *)mTmpAudioBuf);
    else {
        if (mRptr == mWptr) mRptr = mWptr - 2; // if there's an incoming packet stream underrun
        if (mRptr<0) mRptr += mRing;
        mRptr %= mRing;
        memcpy(mTmpAudioBuf, mRingBuffer[mRptr], Hapitrip::as.audioDataLen); // audio output of next ring buffer slot
        mRptr++; // advance to the next slot
    }
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
int UDP::audioCallback(void *outputBuffer, void *inputBuffer, // called by audio driver for audio transfers
                       unsigned int /* nBufferFrames */,
                       double /* streamTime */,
                       RtAudioStreamStatus /* status */,
                       void * /* data */) // last arg is used for "this"
{
    send((int8_t *)inputBuffer); // send one packet to server with contents from the audio input source
    ringBufferPull();
    memcpy(outputBuffer, mTmpAudioBuf, Hapitrip::as.audioDataLen);
    // audio diagnostics, modify or print output and input buffers
    //    memcpy(outputBuffer, inputBuffer, Hapitrip::mAudioDataLen); // test
    //    straight wire mTest->sineTest((MY_TYPE *)outputBuffer); // output sines
    //    mTest->printSamples((MY_TYPE *)outputBuffer); // print audio signal

    return 0;
}

int Audio::wrapperProcessCallback(void *outputBuffer, void *inputBuffer, // shim to format UDP callback method
                                  unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status, void *arg) {
    return static_cast<UDP *>(arg)->audioCallback( // callback method
                                                   outputBuffer, inputBuffer, nBufferFrames, streamTime, status, arg);
}
#endif
#else // test with the straight wire example in RtAudio examples/duplex

int Audio::audioCallback(void *outputBuffer, void *inputBuffer,
                         unsigned int /* nBufferFrames */,
                         double /* streamTime */,
                         RtAudioStreamStatus /* status */,
                         void * /* data */) // last arg is used for "this"
{
    // audio diagnostics, modify or print output and input buffers
    memcpy(outputBuffer, inputBuffer,
           Hapitrip::as.audioDataLen); // test straight wire
    //        mTest->sineTest((MY_TYPE *)outputBuffer); // output sines
    //        mTest->printSamples((MY_TYPE *)outputBuffer); // print audio signal

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
void Audio::start() {
    m_streamTimePrintIncrement = 1.0; // seconds -- (unused) from RtAudio examples/duplex
    m_streamTimePrintTime = 1.0;      // seconds -- (unused) from RtAudio examples/duplex

    // various RtAudio API's
    m_adac = new RtAudio(RtAudio::Api(Hapitrip::as.rtAudioAPI)); // reference by enum
    //    m_adac = new RtAudio(); // test with win10

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
    // setup borrowed from RtAudio examples/duplex
    m_channels = Hapitrip::as.channels;
    m_fs = Hapitrip::as.sampleRate;
    m_iDevice = m_oDevice = 0; // unused
    m_iOffset = m_oOffset = 0; // channel 0 is first channel
    m_adac->showWarnings(true);
    // copy info into audio input and audio output streams
    m_iParams.nChannels = m_channels;
    m_iParams.firstChannel = m_iOffset;
    m_oParams.nChannels = m_channels;
    m_oParams.firstChannel = m_oOffset;
    m_iParams.deviceId = m_adac->getDefaultInputDevice();
    m_oParams.deviceId = m_adac->getDefaultOutputDevice();
    options.flags = RTAUDIO_NONINTERLEAVED | RTAUDIO_SCHEDULE_REALTIME; // non-interleaved
    options.numberOfBuffers =
            Hapitrip::as.numberOfBuffersSuggestionToRtAudio;
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
    if (m_adac->openStream(&m_oParams, &m_iParams, FORMAT, Hapitrip::as.sampleRate,
                           &bufferFrames, &Audio::wrapperProcessCallback,
                           (void *)mUdp, &options)) // specify UDP class callback
        std::cout << "\nCouldn't open audio device streams!\n";
#else
    // from RtAudio examples/duplex
    if (m_adac->openStream(&m_oParams, &m_iParams, FORMAT, Hapitrip::as.sampleRate,
                           &bufferFrames, &Audio::wrapperProcessCallback,
                           (void *)this, &options)) // specify Audio class callback
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
    if (m_adac->startStream())
        std::cout << "\nCouldn't start streams!\n";
    std::cout << "\nAudio stream started" << std::endl; // phew...
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
