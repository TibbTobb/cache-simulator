#ifndef PROJECT_CACHE_H
#define PROJECT_CACHE_H

#include "../common/memref.h"
#include "cache_line.h"
#include "replacement_policy.h"
#include <vector>
#include <unordered_set>

class replacement_policy;

class cache {
public:
    void init(bool use_coherence, int associativity, int line_size, int total_size,
            bool inclusive_, bool exclusive_, replacement_policy *replacementPolicy);
    void add_to_coherence_set(cache *c);
    void set_parent(cache *parent_);
    bool has_parent();
    void add_child(cache* child);
    bool has_child();
    void request(const mem_ref_t &mem_ref_in);
    bool processor_cache;
    int get_hit_total();
    int get_miss_total();
    int get_coherence_invalidates();
    int get_coherence_shares();
    void invalidate(uint64_t tag);
    uint64_t get_total_reuse_dist();
    std::unordered_set<cache*> children;
    int core_num;
    int reads;
    int writes;
    int coherence_e_to_m;
    int coherence_m_to_s;

    int get_set_idx(uint64_t addr);
    int get_line_idx(int set_idx, int way);
    int get_associativity();

    cache_line **lines;

protected:
    void init_lines();
    uint64_t compute_tag(uint64_t addr);
    void replace_line(uint64_t tag, COHERENCE_STATE coherence_state);
    bool coherence_request(mem_ref_t mem_ref_in);
    cache *parent;
    int associativity;
    int num_lines;
    int line_size;
    int total_size;
    int line_size_bits;
    int set_mask;
    int assoc_bits;
    bool use_coherence;
    bool inclusive;
    bool exclusive;
    int hit_total;
    int miss_total;
    int coherence_invalidates;
    int coherence_shares;
    uint64_t total_reuse_dist;
    std::vector<cache *> coherence_set;
    replacement_policy *replacementPolicy;
};


#endif //PROJECT_CACHE_H
