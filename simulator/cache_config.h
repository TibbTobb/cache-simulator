#ifndef PROJECT_CACHE_CONFIG_H
#define PROJECT_CACHE_CONFIG_H

#include <string>
enum EXCLUSIVITY {
    EXCLUSIVE,
    INCLUSIVE,
    NINE,
};

struct cache_config {
    std::string name; int associativity{8}; int line_size{64}; int total_size{32768};
    std::string parent{""}; EXCLUSIVITY exclusivity{NINE};
    std::string replacement_policy{"LRU"};
};

#endif //PROJECT_CACHE_CONFIG_H
