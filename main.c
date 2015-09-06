#include <stdio.h>
#include <gc/gc.h>

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

void print_rt(void *tp) {
    task_rm *t = tp;
    printf("%ld-%ld:%ld/%ld/%ld/%s", task_no(t->tid), job_no(t->tid), t->period, t->st, t->rt,
           t->paused ? "TRUE" : "FALSE");
}

int main(void) {
    period_task_info ts[] = {{24, 14, 2},
                             {17, 6,  2},
                             {10, 8,  2}};
    statue_ptba *statue = prepare_statue_ptba(ts, 3);
    time_hrts ct = 0;
    action_schedule *action = GC_MALLOC(sizeof(action_schedule));

    //print_statue(statue);

    while (ct >= 0) {
        ct = schedule_ptba(statue, ct, REASON_SCHEDULE_POINT, action);
        //print_statue(statue);
        printf("ct:%ld, action:[%ld->%u]\n", ct, action->task_no, action->action);
    }

    return 0;
}