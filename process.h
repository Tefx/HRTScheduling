//
// Created by tefx on 9/8/15.
//

#ifndef HRTSCHEDULING_PROCESS_H
#define HRTSCHEDULING_PROCESS_H

#include <sys/types.h>
#include "scheduler.h"
#include "ptba.h"

typedef struct {
    task_info *info;
    pid_t *pids;
    task_hrts *job_count;
    task_hrts num;
    statue_ptba *schedule_statue;
    time_hrts last_point;
    task_hrts running;
} task_record;

void start_scheduling(task_info *, task_hrts);

void start_primary(task_hrts);

void start_alternate(task_hrts);

int task_action(task_hrts, int);

void sighandler_SIGCHLD(int sig);

void sighandler_SIGALRM(int sig);

bool schedule(schedule_reason reason);

#endif //HRTSCHEDULING_PROCESS_H
