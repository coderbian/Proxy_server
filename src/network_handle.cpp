#include "network_handle.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>
#include <openssl/bn.h> // Thêm để sử dụng BIGNUM
#include <memory>

namespace NetworkHandle {
    // Biến toàn cục
    std::atomic<int> activeThreads(0);                      // Quản lý các luồng đang hoạt động
    std::map<std::thread::id, std::pair<std::string, std::string>> threadMap;       // Danh sách luồng và URL
    std::mutex threadMapMutex;                              // Mutex để đồng bộ
    std::map<std::thread::id, std::atomic<bool>> stopFlags; // Cờ dừng cho từng luồng

    std::map<std::string, std::string> hostRequestMap;

    SSL_CTX* clientSSLContext = nullptr;
    SSL_CTX* serverSSLContext = nullptr;
    X509* caCert = nullptr;
    EVP_PKEY* caKey = nullptr;
    std::mutex sslMutex;                                    // Mutex để bảo vệ các thao tác SSL

    // Hàm khởi tạo OpenSSL và tải chứng chỉ CA và khóa riêng
    bool initializeSSL(const std::string& caCertPath, const std::string& caKeyPath) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        // Tải chứng chỉ CA
        FILE* caCertFile = fopen(caCertPath.c_str(), "r");
        if (!caCertFile) {
            UI_WINDOW::UpdateLog("Không thể mở file chứng chỉ CA.");
            return false;
        }
        caCert = PEM_read_X509(caCertFile, NULL, NULL, NULL);
        fclose(caCertFile);
        if (!caCert) {
            UI_WINDOW::UpdateLog("Không thể tải chứng chỉ CA.");
            return false;
        }

        // Tải khóa riêng CA
        FILE* caKeyFile = fopen(caKeyPath.c_str(), "r");
        if (!caKeyFile) {
            UI_WINDOW::UpdateLog("Không thể mở file khóa riêng CA.");
            return false;
        }
        caKey = PEM_read_PrivateKey(caKeyFile, NULL, NULL, NULL);
        fclose(caKeyFile);
        if (!caKey) {
            UI_WINDOW::UpdateLog("Không thể tải khóa riêng CA.");
            return false;
        }

        // Tạo SSL context cho client
        clientSSLContext = SSL_CTX_new(TLS_server_method()); // Sử dụng TLS_server_method thay vì SSLv23_server_method
        if (!clientSSLContext) {
            UI_WINDOW::UpdateLog("Không thể tạo SSL context cho client.");
            return false;
        }
        SSL_CTX_set_options(clientSSLContext, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

        // Tạo SSL context cho server
        serverSSLContext = SSL_CTX_new(TLS_client_method()); // Sử dụng TLS_client_method thay vì SSLv23_client_method
        if (!serverSSLContext) {
            UI_WINDOW::UpdateLog("Không thể tạo SSL context cho server.");
            return false;
        }
        SSL_CTX_set_options(serverSSLContext, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

        return true;
    }

    // Hàm dọn dẹp SSL khi proxy dừng
    void cleanupSSL() {
        if (clientSSLContext) {
            SSL_CTX_free(clientSSLContext);
            clientSSLContext = nullptr;
        }
        if (serverSSLContext) {
            SSL_CTX_free(serverSSLContext);
            serverSSLContext = nullptr;
        }
        if (caCert) {
            X509_free(caCert);
            caCert = nullptr;
        }
        if (caKey) {
            EVP_PKEY_free(caKey);
            caKey = nullptr;
        }
        EVP_cleanup();
        ERR_free_strings();
    }

    // Hàm tạo chứng chỉ cho máy chủ đích
    X509* generateCertificate(const std::string& host) {
        X509* cert = X509_new();
        if (!cert) return nullptr;

        ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
        X509_gmtime_adj(X509_get_notBefore(cert), 0);
        X509_gmtime_adj(X509_get_notAfter(cert), 31536000L); // 1 năm

        // Tạo khóa riêng tạm thời cho chứng chỉ
        EVP_PKEY* pkey = EVP_PKEY_new();
        if (!pkey) {
            X509_free(cert);
            return nullptr;
        }

        // Thay thế RSA_generate_key bằng RSA_generate_key_ex
        RSA* rsa = RSA_new();
        BIGNUM* bn = BN_new();
        if (!rsa || !bn) {
            X509_free(cert);
            EVP_PKEY_free(pkey);
            if (rsa) RSA_free(rsa);
            if (bn) BN_free(bn);
            return nullptr;
        }
        if (!BN_set_word(bn, RSA_F4)) {
            X509_free(cert);
            EVP_PKEY_free(pkey);
            RSA_free(rsa);
            BN_free(bn);
            return nullptr;
        }
        if (!RSA_generate_key_ex(rsa, 2048, bn, NULL)) {
            X509_free(cert);
            EVP_PKEY_free(pkey);
            RSA_free(rsa);
            BN_free(bn);
            return nullptr;
        }
        BN_free(bn);

        // Thay thế EVP_PKEY_assign_RSA bằng EVP_PKEY_set1_RSA
        if (!EVP_PKEY_set1_RSA(pkey, rsa)) {
            X509_free(cert);
            EVP_PKEY_free(pkey);
            RSA_free(rsa);
            return nullptr;
        }
        RSA_free(rsa); // EVP_PKEY_set1_RSA đã tăng reference count, nên giải phóng RSA

        X509_set_pubkey(cert, pkey);

        // Thiết lập tên chủ thể
        X509_NAME* name = X509_get_subject_name(cert);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   (unsigned char*)host.c_str(), -1, -1, 0);
        X509_set_issuer_name(cert, X509_get_subject_name(caCert));

        // Thêm các mở rộng nếu cần thiết (ví dụ: Subject Alternative Name)
        // Ở đây bạn có thể thêm SAN để tránh lỗi chứng chỉ trên trình duyệt
        X509_EXTENSION* ext;
        char san[256];
        snprintf(san, sizeof(san), "DNS:%s", host.c_str());
        ext = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, san);
        if (ext == NULL) {
            X509_free(cert);
            EVP_PKEY_free(pkey);
            return nullptr;
        }
        X509_add_ext(cert, ext, -1);
        X509_EXTENSION_free(ext);

