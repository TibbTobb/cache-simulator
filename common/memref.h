#ifndef PROJECT_MEMREF_H
#define PROJECT_MEMREF_H

#endif //PROJECT_MEMREF_H

#include <cstdint>

enum REF_TYPE {
    READ = 0,
    WRITE = 1,
    THREAD_INIT = 2,
    THREAD_EXIT = 3,
    SKIP = 4
};

typedef struct mem_ref_t {
    REF_TYPE ref_type;
    unsigned short size;
    uint64_t addr;
    int tid;
} mem_ref_t;