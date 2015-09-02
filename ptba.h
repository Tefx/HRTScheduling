//
// Created by tefx on 8/26/15.
//

#ifndef HRTSCHEDULING_PTBA_H
#define HRTSCHEDULING_PTBA_H

#include <stdbool.h>
#include "scheduler.h"
#include "ratemono.h"
#include "utils.h"

typedef unsigned int ptba_time_statue;

#define PTBA_FREE 0x01
#define PTBA_ALTERNATE_START 0x02
#define PTBA_ALTERNATE_RESUME 0x04
#define PTBA_ALTERNATE_PAUSE 0x08
#define PTBA_ALTERNATE_FINISH 0x10
#define PTBA_ALTERNATE_ADVANCED 0x20
#define PTBA_ALTERNATE_PRIMARY_CANCELLED 0x80
#define PTBA_PRIMARY_USED 0x100

typedef struct {
    time_hrts start;
    time_hrts end;
    task_hrts task_id;
    ptba_time_statue statue;
} time_slice;

#define ts_data(t) ((time_slice *) data_of(t))

typedef list *time_slice_list;
typedef list *task_hrts_list;

typedef struct {
    bool is_primary;
    task_hrts job_no;
    time_hrts last_start_point;
    time_hrts remaining_time;
    job_statue statue;
} task_statue;

typedef struct {
    time_slice_list alternate_ts;
    task_statue *task_statue;
    time_hrts current_time;

    period_task_info *task_info;
    size_t num_task;
    time_hrts cycle_length;
} statue_ptba;

statue_ptba *prepare_statue_ptba(period_task_info *ts, size_t n, time_hrts ct);

time_slice_list rm_backward(period_task_info *ts, size_t n, time_hrts);

time_slice_list rm_backward_from_task_list(task_list_rm ts, time_hrts current_time, time_hrts cycle_length);

void cancel_n_adjust_alternate(time_slice_list, task_hrts, period_task_info *);

#endif //HRTSCHEDULING_PTBA_H

