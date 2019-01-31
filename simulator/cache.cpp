//
// Created by toby on 21/11/18.
//
#include "cache.h"
#include "cache_line.h"
#include "cache_simulator.h"
#include <vector>
#include <cmath>

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

cache_line **lines;
cache *parent;

//set associative lru
void cache::init(int associativity_, int line_size_, int total_size_){
    
    associativity = associativity_;
    line_size = line_size_;
    total_size = total_size_;

    num_lines = total_size / line_size;
    num_sets = num_lines / associativity;

    line_size_bits = int(ceil(log2(line_size)));
    set_index_bits = int(ceil(log2(num_sets)));
    assoc_bits = int(ceil(log2(associativity)));

    hit_total = 0;
    miss_total = 0;

    lines = new cache_line *[num_lines];
    init_lines();
}

void cache::init_lines() {
    for(int i = 0; i < num_lines; i++) {
        lines[i] = new cache_line();
    }
}

void cache::set_parent(cache *parent_) {
    parent = parent_;
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

//cache* cache::get_coherance_caches();

/*
void cache::set_line_coherance() {

}
*/

void cache::request(const mem_ref_t &mem_ref_in) {
    uint64_t final_addr = mem_ref_in.addr + mem_ref_in.size-1;
    uint64_t final_tag = compute_tag(final_addr);
    uint64_t tag = compute_tag(mem_ref_in.addr);

    //create copy of memref to hand to parent caches
    mem_ref_t memref = mem_ref_in;
    memref.size = 1;

    //check each line for valid tag
    int way;
    for(; tag <= final_tag; ++tag) {
        memref.addr = tag << line_size_bits;
        int set_idx = get_set_idx(tag);
        //check each line in set
        for(way = 0; way<associativity; ++way) {
            int line_idx = get_line_idx(set_idx, way);
            if(lines[line_idx]->tag == tag) {
                //cache hit
                hit_total++;
                update_lru_counters(set_idx, way);
                break;
            }
        }

        if(way == associativity) {
            //cache miss
            miss_total++;

            //TODO: check other caches
            int lru_way = get_lru(set_idx);
            int lru_line = get_line_idx(set_idx, lru_way);
            lines[lru_line]->tag = tag;
            update_lru_counters(set_idx, lru_way);
            if(parent != nullptr) {
                memref.addr = tag << line_size_bits;
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