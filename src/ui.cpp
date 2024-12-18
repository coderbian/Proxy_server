#include "ui.h"

namespace UI {
    // Global variables for UI elements
    static HWND hBlackListView;
    static HWND hAddBlacklistBtn;
    static HWND hStartProxyBtn;
    static HWND hBlackListBox;
    static HWND hDeleteBlacklistBtn;
    static HWND hBlacklistInstructions;
    static HWND hRunningHostsBox;       // Hộp hiển thị danh sách host
    static HWND hBlacklistLabel;        
    static HWND hRunningHostsLabel;     
    static HWND hLogBox;  // Log box
    static HWND hStatusBar; // Status bar
    static HWND hLogBoxLable;

    // Global variable for controlling the proxy server
    std::atomic<bool> isProxyRunning(false);

    // Internal function to start the proxy server
    static void startProxyServer() {
        SOCKET listenSocket = NetworkInit::startInitSocket();

        while (isProxyRunning) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(listenSocket, &readfds);

            struct timeval timeout = {1, 0}; // Timeout 1 giây
            int activity = select(0, &readfds, NULL, NULL, &timeout);

            if (activity > 0 && FD_ISSET(listenSocket, &readfds)) {
                SOCKET clientSocket = accept(listenSocket, NULL, NULL);
                if (clientSocket != INVALID_SOCKET) {
                    std::thread clientThread(NetworkHandle::handleClient, clientSocket);
                    clientThread.detach();
                }
            }
        }

