cmake_minimum_required(VERSION 3.3)
project(HRTScheduling)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O3 -Wall -Werror")

add_executable(HRTScheduling main.c scheduler.h scheduler.c ratemono.c ratemono.h ptba.c ptba.h utils.c utils.h process.c process.h)
target_link_libraries(HRTScheduling gc)

add_executable(PProc pseudoproc.c)