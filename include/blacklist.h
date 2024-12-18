#ifndef BLACKLIST_H
#define BLACKLIST_H

#include <winsock2.h>
#include <string>
#include <fstream>
#include <set>
#include <mutex>

#include "ui.h"
#include "constants.h"

namespace Blacklist {
    extern std::set<std::string> blacklist;
    extern std::mutex blacklistMutex;

    void add(const std::string& host);
    bool isBlocked(const std::string& host);
    void remove(const std::string& host);
    void load(const std::string& filename);
    void updateListBox(HWND hBlackListBox);
}

#endif