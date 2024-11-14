#include "network_utils.h"

int main() {
    initWinsock();
    SOCKET listenSocket = createSocket();
    bindSocket(listenSocket);
    startListening(listenSocket);

    loadBlacklist("config\\blacklist.txt");

    startServer(listenSocket);
    
    return 0;
}
