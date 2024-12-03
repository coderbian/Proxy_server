#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <atomic>
#include <commctrl.h>  // Để sử dụng CheckBox

#include "blacklist.h"
#include "network_init.h"
#include "network_handle.h"

// Global variable for stopping the proxy server
std::atomic<bool> isProxyRunning(false);

SOCKET listenSocket = INVALID_SOCKET;

// Proxy server logic to be run in a separate thread
void startProxyServer() {
    listenSocket = NetworkInit::startInitSocket();

    while (isProxyRunning) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            std::thread clientThread(NetworkHandle::handleClient, clientSocket);
            clientThread.detach(); 
        }
    }

    closesocket(listenSocket);
    listenSocket = INVALID_SOCKET;
    WSACleanup();
}

// Global variables for the UI elements
HWND hBlackListView, hAddBlacklistBtn, hStartProxyBtn, hViewBlacklistBtn, hBlackListBox, hStopProxyBtn, hDeleteBlacklistBtn;
HWND hBlacklistInstructions; // New control for instructions

// Global variable for font
HFONT hFont;

// Khởi tạo phông chữ
void CreateCustomFont(HINSTANCE hInstance) {
    hFont = CreateFontA( // Hoặc CreateFontW nếu dùng Unicode
        16,                  // Kích thước phông chữ
        0,                   // Chiều rộng chữ (0 cho tự động)
        0,                   // Góc xoay
        0,                   // Góc xoay
        FW_NORMAL,           // Độ dày phông chữ (bình thường)
        FALSE,               // Không in đậm
        FALSE,               // Không in nghiêng
        FALSE,               // Không gạch dưới
        ANSI_CHARSET,        // Mã ký tự (ANSI)
        OUT_DEFAULT_PRECIS,  // Kiểu bố cục
        CLIP_DEFAULT_PRECIS, // Kiểu cắt
        DEFAULT_QUALITY,     // Chất lượng
        DEFAULT_PITCH,       // Định dạng
        "Segoe UI"           // Tên phông chữ
    );
}

// Cập nhật phông chữ cho các điều khiển
void SetFontForControls(HWND hwnd) {
    SendMessage(hAddBlacklistBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hViewBlacklistBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hStartProxyBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hStopProxyBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hDeleteBlacklistBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBlacklistInstructions, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// Function to handle button clicks in the UI
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static std::thread proxyThread;

    switch (uMsg) {
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) {  // Add to blacklist
                char buffer[256];
                GetWindowText(hBlackListView, buffer, 256);
                BlackList::add(buffer);
                BlackList::updateListBox(hBlackListBox);
            }
            else if (LOWORD(wParam) == 2) {  // View blacklist
                BlackList::updateListBox(hBlackListBox);
            }
            else if (LOWORD(wParam) == 3) {  // Start proxy
                if (!isProxyRunning) {
                    isProxyRunning = true;
                    proxyThread = std::thread(startProxyServer); // Start proxy in a separate thread
                }
            }
            else if (LOWORD(wParam) == 4) {  // Stop proxy
                if (isProxyRunning) {
                    isProxyRunning = false; // Stop the proxy server
                    if (listenSocket != INVALID_SOCKET) {
                        closesocket(listenSocket); // Close the listening socket
                    }
                    if (proxyThread.joinable()) {
                        proxyThread.join(); // Wait for the thread to finish
                    }
                }
            }
            else if (LOWORD(wParam) == 5) {  // Delete from blacklist
                int count = SendMessage(hBlackListBox, LB_GETCOUNT, 0, 0);
                for (int i = 0; i < count; i++) {
                    if (SendMessage(hBlackListBox, LB_GETSEL, i, 0) == TRUE) { // Check if item is selected
                        char buffer[256];
                        SendMessage(hBlackListBox, LB_GETTEXT, i, (LPARAM)buffer);
                        BlackList::remove(buffer);  // Remove URL from blacklist
                    }
                }
                BlackList::updateListBox(hBlackListBox);
            }
            break;
        }
        case WM_DESTROY: {
            // Stop the proxy server when the window is closed
            isProxyRunning = false;
            if (listenSocket != INVALID_SOCKET) {
                closesocket(listenSocket);
            }
            if (proxyThread.joinable()) {
                proxyThread.join(); // Wait for the proxy thread to finish
            }
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int main() {
    // Initialize the Winsock library
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Load blacklist from the URL
    BlackList::load(BLACKLIST_URL);

    // Register the window class
    const wchar_t CLASS_NAME[] = L"Proxy Server Window";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    // Create the window
    HWND hwnd = CreateWindowW(CLASS_NAME, L"Proxy Server UI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 450, NULL, NULL, wc.hInstance, NULL);

    // Create UI elements
    hBlackListView      = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 20, 20, 250, 25, hwnd, NULL, wc.hInstance, NULL);
    hAddBlacklistBtn    = CreateWindowW(L"BUTTON", L"Add to Blacklist", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 60, 150, 30, hwnd, (HMENU)1, wc.hInstance, NULL);
    hViewBlacklistBtn   = CreateWindowW(L"BUTTON", L"View Blacklist", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 150, 30, hwnd, (HMENU)2, wc.hInstance, NULL);
    hStartProxyBtn      = CreateWindowW(L"BUTTON", L"Start Proxy", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 140, 150, 30, hwnd, (HMENU)3, wc.hInstance, NULL);
    hStopProxyBtn       = CreateWindowW(L"BUTTON", L"Stop Proxy", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 180, 140, 150, 30, hwnd, (HMENU)4, wc.hInstance, NULL);
    hDeleteBlacklistBtn = CreateWindowW(L"BUTTON", L"Delete from Blacklist", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 180, 60, 150, 30, hwnd, (HMENU)5, wc.hInstance, NULL);

    // Create a ListBox to display the blacklist
    hBlackListBox = CreateWindowW(
        L"LISTBOX", 
        L"", 
        WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_STANDARD | LBS_MULTIPLESEL, 
        20, 180, 400, 150, hwnd, NULL, wc.hInstance, NULL
    );

    // Create instructions
    std::wstring instructionsText = L"Hướng dẫn:\n"
                                    L"1. Thêm URL vào danh sách đen\n"
                                    L"2. Xem danh sách đen\n"
                                    L"3. Bật/Tắt máy chủ Proxy";
    hBlacklistInstructions = CreateWindowW(
        L"STATIC", 
        instructionsText.c_str(), 
        WS_CHILD | WS_VISIBLE | SS_LEFT, 
        20, 340, 540, 100, hwnd, NULL, wc.hInstance, NULL
    );

    // Apply custom font
    CreateCustomFont(wc.hInstance);
    SetFontForControls(hwnd);

    // Show the window
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    // Run the message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
