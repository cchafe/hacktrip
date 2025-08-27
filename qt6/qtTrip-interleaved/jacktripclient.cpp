#include "jacktripclient.h"
#include <QDebug>
#include <QMediaDevices>
#include <QtEndian>
#include <QDateTime>

// Configuration constants
const quint16 TCP_PORT = 4464;
const int SAMPLE_RATE = 48000;
const int CHANNELS = 2;
const int SAMPLE_SIZE_BITS = 16;
const int FRAMES_PER_PACKET = 128; // Standard FPP for JackTrip

JackTripClient::JackTripClient(QObject *parent)
    : QObject(parent),
      m_tcpSocket(nullptr),
      m_udpSocket(nullptr),
      m_serverEphemeralUdpPort(0),
      m_localUdpPort(4464), // Default local port to listen on and report to server
      m_audioSource(nullptr),
      m_audioSink(nullptr),
      m_audioInputDevice(nullptr),
      m_audioOutputDevice(nullptr),
      m_sequenceNumber(0)
{
    setupAudio();
}

JackTripClient::~JackTripClient()
{
    stop();
    delete m_audioSource;
    delete m_audioSink;
}

void JackTripClient::setupAudio()
{
    m_audioFormat.setSampleRate(SAMPLE_RATE);
    m_audioFormat.setChannelCount(CHANNELS);
    m_audioFormat.setSampleFormat(QAudioFormat::Int16);

    const auto &defaultInputDevice = QMediaDevices::defaultAudioInput();
    if (!defaultInputDevice.isFormatSupported(m_audioFormat)) {
        qWarning() << "Default input device does not support the audio format.";
        return;
    }

    const auto &defaultOutputDevice = QMediaDevices::defaultAudioOutput();
    if (!defaultOutputDevice.isFormatSupported(m_audioFormat)) {
        qWarning() << "Default output device does not support the audio format.";
        return;
    }

    m_audioSource = new QAudioSource(m_audioFormat, this);
    m_audioSink = new QAudioSink(m_audioFormat, this);
    
    m_bufferSizeInBytes = FRAMES_PER_PACKET * CHANNELS * (SAMPLE_SIZE_BITS / 8);
    m_audioSource->setBufferSize(m_bufferSizeInBytes);
    // Use a larger buffer for the sink to help absorb network jitter
    m_audioSink->setBufferSize(m_bufferSizeInBytes * 8);
}

void JackTripClient::connectToServer(const QString &serverIp)
{
    if (m_tcpSocket) return;

    m_serverIp = serverIp;
    m_tcpSocket = new QTcpSocket(this);

    connect(m_tcpSocket, &QTcpSocket::connected, this, &JackTripClient::onTcpConnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &JackTripClient::onTcpReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &JackTripClient::onTcpError);

    qDebug() << "TCP: Connecting to" << m_serverIp << "on port" << TCP_PORT;
    m_tcpSocket->connectToHost(m_serverIp, TCP_PORT);
}

void JackTripClient::stop()
{
    qDebug() << "Stopping client...";
    if (m_audioSource) m_audioSource->stop();
    if (m_audioSink) m_audioSink->stop();
    
    if (m_udpSocket) {
        m_udpSocket->close();
        delete m_udpSocket;
        m_udpSocket = nullptr;
    }
    
    if (m_tcpSocket) {
        m_tcpSocket->close();
        delete m_tcpSocket;
        m_tcpSocket = nullptr;
    }

    m_audioInputDevice = nullptr;
    m_audioOutputDevice = nullptr;
}

void JackTripClient::onTcpConnected()
{
    qDebug() << "TCP: Connected to hub server.";
    // Per the hub server protocol, send our local UDP port number as an ASCII string.
    QByteArray payload = QByteArray::number(m_localUdpPort);
    qDebug() << "TCP: Sending local UDP port:" << payload;
    m_tcpSocket->write(payload);
}

void JackTripClient::onTcpReadyRead()
{
    if (m_tcpSocket->bytesAvailable() < sizeof(quint32)) return;

    // The server replies with a 32-bit little-endian integer for the ephemeral UDP port.
    QByteArray data = m_tcpSocket->read(sizeof(quint32));
    m_serverEphemeralUdpPort = qFromLittleEndian<quint32>(data.constData());

    qDebug() << "TCP: Received ephemeral UDP port:" << m_serverEphemeralUdpPort;

    // Handshake is complete.
    m_tcpSocket->close();
    
    if (m_serverEphemeralUdpPort > 0) {
        startAudioStream();
    } else {
        qWarning() << "Received invalid ephemeral port. Aborting.";
        stop();
    }
}

void JackTripClient::onTcpError(QTcpSocket::SocketError error)
{
    qWarning() << "TCP Error:" << m_tcpSocket->errorString();
    stop();
}

void JackTripClient::startAudioStream()
{
    qDebug() << "Starting audio stream...";
    m_udpSocket = new QUdpSocket(this);
    if (!m_udpSocket->bind(QHostAddress::Any, m_localUdpPort)) {
        qWarning() << "UDP: Failed to bind to port" << m_localUdpPort;
        stop();
        return;
    }
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &JackTripClient::onUdpReadyRead);
    qDebug() << "UDP: Bound to local port" << m_localUdpPort;

    // Prepare the constant fields of the JackTrip header.
    m_header.BufferSize = qToLittleEndian(static_cast<uint16_t>(FRAMES_PER_PACKET));
    m_header.SamplingRate = 3; // Corresponds to 48000 Hz
    m_header.BitResolution = SAMPLE_SIZE_BITS;
    m_header.NumIncomingChannelsFromNet = CHANNELS;
    m_header.NumOutgoingChannelsToNet = CHANNELS;

    m_audioOutputDevice = m_audioSink->start();
    m_audioInputDevice = m_audioSource->start();
    connect(m_audioInputDevice, &QIODevice::readyRead, this, &JackTripClient::sendAudioData);
    
    qDebug() << "Audio stream started. Streaming to" << m_serverIp << ":" << m_serverEphemeralUdpPort;
}

void JackTripClient::sendAudioData()
{
    if (!m_audioInputDevice || !m_udpSocket || m_audioInputDevice->bytesAvailable() < m_bufferSizeInBytes) return;

    QByteArray audioData = m_audioInputDevice->read(m_bufferSizeInBytes);
    if (audioData.size() != m_bufferSizeInBytes) return;
    
    QByteArray packet;
    packet.reserve(sizeof(JackTripHeader) + m_bufferSizeInBytes);

    // Update sequence number and timestamp for each packet.
    m_header.SeqNumber = qToLittleEndian(m_sequenceNumber++);
    m_header.TimeStamp = qToLittleEndian(static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch()));

    packet.append(reinterpret_cast<const char*>(&m_header), sizeof(JackTripHeader));
    packet.append(audioData);
    
    m_udpSocket->writeDatagram(packet, QHostAddress(m_serverIp), m_serverEphemeralUdpPort);
}

void JackTripClient::onUdpReadyRead()
{
    while (m_udpSocket && m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size());
        
        if (m_audioOutputDevice && datagram.size() > sizeof(JackTripHeader)) {
            // This basic implementation writes audio directly to the output device.
            // A production-ready client would use a jitter buffer here.
            m_audioOutputDevice->write(datagram.mid(sizeof(JackTripHeader)));
        }
    }
}

