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
#define PTBA_ALTERNATE_PRIMARY_CANCELLED 0x80

typedef struct {
    time_hrts start;
    time_hrts end;
    task_hrts task_id;
    ptba_time_statue statue;
} time_slice;

#define ts_data(t) ((time_slice *) data_of(t))

typedef list *time_slice_list;

time_slice_list rm_backward(period_task_info *ts, size_t n);

time_slice_list rm_backward_from_task_list(task_list_rm ts, time_hrts current_time, time_hrts cycle_length);

void cancel_n_adjust_alternate(time_slice_list, task_hrts, period_task_info *);

#endif //HRTSCHEDULING_PTBA_H

