//
// Created by tefx on 8/27/15.
//

#include "scheduler.h"

time_hrts gcd(time_hrts m, time_hrts n) {
    time_hrts tmp;
    while (m) {
        tmp = m;
        m = n % m;
        n = tmp;
    }
    return n;
}

time_hrts lcm(time_hrts m, time_hrts n) {
    return m / gcd(m, n) * n;
}

time_hrts cycle_length(period_task_info *ts, task_hrts n) {
    time_hrts cycle_length = 1;

    for (task_hrts i = 0; i < n; i++)
        cycle_length = lcm(cycle_length, ts[i].period);

    return cycle_length;
}

task_hrts task_id(task_hrts x, task_hrts y) {
    return (x << 32 | y);
}