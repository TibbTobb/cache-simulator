#include "cache.h"
#include "cache_line.h"
#include "cache_simulator.h"
#include <vector>
#include <cmath>
void cache::init(bool use_coherence_, int associativity_, int line_size_, int total_size_,
        bool inclusive_, bool exclusive_, replacement_policy *replacementPolicy_){
    use_coherence = use_coherence_;
    associativity = associativity_;
    line_size = line_size_;
    total_size = total_size_;
    inclusive = inclusive_;
    exclusive = exclusive_;
    replacementPolicy = replacementPolicy_;

    //encodes not processor cache
    core_num = -1;
    processor_cache = false;

    num_lines = total_size / line_size;

    line_size_bits = int(ceil(log2(line_size)));
    assoc_bits = int(ceil(log2(associativity)));

    int num_sets = num_lines/associativity;
    set_mask = num_sets - 1;

    hit_total = 0;
    miss_total = 0;
    reads = 0;
    writes = 0;
    coherence_invalidates = 0;
    coherence_shares = 0;
    coherence_e_to_m = 0;
    coherence_m_to_s = 0;

    lines = new cache_line *[num_lines];
    init_lines();

    total_reuse_dist = 0;
}

void cache::init_lines() {
    for(int i = 0; i < num_lines; i++) {
        lines[i] = new cache_line();
    }
}

void cache::set_parent(cache *parent_) {
    parent = parent_;
}

bool cache::has_parent() {
    return parent != nullptr;
}

uint64_t cache::compute_tag(uint64_t addr) {
    //include set index bits in tag
    return addr >> line_size_bits;
}

int cache::get_set_idx(uint64_t tag) {
    return int(tag & set_mask);
}

int cache::get_line_idx(int set_idx, int way) {
    return (set_idx << assoc_bits) + way;
}

bool cache::coherence_request(mem_ref_t mem_ref_in) {
    //uses mem_ref_t as line size may be different
    uint64_t final_addr = mem_ref_in.addr + mem_ref_in.size - 1;
    uint64_t final_tag = compute_tag(final_addr);
    uint64_t tag = compute_tag(mem_ref_in.addr);
    bool shared = false;
    //check each line for valid tag
    int way, set_idx;
    for (; tag <= final_tag; ++tag) {
        set_idx = get_set_idx(tag);
        //check each line in set
        for (way = 0; way < associativity; ++way) {
            int line_idx = get_line_idx(set_idx, way);
            if (lines[line_idx]->tag == tag) {
                //hit
                    if(mem_ref_in.ref_type == WRITE) {
                        invalidate(tag);
                        coherence_invalidates++;
                    } else {
                        if(processor_cache) {
                            if (lines[line_idx]->coherence_state == COHERENCE_STATE::M) {
                                coherence_m_to_s++;
                            }
                            lines[line_idx]->coherence_state = COHERENCE_STATE::S;
                        }
                        coherence_shares++;
                    }
                    shared = true;
                }
            }
        }
    return shared;
    }

uint64_t cache::get_total_reuse_dist() {
    return total_reuse_dist;
}


