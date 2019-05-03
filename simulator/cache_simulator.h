//
// Created by toby on 21/11/18.
//

#ifndef PROJECT_CACHE_SIMULATOR_H
#define PROJECT_CACHE_SIMULATOR_H


#include "cache.h"
#include "cache_config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <string>

class cache_simulator {
public:
    void run(bool online, std::string input_name, std::vector<cache_config> &cache_configs);
    map<string, cache*> cache_map;
    //todo: implement levels
    cache** levels;
protected:
    bool online;
    std::string input_name;
    map<int, cache*> thread_map;
    vector<cache*> cores;
};


#endif //PROJECT_CACHE_SIMULATOR_H
