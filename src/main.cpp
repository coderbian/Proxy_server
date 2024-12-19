#include "ui.h"

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    const char CLASS_NAME[] = "Proxy Server - fit.hcmus.edu.vn";  // Changed to char
    WNDCLASSA wc = {};  // Changed to WNDCLASSA for ANSI version
    wc.lpfnWndProc = UI::WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassA(&wc);  // Changed to RegisterClassA

    HWND hwnd = CreateWindowA(
        CLASS_NAME, 
        "Proxy Server - fit.hcmus.edu.vn",  // Changed to char
        WS_OVERLAPPEDWINDOW, // Bao gồm hỗ trợ phóng to, thu nhỏ
        CW_USEDEFAULT, CW_USEDEFAULT, 
        800, 600, 
        NULL, NULL,     
        wc.hInstance, 
        NULL
    );

    Blacklist::load(BLACKLIST_URL);
    Whitelist::load(WHITELIST_URL);

    UI::Init(hwnd, wc.hInstance);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}