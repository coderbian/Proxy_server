#include "network_utils.h"

// Biến toàn cục
std::atomic<int> activeThreads(0);                // Quản lý các luồng đang hoạt động
std::map<std::thread::id, std::string> threadMap; // Danh sách luồng và URL
std::mutex threadMapMutex;                        // Mutex để đồng bộ
std::set<std::string> blacklist;

// Cấu trúc (structure) chứa thông tin về việc khởi tạo Winsock trong môi trường Winsock API.
void initWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        
        exit(EXIT_FAILURE);
    }
}

// socket(): Đây là hàm được sử dụng để tạo ra một socket. Socket là một điểm cuối trong giao tiếp mạng. 
//           Các tham số của hàm này bao gồm:
//     AF_INET: Đây là family của địa chỉ, chỉ định sử dụng giao thức IPv4.
//     SOCK_STREAM: Xác định loại socket, trong trường hợp này là một socket dòng (stream), 
//                  nghĩa là một kết nối TCP, nơi dữ liệu được truyền tải dưới dạng dòng liên tục.
//     IPPROTO_TCP: Chỉ định giao thức sẽ được sử dụng, ở đây là giao thức TCP.
SOCKET createSocket() {
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    return listenSocket;
}

void bindSocket(SOCKET listenSocket) {
    sockaddr_in serverAddr;                  // Định nghĩa địa chỉ của server
    serverAddr.sin_family = AF_INET;         // Định dạng địa chỉ IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Chấp nhận kết nối từ mọi địa chỉ IP
    serverAddr.sin_port = htons(PORT);       // Chuyển cổng sang định dạng network byte order

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        
        closesocket(listenSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
}

void startListening(SOCKET listenSocket) {
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        
        closesocket(listenSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    std::cerr << "Proxy server started. Listening on port " << PORT << "...\n";
}

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

            if(isBlocked(host)) {
                std::cerr << "Access to " << url << " is blocked.\n";
                sendErrorResponse(clientSocket);
                
                closesocket(clientSocket);
                activeThreads--;
                return;
            }

            // Thêm URL vào danh sách luồng
            {
                std::lock_guard<std::mutex> lock(threadMapMutex);
                threadMap[std::this_thread::get_id()] = url;
            }

            printActiveThreads(); // Hiển thị danh sách luồng
            
            std::cerr << "Accessed URL: " << url << " || " << host << ':' << port << '\n';
            
            handleConnectMethod(clientSocket, host, port);
        }
    }

    // Xóa luồng khỏi danh sách và đóng kết nối
    {
        std::lock_guard<std::mutex> lock(threadMapMutex);
        threadMap.erase(std::this_thread::get_id());
    }

    closesocket(clientSocket);

    activeThreads--;
}

void startServer(SOCKET listenSocket) {
    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            std::thread clientThread(handleClient, clientSocket);
            clientThread.detach(); 
        }
    }

    closesocket(listenSocket);
    WSACleanup();
}

void loadBlacklist(const std::string& filename) {
    std::ifstream infile(filename);
    std::string line;
    while(std::getline(infile, line)) {
        blacklist.insert(line);
    }
    for (std::string s : blacklist) {
        std::cerr << s << '\n';
    }
}

bool isBlocked(const std::string& url) {
    return blacklist.find(url) != blacklist.end();
}

void sendErrorResponse(SOCKET clientSocket) {
    const char* errorResponse =
        "HTTP/1.1 403 Forbidden\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 138\r\n"
        "\r\n"
        "<html>"
        "<head><title>403 Forbidden</title></head>"
        "<body style='font-family: Arial, sans-serif; text-align: center;'>"
        "<h1>403 Forbidden</h1>"
        "<p>Access Denied</p>"
        "<hr>"
        "<p>Proxy Server</p>"
        "</body>"
        "</html>";
    send(clientSocket, errorResponse, strlen(errorResponse), 0);
}