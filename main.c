#include <stdio.h>
#include <gc/gc.h>
#include <unistd.h>

#include "scheduler.h"
#include "ptba.h"

int main(void) {
    GC_INIT();

//    period_task_info ts[] = {{10, 4, 3},
//                             {8,  3, 2}};

//    period_task_info ts[] = {{15, 3, 2},
//                             {16, 4, 3},
//                             {20, 4, 4},
//                             {21, 7, 4}};

    period_task_info ts[] = {{13,  3,  2},
                             {24,  7,  3},
                             {39,  9,  7},
                             {144, 23, 17}};

    time_hrts current_time, next_time;

    statue_ptba *statue = prepare_statue_ptba(ts, 4);
    statue_ptba *s = NULL;
    action_schedule *action = GC_MALLOC_ATOMIC(sizeof(action_schedule));

    for (size_t i = 0; i < 1; i++) {
        s = copy_statue(statue);
        next_time = 0;
        do {
            current_time = next_time;
            next_time = schedule_ptba(s, current_time, REASON_SCHEDULE_POINT, action);
            printf(">>>> ct:%ld, action:[%ld->%u]\n", current_time, action->task_no, action->action);
        } while (action->action != ACTION_FINISH);
    }
    return 0;
}