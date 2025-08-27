#ifndef JACKTRIPCLIENT_H
#define JACKTRIPCLIENT_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtMultimedia/QAudioSource>
#include <QtMultimedia/QAudioSink>
#include <QAudioFormat>
#include <QIODevice>

// JackTrip protocol header structure. The #pragma pack directive ensures
// the struct has no padding and its size is the sum of its members,
// which is critical for network serialization.
#pragma pack(push, 1)
struct JackTripHeader {
    uint64_t TimeStamp;
    uint16_t SeqNumber;
    uint16_t BufferSize;
    uint8_t  SamplingRate;
    uint8_t  BitResolution;
    uint8_t  NumIncomingChannelsFromNet;
    uint8_t  NumOutgoingChannelsToNet;
};
#pragma pack(pop)

class JackTripClient : public QObject
{
    Q_OBJECT
public:
    explicit JackTripClient(QObject *parent = nullptr);
    ~JackTripClient();

    void connectToServer(const QString &serverIp);
    void stop();

private slots:
    // TCP Handshake slots
    void onTcpConnected();
    void onTcpReadyRead();
    void onTcpError(QTcpSocket::SocketError error);

    // UDP Audio slots
    void onUdpReadyRead();
    void sendAudioData();

private:
    void setupAudio();
    void startAudioStream();

    // Network members
    QTcpSocket *m_tcpSocket;
    QUdpSocket *m_udpSocket;
    QString m_serverIp;
    quint16 m_serverEphemeralUdpPort;
    quint16 m_localUdpPort;

    // Audio members
    QAudioSource *m_audioSource;
    QAudioSink *m_audioSink;
    QIODevice *m_audioInputDevice;
    QIODevice *m_audioOutputDevice;
    QAudioFormat m_audioFormat;
    int m_bufferSizeInBytes;

    // JackTrip protocol members
    JackTripHeader m_header;
    uint16_t m_sequenceNumber;
};

#endif // JACKTRIPCLIENT_H

