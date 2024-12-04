#include "ui.h"

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    const wchar_t CLASS_NAME[] = L"Proxy Server Window";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = UI::WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        CLASS_NAME, 
        L"Proxy Server UI", 
        WS_OVERLAPPEDWINDOW, // Bao gồm hỗ trợ phóng to, thu nhỏ
        CW_USEDEFAULT, CW_USEDEFAULT, 
        800, 600, 
        NULL, NULL, 
        wc.hInstance, 
        NULL
    );

    BlackList::load(BLACKLIST_URL);
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

