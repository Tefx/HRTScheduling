//
// Created by tefx on 8/25/15.
//

#include <stdio.h>
#include <gc/gc.h>
#include "ratemono.h"

int compare_start_time(const void *x, const void *y) {
    return COMPARE(x, y, task_rm, st);
}

int compare_priority(const void *x, const void *y) {
    return COMPARE(x, y, task_rm, period);
}

schedule_status_rm *init_status_rm(task_list_rm ts) {
    schedule_status_rm *ss = GC_MALLOC(sizeof(schedule_status_rm));
    list_node *tmp = head_of(ts);

    ss->waiting_queue = new_list();
    ss->available_queue = new_list();
    ss->current_running = NULL;

    while (tmp) {
        insert_ordered(ss->waiting_queue, data_of(tmp), compare_start_time);
        tmp = tmp->next;
    }

    return ss;
}

void move_tasks_wq2aq(schedule_status_rm *ss, time_hrts ct) {
    task_rm *tmp = first(ss->waiting_queue);

    while (tmp && tmp->st <= ct) {
        insert_ordered(ss->available_queue, pop(ss->waiting_queue), compare_priority);
        tmp = first(ss->waiting_queue);
    }
}

time_hrts calculate_next_point(schedule_status_rm *ss, time_hrts ct) {
    task_rm *first_waiting = first(ss->waiting_queue);

    if (first_waiting) {
        if (!ss->current_running) {
            ct = first_waiting->st;
        } else if (ss->current_running->rt + ct > first_waiting->st) {
            ss->current_running->rt -= first_waiting->st - ct;
            ct = first_waiting->st;
        } else {
            ct += ss->current_running->rt;
            ss->current_running->rt = 0;
        }
    } else if (ss->current_running) {
        ct += ss->current_running->rt;
        ss->current_running->rt = 0;
    } else
        ct = -1;
    return ct;
}

time_hrts schedule_rm(schedule_status_rm *ss, list *es, time_hrts ct) {
    event_rm *ev;
    task_rm *first_avail;

    move_tasks_wq2aq(ss, ct);

    if (ss->current_running && ss->current_running->rt <= 0) {
        ev = GC_MALLOC(sizeof(event_rm));
        ev->event = RM_EVENT_FINISHED;
        ev->task = ss->current_running->tid;
        ev->time = ct;
        push(es, ev);
        ss->current_running = NULL;
    }

    if ((first_avail = (task_rm *) first(ss->available_queue))) {
        if (ss->current_running) {
            if (ss->current_running->period > first_avail->period) {
                ev = GC_MALLOC(sizeof(event_rm));
                ev->event = RM_EVENT_PAUSED;
                ev->task = ss->current_running->tid;
                ev->time = ct;
                push(es, ev);
                ss->current_running->paused = true;
                insert_ordered(ss->available_queue, ss->current_running, compare_priority);
            } else {
                goto finish;
            }
        }

        ss->current_running = pop(ss->available_queue);
        ev = GC_MALLOC(sizeof(event_rm));
        if (ss->current_running->paused)
            ev->event = RM_EVENT_RESUMED;
        else
            ev->event = RM_EVENT_STARTED;
        ev->task = ss->current_running->tid;
        ev->time = ct;
        push(es, ev);
    }

    finish:
    return calculate_next_point(ss, ct);
}

task_list_rm alternate_to_backward_task_rm(period_task_info *ts, size_t n, time_hrts cl) {
    task_list_rm tl = new_list();
    task_rm *task;

    for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < cl / ts[i].period; j++) {
            task = GC_MALLOC(sizeof(task_rm));
            task->tid = task_id(i, j);
            task->period = ts[i].period;
            task->rt = ts[i].alternate_time;
            task->st = cl - (j + 1) * ts[i].period;
            task->paused = false;
            push_right(tl, task);
        }
    return tl;
}

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