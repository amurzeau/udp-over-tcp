#include "tcp-to-udp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void TcpToUdp::run(const char *listenIp, uint16_t listenPort, const char *targetIp, uint16_t targetPort) {
    /*
     * Get a socket for accepting connections.
     */
    socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        logWithErrno("socket() failed\n");
        exit(2);
    }

    const int enable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&enable, sizeof(enable)) < 0) {
        logWithErrno("setsockopt(SO_REUSEADDR) failed");
    }

    /*
     * Bind the socket to the server address.
     */
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(listenPort);
    inet_pton(AF_INET, listenIp, &server.sin_addr.s_addr);

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        logWithErrno("Bind to TCP %s:%d failed\n", listenIp, listenPort);
        exit(3);
    }

    /*
     * Listen for connections. Specify the backlog as 1.
     */
    if (listen(s, 1) != 0) {
        logWithErrno("Listen on TCP %s:%d failed\n", listenIp, listenPort);
        exit(4);
    }

    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(targetPort);
    inet_pton(AF_INET, targetIp, &target.sin_addr.s_addr);

    printf("Listening TCP connection on %s:%d\n", listenIp, listenPort);
    while (true) {
        /*
         * Accept a connection.
         */
        struct sockaddr_in client;
        socklen_t namelen = sizeof(client);
        socket_t tcpConnectionSocket = accept(s, (struct sockaddr *)&client, &namelen);
        if (tcpConnectionSocket == INVALID_SOCKET) {
            logWithErrno("Accept connection on %s:%d failed\n", listenIp, listenPort);
            msleep(1000);
        }

        char clientIp[32] = "";
        inet_ntop(AF_INET, &client.sin_addr.s_addr, clientIp, sizeof(clientIp));
        printf("Got TCP connection from %s:%d, forwarding to %s:%d\n", clientIp, htons(client.sin_port), targetIp,
               targetPort);
        (void)new TcpConnection(tcpConnectionSocket, target);
    }

    closesocket(s);
}

TcpConnection::TcpConnection(socket_t tcpConnectionSocket, sockaddr_in targetAddress)
    : tcpConnectionSocket(tcpConnectionSocket), targetAddress(targetAddress) {
    tcpThread.reset(new std::thread(&TcpConnection::processPackets, this));
}

void TcpConnection::processPackets() {
    while (true) {
        if (!readPacketToBuffer(tcpConnectionSocket, &streamBuffer)) {
            logWithErrno("Recv on TCP socket failed\n");
            break;
        }

        UDPPacket *packet;
        while ((packet = readPacket(&streamBuffer)) != nullptr) {

            // Find which stream it is
            auto it = udpStreams.find(packet->associatedSourcePort);

            UdpStream *stream;

            if (it == udpStreams.end()) {
                // Not yet known, add it to the hashmap
                stream = new UdpStream(this, packet->associatedSourcePort, targetAddress);
                udpStreams[packet->associatedSourcePort] = std::unique_ptr<UdpStream>{stream};
            } else {
                stream = it->second.get();
            }

            stream->sendPacketToTarget(packet->data, packet->data_size);

            removePacketFromBuffer(packet, &streamBuffer);
        }
    }

    tcpThread->detach();
    closesocket(tcpConnectionSocket);
    delete this;
}

void TcpConnection::sendPacket(const void *data, size_t size) {
    // Send the message to target UDP endpoint
    if (send(tcpConnectionSocket, (const char *)data, size, 0) < 0) {
        logWithErrno("Send to TCP client failed\n");
        exit(7);
    }
}

UdpStream::UdpStream(TcpConnection *tcpConnection, uint16_t sourcePort, sockaddr_in targetAddress)
    : tcpConnection(tcpConnection), sourcePort(sourcePort), targetAddress(targetAddress), stopThread(false) {
    /*
     * Get a socket for accepting connections.
     */
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == INVALID_SOCKET) {
        logWithErrno("socket UDP() failed\n");
        exit(2);
    }

    /*
     * Bind the socket to the server address.
     */
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(0); // Dynamic port allocation
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(udpSocket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        logWithErrno("Bind to UDP 0.0.0.0:0 for replies failed\n");
        exit(3);
    }

    struct sockaddr_in boundAddress;
    socklen_t boundAddressSize = sizeof(boundAddress);
    getsockname(udpSocket, (sockaddr *)&boundAddress, &boundAddressSize);
    printf("New UDP stream from source port %d, listening replies on port %d\n", sourcePort,
           htons(boundAddress.sin_port));

    tcpThread.reset(new std::thread(&UdpStream::processPackets, this));
}

UdpStream::~UdpStream() {
    stopThread = true;
    closesocket(udpSocket);
    tcpThread->join();
}

void UdpStream::processPackets() {
    while (!stopThread) {
        char buffer[2000];
        sockaddr_in peerAddress;
        socklen_t peerAddressSize = sizeof(peerAddress);
        int result = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (sockaddr *)&peerAddress, &peerAddressSize);

        if (result < 0) {
            logWithErrno("recvfrom UDP replies failed\n");
            break;
        }

        if (peerAddress.sin_addr.s_addr != targetAddress.sin_addr.s_addr ||
            peerAddress.sin_port != targetAddress.sin_port) {
            // Packet comes from somewhere else, ignore it
            continue;
        }

        // Send to TCP socket
        UDPPacket *packet = (UDPPacket *)malloc(sizeof(UDPPacket) + result);
        packet->associatedSourcePort = sourcePort;
        packet->data_size = result;
        memcpy(packet->data, buffer, packet->data_size);

        tcpConnection->sendPacket(packet, sizeof(UDPPacket) + result);

        free(packet);
    }
}

void UdpStream::sendPacketToTarget(const void *data, size_t size) {
    // Send the message to target UDP endpoint
    if (sendto(udpSocket, (const char *)data, size, 0, (const sockaddr *)&targetAddress, sizeof(targetAddress)) < 0) {
        logWithErrno("Send to UDP target failed\n");
        exit(7);
    }
}
