//
// Created by tefx on 8/25/15.
//

#include <stddef.h>
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
    ACTION_INIT = 0,
    ACTION_NOTHING = 0x01,
    ACTION_START_OR_RESUME_PRIMARY = 0x02,
    ACTION_CANCEL_PRIMARY = 0x04,
    ACTION_START_OR_RESUME_ALTERNATE = 0x08
} action_type_schedule;

typedef struct {
    time_hrts period;
    time_hrts running_time;
    time_hrts alternate_time;
} period_task_info;

typedef struct {
    action_type_schedule action;
    task_hrts task_no;
} action_schedule;

time_hrts cycle_length(period_task_info *ts, size_t);

#endif //HRTSCHEDULING_SCHEDULER_H_H
