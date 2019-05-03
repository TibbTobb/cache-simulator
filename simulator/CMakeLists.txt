cmake_minimum_required(VERSION 3.12)
add_executable(simulator main.cpp cache.cpp cache.h cache_line.h cache_simulator.cpp cache_simulator.h ../common/memref.h main.cpp cache_config.h)
set_property(TARGET simulator PROPERTY CXX_STANDARD 14)