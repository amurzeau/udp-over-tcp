#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "tcp-to-udp.h"
#include "udp-to-tcp.h"

enum mode_e {
    MODE_UNKNOWN,
    MODE_UDP_TO_TCP,
    MODE_TCP_TO_UDP,
};

void help(const char *programName) {
    printf("Usage %s --tcp-to-udp|--udp-to-tcp <interface_ip>:<listen port> --target <remote ip>:<remote port>\n"
           "  --tcp-to-udp <interface_ip>:<listen port>      Run in tcp-to-udp mode and listen for TCP connections on "
           "<interface_ip>:<listen port>\n"
           "  --udp-to-tcp <interface_ip>:<listen port>      Run in udp-to-tcp mode and listen for UDP datagrams on "
           "<interface_ip>:<listen port>\n"
           "  --target <remote ip>:<remote port>             Send received packets to this remote IP:port.\n"
           "                                                 In tcp-to-udp mode, this point to a remote UDP server\n"
           "                                                 In udp-to-tcp mode, this point to a remote TCP server\n",
           programName);

    exit(0);
}

bool parseIpPortAddress(const char *address, std::string *outIp, uint16_t *outPort) {
    const char *p = strchr(address, ':');
    if (p == nullptr) {
        printf("ERROR: %s is not a valid ip:port address\n", address);
        return false;
    }

    // Check if there is only valid ip characters
    outIp->assign(address, p);
    for (const char &c : *outIp) {
        if ((c < '0' || c > '9') && c != '.') {
            printf("ERROR: %s is not a valid IP, must contains only digit or dot\n", outIp->c_str());
            return false;
        }
    }

    // Check if there is only valid port characters
    char *endPtr = nullptr;
    long port = strtol(p + 1, &endPtr, 0);
    if (endPtr == nullptr || *endPtr != '\0') {
        printf("ERROR: %s is not a valid port number\n", p + 1);
        return false;
    }

    if (port < 0 || port > UINT16_MAX) {
        printf("ERROR: %ld is not a valid port number (too large or negative)\n", port);
        return false;
    }

    *outPort = port;

    return true;
}

void parseIpAddressArgument(const char *address, std::string *outIp, uint16_t *outPort) {
    if (!parseIpPortAddress(address, outIp, outPort)) {
        printf("ERROR: invalid address %s\n", address);
        exit(1);
    }
}

int main(int argc, const char *argv[]) {
    // Usage: $0 --tcp-to-udp <interface_ip>:<tcp port> --target <ip>:<udp port>
    // Usage: $0 --udp-to-tcp <ip>:<tcp port> --target <interface_ip>:<udp port>

    mode_e mode = MODE_UNKNOWN;

    // Where to listen, UDP or TCP
    std::string listenIp;
    uint16_t listenPort = 0;

    // Where to send, UDP or TCP
    std::string targetIp;
    uint16_t targetPort = 0;

    enableDebugLogs = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tcp-to-udp") == 0) {
            mode = MODE_TCP_TO_UDP;
            if (i + 1 < argc) {
                parseIpAddressArgument(argv[i + 1], &listenIp, &listenPort);
            } else {
                printf("ERROR: missing argument after %s\n", argv[i]);
                exit(1);
            }
        } else if (strcmp(argv[i], "--udp-to-tcp") == 0) {
            mode = MODE_UDP_TO_TCP;
            if (i + 1 < argc) {
                parseIpAddressArgument(argv[i + 1], &listenIp, &listenPort);
            } else {
                printf("ERROR: missing argument after %s\n", argv[i]);
                exit(1);
            }
        } else if (strcmp(argv[i], "--target") == 0) {
            if (i + 1 < argc) {
                parseIpAddressArgument(argv[i + 1], &targetIp, &targetPort);
            } else {
                printf("ERROR: missing argument after %s\n", argv[i]);
                exit(1);
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            help(argv[0]);
        } else if (strcmp(argv[i], "--debug") == 0) {
            enableDebugLogs = true;
        }
    }

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    switch (mode) {
    case MODE_TCP_TO_UDP: {
        TcpToUdp handler;
        handler.run(listenIp.c_str(), listenPort, targetIp.c_str(), targetPort);
        break;
    }
    case MODE_UDP_TO_TCP: {
        UdpToTcp handler;
        handler.run(listenIp.c_str(), listenPort, targetIp.c_str(), targetPort);
        break;
    }
    default:
        help(argv[0]);
    }

    return 0;
}
