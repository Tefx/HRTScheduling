//
// Created by tefx on 8/26/15.
//

#ifndef HRTSCHEDULING_PTBA_H
#define HRTSCHEDULING_PTBA_H

#include <stdbool.h>
#include "def.h"
#include "ratemono.h"
#include "list.h"

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

typedef struct {
    time_hrts period;
    time_hrts primary_time;
    time_hrts alternate_time;
} period_info_ptba;

typedef enum {
    JOB_STATUE_RELEASED = 0x01,
    JOB_STATUE_FINISHED = 0x02,
    JOB_STATUE_PRIMARY_RUNNING = 0x04,
    JOB_STATUE_PRIMARY_TERMINATED = 0x08,
    JOB_STATUE_ALTERNATE_RUNNING = 0x10,
    JOB_STATUE_ALTERNATE_ADV_RUNNING = 0x20
} job_statue_ptba;

typedef struct {
    task_hrts job;
    time_hrts remaining_time;
    job_statue_ptba statue;
} task_statue_ptba;

typedef struct {
    period_info_ptba *task_info;
    task_hrts num_task;
    time_hrts cycle_length;

    time_slice_list alternate_ts;


    time_hrts last_scheduling_point;
    task_hrts current_running;

    time_slice_list predict_table;
    task_hrts pt_task;

    list_node *alter_orig;


    task_statue_ptba *task_statue;
} statue_ptba;

time_slice_list rm_backward(period_info_ptba *ts, task_hrts n, time_hrts);
time_slice_list rm_backward_from_task_list(task_list_rm ts, time_hrts cl, time_hrts ct, time_hrts end_time);

task_list_rm alternate_to_backward_task_rm(period_info_ptba *ts, task_hrts n, time_hrts cycle_length);

void cancel_n_adjust_alternate(time_slice_list, task_hrts, period_info_ptba *);
time_slice *strip_time(time_slice_list tsl, time_hrts current_time);
void adjust_released(statue_ptba *statue, time_hrts current);
task_hrts find_task_with_earliest_notice_time(statue_ptba *);

bool try_EIT(statue_ptba *, action_type *);
void clean_EIT(statue_ptba *, time_hrts);

time_hrts schedule_ptba_nonPT(statue_ptba *statue, time_hrts current_time, schedule_reason reason,
                              action_type *);

time_hrts schedule_ptba_PT(statue_ptba *statue, time_hrts current_time, schedule_reason reason, action_type *);

void print_ts(void *tp);
void print_statue(statue_ptba *statue, size_t count);

//Export
statue_ptba *prepare_statue_ptba(task_info *ts, task_hrts n);
statue_ptba *copy_statue(statue_ptba *);

time_hrts schedule_ptba(statue_ptba *, time_hrts, schedule_reason, action_type *);

#endif //HRTSCHEDULING_PTBA_H

