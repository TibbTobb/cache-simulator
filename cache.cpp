//
// Created by toby on 21/11/18.
//
#include "cache.h"
#include "cache_line.h"
#include <vector>
#include <cmath>

int num_blocks;
int line_size;
int block_size;
int total_size;
int block_size_bits;

cache_line **blocks;

//fully associative
void cache::init(int line_size_, int total_size_){
    line_size = line_size_;
    total_size = total_size_;

    //one block per line as fully associative
    block_size = line_size;

    num_blocks = total_size/line_size;

    block_size_bits = int(ceil(log2(block_size)));
}

void cache::init_blocks() {
    for(int i = 0; i < num_blocks; i++) {
        blocks[i] = new cache_line();
    }
}

uint64 cache::compute_tag(uint64 addr) {
    return addr >> block_size_bits;
}

void cache::request(const mem_ref_t mem_ref) {
    uint64 final_addr = mem_ref.addr + mem_ref.size-1;
    uint64 final_tag = compute_tag(final_addr);
    uint64 tag = compute_tag(mem_ref.addr);

    int way;
    for(; tag <= final_tag; ++tag) {
        for(way = 0; way<num_blocks; ++way) {
            if(blocks[way]->tag == tag) {
                //cache hit
                update_lru_counters(way);
                break;
            }
        }

        if(way == num_blocks) {
            //cache miss
            int lru_line = get_lru();
            blocks[lru_line]->tag = tag;
            update_lru_counters(way);
        }
    }
}

int cache::get_lru() {
    int max = 0;
    int oldest_line = 0;
    for(int way = 0; way<num_blocks; ++way) {
        int v = blocks[way]->counter;
        if(v >= max) {
            oldest_line = way;
            max = v;
        }
    }
    return oldest_line;
}

void cache::update_lru_counters(int way) {
    int count = blocks[way]->counter;
    if(count==0)
        return;
    for(int i = 0; i<num_blocks; ++i) {
        if(blocks[i]->counter <= count) {
            blocks[i]->counter++;
        }
    }
    blocks[way]->counter = 0;
}


