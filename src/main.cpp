#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <atomic>

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
HWND hBlackListView, hAddBlacklistBtn, hStartProxyBtn, hViewBlacklistBtn, hBlackListBox, hStopProxyBtn;

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
    const char CLASS_NAME[] = "ProxyServerWindow";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Proxy Server UI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400, NULL, NULL, wc.hInstance, NULL);
    
    // Create UI elements
    hBlackListView    = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 20, 20, 250, 25, hwnd, NULL, wc.hInstance, NULL);
    hAddBlacklistBtn  = CreateWindow("BUTTON", "Add to Blacklist", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 60, 150, 30, hwnd, (HMENU)1, wc.hInstance, NULL);
    hViewBlacklistBtn = CreateWindow("BUTTON", "View Blacklist", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 150, 30, hwnd, (HMENU)2, wc.hInstance, NULL);
    hStartProxyBtn    = CreateWindow("BUTTON", "Start Proxy", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 140, 150, 30, hwnd, (HMENU)3, wc.hInstance, NULL);
    hStopProxyBtn     = CreateWindow("BUTTON", "Stop Proxy", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 180, 140, 150, 30, hwnd, (HMENU)4, wc.hInstance, NULL);

    // Create a ListBox to display the blacklist
    hBlackListBox     = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_STANDARD, 20, 180, 400, 150, hwnd, NULL, wc.hInstance, NULL);

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
