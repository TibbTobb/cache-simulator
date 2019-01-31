//
// Created by toby on 21/11/18.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include "cache_simulator.h"
using namespace std;

void cache_simulator::create_cache() {

}

void cache_simulator::run() {
    //initialise cache
    cache *c = new cache();
    //cache *p = new cache();

    c->init(8, 64, 32768);
    //p->init(8, 64, 32768);

    //c->set_parent(p);
    int reads = 0;
    int writes = 0;

    //read mem refs from file
    string line;
    ifstream infile;
    infile.open("/home/toby/CLionProjects/cache-simulator/tracer/cmake-build-debug/memref-output-13394.dat");
    while(getline (infile,line)) {
        //cout << line << '\n';
        istringstream iss(line);
        uint64_t addr;
        uint read;
        unsigned short size;
        iss >> addr;
        iss >> size;
        iss >> read;
        //mem_ref_t *m = create_mem_ref(addr, read, size);
        if(read == 1) {
            writes++;
        } else {
            reads++;
        }

        mem_ref_t m = {read ? WRITE : READ, size, addr};
        //printf("Address: 0x%lu, Size: %i, Type: %i \n", m.addr, m.size, m.ref_type);
        c->request(m);
    }
    infile.close();
    double miss_rate = double(c->get_miss_total())/double((c->get_miss_total()+c->get_hit_total()));
    printf("Hit total: %i, Miss total: %i, Miss rate: %f, Reads: %i, Writes: %i",
            c->get_hit_total(), c->get_miss_total(), miss_rate, reads, writes);

    //print miss numbers
}