cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

project (ArcMon)

add_executable (arcmon daemon.cpp)
target_link_libraries (arcmon arclib)
