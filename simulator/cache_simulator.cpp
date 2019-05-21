#include "cache_simulator.h"
#include "replacement_policy.h"
#include "lru.cpp"

using namespace std;

void cache_simulator::run(bool raw, bool output, const string &output_file, bool online, bool use_coherence,
        string input_name, vector<cache_config> &cache_configs) {
    //initialise
    if(!raw)
        printf("Starting simulator\n");
    //todo: use different replacement policy
    replacement_policy *lru1 = new lru();
    replacement_policy *other = new lru();

    //create a cache for each cache in config
    for(const cache_config &config : cache_configs) {
        auto *c = new cache();
        //default option lru
        replacement_policy *replacementPolicy;
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
            if(c1 != c2) {
                auto child_it = c2->children.find(c1);
                auto parent_it = c1->children.find(c2);
                if (child_it == c2->children.end() ) {
                    if (parent_it == c1->children.end()) {
                    //if c1 and c2 not children of each other then add to coherence set
                    c1->add_to_coherence_set(c2);
                    } else {
                        //c1 not child of c2, and c2 child of c1

                    }
                }
            }
        }
    }

    //find caches with no children, these are low level and threads can be assigned to them
    int i = 0;
    for(const auto &cache_map_pair : cache_map) {
        if(!cache_map_pair.second->has_child()) {
            cores.push_back(cache_map_pair.second);
            cache_map_pair.second->core_num = i;
            cache_map_pair.second->processor_cache = true;
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
            while (0 < read(fd, &m, sizeof(mem_ref_t))) {

                process_memref(m);
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
        }
        infile.close();
    }
    std::ofstream ofs;
    if(output) {
        ofs.open(output_file, ostream::out);
        ofs << "Hit total, Miss total, Reads, Writes, Average reuse dist";
        if(use_coherence) {
            printf("Coherence shares, Coherence invalidates, Coherence E to M, Coherence M to S");
        }
        ofs << std::endl;
    }
    if(!raw) {
        printf("\nSimulation statistics:\n");
    }
    for(const auto &cache_map_pair : cache_map) {
        string name = cache_map_pair.first;
        cache* c = cache_map_pair.second;
        double average_reuse_dist = double(c->get_total_reuse_dist()) / double(c->get_hit_total());
        double miss_rate = double(c->get_miss_total()) / double((c->get_miss_total() + c->get_hit_total()));
        if(raw) {
            std::cout << name.c_str() << " " << c->get_hit_total() << " " <<c->get_miss_total() << " " <<
                     c->reads << " " << c->writes << " " << average_reuse_dist;
            if(use_coherence) {
                std::cout << " " << c->get_coherence_shares() << " " << c->get_coherence_invalidates() << " " <<
                    c->coherence_e_to_m << " " << c->coherence_m_to_s;
            }
            std::cout << std::endl;
        } else {
            printf("\n%s: Hit total: %i, Miss total: %i, Miss rate: %f%%, Reads: %i, Writes: %i, Average reuse dist: %f\n, ",
                   name.c_str(), c->get_hit_total(), c->get_miss_total(), miss_rate * 100, c->reads, c->writes,
                   average_reuse_dist);
            if (use_coherence) {
                printf("Coherence shares: %i, Coherence invalidates: %i, Coherence E to M: %i, Coherence M to S: %i\n",
                       c->get_coherence_shares(), c->get_coherence_invalidates(), c->coherence_e_to_m,
                       c->coherence_m_to_s);
            }
        }
        if(output) {
            ofs << name.c_str() << " " << c->get_hit_total() << " " <<c->get_miss_total() << " " <<
            c->reads << " " << c->writes << " " << average_reuse_dist;
            if(use_coherence) {
                ofs << c->get_coherence_shares() << " " << c->get_coherence_invalidates() << " " <<
                c->coherence_e_to_m << " " << c->coherence_m_to_s;
            }
            ofs << std::endl;
        }
    }
    if(output) {
        ofs.close();
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