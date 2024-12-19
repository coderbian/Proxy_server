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
#include "whitelist.h"

namespace NetworkHandle {
    // Biến toàn cục để lưu thông tin host và yêu cầu tương ứng
    extern std::map<std::string, std::string> hostRequestMap;

    /**
     * @brief Phân tích yêu cầu HTTP để lấy thông tin "Host" từ tiêu đề.
     * @param request Chuỗi chứa nội dung của yêu cầu HTTP.
     * @return Chuỗi URL (ví dụ: "https://example.com"), hoặc chuỗi rỗng nếu không tìm thấy.
     */
    std::string parseHttpRequest(const std::string &request);

    /**
     * @brief Xử lý phương thức CONNECT và thiết lập proxy MITM giữa client và server.
     * @param clientSocket Socket kết nối với client.
     * @param host Tên miền hoặc địa chỉ IP của server đích.
     * @param port Cổng của server đích.
     */
    void handleConnectMethod(SOCKET clientSocket, const std::string& host, int port);

    /**
     * @brief Hiển thị thông tin các luồng đang hoạt động lên giao diện.
     */
    void printActiveThreads();

    /**
     * @brief Xử lý kết nối từ client và chuyển tiếp tới server thông qua proxy.
     * @param clientSocket Socket kết nối với client.
     */
    void handleClient(SOCKET clientSocket);

    /**
     * @brief Kiểm tra và dừng các luồng đang hoạt động nếu host bị chặn (blacklist).
     */
    void checkAndStopBlacklistedThreads();
}

#endif // NETWORK_HANDLE_H
