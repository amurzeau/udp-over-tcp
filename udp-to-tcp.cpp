#include "udp-to-tcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void run_udp_to_tcp() {}

void UdpToTcp::run(const char *listenIp, uint16_t listenPort, const char *targetIp, uint16_t targetPort) {
    stopThread = false;

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
    struct sockaddr_in udpServer;
    udpServer.sin_family = AF_INET;
    udpServer.sin_port = htons(listenPort);
    inet_pton(AF_INET, listenIp, &udpServer.sin_addr.s_addr);
    this->listenIp = udpServer.sin_addr;

    if (bind(udpSocket, (struct sockaddr *)&udpServer, sizeof(udpServer)) < 0) {
        logWithErrno("Bind to UDP %s:%d failed\n", listenIp, listenPort);
        exit(3);
    }

    // TCP Socket
    /*
     * Get a socket for accepting connections.
     */
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == INVALID_SOCKET) {
        logWithErrno("socket TCP() failed\n");
        exit(2);
    }

    /*
     * Bind the socket to the server address.
     */
    struct sockaddr_in tcpServer;
    tcpServer.sin_family = AF_INET;
    tcpServer.sin_port = htons(targetPort);
    inet_pton(AF_INET, targetIp, &tcpServer.sin_addr.s_addr);

    if (connect(tcpSocket, (struct sockaddr *)&tcpServer, sizeof(tcpServer)) < 0) {
        logWithErrno("Connection to TCP %s:%d failed\n", targetIp, targetPort);
        exit(3);
    }

    printf("Listening UDP message on %s:%d, forwarding to %s:%d\n", listenIp, listenPort, targetIp, targetPort);
    udpThread.reset(new std::thread(&UdpToTcp::processUdpPackets, this));

    processTcpPackets();

    exit(0);
}

void UdpToTcp::processTcpPackets() {
    while (!stopThread) {
        if (!readPacketToBuffer(tcpSocket, &streamBuffer)) {
            logWithErrno("Recv from TCP failed\n");
            break;
        }

        UDPPacket *packet;
        while ((packet = readPacket(&streamBuffer)) != nullptr) {

            struct sockaddr_in peer;
            peer.sin_family = AF_INET;
            peer.sin_port = htons(packet->associatedSourcePort);
            peer.sin_addr = this->listenIp;

            sendto(udpSocket, packet->data, packet->data_size, 0, (const sockaddr *)&peer, sizeof(peer));

            removePacketFromBuffer(packet, &streamBuffer);
        }
    }
}

void UdpToTcp::processUdpPackets() {
    while (!stopThread) {
        char buffer[2000];
        sockaddr_in peerAddress;
        socklen_t peerAddressSize = sizeof(peerAddress);
        int result = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (sockaddr *)&peerAddress, &peerAddressSize);

        if (result < 0) {
            logWithErrno("recvfrom from UDP failed\n");
            break;
        }

        // Send to TCP socket
        UDPPacket *packet = (UDPPacket *)malloc(sizeof(UDPPacket) + result);
        packet->associatedSourcePort = htons(peerAddress.sin_port);
        packet->data_size = result;
        memcpy(packet->data, buffer, packet->data_size);

        send(tcpSocket, (const char *)packet, sizeof(UDPPacket) + result, 0);

        free(packet);
    }
}