        // Ký chứng chỉ bằng khóa riêng CA
        if (!X509_sign(cert, caKey, EVP_sha256())) {
            X509_free(cert);
            EVP_PKEY_free(pkey);
            return nullptr;
        }

        EVP_PKEY_free(pkey);
        return cert;
    }

    // Hàm khởi tạo proxy
    bool initializeProxy() {
        // Đường dẫn đến chứng chỉ CA và khóa riêng
        std::string caCertPath = "path/to/ca.crt"; // Thay đổi đường dẫn
        std::string caKeyPath = "path/to/ca.key";   // Thay đổi đường dẫn
        if (!initializeSSL(caCertPath, caKeyPath)) {
            UI_WINDOW::UpdateLog("Khởi tạo SSL thất bại.");
            return false;
        }
        return true;
    }

    // Hàm xử lý phương thức CONNECT để thực hiện MITM
    void handleConnectMethod(SOCKET clientSocket, const std::string& host, int port) {
        // Tạo socket để kết nối tới server
        SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (remoteSocket == INVALID_SOCKET) {
            UI_WINDOW::UpdateLog("Không thể tạo socket tới server.");
            return;
        }

        // Định nghĩa địa chỉ của server
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port); 

        struct hostent* remoteHost = gethostbyname(host.c_str());
        if (remoteHost == NULL) {
            UI_WINDOW::UpdateLog("Không thể giải quyết hostname.");
            closesocket(remoteSocket);
            return;
        }
        memcpy(&serverAddr.sin_addr.s_addr, remoteHost->h_addr, remoteHost->h_length);

        // Kết nối tới server
        if (connect(remoteSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            UI_WINDOW::UpdateLog("Không thể kết nối tới server.");
            closesocket(remoteSocket);
            return;
        }

        // Gửi phản hồi 200 Connection Established cho client
        const char* established = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(clientSocket, established, strlen(established), 0);

        // Khởi tạo đối tượng SSL cho client và server
        SSL* clientSSL = nullptr;
        SSL* serverSSL = nullptr;
        X509* cert = nullptr;

        {
            std::lock_guard<std::mutex> lock(sslMutex);
            // Tạo chứng chỉ cho host
            cert = generateCertificate(host);
            if (!cert) {
                UI_WINDOW::UpdateLog("Không thể tạo chứng chỉ cho host.");
                closesocket(remoteSocket);
                return;
            }

            // Sử dụng chứng chỉ và khóa riêng trong SSL context cho client
            if (SSL_CTX_use_certificate(clientSSLContext, cert) <= 0) {
                UI_WINDOW::UpdateLog("Không thể sử dụng chứng chỉ cho client.");
                X509_free(cert);
                closesocket(remoteSocket);
                return;
            }
            if (SSL_CTX_use_PrivateKey(clientSSLContext, caKey) <= 0) {
                UI_WINDOW::UpdateLog("Không thể sử dụng khóa riêng cho client.");
                X509_free(cert);
                closesocket(remoteSocket);
                return;
            }

            // Tạo SSL object cho client
            clientSSL = SSL_new(clientSSLContext);
            if (!clientSSL) {
                UI_WINDOW::UpdateLog("Không thể tạo SSL object cho client.");
                X509_free(cert);
                closesocket(remoteSocket);
                return;
            }
            SSL_set_fd(clientSSL, clientSocket);
            if (SSL_accept(clientSSL) <= 0) {
                UI_WINDOW::UpdateLog("SSL handshake với client thất bại.");
                SSL_free(clientSSL);
                X509_free(cert);
                closesocket(remoteSocket);
                return;
            }

            // Tạo SSL object cho server
            serverSSL = SSL_new(serverSSLContext);
            if (!serverSSL) {
                UI_WINDOW::UpdateLog("Không thể tạo SSL object cho server.");
                SSL_shutdown(clientSSL);
                SSL_free(clientSSL);
                X509_free(cert);
                closesocket(remoteSocket);
                return;
            }
            SSL_set_fd(serverSSL, remoteSocket);
            if (SSL_connect(serverSSL) <= 0) {
                UI_WINDOW::UpdateLog("SSL handshake với server thất bại.");
                SSL_shutdown(clientSSL);
                SSL_free(clientSSL);
                SSL_free(serverSSL);
                X509_free(cert);
                closesocket(remoteSocket);
                return;
            }

            X509_free(cert); // Giải phóng chứng chỉ sau khi đã sử dụng
        }

        // Vòng lặp chuyển tiếp dữ liệu giữa client và server
        fd_set readfds;
        char buffer[BUFFER_SIZE];
        while (!stopFlags[std::this_thread::get_id()]) {  // Kiểm tra nếu thread cần dừng 
            if (UI_WINDOW::isProxyRunning == false) {
                UI_WINDOW::UpdateLog("Ngắt kết nối: " + host + " || Lý do: Proxy dừng.");
                break;
            }
            FD_ZERO(&readfds);
            FD_SET(clientSocket, &readfds);
            FD_SET(remoteSocket, &readfds);

            struct timeval timeout;
            timeout.tv_sec = 10;
            timeout.tv_usec = 0;
            int selectResult = select(0, &readfds, NULL, NULL, &timeout);
            if (selectResult <= 0) {
                UI_WINDOW::UpdateLog("Ngắt kết nối: " + host + " || Lý do: Hết thời gian chờ.");
                break;
            }

            // Dữ liệu từ client tới server
            if (FD_ISSET(clientSocket, &readfds)) {
                int receivedBytes = SSL_read(clientSSL, buffer, BUFFER_SIZE);
                if (receivedBytes <= 0) break;

                // Có thể kiểm tra hoặc sửa đổi dữ liệu ở đây nếu cần
                int sentBytes = SSL_write(serverSSL, buffer, receivedBytes);
                if (sentBytes <= 0) break;
            }

            // Dữ liệu từ server tới client
            if (FD_ISSET(remoteSocket, &readfds)) {
                int receivedBytes = SSL_read(serverSSL, buffer, BUFFER_SIZE);
                if (receivedBytes <= 0) break;

                // Có thể kiểm tra hoặc sửa đổi dữ liệu ở đây nếu cần
                int sentBytes = SSL_write(clientSSL, buffer, receivedBytes);
                if (sentBytes <= 0) break;
            }
        }

        // Đóng kết nối SSL và socket
        if (clientSSL) {
            SSL_shutdown(clientSSL);
            SSL_free(clientSSL);
        }
        if (serverSSL) {
            SSL_shutdown(serverSSL);
            SSL_free(serverSSL);
        }

        closesocket(remoteSocket);
    }

    void printActiveThreads() {
        // std::lock_guard<std::mutex> lock(threadMapMutex);
        UI_WINDOW::UpdateRunningHosts(threadMap); // Gửi thông tin lên giao diện
    }

    // Function to check active threads and stop the ones with a Blacklisted HOST
    void checkAndStopBlacklistedThreads() {
        std::lock_guard<std::mutex> lock(threadMapMutex); 
        for (auto& [id, host] : threadMap) {
            if (UI_WINDOW::listType == 0) {
                if (Blacklist::isBlocked(host.first)) {
                    stopFlags[id] = true;  // Set flag to true to stop the thread
                }
            } else {
                if (!Whitelist::isAble(host.first)) {
                    stopFlags[id] = true;
                }
            }
        }
    }

    // Hàm xử lý kết nối của client
    void handleClient(SOCKET clientSocket) {
        char buffer[BUFFER_SIZE];
        int receivedBytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (receivedBytes <= 0) {
            closesocket(clientSocket);
            return;
        }

        std::string request(buffer, receivedBytes);
        std::string url = parseHttpRequest(request);
        if (url.empty()) {
            closesocket(clientSocket);
            return;
        }

        size_t hostPos = request.find(' ') + 1;
        size_t colonPos = request.find(':', hostPos);
        size_t spacePos = request.find(' ', colonPos);
        if (colonPos == std::string::npos || spacePos == std::string::npos) {
            closesocket(clientSocket);
            return;
        }

        std::string host = request.substr(hostPos, colonPos - hostPos);
        int port = stoi(request.substr(colonPos + 1, spacePos - colonPos - 1));

        // Kiểm tra blacklist hoặc whitelist
        if (UI_WINDOW::listType == 0) {
            if (Blacklist::isBlocked(host)) {
                UI_WINDOW::UpdateLog("Access to " + host + " is blocked.");
                const char* forbiddenResponse = 
                    "HTTP/1.1 403 Forbidden\r\n"
                    "Connection: close\r\n"
                    "Proxy-Agent: CustomProxy/1.0\r\n"
                    "\r\n";

                send(clientSocket, forbiddenResponse, strlen(forbiddenResponse), 0);
                closesocket(clientSocket);
                return;
            }
        } else {
            if (!Whitelist::isAble(host)) {
                UI_WINDOW::UpdateLog("Access to " + host + " is not allowed.");
                const char* forbiddenResponse = 
                    "HTTP/1.1 403 Forbidden\r\n"
                    "Connection: close\r\n"
                    "Proxy-Agent: CustomProxy/1.0\r\n"
                    "\r\n";

                send(clientSocket, forbiddenResponse, strlen(forbiddenResponse), 0);
                closesocket(clientSocket);
                return;
            }
        }

        // Thêm HOST vào danh sách luồng
        {
            std::lock_guard<std::mutex> lock(threadMapMutex);
            threadMap[std::this_thread::get_id()] = std::make_pair(host, request);
            hostRequestMap[host] = request;
            stopFlags[std::this_thread::get_id()] = false; // Đặt cờ dừng ban đầu là false

            printActiveThreads(); // Hiển thị danh sách luồng
        }

        // Kiểm tra blacklist trong quá trình xử lý client
        checkAndStopBlacklistedThreads();  

        activeThreads++;
        
        UI_WINDOW::UpdateLog("Connecting: " + host);
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

    // Hàm phân tích yêu cầu HTTP để lấy URL
    std::string parseHttpRequest(const std::string& request) {
        size_t pos = request.find("Host: ");
        if (pos == std::string::npos) return std::string();

        size_t start = pos + 6;
        size_t end = request.find("\r\n", start);
        if (end == std::string::npos) return std::string();
        
        return "https://" + request.substr(start, end - start);
    }

    // Hàm khởi tạo và chạy proxy server
    bool runProxyServer(int port) {
        // Khởi tạo Winsock (cho Windows)
        #ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                UI_WINDOW::UpdateLog("WSAStartup failed.");
                return false;
            }
        #endif

        // Tạo socket lắng nghe
        SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            UI_WINDOW::UpdateLog("Không thể tạo listen socket.");
            return false;
        }

        // Thiết lập địa chỉ
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        // Bind socket
        if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            UI_WINDOW::UpdateLog("Bind socket thất bại.");
            closesocket(listenSocket);
            return false;
        }

        // Lắng nghe kết nối
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            UI_WINDOW::UpdateLog("Listen socket thất bại.");
            closesocket(listenSocket);
            return false;
        }

        UI_WINDOW::UpdateLog("Proxy server đang chạy trên cổng " + std::to_string(port));

        // Vòng lặp chấp nhận kết nối
        while (UI_WINDOW::isProxyRunning) {
            sockaddr_in clientAddr;
            #ifdef _WIN32
                int clientAddrSize = sizeof(clientAddr);
            #else
                socklen_t clientAddrSize = sizeof(clientAddr);
            #endif
            SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
            if (clientSocket == INVALID_SOCKET) {
                if (UI_WINDOW::isProxyRunning) {
                    UI_WINDOW::UpdateLog("Accept socket thất bại.");
                }
                break;
            }

            // Tạo luồng mới để xử lý client
            std::thread clientThread(handleClient, clientSocket);
            clientThread.detach();
        }

        // Đóng listen socket
        closesocket(listenSocket);

        // Dọn dẹp Winsock (cho Windows)
        #ifdef _WIN32
            WSACleanup();
        #endif

        return true;
    }
}
