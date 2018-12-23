//
// Created by toby on 21/11/18.
//

#include "cache_simulator.h"
#include "cache.h"

void cache_simulator::create_cache() {
}

void cache_simulator::run() {
    //initilise cache
    Cache *cache = new Cache();
    cache->init(64, 640000);

    //read mem refs from pipe

    //print miss numbers
}
