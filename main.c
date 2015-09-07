#include <stdio.h>
#include <gc/gc.h>

#include "scheduler.h"
#include "ptba.h"

int main(void) {
    period_task_info ts[] = {{10, 4, 3},
                             {8,  3, 2}};
    statue_ptba *statue = prepare_statue_ptba(ts, 2);
    time_hrts ct = 0, ct2 = 0;
    action_schedule *action = GC_MALLOC(sizeof(action_schedule));

    while (ct >= 0) {
        ct2 = schedule_ptba(statue, ct, REASON_SCHEDULE_POINT, action);
        printf("ct:%ld, action:[%ld->%u]\n", ct, action->task_no, action->action);
        if (ct == 16) {
            ct2 = schedule_ptba(statue, 18, REASON_PRIMARY_CRASHED, action);
            printf("ct:18, action:[%ld->%u]\n", action->task_no, action->action);
        }
        ct = ct2;
    }

    return 0;
}