#include "ui.h"

#define ID_LISTBOX_HOSTRUNNING 1001
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
    static HWND hLogBoxLabel;
    static HWND hStatusBar; // Status bar
    static HWND hRequestBox;
    static HWND hRequestBoxLabel;

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
        Font::ApplyFontToControl(hLogBoxLabel);
        Font::ApplyFontToControl(hStatusBar);
        Font::ApplyFontToControl(hRequestBox);
        Font::ApplyFontToControl(hRequestBoxLabel);     
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
        // hRunningHostsBox = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_STANDARD, 450, 100, 300, 200, hwnd, (HMENU)ID_LISTBOX_HOSTRUNNING, hInstance, NULL);
        // Initialize ListView for Running Hosts
        hRunningHostsBox = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
            450, 100, 300, 200, hwnd, (HMENU)ID_LISTBOX_HOSTRUNNING, hInstance, NULL);
        
        SendMessage(hRunningHostsBox, LVM_SETBKCOLOR, 0, (LPARAM)RGB(255, 255, 224));
        SendMessage(hRunningHostsBox, LVM_SETTEXTBKCOLOR, 0, (LPARAM)RGB(255, 255, 224));

        // Define columns for ListView
        LVCOLUMN lvCol = {};
        lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvCol.cx = 150; // Width of each column

        // Add "Host" column
        lvCol.pszText = "Host";
        ListView_InsertColumn(hRunningHostsBox, 0, &lvCol);

        // Add "Method" column
        lvCol.pszText = "Method";
        ListView_InsertColumn(hRunningHostsBox, 1, &lvCol);

        // Add "URI" column
        lvCol.pszText = "URI";
        ListView_InsertColumn(hRunningHostsBox, 2, &lvCol);

        // Add "HTTP Version" column
        lvCol.pszText = "Version";
        ListView_InsertColumn(hRunningHostsBox, 3, &lvCol);

        // Add "Hostname" column
        lvCol.pszText = "Hostname";
        ListView_InsertColumn(hRunningHostsBox, 4, &lvCol);

        // Initialize log box
        hLogBoxLabel = CreateWindowA("STATIC", " Log", WS_CHILD | WS_VISIBLE | WS_BORDER, 450, 20, 300, 25, hwnd, NULL, hInstance, NULL);
        hLogBox = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_STANDARD, 20, 320, 730, 150, hwnd, NULL, hInstance, NULL);

        // Initialize request box
        hRequestBoxLabel = CreateWindowA("STATIC", " Request Information", WS_CHILD | WS_VISIBLE | WS_BORDER, 450, 20, 300, 25, hwnd, NULL, hInstance, NULL);
        hRequestBox = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_WANTRETURN | ES_READONLY, 20, 320, 730, 150, hwnd, NULL, hInstance, NULL);

        //Initialize status bar
        hStatusBar = CreateWindowA("STATIC", "Status: Ready.\t", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 590, 730, 25, hwnd, NULL, hInstance, NULL);

        // Initialize instructions
        std::string instructionsText = "\n"
                                       "    Instructions:\n"
                                       "    1. Add Host to Blacklist (Type in the box and press 'Add to Blacklist').\n"
                                       "    2. Remove Host from Blacklist (Select and press 'Delete from Blacklist').\n"
                                       "    3. Start/Stop the Proxy Server using the button.\n"
                                       "    4. Logs are saved daily in the 'logs' folder.";
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
                        SetWindowTextA(hStatusBar, "Status: Host added to blacklist.\t");
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
                            SetWindowTextA(hStatusBar, "Status: Proxy server stopped.\t");
                            UpdateLog("Proxy server stopped.");
                        } else {
                            // Start the proxy server
                            isProxyRunning = true;
                            proxyThread = std::thread(startProxyServer);
                            SetWindowTextA(hStartProxyBtn, "Stop Proxy");  // Change the button text
                            SetWindowTextA(hStatusBar, "Status: Proxy server running.\t");
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
                        SetWindowTextA(hStatusBar, "Status: Host removed from blacklist.\t");
                        break;
                    }
                }

                if (LOWORD(wParam) == ID_LISTBOX_HOSTRUNNING && HIWORD(wParam) == LVN_ITEMCHANGED) {
                    // Lấy chỉ số của dòng được chọn
                    int index = ListView_GetNextItem(hRunningHostsBox, -1, LVNI_SELECTED);
                    if (index != -1) { // Kiểm tra nếu có dòng nào được chọn
                        char buffer[256];
                        
                        // Lấy giá trị của cột đầu tiên ("Host") từ dòng được chọn
                        ListView_GetItemText(hRunningHostsBox, index, 0, buffer, sizeof(buffer));

                        std::cout << "buffer: " << buffer << '\n';
                        // Giả lập gửi request để lấy thông tin liên quan từ hostRequestMap
                        std::string requestMessage = NetworkHandle::hostRequestMap[buffer];

                        // Hiển thị nội dung request trong `hRequestBox`
                        SetWindowText(hRequestBox, requestMessage.c_str());
                    }
                }

                break;
            }
            case WM_NOTIFY: {
                LPNMHDR nmhdr = (LPNMHDR)lParam;
                if (nmhdr->idFrom == ID_LISTBOX_HOSTRUNNING && nmhdr->code == LVN_ITEMCHANGED) {
                    // Gọi logic xử lý khi một dòng trong ListView được chọn
                    NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
                    if (pnmv->uNewState & LVIS_SELECTED) {
                        int index = pnmv->iItem;
                        char buffer[256];
                        ListView_GetItemText(hRunningHostsBox, index, 0, buffer, sizeof(buffer));
                        std::string requestMessage = NetworkHandle::hostRequestMap[buffer];
                        SetWindowText(hRequestBox, requestMessage.c_str());
                    }
                }
                break;
            }
            case WM_CTLCOLORSTATIC: {
                // Kiểm tra nếu control là một trong các hộp cần thay đổi màu nền
                HWND hControl = (HWND)lParam;
                if (hControl == hBlackListBox || hControl == hRunningHostsBox || hControl == hLogBox || hControl == hRequestBox) {
                    HDC hdc = (HDC)wParam;
                    // Tạo brush với màu nền xám
                    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 224)); // Màu xám
                    SetBkColor(hdc, RGB(255, 255, 224)); // Màu nền xám
                    SetTextColor(hdc, RGB(0, 0, 0)); // Màu chữ trắng

                    // Trả về brush để tô nền
                    return (LRESULT)hBrush;
                }
                break;
            }
            case WM_CTLCOLORLISTBOX: {
                // Nếu là ListBox, áp dụng màu nền xám
                HWND hControl = (HWND)lParam;
                if (hControl == hBlackListBox || hControl == hRunningHostsBox || hControl == hLogBox || hControl == hRequestBox) {
                    HDC hdc = (HDC)wParam;
                    // Tạo brush với màu nền xám
                    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 224)); // Màu xám
                    SetBkColor(hdc, RGB(255, 255, 224)); // Màu nền xám
                    SetTextColor(hdc, RGB(0, 0, 0)); // Màu chữ trắng
                    // Trả về brush để tô nền
                    return (LRESULT)hBrush;
                }
                break;
            }
            case WM_CTLCOLOREDIT: {
                // Nếu là Edit control, áp dụng màu nền xám
                HWND hControl = (HWND)lParam;
                if (hControl == hBlackListBox || hControl == hRunningHostsBox || hControl == hLogBox || hControl == hRequestBox) {
                    HDC hdc = (HDC)wParam;
                    // Tạo brush với màu nền xám
                    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 224)); // Màu xám
                    SetBkColor(hdc, RGB(255, 255, 224)); // Màu nền xám
                    SetTextColor(hdc, RGB(0, 0, 0)); // Màu chữ trắng

                    // Trả về brush để tô nền
                    return (LRESULT)hBrush;
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
                int listBoxHeight = (height - 280) / 2;  // Adjust height for listboxes
                int instructionsHeight = 100;  // Instructions size
                int column2X = width / 2;

                // Ensure both listboxes are aligned and occupy space properly
                int listBoxWidth = column2X - 2 * padding;

                // Position the log box label above the Blacklist box
                int x = padding, y = 20;
                MoveWindow(hBlacklistLabel, x, y, listBoxWidth, buttonHeight, TRUE);
                MoveWindow(hBlackListBox, x, y + buttonHeight, listBoxWidth, listBoxHeight, TRUE);

                // Position the request label above the Running Hosts box
                MoveWindow(hLogBoxLabel, column2X + x, y, width - column2X - 2 * padding, buttonHeight, TRUE);
                MoveWindow(hLogBox, column2X + x, y + buttonHeight, width - column2X - 2 * padding, listBoxHeight, TRUE);

                // Position the black list below the listboxes but above the buttons
                x = padding, y = 20 + buttonHeight + listBoxHeight + padding;
                MoveWindow(hRunningHostsLabel, x, y, listBoxWidth, buttonHeight, TRUE);
                MoveWindow(hRunningHostsBox, x, y + buttonHeight, listBoxWidth, listBoxHeight, TRUE);

                // Position the Running Hosts box below the listboxes but above the buttons
                MoveWindow(hRequestBoxLabel, column2X + x, y, listBoxWidth, buttonHeight, TRUE);
                MoveWindow(hRequestBox, column2X + x, y + buttonHeight, listBoxWidth, listBoxHeight, TRUE);

                // Move the input field and the Add to Blacklist button below the listboxes
                x = padding, y = 20 + buttonHeight + listBoxHeight + padding + buttonHeight + listBoxHeight + padding;
                MoveWindow(hBlackListView, x, y, listBoxWidth - buttonWidth - padding, buttonHeight, TRUE); // Input field
                MoveWindow(hAddBlacklistBtn, x + listBoxWidth - buttonWidth, y, buttonWidth, buttonHeight, TRUE); // Add button next to input field

                // Position Start/Stop proxy buttons
                MoveWindow(hStartProxyBtn, column2X + x, y, buttonWidth, buttonHeight, TRUE);

                // Position the buttons below the listboxes and input section
                MoveWindow(hDeleteBlacklistBtn, x + listBoxWidth - buttonWidth, y + buttonHeight + padding, buttonWidth, buttonHeight, TRUE);

                // Move instructions to the bottom
                MoveWindow(hBlacklistInstructions, padding, height - instructionsHeight - padding, width - 2 * padding, instructionsHeight, TRUE);

                MoveWindow(hStatusBar, padding, height - instructionsHeight - padding - buttonHeight / 2, width - 2 * padding, buttonHeight, TRUE);

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

    // void UpdateRunningHosts(std::map<std::thread::id, std::pair<std::string, std::string>> threadMap) {
    //     SendMessage(hRunningHostsBox, LB_RESETCONTENT, 0, 0); // Xóa nội dung cũ

    //     for (const auto& [id, host] : threadMap) {
    //         SendMessage(hRunningHostsBox, LB_ADDSTRING, 0, (LPARAM)host.first.c_str());
    //     }
    // }

    void UpdateRunningHosts(std::map<std::thread::id, std::pair<std::string, std::string>> threadMap) {
        ListView_DeleteAllItems(hRunningHostsBox); // Clear existing items
        for (const auto& [id, host] : threadMap) {
            std::string request = host.second;
            std::string method, uri, httpVersion, hostName;

            // Parse the request string
            std::istringstream requestStream(request);
            requestStream >> method >> uri >> httpVersion;

            // Find "Host: " in the request to extract hostName
            std::string line;
            while (std::getline(requestStream, line)) {
                if (line.find("Host: ") == 0) {
                    hostName = line.substr(6);
                    break;
                }
            }

            // Insert row into ListView
            LVITEM lvItem = {};
            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = ListView_GetItemCount(hRunningHostsBox); // Row index

            // Add Host column
            lvItem.iSubItem = 0;
            lvItem.pszText = (LPSTR)host.first.c_str();
            ListView_InsertItem(hRunningHostsBox, &lvItem);

            // Add Method column
            ListView_SetItemText(hRunningHostsBox, lvItem.iItem, 1, (LPSTR)method.c_str());

            // Add URI column
            ListView_SetItemText(hRunningHostsBox, lvItem.iItem, 2, (LPSTR)uri.c_str());

            // Add Version column
            ListView_SetItemText(hRunningHostsBox, lvItem.iItem, 3, (LPSTR)httpVersion.c_str());

            // Add Hostname column
            ListView_SetItemText(hRunningHostsBox, lvItem.iItem, 4, (LPSTR)hostName.c_str());
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
