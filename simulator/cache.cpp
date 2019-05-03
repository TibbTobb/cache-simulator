//
// Created by toby on 21/11/18.
//
#include "cache.h"
#include "cache_line.h"
#include "cache_simulator.h"
#include <vector>
#include <cmath>

//set associative lru
void cache::init(cache_simulator* simulator_, int level_, int associativity_, int line_size_, int total_size_, 
        bool inclusive_, bool exclusive_){
    simulator = simulator_;
    level = level_;
    associativity = associativity_;
    line_size = line_size_;
    total_size = total_size_;
    inclusive = inclusive_;
    exclusive = exclusive_;

    num_lines = total_size / line_size;
    num_sets = num_lines / associativity;

    line_size_bits = int(ceil(log2(line_size)));
    set_index_bits = int(ceil(log2(num_sets)));
    assoc_bits = int(ceil(log2(associativity)));

    hit_total = 0;
    miss_total = 0;

    lines = new cache_line *[num_lines];
    init_lines();

    reuse_dist = 0;
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
    return parent == nullptr;
}

uint64_t cache::compute_tag(uint64_t addr) {
    //include set index bits in tag
    return addr >> line_size_bits;
}

int cache::get_set_idx(uint64_t tag) {
    uint64_t mask = (1ULL << set_index_bits) - 1ULL;
    return int(tag & mask);
}

int cache::get_line_idx(int set_idx, int way) {
    return (set_idx << assoc_bits) + way;
}

//cache* cache::get_coherence_caches();

/*
void cache::set_line_coherence() {

}
*/

void cache::request(const mem_ref_t &mem_ref_in) {
    uint64_t final_addr = mem_ref_in.addr + mem_ref_in.size - 1;
    uint64_t final_tag = compute_tag(final_addr);
    uint64_t tag = compute_tag(mem_ref_in.addr);

    //create copy of memref to hand to parent caches
    mem_ref_t memref = mem_ref_in;
    //TODO: work out if this is correct
    memref.size = static_cast<unsigned short>(line_size);

    //check each line for valid tag
    int way;
    for (; tag <= final_tag; ++tag) {
        memref.addr = tag << line_size_bits;
        int set_idx = get_set_idx(tag);
        //check each line in set
        for (way = 0; way < associativity; ++way) {
            int line_idx = get_line_idx(set_idx, way);
            if (lines[line_idx]->tag == tag) {
                //cache hit
                hit_total++;
                reuse_dist += lines[line_idx]->counter;
                if (exclusive) {
                    lines[line_idx]->tag = TAG_INVALID;
                } else {
                    update_lru_counters(set_idx, way);
                }
                if(memref.ref_type == WRITE) {
                    lines[line_idx]->coherence_state = COHERENCE_STATE::M;
                    //if a write and other caches may have a copy (shared state), run coherence protocol
                    if(lines[line_idx]->coherence_state == COHERENCE_STATE::S) {
                        for (cache *c : simulator->levels[level]) {
                            if (c != this) {
                                c->coherence_request(tag, true);
                            }
                        }
                    }
                }
                break;
            }
        }

        if (way == associativity) {
            //cache miss
            miss_total++;
            //todo: fix level of cache
            bool shared = false;
            for (cache *c : simulator->levels[level]) {
                if (c != this) {
                    shared = shared || c->coherence_request(tag, memref.ref_type == WRITE);
                }
            }
            if (!exclusive) {
                int lru_way = get_lru(set_idx);
                int lru_line = get_line_idx(set_idx, lru_way);
                uint64_t replaced_tag = lines[lru_line]->tag;
                COHERENCE_STATE replaced_coherence_state = lines[lru_line]->coherence_state;
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
                                child->invalidate(invalid_tag);
                            }
                        }
                    }
                    if (has_parent()) {
                        if (parent->exclusive) {
                            //if exclusive put evicted line in parent cache
                            //TODO: assuming line size is the same
                            parent->replace_line(replaced_tag, replaced_coherence_state);
                        }
                    }
                }
                lines[lru_line]->tag = tag;
                if(memref.ref_type == WRITE) {
                    lines[lru_line]->coherence_state = COHERENCE_STATE::M;
                } else if(shared) {
                    lines[lru_line]->coherence_state = COHERENCE_STATE::S;
                } else {
                    lines[lru_line]->coherence_state = COHERENCE_STATE::E;
                }
                update_lru_counters(set_idx, lru_way);
            }
            if (has_parent()) {
                parent->request(memref);
            }
        }
    }
}

int cache::get_lru(int set_idx) {
    //find way of least recently used line
    int max = 0;
    int oldest_way = 0;
    for(int way = 0; way<associativity; ++way) {
        int line_idx = get_line_idx(set_idx, way);
        if(lines[line_idx]->tag == TAG_INVALID) {
            oldest_way = way;
            break;
        }
        int v = lines[line_idx]->counter;
        if(v >= max) {
            oldest_way = way;
            max = v;
        }
    }
    int line_idx = get_line_idx(set_idx, oldest_way);
    lines[line_idx]->counter = 1;
    return oldest_way;
}

void cache::update_lru_counters(int set_idx, int way) {
    int line_idx = get_line_idx(set_idx, way);
    int count = lines[line_idx]->counter;
    //increment all lines with count less than or equal to accessed line
    if(count == 0)
        return;
    for(int i = 0; i<associativity; ++i) {
        line_idx = get_line_idx(set_idx, i);
        if(lines[line_idx]->counter <= count) {
            lines[line_idx]->counter++;
        }
    }
    //set accessed line to 0
    lines[line_idx]->counter = 0;
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
    children.push_back(child);
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
    //get line to replace
    int set_idx = get_set_idx(tag);
    int lru_way = get_lru(set_idx);
    int lru_line = get_line_idx(set_idx, lru_way);
    uint64_t replaced_tag = lines[lru_line]->tag;
    COHERENCE_STATE replaced_coherence_state = lines[lru_line]->coherence_state;
    if(replaced_tag != TAG_INVALID) {
        if (inclusive) {
            for (cache *child : children) {
                //invalidate line in each child
                //as parents line may be larger than child's then we may need to send multiple invalidates
                //note inclusive parents line size >= child line size
                int line_size_diff_bits = line_size_bits - child->line_size_bits;
                int line_size_diff = 1 << line_size_diff_bits;
                uint64_t invalid_tag = replaced_tag << line_size_diff_bits;
                for (int i = 0; i < line_size_diff; i++) {
                    child->invalidate(invalid_tag, true);
                }
            }
        }
        if (has_parent()) {
            //TODO: or is dirty
            if (parent->exclusive) {
                //if exclusive put evicted line in parent cache
                //TODO: assuming line size is the same
                parent->replace_line(replaced_tag, replaced_coherence_state);
                //TODO: fix
                //parent->update_lru_counters(parent->get_set_idx();
            }
        }
    }
    lines[lru_line]->tag = tag;
    lines[lru_line]->coherence_state = coherence_state;
    update_lru_counters(set_idx, lru_way);
}

bool cache::coherence_request(uint64_t tag, bool write) {
    //this works because cache line size must be the same for caches on same level
        int set_idx = get_set_idx(tag);
        //check each line in set
        for(int way = 0; way<associativity; ++way) {
            int line_idx = get_line_idx(set_idx, way);
            if(lines[line_idx]->tag == tag) {
                //cache hit
                //hit_total++;
                if(write) {
                    invalidate(tag);
                } else {
                    lines[line_idx]->coherence_state = COHERENCE_STATE::S;
                }
                return true;
            }

        }
    return false;
}

long long cache::get_reuse_dist() {
    return reuse_dist;
}
