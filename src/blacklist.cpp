#include "blacklist.h"

namespace BlackList {
    std::set<std::string> blacklist;  // Định nghĩa thực sự
    std::mutex blacklistMutex;        // Định nghĩa thực sự

    void add(const std::string& host) {
        std::lock_guard<std::mutex> lock(blacklistMutex); // Bảo vệ danh sách bằng mutex
        std::string clear_host = host;
        while (!clear_host.empty() && clear_host.back() == '/') clear_host.pop_back();
        
        if (clear_host.find("https://") != std::string::npos) {
            clear_host = clear_host.substr(8, clear_host.size() - 8);
        }
        if (clear_host.find("http://") != std::string::npos) {
            clear_host = clear_host.substr(7, clear_host.size() - 7);
        }

        blacklist.insert(clear_host);

        std::ofstream blacklistFile(BLACKLIST_URL, std::ios::app);
        if (blacklistFile.is_open()) {
            blacklistFile << clear_host << '\n';
            blacklistFile.close();
            UI::UpdateLog("Adding " + clear_host + " to blacklist file.");
        } else {
            UI::UpdateLog("[Error] Could not write to blacklist file.");
        }
    }

    bool isBlocked(const std::string& host) {
        std::lock_guard<std::mutex> lock(blacklistMutex); // Bảo vệ danh sách bằng mutex
        return blacklist.find(host) != blacklist.end();
    }

    void remove(const std::string& host) {
        std::lock_guard<std::mutex> lock(blacklistMutex); // Bảo vệ danh sách bằng mutex
        blacklist.erase(host);

        std::ofstream blacklistFile(BLACKLIST_URL, std::ios::out);
        if (blacklistFile.is_open()) {
            for (std::string clear_host : blacklist) {
                blacklistFile << clear_host << '\n';
            }
            blacklistFile.close();
            UI::UpdateLog("Remove " + host + " from blacklist file.");
        } else {
            UI::UpdateLog("[Error] Could not write to blacklist file.");
        }
    }

    void load(const std::string& filename) {
        std::ifstream infile(filename);
        std::string line;
        while (std::getline(infile, line)) {
            blacklist.insert(line);
        }
    }

    void updateListBox(HWND hListBox) {
        std::lock_guard<std::mutex> lock(blacklistMutex); // Bảo vệ danh sách bằng mutex
        SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

        for (const auto& ip : blacklist) {
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)ip.c_str());
        }
    }
}
