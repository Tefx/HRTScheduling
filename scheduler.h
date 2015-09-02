//
// Created by tefx on 8/25/15.
//

#include <stddef.h>
#include <stdbool.h>
#include <bits/types.h>
#include "utils.h"

#ifndef HRTSCHEDULING_SCHEDULER_H_H
#define HRTSCHEDULING_SCHEDULER_H_H

typedef __int64_t task_hrts;

#define task_no(x) ((x)>>32)
#define job_no(x) ((x)&0xffffffff)

task_hrts task_id(__int64_t x, __int64_t y);

typedef long time_hrts;

typedef struct {
    time_hrts period;
    time_hrts running_time;
    time_hrts alternate_time;
} period_task_info;

time_hrts cycle_length(period_task_info *ts, size_t);

#endif //HRTSCHEDULING_SCHEDULER_H_H
