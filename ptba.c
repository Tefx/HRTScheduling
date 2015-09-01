//
// Created by tefx on 8/26/15.
//

#include <malloc.h>
#include "ptba.h"

time_slice_list build_alternate_list(list *es, time_hrts cycle_length) {
    event_rm *ev = pop(es);
    time_slice_list ts = new_list();
    time_slice *cur = malloc(sizeof(time_slice));

    cur->start = 0;
    cur->task_id = 0;
    cur->statue = PTBA_FREE;

    while (ev) {
        cur->end = cycle_length - ev->time;

        if (cur->start == cur->end)
            free(cur);
        else {
            if (ev->event == RM_EVENT_STARTED)
                cur->statue |= PTBA_ALTERNATE_FINISH;
            else if (ev->event == RM_EVENT_RESUMED)
                cur->statue |= PTBA_ALTERNATE_PAUSE;
            push_right(ts, cur);
        }

        cur = malloc(sizeof(time_slice));
        cur->start = cycle_length - ev->time;
        cur->statue = 0;

        switch (ev->event) {
            case RM_EVENT_STARTED:
            case RM_EVENT_RESUMED:
                cur->task_id = 0;
                cur->statue |= PTBA_FREE;
                break;
            case RM_EVENT_PAUSED:
                cur->task_id = ev->task;
                cur->statue |= PTBA_ALTERNATE_RESUME;
                break;
            case RM_EVENT_FINISHED:
                cur->task_id = ev->task;
                cur->statue |= PTBA_ALTERNATE_START;
                break;
        }

        ev = pop(es);
    }
    return ts;
}

bool captured_by_task(const void *t, const void *id) {
    return (((time_slice *) t)->task_id == *((task_hrts *) id));
}

list_node *cancel_alternate(time_slice_list l, task_hrts tid) {
    list_node *t = head_of(l);

    while ((t = find_node(t, captured_by_task, &tid))) {
        if (t->prev && ts_data(t->prev)->statue & PTBA_FREE) {
            ts_data(t)->start = ts_data(t->prev)->start;
            free(delete_node(l, t->prev));
        }

        if (t->next && ts_data(t->next)->statue & PTBA_FREE) {
            ts_data(t)->end = ts_data(t->next)->end;
            free(delete_node(l, t->next));
        }

        ts_data(t)->task_id = 0;

        if (ts_data(t)->statue & PTBA_ALTERNATE_FINISH) {
            ts_data(t)->statue = PTBA_FREE;
            break;
        } else {
            ts_data(t)->statue = PTBA_FREE;
        }
    };

    return t;
}

int add_alternate(time_slice_list l, task_hrts tid, period_task_info *ti) {
    time_hrts rt = ti[task_no(tid)].alternate_time;
    list_node *node = head_of(l);
    time_slice *ts = NULL;
    time_slice *new_node = NULL;
    bool started = false;

    while (node && rt > 0) {
        ts = (time_slice *) data_of(node);
        if (ts->statue & PTBA_FREE) {
            if (ts->end - ts->start > rt) {
                new_node = malloc(sizeof(time_slice));
                new_node->task_id = tid;
                new_node->start = ts->start;
                new_node->end = ts->start + rt;
                new_node->statue = PTBA_ALTERNATE_START | PTBA_ALTERNATE_FINISH;
                ts->start = ts->end - rt;
                rt = 0;
                if (node->prev == NULL)
                    push(l, new_node);
                else
                    insert_after(l, node->prev, new_node);
            } else if (ts->end - ts->start == rt) {
                ts->task_id = tid;
                ts->statue = PTBA_ALTERNATE_START | PTBA_ALTERNATE_FINISH;
            } else {
                ts->task_id = tid;
                ts->statue = PTBA_ALTERNATE_PAUSE;
                if (!started) {
                    ts->statue |= PTBA_ALTERNATE_START;
                    started = true;
                } else
                    ts->statue |= PTBA_ALTERNATE_RESUME;
                rt -= ts->end - ts->start;
            }
        }
        node = node->next;
    }

    if (node == NULL && rt > 0)
        return -1;
    else
        return 0;
}

bool tid_eq(const void *t, const void *id) {
    return (((task_rm *) t)->tid == *((task_hrts *) id));
}

task_list_rm pop_alternates_before(time_slice_list l, list_node *node, period_task_info *task_info,
                                   time_hrts cycle_length) {
    task_list_rm ts = new_list();
    list_node *tn;
    task_rm *task;
    time_hrts max_ft = (node) ? ts_data(node)->end : 0;
    time_hrts ft;
    list_node *tmp;

    while (node) {
        if (ts_data(node)->statue & PTBA_FREE)
            goto next_loop;

        if ((tn = find_node(head_of(ts), tid_eq, &(ts_data(node)->task_id)))) {
            ((task_rm *) data_of(tn))->rt += ts_data(node)->end - ts_data(node)->start;
        } else {
            task = malloc(sizeof(task_rm));
            task->tid = ts_data(node)->task_id;
            task->rt = ts_data(node)->end - ts_data(node)->start;
            task->period = task_info[task_no(ts_data(node)->task_id)].period;
            ft = task->period * (job_no(ts_data(node)->task_id) + 1);
            if (ft > max_ft) ft = max_ft;
            task->st = cycle_length - ft;

            if (ts_data(node)->statue & PTBA_ALTERNATE_PAUSE)
                task->paused = true;
            else
                task->paused = false;
            push(ts, task);
        }

        next_loop:
        tmp = node;
        node = node->prev;
        delete_node(l, tmp);
    }

    return ts;
}

time_slice_list rm_backward_from_task_list(task_list_rm ts, time_hrts ct, time_hrts cl) {
    schedule_status_rm *ss = init_status_rm(ts);
    event_list_rm es = new_list();
    time_slice_list ret;

    while ((ct = schedule_rm(ss, es, ct)) >= 0);
    ret = build_alternate_list(es, cl);

    free_status_rm(ss);
    deep_free_list(es);
    return ret;
}

void cancel_n_adjust_alternate(time_slice_list l, task_hrts tid, period_task_info *ts) {
    time_hrts cl = ts_data(tail_of(l))->end;
    list_node *cancelled = cancel_alternate(l, tid);
    task_list_rm ts_before = pop_alternates_before(l, cancelled, ts, cl);
    time_hrts ct = cl - ((cancelled) ? ts_data(cancelled)->end : 0);
    time_slice_list l_before = rm_backward_from_task_list(ts_before, ct, cl);

    if (not_empty(l_before) && not_empty(l) &&
        ts_data(tail_of(l_before))->task_id == ts_data(head_of(l))->task_id) {
        ts_data(head_of(l))->start = ts_data(tail_of(l_before))->start;
        free(pop_right(l_before));
    }

    concat_before(l, l_before);
}

time_slice_list rm_backward(period_task_info *ts, size_t n) {
    time_hrts cl = cycle_length(ts, n);
    task_list_rm tl = alternate_to_backward_task_rm(ts, n, cl);
    return rm_backward_from_task_list(tl, 0, cl);
}