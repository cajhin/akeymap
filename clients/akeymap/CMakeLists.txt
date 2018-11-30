cmake_minimum_required(VERSION 3.12)
project(akeymap)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(akeymap
        akeymap.cpp
        akeymap.h
        scancodes.h
        utils.c
        utils.h)
