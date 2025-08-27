const WebSocket = require('ws');
const dgram = require('dgram');
const net = require('net');

const JACKTRIP_SERVER_IP = '171.64.197.158';
const JACKTRIP_TCP_PORT = 4464;
const PROXY_WS_PORT = 8080;

// --- JackTrip Header Constants ---
const HEADER_SIZE = 16; // 8+2+2+1+1+1+1
const BUFFER_SIZE = 128; // Frames per packet
const SAMPLE_RATE_CODE = 3; // 48000 Hz
const BIT_RESOLUTION = 16;
const NUM_CHANNELS = 2;

const wss = new WebSocket.Server({ port: PROXY_WS_PORT });
console.log(`WebSocket proxy server started on port ${PROXY_WS_PORT}`);

wss.on('connection', ws => {
    console.log('Web client connected.');

    let udpSocket = dgram.createSocket('udp4');
    let tcpSocket = new net.Socket();
    let jacktripEphemeralPort = 0;
    let localUdpPort = 0;
    let sequenceNumber = 0; // Per-client sequence number

    udpSocket.bind(() => {
        localUdpPort = udpSocket.address().port;
        console.log(`Proxy UDP socket bound to local port ${localUdpPort}`);
        
        tcpSocket.connect(JACKTRIP_TCP_PORT, JACKTRIP_SERVER_IP, () => {
            console.log(`TCP: Connected to JackTrip server ${JACKTRIP_SERVER_IP}:${JACKTRIP_TCP_PORT}`);
            
            const buffer = Buffer.alloc(4);
            buffer.writeUInt32LE(localUdpPort, 0);

            console.log(`TCP: Sending local UDP port ${localUdpPort} as a 4-byte LE integer.`);
            tcpSocket.write(buffer);
        });
    });

    tcpSocket.on('data', data => {
        if (data.length >= 4) {
            jacktripEphemeralPort = data.readUInt32LE(0);
            console.log(`TCP: Received ephemeral UDP port ${jacktripEphemeralPort} from server.`);
            tcpSocket.end();
        }
    });

    tcpSocket.on('close', () => {
        console.log('TCP: Connection closed.');
        if (jacktripEphemeralPort === 0) {
            console.error('TCP: Handshake failed, did not receive ephemeral port.');
            ws.close();
        } else {
             console.log(`Audio bridge ready. Forwarding audio between WebSocket and UDP ${JACKTRIP_SERVER_IP}:${jacktripEphemeralPort}`);
        }
    });

    tcpSocket.on('error', err => {
        console.error('TCP Error:', err.message);
        ws.close();
    });

    // --- Data Forwarding ---

    // From Web App (WebSocket) to JackTrip Server (UDP)
    ws.on('message', audioPayload => {
        if (jacktripEphemeralPort > 0 && udpSocket) {
            // 1. Create a new buffer for the header + audio
            const packet = Buffer.alloc(HEADER_SIZE + audioPayload.length);

            // 2. Write the JackTrip header (all Little Endian)
            // TimeStamp (uint64_t) - use milliseconds since epoch
            packet.writeBigUInt64LE(BigInt(Date.now()), 0);
            // SeqNumber (uint16_t)
            packet.writeUInt16LE(sequenceNumber++, 8);
            // BufferSize (uint16_t)
            packet.writeUInt16LE(BUFFER_SIZE, 10);
            // SamplingRate (uint8_t)
            packet.writeUInt8(SAMPLE_RATE_CODE, 12);
            // BitResolution (uint8_t)
            packet.writeUInt8(BIT_RESOLUTION, 13);
            // NumIncomingChannelsFromNet (uint8_t)
            packet.writeUInt8(NUM_CHANNELS, 14);
            // NumOutgoingChannelsToNet (uint8_t)
            packet.writeUInt8(NUM_CHANNELS, 15);

            // 3. Copy the audio payload after the header
            audioPayload.copy(packet, HEADER_SIZE);

            // 4. Send the complete packet
            udpSocket.send(packet, jacktripEphemeralPort, JACKTRIP_SERVER_IP, (err) => {
                if (err) console.error('UDP Send Error:', err.message);
            });
        }
    });

    // From JackTrip Server (UDP) to Web App (WebSocket)
    udpSocket.on('message', (packet, rinfo) => {
        if (ws.readyState === WebSocket.OPEN) {
            if (packet.length > HEADER_SIZE) {
                // Strip the header and send only the raw audio payload to the web client
                const audioPayload = packet.slice(HEADER_SIZE);
                ws.send(audioPayload);
            }
        }
    });

    // --- Cleanup ---
    const cleanup = () => {
        console.log('Cleaning up resources.');
        if (tcpSocket) tcpSocket.destroy();
        if (udpSocket) udpSocket.close();
        udpSocket = null;
        tcpSocket = null;
    };

    ws.on('close', () => {
        console.log('Web client disconnected.');
        cleanup();
    });

    ws.on('error', (err) => {
        console.error('WebSocket Error:', err.message);
        cleanup();
    });
    
    udpSocket.on('error', (err) => {
        console.error('UDP Socket Error:', err.message);
        ws.close();
    });
});

