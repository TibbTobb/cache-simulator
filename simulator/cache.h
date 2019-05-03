//
// Created by toby on 21/11/18.
//

#ifndef PROJECT_CACHE_H
#define PROJECT_CACHE_H

#include "../common/memref.h"
#include "cache_line.h"
#include "cache_simulator.h"
#include <vector>

class cache {
public:
    void init(cache_simulator* simulator_, int level, int associativity, int line_size, int total_size, bool inclusive_, bool exclusive_);
    void set_parent(cache *parent_);
    void add_parent(cache *parent_)
    bool has_parent();
    void add_child(cache* child);
    bool has_child();
    void request(const mem_ref_t &mem_ref_in);
    int get_hit_total();
    int get_miss_total();
    void invalidate(uint64_t tag);
    long long get_reuse_dist();
    std::vector<cache*> children;

protected:
    int level;
    cache_simulator* simulator;
    void init_lines();
    uint64_t compute_tag(uint64_t addr);
    int get_set_idx(uint64_t addr);
    int get_line_idx(int set_idx, int way);
    int get_lru(int set_idx);
    void update_lru_counters(int set_idx, int way);
    void replace_line(uint64_t tag, COHERENCE_STATE coherence_state);
    //cache[] *get_coherent_caches();
    bool cache::coherence_request(uint64_t tag, bool write);
    cache_line **lines;
    cache *parent;
    std::vector<parents> parents;
    int associativity;
    int num_lines;
    int line_size;
    int total_size;
    int line_size_bits;
    int set_index_bits;
    int assoc_bits;
    int num_sets;
    int hit_total;
    int miss_total;
    bool inclusive;
    bool exclusive;
    long long reuse_dist;
};


#endif //PROJECT_CACHE_H
