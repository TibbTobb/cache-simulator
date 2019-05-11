//
// Created by toby on 11/05/19.
//

#ifndef PROJECT_REUSE_DIST_H
#define PROJECT_REUSE_DIST_H

#include<unordered_map>
#include<list>

class reuse_dist {
public:
    void process_memref(mem_ref_t mem_ref) {
        printf("%lu\n", mem_ref.addr);
        auto find = cache_map.find(mem_ref.addr);
        if(find != cache_map.end()){
            uint64_t pos = 0;
            for(ref_list_node *n : ref_list) {
                if(n == (find->second)) {
                    add_to_dist_map(pos);
                    ref_list.remove(n);
                    ref_list.push_front(n);
                    n->reference_count++;
                    break;
                }
            }
        } else {
            auto *node = new ref_list_node();
            ref_list.push_front(node);
            cache_map.insert({mem_ref.addr, node});
        }
    }

    void print_statistics() {
        printf("Distance  Count");
        for(auto entry : dist_map) {
            printf("\n%lu  %lu", entry.first, entry.second);
        }
    }

private:
    struct ref_list_node{
        uint64_t reference_count{0};
    };

    std::unordered_map<uint64_t, ref_list_node *> cache_map;
    std::unordered_map<uint64_t, uint64_t> dist_map;
    std::list<ref_list_node *> ref_list;


    void add_to_dist_map(uint64_t dist) {
        auto find = dist_map.find(dist);
        if(find != dist_map.end()) {
            dist_map.at(dist)++;
        } else {
            dist_map.insert({dist, 0});
        }
    }
};

#endif //PROJECT_REUSE_DIST_H
