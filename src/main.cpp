#include "constants.h"
#include "blacklist.h"
#include "network_init.h"
#include "network_handle.h"

int main() {
    SOCKET listenSocket = NetworkInit::startInitSocket();

    BlackList::load(BLACKLIST_URL);

    NetworkHandle::startServer(listenSocket);
    
    return 0;
}
