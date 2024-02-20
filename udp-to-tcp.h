#pragma once

#include "common.h"
#include <memory>
#include <stdint.h>
#include <thread>
#include <vector>

class UdpToTcp : public CommonServer {
  public:
    void run(const char *listenIp, uint16_t listenPort, const char *targetIp, uint16_t targetPort);

    void processTcpPackets();
    void processUdpPackets();

  private:
    socket_t udpSocket;
    socket_t tcpSocket;

    bool stopThread;
    std::unique_ptr<std::thread> udpThread;
    std::vector<uint8_t> streamBuffer;

    struct in_addr listenIp;
};