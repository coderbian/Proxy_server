#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>

#define PORT 8080
#define BUFFER_SIZE 4096

using std::string;
using std::atomic;

void   initWinsock();
SOCKET createSocket();
void   bindSocket(SOCKET listenSocket);
void   startListening(SOCKET listenSocket);
string parseHttpRequest(const string &request);
void   handleConnectMethod(SOCKET clientSocket, const string& host, int port);
void   handleClient(SOCKET clientSocket);
void   startServer(SOCKET listenSocket);
extern atomic<int> activeThreads;

#endif NETWORK_UTILS_H