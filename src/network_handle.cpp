#include "network_handle.h"

namespace NetworkHandle {
    // Biến toàn cục
    std::atomic<int> activeThreads(0);                      // Quản lý các luồng đang hoạt động
    std::map<std::thread::id, std::pair<std::string, std::string>> threadMap;       // Danh sách luồng và URL
    std::mutex threadMapMutex;                              // Mutex để đồng bộ
    std::map<std::thread::id, std::atomic<bool>> stopFlags; // Cờ dừng cho từng luồng

    std::map<std::string, std::string> hostRequestMap;

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
            UI::UpdateLog("Cannot create remote socket.");
            return;
        }

        // Định nghĩa địa chỉ của server đích
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port); 

        struct hostent* remoteHost = gethostbyname(host.c_str());
        if (remoteHost == NULL) {
            UI::UpdateLog("Cannot resolve hostname.");
            closesocket(remoteSocket);
            return;
        }
        memcpy(&serverAddr.sin_addr.s_addr, remoteHost->h_addr, remoteHost->h_length);

        // Kết nối đến server đích
        if (connect(remoteSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            UI::UpdateLog("Cannot connect to remote server.");
            closesocket(remoteSocket);
            return;
        }

        // Gửi phản hồi 200 Connection Established cho client
        const char* established = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(clientSocket, established, strlen(established), 0);

        // Tạo kết nối hai chiều giữa client và server
        fd_set readfds;                                      // Tập các socket đang đợi để đọc
        char buffer[BUFFER_SIZE];
        while (not stopFlags[std::this_thread::get_id()]) {  // Kiểm tra nếu thread cần dừng 
            if (UI::isProxyRunning == false) {
                UI::UpdateLog("Disconnecting: " + host + " || Reason: Stopped proxy.");
                break;
            }
            FD_ZERO(&readfds);                               // Xóa tập readfds
            FD_SET(clientSocket, &readfds);                  // Thêm clientSocket vào tập readfds
            FD_SET(remoteSocket, &readfds);                  // Thêm remoteSocket vào tập readfds

            struct timeval timeout;
            timeout.tv_sec = 10;                             // Chờ tối đa 10 giây
            timeout.tv_usec = 0;
            if (select(0, &readfds, NULL, NULL, &timeout) <= 0) { // select() trả socket chứa dữ liệu có thể đọc
                UI::UpdateLog("Disconnecting: " + host + " || Reason: Timeout occurred, closing connection.");
                break;
            }

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
        }

        closesocket(remoteSocket);
    }

    void printActiveThreads() {
        // std::lock_guard<std::mutex> lock(threadMapMutex);
        UI::UpdateRunningHosts(threadMap); // Gửi thông tin lên giao diện
    }

    // Function to check active threads and stop the ones with a Blacklisted HOST
    void checkAndStopBlacklistedThreads() {
        std::lock_guard<std::mutex> lock(threadMapMutex); 
        for (auto& [id, host] : threadMap) {
            if (Blacklist::isBlocked(host.first)) {
                stopFlags[id] = true;  // Set flag to true to stop the thread
            }
        }
    }

    void handleClient(SOCKET clientSocket) {
        char buffer[BUFFER_SIZE];
        int receivedBytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (receivedBytes <= 0) {
            return;
        }

        std::string request(buffer, receivedBytes);
        std::string url = parseHttpRequest(request);
        if (url.empty()) {
            return;
        }

        size_t hostPos = request.find(' ') + 1;
        if (std::string(url.begin() + 7, url.end()).find(':') == std::string::npos) {
            closesocket(clientSocket);
            return;
        }

        size_t portPos = request.find(':', hostPos);
        std::string host = request.substr(hostPos, portPos - hostPos);
        int port = stoi(request.substr(portPos + 1, request.find(' ', portPos) - portPos - 1));

        if (Blacklist::isBlocked(host)) {
            UI::UpdateLog("Access to " + host + " is blocked.");
            const char* forbiddenResponse = 
                "HTTP/1.1 403 Forbidden\r\n"
                "Connection: close\r\n"
                "Proxy-Agent: CustomProxy/1.0\r\n"
                "\r\n";

            send(clientSocket, forbiddenResponse, strlen(forbiddenResponse), 0);
            closesocket(clientSocket);
            return;
        }

        // Thêm HOST vào danh sách luồng
        {
            threadMap[std::this_thread::get_id()] = make_pair(host, request);
            hostRequestMap[host] = request;
            stopFlags[std::this_thread::get_id()] = false; // Đặt cờ dừng ban đầu là false

            printActiveThreads(); // Hiển thị danh sách luồng
        }

        // Checking if the HOST is Blacklisted while handling the client
        checkAndStopBlacklistedThreads();  

        activeThreads++;
        
        UI::UpdateLog("Connecting: " + host);
        handleConnectMethod(clientSocket, host, port);
        
        activeThreads--;

        // Xóa luồng khỏi danh sách và đóng kết nối
        {
            std::lock_guard<std::mutex> lock(threadMapMutex);
            hostRequestMap.erase(threadMap[std::this_thread::get_id()].first);
            threadMap.erase(std::this_thread::get_id());
            stopFlags.erase(std::this_thread::get_id());
        }

        printActiveThreads(); // Hiển thị danh sách luồng

        closesocket(clientSocket);
    }
}
