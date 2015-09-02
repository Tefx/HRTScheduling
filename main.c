#include <stdio.h>

#include "scheduler.h"
#include "ratemono.h"
#include "ptba.h"

void print_ev(void *ep) {
    event_rm *ev = (event_rm *) ep;
    if (ev->event == RM_EVENT_STARTED)
        printf("[%ld][S %ld-%ld]", ev->time, task_no(ev->task), job_no(ev->task));
    else if (ev->event == RM_EVENT_RESUMED)
        printf("[%ld][R %ld-%ld]", ev->time, task_no(ev->task), job_no(ev->task));
    else if (ev->event == RM_EVENT_RESUMED)
        printf("[%ld][P %ld-%ld]", ev->time, task_no(ev->task), job_no(ev->task));
    else
        printf("[%ld][F %ld-%ld]", ev->time, task_no(ev->task), job_no(ev->task));
}

void print_ts(void *tp) {
    time_slice *ts = tp;
    printf("[%ld-%ld][%ld-%ld/%d]", ts->start, ts->end, task_no(ts->task_id), job_no(ts->task_id), ts->statue);
}

void print_rt(void *tp) {
    task_rm *t = tp;
    printf("%ld-%ld:%ld/%ld/%ld/%s", task_no(t->tid), job_no(t->tid), t->period, t->st, t->rt,
           t->paused ? "TRUE" : "FALSE");
}

int main(void) {
    period_task_info ts[] = {{30, 15, 15},
                             {50, 20, 20}};
    time_hrts cl = cycle_length(ts, 2);
    time_slice_list tsl = rm_backward(ts, 2, cl);

    print_list(tsl, print_ts);
    cancel_n_adjust_alternate(tsl, task_id(0, 2), ts);
    print_list(tsl, print_ts);

    return 0;
}