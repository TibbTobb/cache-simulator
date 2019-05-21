#ifndef PROJECT_REPLACEMENT_POLICY_H
#define PROJECT_REPLACEMENT_POLICY_H

#include "cache.h"

class cache;

class replacement_policy {
public:
    virtual int get_replacement_line(cache *c, int set_idx) = 0;
    virtual void update(cache *c, int set_idx, int way) = 0;
};


#endif //PROJECT_REPLACEMENT_POLICY_H
