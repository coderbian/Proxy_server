#ifndef NETWORK_HANDLE_H
#define NETWORK_HANDLE_H

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>

#include "ui.h"
#include "constants.h"
#include "blacklist.h"

namespace NetworkHandle {
    extern std::map<std::string, std::string> hostRequestMap;
    
    std::string parseHttpRequest(const std::string &request);
    void        handleConnectMethod(SOCKET clientSocket, const std::string& host, int port);
    void        printActiveThreads();
    void        handleClient(SOCKET clientSocket);
    void        checkAndStopBlacklistedThreads();
}

#endif 