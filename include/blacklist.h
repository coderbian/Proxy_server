#ifndef BLACKLIST_H
#define BLACKLIST_H

#include <winsock2.h>
#include <string>
#include <fstream>
#include <set>
#include <mutex>

namespace BlackList {
    extern std::set<std::string> blacklist;
    extern std::mutex blacklistMutex;

    void add(const std::string& url);
    bool isBlocked(const std::string& url);
    void remove(const std::string& url);
    void load(const std::string& filename);
    void updateListBox(HWND hListBox);
}

#endif