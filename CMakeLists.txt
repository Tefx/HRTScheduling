cmake_minimum_required(VERSION 3.3)
project(HRTScheduling)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -O3 -Werror")

set(SOURCE_FILES main.c scheduler.h scheduler.c ratemono.c ratemono.h ptba.c ptba.h utils.c utils.h)
add_executable(HRTScheduling ${SOURCE_FILES})