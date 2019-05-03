//
// Created by toby on 21/11/18.

#include "cache_simulator.h"

using namespace std;

void cache_simulator::run(bool online, string input_name, vector<cache_config> &cache_configs) {
    //initialise
    for(cache_config config : cache_configs) {
        //create a cache for each cache in config
        auto *c = new cache();
        c->init(this, config.associativity, config.line_size, config.total_size,
                config.exclusivity == INCLUSIVE, config.exclusivity == EXCLUSIVITY::EXCLUSIVE);
        cache_map.insert(pair<string, cache*>(config.name, c));
    }

    //on second pass set parents
    for(const cache_config &config : cache_configs) {
        auto child_it = cache_map.find(config.name);
        if(!config.parent.empty()) {
            auto parent_it = cache_map.find(config.parent);
            //TODO: fix if cache has invalid parent will not be used
            if(parent_it != cache_map.end()){
                parent_it->second->add_child(child_it->second);
                child_it->second->set_parent(parent_it->second);
            }
        }
    }


    for(const auto &cache_map_pair : cache_map) {
        //find caches with no children, these are low level and threads can be assigned to them
        if(!cache_map_pair.second->has_child()) {
            cores.push_back(cache_map_pair.second);
        }
    }

    int num_cores = static_cast<int>(cores.size());

    int reads = 0;
    int writes = 0;

    int cores_thread_counts[num_cores];
    for(int i=0; i<num_cores; i++) {
        cores_thread_counts[i] = 0;
    }

    //TODO: combine duplicated code
    if (online) {
        //read mem refs from pipe
        int fd;
        fd = ::open(input_name.c_str(), O_RDONLY);
        mem_ref_t m;
        //read until EOF
        int i = 0;
        while(0 < read(fd, &m, sizeof(mem_ref_t))) {
            //TODO: remove this after testing
            if(i>10000000) break;
            i++;
            if (m.ref_type == 1) {
                writes++;
            } else {
                reads++;
            }
            //printf("Address: 0x%lu, Size: %i, Type: %i TID: %i\n", m.addr, m.size, m.ref_type, m.tid);

            //TODO: THIS IS NOT IN THE FILE READER
            //thread scheduling
            auto thread_it = thread_map.find(m.tid);
            if(thread_it != thread_map.end()) {
                thread_it->second->request(m);
            } else {
                //if new thread schedule to core
                int min = cores_thread_counts[0];
                int min_core = 0;
                for(int j=1; j<num_cores; j++) {
                    int n = cores_thread_counts[j];
                    if(n < min) {
                        min = n;
                        min_core = j;
                    }
                }
                thread_map.insert(pair<int, cache*>(m.tid, cores.at(static_cast<unsigned long>(min_core))));
            }
        }
        //close files
        close(fd);
        unlink(input_name.c_str());
    } else {
        //TODO: fix reading from file
        /*
        //read mem refs from file
        string line;
        ifstream infile;
        infile.open(input_name);
        while (getline(infile, line)) {
            //cout << line << '\n';
            istringstream iss(line);
            uint64_t addr;
            uint read;
            int tid;

            unsigned short size;
            iss >> addr;
            iss >> size;
            iss >> read;
            iss >> tid;
            //mem_ref_t *m = create_mem_ref(addr, read, size);
            if (read == 1) {
                writes++;
            } else {
                reads++;
            }

            mem_ref_t m = {read ? WRITE : READ, size, addr, tid};
            //printf("Address: 0x%lu, Size: %i, Type: %i \n", m.addr, m.size, m.ref_type);
            c->request(m);
        }
        infile.close();
        */
    }
    for(const auto &cache_map_pair : cache_map) {
        string name = cache_map_pair.first;
        cache* c = cache_map_pair.second;
        double avg_reuse_dist = c->get_reuse_dist()/(reads+writes);
        double miss_rate = double(c->get_miss_total()) / double((c->get_miss_total() + c->get_hit_total()));
        printf("%s: Hit total: %i, Miss total: %i, Miss rate: %f, Reads: %i, Writes: %i, reuse dist: %f\n",
               name.c_str(), c->get_hit_total(), c->get_miss_total(), miss_rate, reads, writes, avg_reuse_dist);
    }

}