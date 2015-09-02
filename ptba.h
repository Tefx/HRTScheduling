//
// Created by tefx on 8/26/15.
//

#ifndef HRTSCHEDULING_PTBA_H
#define HRTSCHEDULING_PTBA_H

#include <stdbool.h>
#include "scheduler.h"
#include "ratemono.h"
#include "utils.h"

typedef char ptba_time_statue;

#define PTBA_FREE 0x01
#define PTBA_ALTERNATE_START 0x02
#define PTBA_ALTERNATE_RESUME 0x04
#define PTBA_ALTERNATE_PAUSE 0x08
#define PTBA_ALTERNATE_FINISH 0x10
#define PTBA_ALTERNATE_ADVANCED 0x20
#define PTBA_PRIMARY_USED 0x40

typedef struct {
    time_hrts start;
    time_hrts end;
    task_hrts task_id;
    ptba_time_statue statue;
} time_slice;

#define ts_data(t) ((time_slice *) data_of(t))

typedef list *time_slice_list;

typedef enum {
    JOB_STATUE_PRIMARY_STARTED_OR_RESUMED,
    JOB_STATUE_PRIMARY_PAUSED,
    JOB_STATUE_PRIMARY_CANCELLED,
    JOB_STATUE_PRIMARY_CRASHED,

    JOB_STATUE_ALTERNATE_STARTED_OR_RESUMED,
    JOB_STATUE_ALTERNATE_PAUSED,

    JOB_STATUE_RELEASED,
    JOB_STATUE_FINISHED,
} job_statue_ptba;

typedef enum {
    REASON_PRIMARY_CRASHED,
    REASON_SCHEDULE_POINT
} schedule_reason_ptba;

typedef struct {
    task_hrts job;
    time_hrts remaining_time;
    job_statue_ptba statue;
} task_statue_ptba;

typedef struct {
    time_slice_list alternate_ts;
    task_statue_ptba *task_statue;
    time_hrts current_time;
    time_hrts last_scheduling_point;
    task_hrts current_running;

    period_task_info *task_info;
    size_t num_task;
    time_hrts cycle_length;
    time_hrts adv_orig;
} statue_ptba;

statue_ptba *prepare_statue_ptba(period_task_info *ts, size_t n, time_hrts ct);

time_slice_list rm_backward(period_task_info *ts, size_t n, time_hrts);

time_slice_list rm_backward_from_task_list(task_list_rm ts, time_hrts current_time, time_hrts cycle_length);

void cancel_n_adjust_alternate(time_slice_list, task_hrts, period_task_info *);

time_hrts schedule_ptba(statue_ptba *, time_hrts, schedule_reason_ptba);

#endif //HRTSCHEDULING_PTBA_H

