#include <stdio.h>
#include <gc/gc.h>
#include "def.h"
#include "ptba.h"
#include "schedule.h"
#include "parse.h"

void test_alg(task_info *tis, task_hrts n) {
//    period_info_ptba ts[] = {{10, 4, 3},
//                             {8,  3, 2}};

//    period_info_ptba ts[] = {{15, 3, 2},
//                             {16, 4, 3},
//                             {20, 4, 4},
//                             {21, 7, 4}};

//    period_info_ptba ts[] = {{13,  3,  2},
//                             {24,  7,  3},
//                             {39,  9,  7},
//                             {144, 23, 17}};

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

//    char pproc_path[] = "/home/tefx/.clion11/system/cmake/generated/e9341000/e9341000/Debug/PProc";
//
//    char *primary_argv_0[] = {"0-P", "20", "0.6", NULL};
//    char *alternate_argv_0[] = {"0-A", "15", "0", NULL};
//
//    char *primary_argv_1[] = {"1-P", "15", "0.4", NULL};
//    char *alternate_argv_1[] = {"1-A", "10", "0", NULL};
//
//    char *primary_argv_2[] = {"2-P", "4", "0.2", NULL};
//    char *alternate_argv_2[] = {"2-A", "4", "0", NULL};
//
//    char *primary_argv_3[] = {"3-P", "7", "0.2", NULL};
//    char *alternate_argv_3[] = {"3-A", "4", "0", NULL};
//
//    task_info tis[] = {{pproc_path, primary_argv_0, pproc_path, alternate_argv_0, 50, 20, 15},
//                       {pproc_path, primary_argv_1, pproc_path, alternate_argv_1, 40, 15, 10},
//                       {pproc_path, primary_argv_2, pproc_path, alternate_argv_2, 20, 4,  4},
//                       {pproc_path, primary_argv_3, pproc_path, alternate_argv_3, 21, 7,  4},};

    task_hrts num;
    task_info *tis = parse_config(argv[1], &num);

    start_scheduling(tis, num);
}
