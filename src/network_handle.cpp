#include "network_handle.h"

namespace NetworkHandle {
    // Biến toàn cục
    std::atomic<int> activeThreads(0);                // Quản lý các luồng đang hoạt động
    std::map<std::thread::id, std::string> threadMap; // Danh sách luồng và URL
    std::mutex threadMapMutex;                        // Mutex để đồng bộ
    std::map<std::thread::id, std::atomic<bool>> stopFlags; // Cờ dừng cho từng luồng

    std::string parseHttpRequest(const std::string& request) {
        size_t pos = request.find("Host: ");
        if (pos == std::string::npos) return std::string();

        size_t start = pos + 6;
        size_t end = request.find("\r\n", start);
        if (end == std::string::npos) return std::string();
        
        return "https://" + request.substr(start, end - start);
    }

    void handleConnectMethod(SOCKET clientSocket, const std::string& host, int port) {
        // Tạo socket để kết nối đến server đích
        SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (remoteSocket == INVALID_SOCKET) {
            std::cerr << "Cannot create remote socket.\n";

            return;
        }

        // Định nghĩa địa chỉ của server đích
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port); 

        struct hostent* remoteHost = gethostbyname(host.c_str());
        if (remoteHost == NULL) {
            std::cerr << "Cannot resolve hostname.\n";

            closesocket(remoteSocket);
            return;
        }
        memcpy(&serverAddr.sin_addr.s_addr, remoteHost->h_addr, remoteHost->h_length);

        // Kết nối đến server đích
        if (connect(remoteSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Cannot connect to remote server.\n";
            
            closesocket(remoteSocket);
            return;
        }

        // Gửi phản hồi 200 Connection Established cho client
        const char* established = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(clientSocket, established, strlen(established), 0);

        // Tạo kết nối hai chiều giữa client và server
        fd_set readfds;                                      // Tập các socket đang đợi để đọc
        char buffer[BUFFER_SIZE];
        while (true) {   
            // Kiểm tra nếu thread cần dừng
            if (stopFlags[std::this_thread::get_id()]) {
                std::cerr << "Thread " << std::this_thread::get_id() << " is being stopped.\n";
                return;
            }

            FD_ZERO(&readfds);                               // Xóa tập readfds
            FD_SET(clientSocket, &readfds);                  // Thêm clientSocket vào tập readfds
            FD_SET(remoteSocket, &readfds);                  // Thêm remoteSocket vào tập readfds
            if (select(0, &readfds, NULL, NULL, NULL) > 0) { //select() trả socket chứa dữ liệu có thể đọc
                if (FD_ISSET(clientSocket, &readfds)) {
                    int receivedBytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                    if (receivedBytes <= 0) break;
                    send(remoteSocket, buffer, receivedBytes, 0);
                }
                if (FD_ISSET(remoteSocket, &readfds)) {
                    int receivedBytes = recv(remoteSocket, buffer, BUFFER_SIZE, 0);
                    if (receivedBytes <= 0) break;
                    send(clientSocket, buffer, receivedBytes, 0);
                }
            } else break;
        }

        closesocket(remoteSocket);
    }

    void printActiveThreads() {
        std::lock_guard<std::mutex> lock(threadMapMutex);
        std::cerr << "\nCurrent Active Threads: " << activeThreads.load() << '\n';
        for (const auto& [id, url] : threadMap) {
            std::cerr << "Thread ID: " << id << "\t, URL: " << url << '\n';
        }
    }

    // Function to check active threads and stop the ones with a blacklisted URL
    void checkAndStopBlacklistedThreads() {
        std::lock_guard<std::mutex> lock(threadMapMutex); 
        for (auto& [id, url] : threadMap) {
            std::string host = url.substr(8, url.find(':', 8) - 8);
            if (BlackList::isBlocked(host)) {
                std::cerr << "Blocking thread " << id << " due to blacklisted URL: " << url << '\n';
                stopFlags[id] = true;  // Set flag to true to stop the thread
            }
        }
    }

    void handleClient(SOCKET clientSocket) {
        activeThreads++;

        char buffer[BUFFER_SIZE];
        int receivedBytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (receivedBytes > 0) {
            std::string request(buffer, receivedBytes);
            std::string url = parseHttpRequest(request);

            if (not url.empty()) {
                size_t hostPos = request.find(' ') + 1;
                if (std::string(url.begin() + 7, url.end()).find(':') == std::string::npos) {

                    closesocket(clientSocket);
                    activeThreads--;
                    return;
                }

                size_t portPos = request.find(':', hostPos);
                std::string host = request.substr(hostPos, portPos - hostPos);
                int port = stoi(request.substr(portPos + 1, request.find(' ', portPos) - portPos - 1));

                if(BlackList::isBlocked(host)) {
                    std::cerr << "Access to " << url << " is blocked.\n";

                    closesocket(clientSocket);
                    activeThreads--;
                    return;
                }

                // Thêm URL vào danh sách luồng
                {
                    std::lock_guard<std::mutex> lock(threadMapMutex);
                    threadMap[std::this_thread::get_id()] = url;
                    stopFlags[std::this_thread::get_id()] = false;  // Set stop flag to false initially
                }

                printActiveThreads(); // Hiển thị danh sách luồng

                // Checking if the URL is blacklisted while handling the client
                checkAndStopBlacklistedThreads();  // New check for blacklisted threads

                handleConnectMethod(clientSocket, host, port);
            }
        }

        // Xóa luồng khỏi danh sách và đóng kết nối
        {
            std::lock_guard<std::mutex> lock(threadMapMutex);
            threadMap.erase(std::this_thread::get_id());
            stopFlags.erase(std::this_thread::get_id());
        }

        closesocket(clientSocket);
        activeThreads--;
    }
}