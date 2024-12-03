#include "blacklist.h"

namespace BlackList {
    std::set<std::string> blacklist;  // Định nghĩa thực sự
    std::mutex blacklistMutex;        // Định nghĩa thực sự

    void add(const std::string& url) {
        std::lock_guard<std::mutex> lock(blacklistMutex); // Bảo vệ danh sách bằng mutex
        std::string clear_url = url;
        while (!clear_url.empty() && clear_url.back() == '/') clear_url.pop_back();
        
        if (clear_url.find("https://") != std::string::npos) {
            clear_url = clear_url.substr(8, clear_url.size() - 8);
        }
        if (clear_url.find("http://") != std::string::npos) {
            clear_url = clear_url.substr(7, clear_url.size() - 7);
        }

        blacklist.insert(clear_url);
    }

    bool isBlocked(const std::string& url) {
        std::lock_guard<std::mutex> lock(blacklistMutex); // Bảo vệ danh sách bằng mutex
        return blacklist.find(url) != blacklist.end();
    }

    void remove(const std::string& url) {
        std::lock_guard<std::mutex> lock(blacklistMutex); // Bảo vệ danh sách bằng mutex
        blacklist.erase(url);
    }

    void load(const std::string& filename) {
        std::ifstream infile(filename);
        std::string line;
        while (std::getline(infile, line)) { 
            add(line);
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