void cache::request(const mem_ref_t &mem_ref_in) {

    uint64_t final_addr = mem_ref_in.addr + mem_ref_in.size - 1;
    uint64_t final_tag = compute_tag(final_addr);
    uint64_t tag = compute_tag(mem_ref_in.addr);

    //create copy of memref to hand to parent caches
    mem_ref_t memref = mem_ref_in;

    //check each line for valid tag
    int way, set_idx;
    int count = 0;
    for (; tag <= final_tag; ++tag) {
        count++;
        if (tag + 1 <= final_tag)
            memref.size = ((tag + 1) << line_size_bits) - memref.addr;
        set_idx = get_set_idx(tag);
        //check each line in set
        for (way = 0; way < associativity; ++way) {
            int line_idx = get_line_idx(set_idx, way);
            if (lines[line_idx]->tag == tag) {
                //cache hit
                hit_total++;
                total_reuse_dist += lines[line_idx]->counter;
                if (exclusive) {
                    lines[line_idx]->tag = TAG_INVALID;
                } else {
                    replacementPolicy->update(this, set_idx, way);
                }
                if (use_coherence && processor_cache) {
                    if (memref.ref_type == WRITE) {
                        if(lines[line_idx]->coherence_state == COHERENCE_STATE::E) {
                            coherence_e_to_m++;
                        }
                        lines[line_idx]->coherence_state = COHERENCE_STATE::M;
                        //if a write and other caches may have a copy (shared state), run coherence protocol
                        //send coherence request to all non parents
                        for (cache *c : coherence_set) {
                            c->coherence_request(memref);
                        }
                    }
                }
                break;
            }
        }
        if (way == associativity) {
            //cache miss
            miss_total++;

            //top level cache so run coherence
            COHERENCE_STATE coherence_state = COHERENCE_STATE::M;
            if (use_coherence && processor_cache) {
                bool shared = false;
                for (cache *c : coherence_set) {
                    bool s = c->coherence_request(memref);
                    shared =  s || shared;
                }
                //put line in cache if not exclusive

                if (memref.ref_type == WRITE) {
                    coherence_state = COHERENCE_STATE::M;
                } else if (shared) {
                    coherence_state = COHERENCE_STATE::S;
                } else {
                    coherence_state = COHERENCE_STATE::E;
                }
            }
            if (!exclusive) {
                replace_line(tag, coherence_state);
            }
            if(has_parent()) {
                parent->request(memref);
            }
        }

        if (tag + 1 <= final_tag) {
            uint64_t next_addr = (tag + 1) << line_size_bits;
            memref.addr = next_addr;
            /*undo the -1*/
            memref.size = final_addr - next_addr + 1;
        }
    }
    if(memref.ref_type == WRITE) {
        writes += count;
    } else {
        reads += count;
    }
}



int cache::get_hit_total() {
    return hit_total;
}

int cache::get_miss_total() {
    return miss_total;
}

bool cache::has_child() {
    return !children.empty();
}

void cache::add_child(cache *child) {
    children.insert(child);
}

void cache::invalidate(uint64_t tag) {
    int set_idx = get_set_idx(tag);
    int way;
    for (way = 0; way < associativity; ++way) {
        int line_idx = get_line_idx(set_idx, way);
        if (lines[line_idx]->tag == tag) {
            lines[line_idx]->tag = TAG_INVALID;
            if (inclusive) {
                for (cache *child : children) {
                    child->invalidate(tag);
                }
            }
            return;
        }
    }
}

void cache::replace_line(uint64_t tag, COHERENCE_STATE coherence_state) {
    int set_idx = get_set_idx(tag);
    int lru_way = replacementPolicy->get_replacement_line(this, set_idx);
    int lru_line = get_line_idx(set_idx, lru_way);
    uint64_t replaced_tag = lines[lru_line]->tag;
    if(replaced_tag != TAG_INVALID) {
        if (inclusive) {
            for (cache *child : children) {
                //invalidate line in each child
                //as parents line may be larger than child's then we may need to send multiple invalidates
                //note: inclusive parents line size greater than or equal to child line size
                int line_size_diff_bits = line_size_bits - child->line_size_bits;
                int line_size_diff = 1 << line_size_diff_bits;
                uint64_t invalid_tag = replaced_tag << line_size_diff_bits;
                for (int i = 0; i < line_size_diff; i++) {
                    child->invalidate(invalid_tag+i);
                }
            }
        }
        if (has_parent()) {
            if (parent->exclusive) {
                //if exclusive put evicted line in parent cache
                //TODO: assuming line size is the same
                parent->replace_line(replaced_tag, coherence_state);
            }
        }
    }
    lines[lru_line]->tag = tag;
    lines[lru_line]->coherence_state = coherence_state;
    replacementPolicy->update(this, set_idx, lru_way);
}

int cache::get_associativity() {
    return associativity;
}

void cache::add_to_coherence_set(cache *c) {
    coherence_set.push_back(c);
}

int cache::get_coherence_invalidates() {
    return coherence_invalidates;
}

int cache::get_coherence_shares() {
    return coherence_shares;
}
