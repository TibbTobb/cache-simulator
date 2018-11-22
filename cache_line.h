//
// Created by toby on 21/11/18.
//

#ifndef PROJECT_CACHE_LINE_H
#define PROJECT_CACHE_LINE_H

#include <dr_defines.h>

class cache_line {
    uint64 tag;
    //keeps track of how long since line was accessed
    int counter;
};


#endif //PROJECT_CACHE_LINE_H
