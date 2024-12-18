#ifndef UI_H
#define UI_H

#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <commctrl.h>
#include <string>
#include <iostream>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <commctrl.h>

#include "font.h"
#include "blacklist.h"
#include "network_init.h"
#include "network_handle.h"

namespace UI {
    extern std::atomic<bool> isProxyRunning; // Biến điều khiển trạng thái proxy

    // Khai báo các hàm liên quan đến giao diện
    void Init(HWND hwnd, HINSTANCE hInstance);
    void SetFontForControls(HWND hwnd);
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void UpdateRunningHosts(std::map<std::thread::id, std::pair<std::string, std::string>> threadMap);
    void UpdateLog(const std::string& str);
    void ShowRequestInformation(std::string request);
}

#endif
