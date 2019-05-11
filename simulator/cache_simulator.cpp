//
// Created by toby on 21/11/18.

#include "cache_simulator.h"
#include "replacement_policy.h"
#include "lru.cpp"
/*
#include "drCacheSim_reader/trace_entry.h"
#include "drCacheSim_reader/drCacheSim_reader.cpp"
*/
using namespace std;

void cache_simulator::run(bool online, bool use_coherence, bool caculate_reuse_dist_,
        string input_name, vector<cache_config> &cache_configs) {
    //initialise
    //translate_from_dr_cache_sim = false;

    //todo: use different replacement policy
    replacement_policy *lru1 = new lru();
    replacement_policy *other = new lru();

    caculate_reuse_dist = caculate_reuse_dist_;

    //create a cache for each cache in config
    for(const cache_config &config : cache_configs) {
        auto *c = new cache();
        //default option lru
        replacement_policy *replacementPolicy = lru1;
        if(config.replacement_policy == "LRU") {
            replacementPolicy = lru1;
        } else if(config.replacement_policy == "NEED_to_create") {
            replacementPolicy = other;
        } else {
            cerr << "Cache " << config.name << " has invalid replacement policy";
            return;
        }
        c->init(use_coherence, config.associativity, config.line_size, config.total_size,
                config.exclusivity == INCLUSIVE, config.exclusivity == EXCLUSIVITY::EXCLUSIVE, replacementPolicy);
        cache_map.insert(pair<string, cache*>(config.name, c));
    }

    //on second pass set parents
    for(const cache_config &config : cache_configs) {
        auto child_it = cache_map.find(config.name);
        if(!config.parent.empty()) {
            auto parent_it = cache_map.find(config.parent);
            if(parent_it != cache_map.end()){
                parent_it->second->add_child(child_it->second);
                child_it->second->set_parent(parent_it->second);
            } else {
                cerr << "Cache " << config.name << " has invalid parent";
                return;
            }
        }
    }

    //initialise parent and coherence_set
    //note this could be made to o(n) instead of o(n^2) using a tree walk
    for(const auto &cache_map_pair : cache_map) {
        cache *c1 = cache_map_pair.second;
        for(const auto &cache_map_pair2 : cache_map) {
            cache *c2 = cache_map_pair2.second;
            auto child_it = c2->children.find(c1);
            if(child_it == c2->children.end()) {
                //if c1 not a child of c2 then c2 not in parent of c1 and thus add to c1 coherence_set
                c1->add_to_coherence_set(c2);
            } else {
                //otherwise c1 is a child of c2 and thus c2 is a parent of c1
                c1->add_parent(c2);
            }
        }
    }

    //find caches with no children, these are low level and threads can be assigned to them
    int i = 0;
    for(const auto &cache_map_pair : cache_map) {
        if(!cache_map_pair.second->has_child()) {
            cores.push_back(cache_map_pair.second);
            cache_map_pair.second->core_num = i;
            i++;
        }
    }

    num_cores = static_cast<int>(cores.size());

    cores_thread_counts = new int[num_cores];
    for(int i=0; i<num_cores; i++) {
        cores_thread_counts[i] = 0;
    }

    if (online) {
        //read mem refs from pipe
        int fd;
        fd = ::open(input_name.c_str(), O_RDONLY);
        mem_ref_t m;
        //read until EOF
        int i = 0;

        //if(translate_from_dr_cache_sim) {
            /*
            drCacheSim_reader reader;
            trace_entry_t t;
            while(0 < read(fd, &t, sizeof(trace_entry_t))) {
                m = reader.read(t);
                if(m.ref_type != SKIP) {
                    process_memref(m);
                }
                close(fd);
            }
             */
        //} else {
            while (0 < read(fd, &m, sizeof(mem_ref_t))) {
                //TODO: remove this after testing
                //if(i>10000000) break;
                //i++;
                process_memref(m);
                //printf("Address: 0x%lu, Size: %i, Type: %i TID: %i\n", m.addr, m.size, m.ref_type, m.tid);
            }
            //close files
            close(fd);
            unlink(input_name.c_str());
        //}
    } else {
        //read mem refs from file
        string line;
        ifstream infile;
        infile.open(input_name);
        int i = 0;
        while (getline(infile, line)) {
            //TODO: remove this after testing
            //if(i>10000000) break;
            //i++;
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

            mem_ref_t m = {REF_TYPE(read), size, addr, tid};
            process_memref(m);
            //printf("Address: 0x%lu, Size: %i, Type: %i \n", m.addr, m.size, m.ref_type);
        }
        infile.close();
    }

    for(const auto &cache_map_pair : cache_map) {
        string name = cache_map_pair.first;
        cache* c = cache_map_pair.second;
        double average_reuse_dist = double(c->get_total_reuse_dist()) / double(c->get_hit_total());
        double miss_rate = double(c->get_miss_total()) / double((c->get_miss_total() + c->get_hit_total()));
        printf("%s: Hit total: %i, Miss total: %i, Miss rate: %f, Reads: %i, Writes: %i, Average reuse dist: %f\n",
               name.c_str(), c->get_hit_total(), c->get_miss_total(), miss_rate, c->reads, c->writes, average_reuse_dist);
    }
    if(caculate_reuse_dist) {
        reuseDist.print_statistics();
    }
}

void cache_simulator::process_memref(mem_ref_t m) {
    memref_count++;
    if(m.ref_type == THREAD_INIT) {
        schedule_thread(m.tid);
    } else if(m.ref_type == THREAD_EXIT) {
        exit_thread(m.tid);
    } else {
        auto thread_it = thread_map.find(m.tid);
        if(thread_it != thread_map.end()) {
            thread_it->second->request(m);
            if(caculate_reuse_dist) {
                reuseDist.process_memref(m);
            }
        } else {
            cerr << "error thread not initialised\n";
        }
    }
}

void cache_simulator::schedule_thread(int tid){
    //thread scheduling
    //printf("new thread: %i\n", tid);
    int min = cores_thread_counts[0];
    int min_core = 0;
    for(int j=1; j<num_cores; j++) {
        int n = cores_thread_counts[j];
        if(n < min) {
            min = n;
            min_core = j;
        }
    }
    cache *core = cores.at(static_cast<unsigned long>(min_core));
    thread_map.insert(pair<int, cache*>(tid, core));
    cores_thread_counts[min_core]++;
}

void cache_simulator::exit_thread(int tid) {
    //printf("exit thread: %i\n", tid);
    auto thread_it = thread_map.find(tid);
    if(thread_it != thread_map.end()) {
        cores_thread_counts[thread_it->second->core_num]--;
        thread_map.erase(tid);
    } else {
        cerr << "Error thread not initialised";
    }
}