//
// Created by toby on 18/12/18.
//

#include <dr_defines.h>

#ifndef PROJECT_MEMREF_H
#define PROJECT_MEMREF_H

#endif //PROJECT_MEMREF_H

enum REF_TYPE {
    READ = 0,
    WRITE = 1
};

typedef struct mem_ref_t {
    REF_TYPE ref_type;
    ushort size;
    uint64 addr;
} mem_ref_t;