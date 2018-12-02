cmake_minimum_required(VERSION 3.12)
project(Interception)

set(CMAKE_CXX_STANDARD 14)

include_directories(clients/akeymap)
include_directories(library)
include_directories(samples)

add_executable(Interception
        clients/akeymap/akeymap.cpp
        clients/akeymap/akeymap.h
        clients/akeymap/scancodes.h
        clients/akeymap/utils.c
        clients/akeymap/utils.h
        library/interception.c
        library/interception.h
        samples/axes/axes.cpp
        samples/cadstop/cadstop.cpp
        samples/caps2esc/caps2esc.cpp
        samples/hardwareid/hardwareid.cpp
        samples/identify/identify.cpp
        samples/mathpointer/mathpointer.cpp
        samples/x2y/x2y.cpp
        samples/utils.c
        samples/utils.h)
