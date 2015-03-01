cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

project (ArcMon)

add_executable (arcmond src/daemon.c src/card.cpp)
target_link_libraries (arcmond arclib)

install(
    TARGETS arcmond
    DESTINATION sbin
)
