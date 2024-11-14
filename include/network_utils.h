#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>

#define PORT 8080
#define BUFFER_SIZE 4096

void        initWinsock();
SOCKET      createSocket();
void        bindSocket(SOCKET listenSocket);
void        startListening(SOCKET listenSocket);

std::string parseHttpRequest(const std::string &request);
void        handleConnectMethod(SOCKET clientSocket, const std::string& host, int port);
void        printActiveThreads();
void        handleClient(SOCKET clientSocket);
void        startServer(SOCKET listenSocket);

#endif 