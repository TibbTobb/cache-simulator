cmake_minimum_required(VERSION 3.12)
add_executable(simulator main.cpp cache.h cache.cpp  cache_line.h cache_simulator.h cache_simulator.cpp
        ../common/memref.h main.cpp cache_config.h replacement_policy.h lru.cpp)
set_property(TARGET simulator PROPERTY CXX_STANDARD 14)