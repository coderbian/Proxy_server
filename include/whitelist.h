#ifndef WHITELIST_H
#define WHITELIST_H

#include <winsock2.h>
#include <string>
#include <fstream>
#include <set>
#include <mutex>

#include "ui.h"
#include "constants.h"

namespace Whitelist {
    extern std::set<std::string> whitelist;
    extern std::mutex whitelistMutex;

    void add(const std::string& host);
    bool isAble(const std::string& host);
    void remove(const std::string& host);
    void load(const std::string& filename);
    void updateListBox(HWND hWhitelistBox);
}

#endif