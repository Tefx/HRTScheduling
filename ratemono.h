//
// Created by tefx on 8/25/15.
//

#include <stddef.h>
#include <stdbool.h>
#include "scheduler.h"
#include "utils.h"

#ifndef HRTSCHEDULING_RATEMONO_H
#define HRTSCHEDULING_RATEMONO_H

typedef struct {
    task_hrts tid;
    time_hrts st;
    time_hrts period;
    time_hrts rt;
    bool paused;
} task_rm;

typedef list *task_list_rm;

typedef struct {
    list *waiting_queue;
    list *available_queue;
    task_rm *current_running;
} schedule_status_rm;

typedef enum {
    RM_EVENT_PAUSED,
    RM_EVENT_FINISHED,
    RM_EVENT_STARTED,
    RM_EVENT_RESUMED
} event_type_rm;

typedef struct {
    event_type_rm event;
    task_hrts task;
    time_hrts time;
} event_rm;

typedef list *event_list_rm;

schedule_status_rm *init_status_rm(task_list_rm);

long schedule_rm(schedule_status_rm *ss, list *events, time_hrts current_time);

task_list_rm alternate_to_backward_task_rm(period_task_info *ts, size_t n, time_hrts cycle_length);

#endif //HRTSCHEDULING_RATEMONO_H
