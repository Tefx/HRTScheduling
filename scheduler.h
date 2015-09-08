//
// Created by tefx on 8/25/15.
//

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"

#ifndef HRTSCHEDULING_SCHEDULER_H_H
#define HRTSCHEDULING_SCHEDULER_H_H

typedef int64_t task_hrts;

#define task_no(x) ((x)>>32)
#define job_no(x) ((x)&0xffffffff)
task_hrts task_id(task_hrts x, task_hrts y);

typedef long time_hrts;

typedef enum {
    ACTION_FINISH = 0,
    ACTION_NOTHING = 0x01,
    ACTION_START_OR_RESUME_PRIMARY = 0x02,
    ACTION_CANCEL_PRIMARY = 0x04,
    ACTION_START_OR_RESUME_ALTERNATE = 0x08
} operation_type;

typedef struct {
    char *primary_path;
    char **primary_argv;
    char *alternate_path;
    char **alternate_argv;
    time_hrts period;
    time_hrts primary_time;
    time_hrts alternate_time;
} task_info;

typedef struct {
    operation_type action;
    task_hrts task_no;
} action_type;

typedef enum {
    REASON_PRIMARY_CRASHED,
    REASON_SCHEDULE_POINT
} schedule_reason;

time_hrts cycle_length(task_info *ts, task_hrts);

#endif //HRTSCHEDULING_SCHEDULER_H_H
