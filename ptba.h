//
// Created by tefx on 8/26/15.
//

#ifndef HRTSCHEDULING_PTBA_H
#define HRTSCHEDULING_PTBA_H

#include <stdbool.h>
#include "scheduler.h"
#include "ratemono.h"
#include "utils.h"

typedef enum {
    PTBA_EMPTY = 0,
    PTBA_FREE = 0x01,
    PTBA_ALTERNATE_START = 0x02,
    PTBA_ALTERNATE_RESUME = 0x04,
    PTBA_ALTERNATE_PAUSE = 0x08,
    PTBA_ALTERNATE_FINISH = 0x10,
    PTBA_PRIMARY_USED = 0x20
} time_slice_flag;

typedef time_slice_flag ptba_time_statue;


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

    time_hrts last_scheduling_point;
    task_hrts current_running;

    time_slice_list predict_table;
    time_hrts pt_task;

    period_task_info *task_info;
    size_t num_task;
    time_hrts cycle_length;
} statue_ptba;

statue_ptba *prepare_statue_ptba(period_task_info *ts, size_t n);

time_slice_list rm_backward(period_task_info *ts, size_t n, time_hrts);

time_slice_list rm_backward_from_task_list(task_list_rm ts, time_hrts current_time, time_hrts cycle_length);

void cancel_n_adjust_alternate(time_slice_list, task_hrts, period_task_info *);

time_slice *strip_time(time_slice_list tsl, time_hrts current_time);

void adjust_released(statue_ptba *statue, time_hrts current);

task_hrts find_task_with_earliest_notice_time(statue_ptba *);

time_hrts schedule_ptba(statue_ptba *, time_hrts, schedule_reason_ptba, action_schedule *);

time_hrts schedule_ptba_nonPT(statue_ptba *statue, time_hrts current_time, schedule_reason_ptba reason,
                              action_schedule *);

time_hrts schedule_ptba_PT(statue_ptba *statue, time_hrts current_time, schedule_reason_ptba reason, action_schedule *);

void print_ts(void *tp);

void print_statue(statue_ptba *statue);

#endif //HRTSCHEDULING_PTBA_H

