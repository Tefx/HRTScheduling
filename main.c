#include <stdio.h>
#include <gc/gc.h>
#include "def.h"
#include "ptba.h"
#include "schedule.h"
#include "parse.h"

void test_alg(task_info *tis, task_hrts n) {
    time_hrts current_time, next_time;

    statue_ptba *statue = prepare_statue_ptba(tis, n);
    statue_ptba *s = NULL;
    action_type *action = GC_MALLOC_ATOMIC(sizeof(action_type));

    s = copy_statue(statue);
    next_time = 0;
    do {
        current_time = next_time;
        next_time = schedule_ptba(s, current_time, REASON_SCHEDULE_POINT, action);
        printf(">>>> ct:%ld, action:[%ld->%u]\n", current_time, (long) action->task_no, action->action);
    } while (action->action != ACTION_FINISH);
}

int main(int argc, char **argv) {
    GC_INIT();
    GC_enable_incremental();

    task_hrts num;
    task_info *tis = parse_config(argv[1], &num);

    start_scheduling(tis, num);
}
