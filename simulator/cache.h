//
// Created by toby on 21/11/18.
//

#ifndef PROJECT_CACHE_H
#define PROJECT_CACHE_H

#include "../common/memref.h"
#include "cache_line.h"
#include <vector>

class cache {
public:
    void init(int associativity, int line_size, int total_size);
    void set_parent(cache *parent_);
    bool has_parent();
    void add_child(cache* child);
    bool has_child();
    void request(const mem_ref_t &mem_ref_in);
    int get_hit_total();
    int get_miss_total();

protected:
    void init_lines();
    uint64_t compute_tag(uint64_t addr);
    int get_set_idx(uint64_t addr);
    int get_line_idx(int set_idx, int way);
    int get_lru(int set_idx);
    void update_lru_counters(int set_idx, int way);
    cache_line **lines;
    cache *parent;
    std::vector<cache*> children;
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
};


#endif //PROJECT_CACHE_H
