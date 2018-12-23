//
// Created by toby on 21/11/18.
//

#ifndef PROJECT_CACHE_H
#define PROJECT_CACHE_H

#include "memref.h"
#include "cache_line.h"

class cache {
public:
    void init(int line_size, int total_size);
    void request(mem_ref_t );

protected:
    void init_blocks();
    uint64 compute_tag(uint64 addr);
    int get_lru();
    void update_lru_counters(int way);
};


#endif //PROJECT_CACHE_H
