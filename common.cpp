#include "common.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

bool enableDebugLogs = false;

void CommonServer::closesocket(socket_t s) {
#ifndef _WIN32
    close(s);
#else
    ::closesocket(s);
#endif
}

void CommonServer::msleep(int ms) {
#ifndef _WIN32
    usleep(ms * 1000);
#else
    ::Sleep(ms);
#endif
}

void CommonServer::logWithErrno(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("ERROR: %s(%d): ", strerror(errno), errno);
    vprintf(format, args);
    va_end(args);
}

void CommonServer::debug(const char *format, ...) {
    if (enableDebugLogs) {
        va_list args;
        va_start(args, format);
        printf("Debug: ");
        vprintf(format, args);
        va_end(args);
    }
}

bool CommonServer::readPacketToBuffer(socket_t s, std::vector<uint8_t> *buffer) {
    char data[2000];

    int result = recv(s, data, sizeof(data), 0);
    if (result < 0) {
        logWithErrno("Recv() failed\n");
        return false;
    }

    if (result == 0) {
        // Connection closed
        return false;
    }

    buffer->reserve(buffer->size() + result);
    buffer->insert(buffer->end(), data, data + result);

    return true;
}

UDPPacket *CommonServer::readPacket(std::vector<uint8_t> *buffer) {
    if (buffer->size() < sizeof(UDPPacket)) {
        // Not enough buffered data
        return nullptr;
    }

    UDPPacket *packet = (UDPPacket *)buffer->data();
    if (buffer->size() < packet->data_size + sizeof(UDPPacket)) {
        // Not enough buffered data
        return nullptr;
    }

    return packet;
}

void CommonServer::removePacketFromBuffer(UDPPacket *packet, std::vector<uint8_t> *buffer) {
    buffer->erase(buffer->begin(), buffer->begin() + packet->data_size + sizeof(UDPPacket));
}
