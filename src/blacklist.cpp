#include "blacklist.h"

namespace BlackList {
    std::set<std::string> blacklist;
    
    void add(const std::string& url) {
        blacklist.insert(url);
    }

    bool isBlocked(const std::string& url) {
        return blacklist.find(url) != blacklist.end();
    }

    void remove(const std::string& url) {
        blacklist.erase(url);
    }

    void load(const std::string& filename) {
        std::ifstream infile(filename);
        std::string line;
        while(std::getline(infile, line)) {
            add(line);
        }
    }
}