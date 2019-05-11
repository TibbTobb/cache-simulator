#include "replacement_policy.h"
#include "cache.h"

class lru : public replacement_policy {
    int get_replacement_line(cache *c, int set_idx) override {
        //find way of least recently used line
        int max = 0;
        int oldest_way = 0;
        for(int way = 0; way<c->get_associativity(); ++way) {
            int line_idx = c->get_line_idx(set_idx, way);
            if(c->lines[line_idx]->tag == TAG_INVALID) {
                oldest_way = way;
                break;
            }
            int v = c->lines[line_idx]->counter;
            if(v >= max) {
                oldest_way = way;
                max = v;
            }
        }
        int line_idx = c->get_line_idx(set_idx, oldest_way);
        c->lines[line_idx]->counter = 1;
        return oldest_way;
    }

    void update(cache *c, int set_idx, int way) override {
        int line_idx = c->get_line_idx(set_idx, way);
        int count = c->lines[line_idx]->counter;
        //increment all lines with count less than or equal to accessed line
        if(count == 0)
            return;
        for(int i = 0; i<c->get_associativity(); ++i) {
            line_idx = c->get_line_idx(set_idx, i);
            if(c->lines[line_idx]->counter <= count) {
                c->lines[line_idx]->counter++;
            }
        }
        //set accessed line to 0
        c->lines[line_idx]->counter = 0;
    }
};
