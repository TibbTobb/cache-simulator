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
#include <string>
#include <map>

class cache_simulator {
public:
    void run(bool raw, bool output, const std::string &output_file, bool online, bool use_coherence,
        std::string input_name, std::vector<cache_config> &cache_configs);
    std::map<std::string, cache*> cache_map;
protected:
    void process_memref(mem_ref_t m);
    void schedule_thread(int tid);
    void exit_thread(int tid);
    std::map<int, cache*> thread_map;
    std::vector<cache*> cores;
    int num_cores;
    int *cores_thread_counts;
    int memref_count = 0;
};


#endif //PROJECT_CACHE_SIMULATOR_H
