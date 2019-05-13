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

//allow use of K and M symbols for kilobytes and megabytes
int convert_to_int(std::string string) {
    char last = string.at(string.length() - 1);
    if (last == 'K') {
        std::string s = string.substr(0, string.length() - 1);
        return 1024 * stoi(s);
    }
    if (last == 'M') {
        return 1048576 * stoi(string.substr(0, string.length() - 1));
    }
    return stoi(string);

}

bool read_config(const std::string &config_file, std::vector<cache_config> &caches) {
    std::ifstream ifs;
    ifs.open(config_file, std::ifstream::in);
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
                else if (firstWord == "LINESIZE")
                    config.line_size = convert_to_int(str.substr(str.find(' ') + 1, str.length()));
                else if (firstWord == "TOTALSIZE")
                    config.total_size = convert_to_int(str.substr(str.find(' ') + 1, str.length()));
                else if (firstWord == "PARENT")
                    config.parent = str.substr(str.find(' ') + 1, str.length());
                else if(firstWord == "INCLUSIVE")
                    config.exclusivity = INCLUSIVE;
                else if(firstWord == "EXCLUSIVE")
                    config.exclusivity = EXCLUSIVE;
                else if(firstWord == "NINE")
                    config.exclusivity = NINE;
                else if(firstWord == "REPLACEMENT") {
                    config.replacement_policy = str.substr(str.find(' ') + 1, str.length());
                }
                else if (firstWord == "END") {
                    start_cache = true;
                    caches.push_back(config);
                    config = cache_config();
                } else {
                    std::cerr << "Invalid line in config: " << str << "\n";
                    return false;
                }
            }
        }
    }
    return true;
}


int main(int argc, char *argv[]) {
    printf("Starting simulator\n");
    bool online = false;
    bool cache_coherence = false;
    bool caculate_reuse_dist = false;
    for(int i=1; i < argc-2; i++) {
        if(std::string(argv[i]) == "-online") {
            online = true;
        } else if(std::string(argv[i]) == "-coherence") {
            cache_coherence = true;
        } else if(std::string(argv[i]) == "-reuse_dist") {
            caculate_reuse_dist = true;
        }
    }

    std::vector<cache_config> caches;
    if(!read_config(argv[argc-2], caches)) {
        return 1;
    }
    auto *cache_simulator1 = new cache_simulator();
    //cache_simulator1->run(true,"/home/toby/CLionProjects/cache-simulator/tracer/cmake-build-debug/cachesimpipe", caches);
    cache_simulator1->run(online, cache_coherence, caculate_reuse_dist, argv[argc-1], caches);
    return 0;
}
