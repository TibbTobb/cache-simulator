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
            int lru_line = get_lru(set_idx);
            lines[lru_line]->tag = tag;
            if(parent != nullptr) {
                memref.addr = tag << line_size_bits;
                parent->request(memref);
            }
        }
    }
}

int cache::get_lru(int set_idx) {
    int max = 0;
    int oldest_line = 0;
    for(int way = 0; way<associativity; ++way) {
        int line_idx = get_line_idx(set_idx, way);
        int v = lines[line_idx]->counter;
        if(v >= max) {
            oldest_line = line_idx;
            max = v;
        }
    }
    return oldest_line;
}

void cache::update_lru_counters(int set_idx, int way) {
    int line_idx = get_line_idx(set_idx, 0);
    int count = lines[line_idx]->counter;
    if(count==0)
        return;
    //increment all lines in set
    for(int i = 0; i<associativity; ++i) {
        line_idx = get_line_idx(set_idx, i);
        if(lines[line_idx]->counter <= count) {
            lines[line_idx]->counter++;
        }
    }
    //set accessed line to 0
    line_idx = get_line_idx(set_idx, way);
    lines[line_idx]->counter = 0;
}

int cache::get_hit_total() {
    return hit_total;
}

int cache::get_miss_total() {
    return miss_total;
}