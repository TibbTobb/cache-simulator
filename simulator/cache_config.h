//
// Created by toby on 06/02/19.
//

#ifndef PROJECT_CACHE_CONFIG_H
#define PROJECT_CACHE_CONFIG_H

#include <string>
struct cache_config {
    std::string name; int associativity{8}; int line_size{64}; int total_size{32768}; std::string parent{""};
};

#endif //PROJECT_CACHE_CONFIG_H
