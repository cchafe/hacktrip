#include "hapitrip.h"

#include <QtEndian>
#include <math.h>

void Hapitrip::connect() {
#ifndef AUDIO_ONLY
  mUdp.setPeerUdpPort(mTcp.connectToServer());
  mUdp.setTest(Hapitrip::mChannels);
  mAudio.setUdp(&mUdp);
#endif
  mAudio.setTest(Hapitrip::mChannels);
}

void Hapitrip::run() {
#ifndef AUDIO_ONLY
  mUdp.start();
#endif
  mAudio.start();
}

void Hapitrip::stop() {
#ifndef AUDIO_ONLY
  mUdp.stop();
#endif
  mAudio.stop();
}

#ifndef AUDIO_ONLY
int TCP::connectToServer() {
  QHostAddress serverHostAddress;
  if (!serverHostAddress.setAddress(gServer)) {
    QHostInfo info = QHostInfo::fromName(gServer);
    if (!info.addresses().isEmpty()) {
      // use the first IP address
      serverHostAddress = info.addresses().constFirst();
    }
  }
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
  //    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  //    int optval = 1;
  //    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
  //               (void *) &optval, sizeof(optval));
  //    setSocketDescriptor(sockfd, QUdpSocket::UnconnectedState);
  int ret = 0;
  ret = bind(Hapitrip::mLocalAudioUdpPort);
  std::cout << "UDP: start send = " << ret << " "
            << serverHostAddress.toString().toLocal8Bit().data() << std::endl;
  connect(this, &QUdpSocket::readyRead, this, &UDP::readPendingDatagrams);
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
  connect(&mSendTmer, &QTimer::timeout, this, &UDP::sendDummyData);
  mSendTmer.start(Hapitrip::mPacketPeriodMS);
#endif
};

void UDP::rcvTimeout() {
  std::cout << "rcv: ms since last packet = "
            << (double)mRcvTmer.nsecsElapsed() / 1000000.0 << std::endl;
  mRcvTimeout.start();
}

#ifdef FAKE_STREAMS
void UDP::sendDummyData() {
  QByteArray fakeAudioBuf;
  fakeAudioBuf.resize(Hapitrip::mAudioDataLen);
  fakeAudioBuf.fill(0xff, Hapitrip::mAudioDataLen);
  send((int8_t *)&fakeAudioBuf);
}
#endif

void UDP::send(int8_t *audioBuf) {
  mHeader.SeqNumber = (uint16_t)mSendSeq;
  memcpy(mBufSend.data(), &mHeader, sizeof(HeaderStruct));
  memcpy(mBufSend.data() + sizeof(HeaderStruct), audioBuf,
         Hapitrip::mAudioDataLen);
  writeDatagram(mBufSend, serverHostAddress, mPeerUdpPort);
  if (mSendSeq % Hapitrip::mReportAfterPackets == 0)
    std::cout << "UDP send: packet = " << mSendSeq << std::endl;
  mSendSeq++;
  mSendSeq %= 65536;
}

void UDP::stop() {
#ifdef FAKE_STREAMS
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
  close(); // stop rcv
}

void UDP::readPendingDatagrams() {
  // read datagrams in a loop to make sure that all received datagrams are
  // processed since readyRead() is emitted for a datagram only when all
  // previous datagrams are read
  //     QMutexLocker locker(&mMutex);

  mRcvTmer.start();
  mRcvTimeout.start(Hapitrip::mTimeoutMS);
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
