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
    cache *p = new cache();

    c->init(2, 64, 640000);
    p->init(2, 64, 640000);

    c->set_parent(p);

    //read mem refs from file
    string line;
    ifstream infile;
    infile.open("/home/toby/CLionProjects/cache-simulator/cmake-build-debug/memref_output.dat");
    while(getline (infile,line)) {
        cout << line << '\n';
        istringstream iss(line);
        uint64_t addr;
        uint read;
        unsigned short size;
        iss >> addr;
        iss >> size;
        iss >> read;
        //mem_ref_t *m = create_mem_ref(addr, read, size);
        mem_ref_t m = {read ? WRITE : READ, size, addr};
        printf("Address: 0x%lu, Size: %i, Type: %i \n", m.addr, m.size, m.ref_type);
        c->request(m);
    }
    infile.close();
    printf("Hit total: %i, Miss total: %i", c->get_hit_total(), c->get_miss_total());

    //print miss numbers
}