//
// Created by toby on 05/01/19.
//

#include "cache_simulator.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <exception>
#include <fstream>
#include <vector>
#include <algorithm>

void read_config(std::vector<cache_config> &caches) {
    std::ifstream ifs;
    ifs.open("/home/toby/CLionProjects/cache-simulator/simulator/Config.txt", std::ifstream::in);
    std::string str;
    bool start_cache = true;
    cache_config config = cache_config();
    while(!ifs.eof()) {
        while(getline(ifs,str)) {
            std::string::size_type begin = str.find_first_not_of(" \f\t\v");
            //Skips blank lines
            if (begin == std::string::npos)
                continue;
            //Skips #
            if (std::string("#").find(str[begin]) != std::string::npos)
                continue;
            std::string firstWord;
            try {
                firstWord = str.substr(0, str.find(' '));
            }
            catch (std::exception &e) {
                firstWord = str.erase(str.find_first_of(' '), str.find_first_not_of(' '));
            }
            if(start_cache) {
                config.name = firstWord;

                start_cache = false;
            } else {
                std::transform(firstWord.begin(), firstWord.end(), firstWord.begin(), ::toupper);

                if (firstWord == "ASSOCIATIVITY")
                    config.associativity = stoi(str.substr(str.find(' ') + 1, str.length()));
                if (firstWord == "LINESIZE")
                    config.line_size = stoi(str.substr(str.find(' ') + 1, str.length()));
                if (firstWord == "TOTALSIZE")
                    config.total_size = stoi(str.substr(str.find(' ') + 1, str.length()));
                if (firstWord == "PARENT") {
                    config.parent = str.substr(str.find(' ') + 1, str.length());
                }
                if (firstWord == "END") {
                    start_cache = true;
                    caches.push_back(config);
                    config = cache_config();
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    std::vector<cache_config> caches;
    read_config(caches);
    cache_simulator *cache_simulator1 = new cache_simulator();
    cache_simulator1->run(true,"/home/toby/CLionProjects/cache-simulator/tracer/cmake-build-debug/cachesimpipe", caches);
    return 0;
}