        closesocket(listenSocket);
        WSACleanup();
    }

    // Set font for all UI elements
    void SetFontForControls(HWND hwnd) {
        Font::ApplyFontToControl(hBlackListView);
        Font::ApplyFontToControl(hAddBlacklistBtn);
        Font::ApplyFontToControl(hStartProxyBtn);
        Font::ApplyFontToControl(hBlackListBox);
        Font::ApplyFontToControl(hDeleteBlacklistBtn);
        Font::ApplyFontToControl(hBlacklistInstructions);
        Font::ApplyFontToControl(hRunningHostsBox);
        Font::ApplyFontToControl(hBlacklistLabel);
        Font::ApplyFontToControl(hRunningHostsLabel);
        Font::ApplyFontToControl(hLogBox);
        Font::ApplyFontToControl(hLogBoxLable);
        Font::ApplyFontToControl(hStatusBar);
    }

    // Initialize UI elements
    void Init(HWND hwnd, HINSTANCE hInstance) {
        // Initialize labels
        hBlacklistLabel = CreateWindowA("STATIC", " Blacklist", WS_CHILD | WS_VISIBLE | WS_BORDER, 20, 20, 250, 25, hwnd, NULL, hInstance, NULL);
        hRunningHostsLabel = CreateWindowA("STATIC", " Host Running", WS_CHILD | WS_VISIBLE | WS_BORDER, 450, 20, 300, 25, hwnd, NULL, hInstance, NULL);

        // Initialize input field and buttons
        hBlackListView = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 20, 60, 250, 25, hwnd, NULL, hInstance, NULL);
        hAddBlacklistBtn = CreateWindowA("BUTTON", "Add to Blacklist", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 60, 150, 30, hwnd, (HMENU)1, hInstance, NULL);
        hDeleteBlacklistBtn = CreateWindowA("BUTTON", "Delete from Blacklist", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 450, 60, 150, 30, hwnd, (HMENU)3, hInstance, NULL);
        hStartProxyBtn = CreateWindowA("BUTTON", "Start Proxy", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 620, 60, 150, 30, hwnd, (HMENU)2, hInstance, NULL);

        // Initialize listboxes
        hBlackListBox = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_STANDARD | LBS_MULTIPLESEL, 20, 100, 400, 200, hwnd, NULL, hInstance, NULL);
        hRunningHostsBox = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_STANDARD, 450, 100, 300, 200, hwnd, NULL, hInstance, NULL);

        // Initialize log box
        hLogBoxLable = CreateWindowA("STATIC", " Log", WS_CHILD | WS_VISIBLE | WS_BORDER, 450, 20, 300, 25, hwnd, NULL, hInstance, NULL);
        hLogBox = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_STANDARD, 20, 320, 730, 150, hwnd, NULL, hInstance, NULL);

        //Initialize status bar
        hStatusBar = CreateWindowA("STATIC", "Status: Ready", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 590, 730, 25, hwnd, NULL, hInstance, NULL);

        // Initialize instructions
        std::string instructionsText = "Instructions:\n"
                                       "1. Add Host to Blacklist (Type in the box and press 'Add to Blacklist').\n"
                                       "2. Remove Host from Blacklist (Select and press 'Delete from Blacklist').\n"
                                       "3. Start/Stop the Proxy Server using the button.\n"
                                       "4. Logs are saved daily in the 'logs' folder.";
        hBlacklistInstructions = CreateWindowA("STATIC", instructionsText.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 480, 730, 100, hwnd, NULL, hInstance, NULL);

        SetFontForControls(hwnd);
        BlackList::updateListBox(hBlackListBox);
    }

    // Handle UI events
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        static std::thread proxyThread;

        switch (uMsg) {
            case WM_COMMAND: {
                switch (LOWORD(wParam)) {
                    case 1: { // Add to blacklist
                        char buffer[256];
                        GetWindowText(hBlackListView, buffer, 256);
                        
                        BlackList::add(buffer);
                        BlackList::updateListBox(hBlackListBox);
                        SetWindowTextA(hStatusBar, "Status: Host added to blacklist.");
                        break;
                    }
                    case 2: { // Toggle Proxy server (Start/Stop)
                        if (isProxyRunning) {
                            // Stop the proxy server
                            isProxyRunning = false;
                            if (proxyThread.joinable()) {
                                proxyThread.join();
                            }
                            SetWindowTextA(hStartProxyBtn, "Start Proxy");  // Change the button text
                            SetWindowTextA(hStatusBar, "Status: Proxy server stopped.");
                        } else {
                            // Start the proxy server
                            isProxyRunning = true;
                            proxyThread = std::thread(startProxyServer);
                            SetWindowTextA(hStartProxyBtn, "Stop Proxy");  // Change the button text
                            SetWindowTextA(hStatusBar, "Status: Proxy server running.");
                        }
                        break;
                    }
                    case 3: { // Delete from blacklist
                        int count = SendMessage(hBlackListBox, LB_GETCOUNT, 0, 0);
                        for (int i = 0; i < count; ++i) {
                            if (SendMessage(hBlackListBox, LB_GETSEL, i, 0) == TRUE) {
                                char buffer[256];
                                SendMessage(hBlackListBox, LB_GETTEXT, i, (LPARAM)buffer);
                                BlackList::remove(buffer);
                            }
                        }
                        BlackList::updateListBox(hBlackListBox);
                        SetWindowTextA(hStatusBar, "Status: Host removed from blacklist.");
                        break;
                    }
                }
                break;
            }
            case WM_SIZE: {
                int width = LOWORD(lParam);  // Get window width
                int height = HIWORD(lParam); // Get window height

                // Sizes and paddings
                int padding = 10;
                int buttonWidth = 150;
                int buttonHeight = 25;
                int listBoxHeight = height - 280;  // Adjust height for listboxes
                int instructionsHeight = 100;  // Instructions size
                int column2X = width / 2;

                // Ensure both listboxes are aligned and occupy space properly
                int listBoxWidth = column2X - 2 * padding;

                // Position the Blacklist label above the Blacklist box
                MoveWindow(hBlacklistLabel, padding, 20, listBoxWidth, buttonHeight, TRUE);

                // Position the Running Hosts label above the Running Hosts box
                MoveWindow(hRunningHostsLabel, column2X + padding, 20, width - column2X - 2 * padding, buttonHeight, TRUE);

                // Position the Running Hosts box at the top right
                MoveWindow(hRunningHostsBox, column2X + padding, 20 + buttonHeight, width - column2X - 2 * padding, listBoxHeight / 2, TRUE);

                // Position the Blacklist Box below the Running Hosts box and align it properly
                MoveWindow(hBlackListBox, padding, 20 + buttonHeight, listBoxWidth, listBoxHeight / 2, TRUE);

                // Position the log box below the listboxes but above the buttons
                MoveWindow(hLogBoxLable, padding, 20 + buttonHeight + listBoxHeight / 2, width - 2 * padding, buttonHeight, TRUE);
                MoveWindow(hLogBox, padding, 20 + buttonHeight + listBoxHeight / 2 + buttonHeight, width - 2 * padding, listBoxHeight / 2, TRUE);

                // Move the input field and the Add to Blacklist button below the listboxes
                MoveWindow(hBlackListView, padding, 20 + buttonHeight + listBoxHeight + buttonHeight, listBoxWidth - buttonWidth - 10, 25, TRUE); // Input field
                MoveWindow(hAddBlacklistBtn, column2X - buttonWidth - padding, 20 + buttonHeight + listBoxHeight + buttonHeight, buttonWidth, buttonHeight, TRUE); // Add button next to input field

                // Position the buttons below the listboxes and input section
                MoveWindow(hDeleteBlacklistBtn, column2X - buttonWidth - padding, 20 + buttonHeight + listBoxHeight + 2 * buttonHeight, buttonWidth, buttonHeight, TRUE);

                // Position Start/Stop proxy buttons
                MoveWindow(hStartProxyBtn, column2X + padding, 20 + buttonHeight + listBoxHeight + buttonHeight, buttonWidth, buttonHeight, TRUE);

                MoveWindow(hStatusBar, padding, height - buttonHeight - instructionsHeight, width - 2 * padding, buttonHeight, TRUE);

                // Move instructions to the bottom
                MoveWindow(hBlacklistInstructions, padding, height - instructionsHeight, width - 2 * padding, instructionsHeight, TRUE);

                break;
            }
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);

                // Tô nền màu trắng
                RECT rect;
                GetClientRect(hwnd, &rect);
                HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
                FillRect(hdc, &rect, brush);
                DeleteObject(brush);

                EndPaint(hwnd, &ps);
                break;
            }
            case WM_DESTROY: {
                isProxyRunning = false;
                UpdateLog("Exit proxy server ...");
                PostQuitMessage(0);
                break;
            }
            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    void UpdateRunningHosts(std::map<std::thread::id, std::string> threadMap) {
        SendMessage(hRunningHostsBox, LB_RESETCONTENT, 0, 0); // Xóa nội dung cũ

        for (const auto& [id, host] : threadMap) {
            SendMessage(hRunningHostsBox, LB_ADDSTRING, 0, (LPARAM)host.c_str());
        }
    }

    void UpdateLog(const std::string& str) {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&now_time_t);

        // Format time as HH:MM:SS
        std::ostringstream timeStream;
        timeStream << std::put_time(&tm, "%H:%M:%S");

        // Create the final log message with time prefix
        std::string logMessage = "[" + timeStream.str() + "] " + str;

        // Add the log message to the listbox
        SendMessage(hLogBox, LB_ADDSTRING, 0, (LPARAM)logMessage.c_str());

        // Automatically scroll to the bottom of the listbox
        int count = SendMessage(hLogBox, LB_GETCOUNT, 0, 0);
        SendMessage(hLogBox, LB_SETTOPINDEX, count - 1, 0);

        // Prepare log file name: log_dd_mm_yy.txt
        std::ostringstream fileNameStream;
        fileNameStream << "log_"
                    << std::put_time(&tm, "%d_%m_%y") 
                    << ".txt";
        std::string logFileName = fileNameStream.str();

        // Folder to store logs (adjust as needed)
        std::string logFolder = "logs/";

        // Full path to log file
        std::string logFilePath = logFolder + logFileName;

        // Write the log message to the file
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << logMessage << std::endl;
            logFile.close();
        } else {
            // Optionally handle errors when opening the file
            SendMessage(hLogBox, LB_ADDSTRING, 0, (LPARAM)"[Error] Could not write to log file.");
        }
    }
}
