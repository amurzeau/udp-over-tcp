#pragma once

#include "common.h"
#include <memory>
#include <stdint.h>
#include <thread>
#include <unordered_map>
#include <vector>

class TcpConnection;

class UdpStream : public CommonServer {
  public:
    UdpStream(TcpConnection *tcpConnection, uint16_t sourcePort, sockaddr_in targetAddress);
    ~UdpStream();
    void processPackets();
    void sendPacketToTarget(const void *data, size_t size);

  private:
    TcpConnection *tcpConnection;
    uint16_t sourcePort;
    sockaddr_in targetAddress;
    bool stopThread;

    socket_t udpSocket;
    std::unique_ptr<std::thread> tcpThread;
};

class TcpConnection : public CommonServer {
  public:
    TcpConnection(socket_t tcpConnectionSocket, sockaddr_in targetAddress);
    void processPackets();

    void sendPacket(const void *data, size_t size);

  private:
    socket_t tcpConnectionSocket;
    sockaddr_in targetAddress;
    std::unique_ptr<std::thread> tcpThread;

    std::vector<uint8_t> streamBuffer;
    std::unordered_map<uint16_t, std::unique_ptr<UdpStream>> udpStreams;
};

class TcpToUdp : public CommonServer {
  public:
    void run(const char *listenIp, uint16_t listenPort, const char *targetIp, uint16_t targetPort);

  private:
};