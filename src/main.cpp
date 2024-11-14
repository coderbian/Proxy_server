#include "network_utils.h"

int main() {
    initWinsock();
    SOCKET listenSocket = createSocket();
    bindSocket(listenSocket);
    startListening(listenSocket);

    startServer(listenSocket);
    
    return 0;
}
