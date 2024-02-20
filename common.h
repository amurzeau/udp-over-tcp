#pragma once

#ifdef _WIN32
#include <ws2tcpip.h>

#include <windows.h>
typedef SOCKET socket_t;

#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define
#endif

#include <stdint.h>
#include <vector>

extern bool enableDebugLogs;

#pragma pack(push, 1)
struct UDPPacket {
    // Associated client size UDP source port (on udp-to-tcp side)
    // This is used to keep track of request-replies
    uint16_t associatedSourcePort;

    uint16_t data_size;
    char data[];
};
#pragma pack(pop)

class CommonServer {
  public:
    static void closesocket(socket_t s);
    static void msleep(int ms);
    static void logWithErrno(const char *format, ...);
    static void debug(const char *format, ...);

    static bool readPacketToBuffer(socket_t s, std::vector<uint8_t> *buffer);
    static UDPPacket *readPacket(std::vector<uint8_t> *buffer);
    static void removePacketFromBuffer(UDPPacket *packet, std::vector<uint8_t> *buffer);
};