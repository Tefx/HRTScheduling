//
// Created by tefx on 8/26/15.
//

#include <gc/gc.h>
#include <stdio.h>
#include "ptba.h"

time_slice_list build_alternate_list(list *es, time_hrts cycle_length) {
    event_rm *ev = pop(es);
    time_slice_list ts = new_list();
    time_slice *cur = GC_MALLOC(sizeof(time_slice));

    cur->start = 0;
    cur->task_id = 0;
    cur->statue = PTBA_FREE;

    while (ev) {
        cur->end = cycle_length - ev->time;

        if (cur->start != cur->end) {
            if (ev->event == RM_EVENT_STARTED)
                cur->statue |= PTBA_ALTERNATE_FINISH;
            else if (ev->event == RM_EVENT_RESUMED)
                cur->statue |= PTBA_ALTERNATE_PAUSE;
            push_right(ts, cur);
        }

        cur = GC_MALLOC(sizeof(time_slice));
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

    while ((t = find_node_r(t, captured_by_task, &tid))) {
        if (t->prev && ts_data(t->prev)->statue & PTBA_FREE)
            ts_data(t)->start = ts_data(t->prev)->start;

        if (t->next && ts_data(t->next)->statue & PTBA_FREE)
            ts_data(t)->end = ts_data(t->next)->end;

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

        if ((tn = find_node_r(head_of(ts), tid_eq, &(ts_data(node)->task_id)))) {
            ((task_rm *) data_of(tn))->rt += ts_data(node)->end - ts_data(node)->start;
        } else {
            task = GC_MALLOC(sizeof(task_rm));
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

    while ((ct = schedule_rm(ss, es, ct)) >= 0);
    return build_alternate_list(es, cl);
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
    }
    concat_before(l, l_before);
}

time_slice_list rm_backward(period_task_info *ts, size_t n, time_hrts cl) {
    task_list_rm tl = alternate_to_backward_task_rm(ts, n, cl);
    return rm_backward_from_task_list(tl, 0, cl);
}

statue_ptba *prepare_statue_ptba(period_task_info *ts, size_t n, time_hrts ct) {
    statue_ptba *statue = GC_MALLOC(sizeof(statue_ptba));
    statue->current_time = ct;
    statue->task_info = ts;
    statue->num_task = n;
    statue->cycle_length = cycle_length(ts, n);
    statue->alternate_ts = rm_backward(ts, n, statue->cycle_length);
    statue->task_statue = GC_MALLOC_ATOMIC(sizeof(task_statue_ptba) * n);
    statue->adv_orig = 0;
    statue->last_scheduling_point = 0;
    statue->current_running = -1;

    for (size_t i = 0; i < n; i++) {
        statue->task_statue[i].job = 0;
        statue->task_statue[i].remaining_time = ts[i].running_time;
        statue->task_statue[i].statue = JOB_STATUE_FINISHED;
    }

    return statue;
}

bool before_notice_time(const void *ts, const void *tid) {
    return ((time_slice *) ts)->task_id == *((task_hrts *) tid);
}

bool test_availablity_for_primary(statue_ptba *s, time_slice_list tsl, task_hrts tid) {
    list_node *tmp = head_of(tsl);
    time_slice *t = NULL;
    time_hrts needed_time = s->task_statue[task_no(tid)].remaining_time;
    time_hrts release_time = s->task_info[task_no(tid)].period * job_no(tid);

    while (tmp && ts_data(tmp)->task_id != tid) {
        if (ts_data(tmp)->statue & PTBA_FREE) {
            if (ts_data(tmp)->start < release_time) {
                if (ts_data(tmp)->end > release_time)
                    needed_time -= ts_data(tmp)->end - release_time;
            } else {
                needed_time -= ts_data(tmp)->end - ts_data(tmp)->start;
            }
            if (needed_time <= 0) break;
        }
        tmp = tmp->next;
    }

    if (needed_time > 0) return false;

    tmp = head_of(tsl);
    needed_time = s->task_statue[task_no(tid)].remaining_time;

    while (needed_time > 0) {
        if (ts_data(tmp)->statue & PTBA_FREE) {
            if (ts_data(tmp)->start < release_time) {
                if (ts_data(tmp)->end > release_time) {
                    t = GC_MALLOC(sizeof(time_slice));
                    t->start = release_time;
                    t->end = ts_data(tmp)->end;
                    t->task_id = tid;
                    t->statue = PTBA_PRIMARY_USED;
                    ts_data(tmp)->end = release_time;
                    insert_after(tsl, tmp, t);
                    tmp = tmp->next;
                    needed_time -= t->end - t->start;
                } else {

                }
            } else {
                ts_data(tmp)->task_id = tid;
                ts_data(tmp)->statue = PTBA_PRIMARY_USED;
            }

            if (needed_time < 0) {
                t = GC_MALLOC(sizeof(time_slice));
                t->end = ts_data(tmp)->end;
                t->start = t->end + needed_time;
                t->statue = PTBA_FREE;
                t->task_id = 0;
                insert_after(tsl, tmp, t);
                needed_time = 0;
            }
        }
        tmp = tmp->next;
    }
    return true;
}

time_slice_list predict(statue_ptba *statue, task_hrts tid) {
    time_slice_list tsl = copy_until_r(statue->alternate_ts, before_notice_time, sizeof(time_slice), &tid);
    list_node *tmp = head_of(tsl);
    job_statue_ptba jstatue;

    while (tmp) {
        if (ts_data(tmp)->statue & PTBA_ALTERNATE_START) {
            jstatue = statue->task_statue[task_no(ts_data(tmp)->task_id)].statue;
            if (jstatue == JOB_STATUE_FINISHED)
                test_availablity_for_primary(statue, tsl, ts_data(tmp)->task_id);
        }
        tmp = tmp->next;
    }

    if (test_availablity_for_primary(statue, tsl, tid))
        return tsl;
    else
        return NULL;
}

void action(task_statue_ptba *task, job_statue_ptba js) {
    task->statue = js;

    switch (js) {
        case JOB_STATUE_PRIMARY_STARTED_OR_RESUMED:
            printf("JOB_STATUE_PRIMARY_STARTED_OR_RESUMED");
            break;
        case JOB_STATUE_ALTERNATE_STARTED_OR_RESUMED:
            printf("JOB_STATUE_ALTERNATE_STARTED_OR_RESUMED");
            break;
        case JOB_STATUE_PRIMARY_PAUSED:
            printf("JOB_STATUE_PRIMARY_PAUSED");
            break;
        case JOB_STATUE_ALTERNATE_PAUSED:
            printf("JOB_STATUE_ALTERNATE_PAUSED");
            break;
        case JOB_STATUE_PRIMARY_CANCELLED:
            printf("JOB_STATUE_PRIMARY_CANCELLED");
            break;
        default:
            break;
    }
}

time_hrts schedule_ptba(statue_ptba *statue, time_hrts current_time, schedule_reason_ptba reason) {
    job_statue_ptba jstatue;
    time_slice *tmp_slice;
    task_statue_ptba *cur_tstatue;

    //Update the time_slice_list by removing the used time
    tmp_slice = pop(statue->alternate_ts);
    if (current_time < tmp_slice->end) {
        tmp_slice->start = current_time;
        push(statue->alternate_ts, tmp_slice);
    }

    //Handle the current running task;
    if (statue->current_running >= 0) {
        cur_tstatue = statue->task_statue + task_no(statue->current_running);

        if (reason == REASON_PRIMARY_CRASHED) {
            cur_tstatue->statue = JOB_STATUE_PRIMARY_CRASHED;
            cur_tstatue->remaining_time = 0;
        } else {
            cur_tstatue->remaining_time -= current_time - statue->last_scheduling_point;
            if (cur_tstatue->statue == JOB_STATUE_PRIMARY_STARTED_OR_RESUMED) {
                if (cur_tstatue->remaining_time <= 0) {
                    cur_tstatue->statue = JOB_STATUE_FINISHED;
                    cancel_n_adjust_alternate(statue->alternate_ts, statue->current_running, statue->task_info);
                } else {
                    action(cur_tstatue, JOB_STATUE_PRIMARY_PAUSED);
                }
            } else if (cur_tstatue->statue == JOB_STATUE_ALTERNATE_STARTED_OR_RESUMED) {
                if (cur_tstatue->remaining_time <= 0) {
                    cur_tstatue->statue = JOB_STATUE_FINISHED;
                } else {
                    action(cur_tstatue, JOB_STATUE_ALTERNATE_PAUSED);
                }
            }
        }
    }

    tmp_slice = first(statue->alternate_ts);

    //if an alternate is arrived.
    if (!(tmp_slice->statue & PTBA_FREE)) {
        cur_tstatue = statue->task_statue + task_no(tmp_slice->task_id);

        if (tmp_slice->statue & PTBA_ALTERNATE_START) {
            cur_tstatue->remaining_time = statue->task_info[task_no(tmp_slice->task_id)].alternate_time;
            action(cur_tstatue, JOB_STATUE_PRIMARY_CANCELLED);
        }

        action(cur_tstatue, JOB_STATUE_ALTERNATE_STARTED_OR_RESUMED);
        statue->last_scheduling_point = current_time;
        statue->current_running = tmp_slice->task_id;
        return tmp_slice->end;
    }

    // prepare all released primaries and find the next one.
    time_hrts least_notice_time = statue->cycle_length + 1;
    task_hrts selected_next_primary = -1;
    task_hrts tmp_tid;
    time_slice_list predicted_tsl;
    list_node *tmp;

    while (1) {
        for (size_t i = 0; i < statue->num_task; i++) {
            jstatue = statue->task_statue[i].statue;

            if (jstatue == JOB_STATUE_FINISHED &&
                statue->task_info[i].period * (statue->task_statue[i].job + 1) >= current_time) {
                statue->task_statue[i].job++;
                statue->task_statue[i].remaining_time = statue->task_info[i].period;
                statue->task_statue[i].statue = JOB_STATUE_RELEASED;
            }

            if (jstatue == JOB_STATUE_RELEASED ||
                jstatue == JOB_STATUE_PRIMARY_PAUSED) {
                tmp_tid = task_id(i, statue->task_statue[i].job);
                tmp = find_node_r(head_of(statue->alternate_ts), before_notice_time, &tmp_tid);
                if (ts_data(tmp)->start <= least_notice_time) {
                    least_notice_time = ts_data(tmp)->start;
                    selected_next_primary = tmp_tid;
                }
            }
        }

        //No available primary
        if (selected_next_primary < 0) {
            //TODO: EIT algorithm.
            //TODO: calculation next schedule point.
            return 0;
        } else {
            predicted_tsl = predict(statue, selected_next_primary);
            if (predicted_tsl == NULL) {
                cur_tstatue = statue->task_statue + task_no(selected_next_primary);
                action(cur_tstatue, JOB_STATUE_PRIMARY_CANCELLED);
                continue;
            } else {
                tmp = first(predicted_tsl);
                cur_tstatue = statue->task_statue + task_no(ts_data(tmp)->task_id);
                action(cur_tstatue, JOB_STATUE_PRIMARY_STARTED_OR_RESUMED);
                statue->last_scheduling_point = current_time;
                statue->current_running = ts_data(tmp)->task_id;
                return ts_data(tmp)->end;
            }
        }
    }
}