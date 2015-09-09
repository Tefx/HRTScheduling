cmake_minimum_required(VERSION 3.3)
project(HRTScheduling)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O3 -Wall -Werror")

add_executable(HRTScheduling main.c def.h def.c ratemono.c ratemono.h ptba.c ptba.h list.c list.h schedule.c schedule.h cJSON/cJSON.c cJSON/cJSON.h parse.c parse.h)
target_link_libraries(HRTScheduling gc m)

add_executable(PProc pproc.c)