//
// Created by toby on 21/11/18.
//

#ifndef PROJECT_CACHE_LINE_H
#define PROJECT_CACHE_LINE_H

#include <cstdint>

enum COHERENCE_STATE {
    I, S, E
};

static const uint64_t TAG_INVALID = (uint64_t) - 1;

class cache_line {
public:
    cache_line()
    :tag(TAG_INVALID)
    , counter(0)
    {}
    uint64_t tag;
    //keeps track of how long since line was accessed
    int counter;
    //COHERENCE_STATE coherence_state;
};


#endif //PROJECT_CACHE_LINE_H
