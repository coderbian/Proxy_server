#ifndef NETWORK_INIT_H
#define NETWORK_INIT_H

#include <winsock2.h>
#include <windows.h>
#include <iostream>

#include "constants.h"

namespace NetworkInit {
    void   initWinsock();
    SOCKET createSocket();
    void   bindSocket(SOCKET listenSocket);
    void   startListening(SOCKET listenSocket);
    SOCKET startInitSocket();
}

#endif