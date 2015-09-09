//
// Created by tefx on 8/26/15.
//

#include <gc/gc.h>
#include <stdio.h>
#include <string.h>
#include "ptba.h"

time_slice_list build_alternate_list(list *es, time_hrts cl, time_hrts start_time, time_hrts end_time) {
    event_rm *ev = pop(es);
    time_slice_list ts = new_list();
    time_slice *cur = GC_MALLOC_ATOMIC(sizeof(time_slice));

    cur->start = cl - end_time;
    cur->task_id = -1;
    cur->statue = PTBA_FREE;

    while (ev) {
        cur->end = cl - ev->time;

        if (cur->start != cur->end) {
            if (ev->event == RM_EVENT_STARTED)
                cur->statue |= PTBA_ALTERNATE_FINISH;
            else if (ev->event == RM_EVENT_RESUMED)
                cur->statue |= PTBA_ALTERNATE_PAUSE;
            push_right(ts, cur);
        }

        cur = GC_MALLOC_ATOMIC(sizeof(time_slice));
        cur->start = cl - ev->time;
        cur->statue = PTBA_EMPTY;

        switch (ev->event) {
            case RM_EVENT_STARTED:
            case RM_EVENT_RESUMED:
                cur->task_id = -1;
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

    if (cur->start != cl - start_time) {
        cur->end = cl - start_time;
        push_right(ts, cur);
    }

    return ts;
}

bool captured_by_task(const void *t, const void *id) {
    return (((time_slice *) t)->task_id == *((task_hrts *) id));
}

list_node *cancel_alternate(time_slice_list l, task_hrts tid) {
    list_node *t = head_of(l);

    while ((t = find_node_r(t, captured_by_task, &tid))) {

        if (t->prev && (ts_data(t->prev)->statue & PTBA_FREE)) {
            ts_data(t)->start = ts_data(t->prev)->start;
            delete_node(l, t->prev);
        }

        if (t->next && (ts_data(t->next)->statue & PTBA_FREE)) {
            ts_data(t)->end = ts_data(t->next)->end;
            delete_node(l, t->next);
        }

        ts_data(t)->task_id = -1;
        //ts_data(t)->statue = PTBA_FREE;

        if (ts_data(t)->statue & PTBA_ALTERNATE_FINISH) {
            ts_data(t)->statue = PTBA_FREE;
            return t;
        } else {
            ts_data(t)->statue = PTBA_FREE;
        }

    };

    return t;
}

bool tid_eq(const void *t, const void *id) {
    return (((task_rm *) t)->tid == *((task_hrts *) id));
}

task_list_rm pop_alternates_before(time_slice_list l, list_node *node, period_info_ptba *task_info,
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
            if (ts_data(node)->statue & PTBA_ALTERNATE_START)
                ((task_rm *) data_of(tn))->paused = false;
        } else {
            task = GC_MALLOC_ATOMIC(sizeof(task_rm));
            task->tid = ts_data(node)->task_id;
            task->rt = ts_data(node)->end - ts_data(node)->start;
            task->period = task_info[task_no(ts_data(node)->task_id)].period;
            ft = task->period * (job_no(ts_data(node)->task_id) + 1);
            if (ft > max_ft) ft = max_ft;
            task->st = cycle_length - ft;

            if (ts_data(node)->statue & PTBA_ALTERNATE_FINISH)
                task->resumed = false;
            else
                task->resumed = true;

            if (ts_data(node)->statue & PTBA_ALTERNATE_START)
                task->paused = false;
            else
                task->paused = true;


            push(ts, task);
        }

        next_loop:
        tmp = node;
        node = node->prev;
        delete_node(l, tmp);
    }

    return ts;
}

time_slice_list rm_backward_from_task_list(task_list_rm ts, time_hrts cl, time_hrts ct, time_hrts end_time) {
    schedule_status_rm *ss = init_status_rm(ts);
    event_list_rm es = new_list();
    time_hrts start_time = ct;

    while ((ct = schedule_rm(ss, es, ct)) >= 0);
    return build_alternate_list(es, cl, start_time, end_time);
}

void cancel_n_adjust_alternate(time_slice_list l, task_hrts tid, period_info_ptba *ts) {
    time_hrts start_time = ts_data(head_of(l))->start;
    time_hrts cl = ts_data(tail_of(l))->end;
    list_node *cancelled = cancel_alternate(l, tid);
    task_list_rm ts_before = pop_alternates_before(l, cancelled, ts, cl);
    time_hrts end_time = cancelled ? ts_data(cancelled)->end : ts_data(head_of(l))->end;
    time_slice_list l_before = rm_backward_from_task_list(ts_before, cl, cl - end_time, cl - start_time);

    if (not_empty(l_before) && not_empty(l) &&
        ts_data(tail_of(l_before))->task_id == ts_data(head_of(l))->task_id) {
        ts_data(head_of(l))->start = ts_data(tail_of(l_before))->start;
        if (ts_data(tail_of(l_before))->statue & PTBA_ALTERNATE_START) {
            ts_data(head_of(l))->statue &= ~PTBA_ALTERNATE_RESUME;
            ts_data(head_of(l))->statue |= PTBA_ALTERNATE_START;
        }
        delete_node(l_before, tail_of(l_before));
    }
    concat_before(l, l_before);
}

time_slice_list rm_backward(period_info_ptba *ts, task_hrts n, time_hrts cl) {
    task_list_rm tl = alternate_to_backward_task_rm(ts, n, cl);
    return rm_backward_from_task_list(tl, cl, 0, cl);
}

task_list_rm alternate_to_backward_task_rm(period_info_ptba *ts, task_hrts n, time_hrts cl) {
    task_list_rm tl = new_list();
    task_rm *task;

    for (task_hrts i = 0; i < n; i++)
        for (task_hrts j = 0; j < cl / ts[i].period; j++) {
            task = GC_MALLOC_ATOMIC(sizeof(task_rm));
            task->tid = task_id(i, j);
            task->period = ts[i].period;
            task->rt = ts[i].alternate_time;
            task->st = cl - (j + 1) * ts[i].period;
            task->paused = false;
            task->resumed = false;
            push_right(tl, task);
        }
    return tl;
}

statue_ptba *prepare_statue_ptba(task_info *ts, task_hrts n) {
    statue_ptba *statue = GC_MALLOC(sizeof(statue_ptba));
    statue->task_info = GC_MALLOC_ATOMIC(sizeof(period_info_ptba) * n);

    for (time_hrts i = 0; i < n; i++) {
        statue->task_info[i].alternate_time = ts[i].alternate_time;
        statue->task_info[i].primary_time = ts[i].primary_time;
        statue->task_info[i].period = ts[i].period;
    }

    statue->num_task = n;
    statue->cycle_length = cycle_length(ts, n);

    statue->alternate_ts = rm_backward(statue->task_info, n, statue->cycle_length);

    statue->last_scheduling_point = 0;
    statue->current_running = -1;

    statue->predict_table = NULL;
    statue->pt_task = -1;

    statue->alter_orig = NULL;

    statue->task_statue = GC_MALLOC_ATOMIC(sizeof(task_statue_ptba) * n);
    for (task_hrts i = 0; i < n; i++) {
        statue->task_statue[i].job = 0;
        statue->task_statue[i].remaining_time = ts[i].primary_time;
        statue->task_statue[i].statue = JOB_STATUE_RELEASED;
    }

    return statue;
}

statue_ptba *copy_statue(statue_ptba *s) {
    statue_ptba *s1 = GC_MALLOC(sizeof(statue_ptba));
    s1->task_info = GC_MALLOC_ATOMIC(sizeof(period_info_ptba) * s->num_task);
    memcpy(s1->task_info, s->task_info, sizeof(period_info_ptba) * s->num_task);
    s1->num_task = s->num_task;
    s1->cycle_length = s->cycle_length;
    s1->alternate_ts = copy_list(s->alternate_ts, sizeof(time_slice));
    s1->last_scheduling_point = 0;
    s1->current_running = -1;
    s1->predict_table = NULL;
    s1->pt_task = -1;
    s1->alter_orig = NULL;
    s1->task_statue = GC_MALLOC_ATOMIC(sizeof(task_statue_ptba) * s1->num_task);
    for (task_hrts i = 0; i < s1->num_task; i++) {
        s1->task_statue[i].job = 0;
        s1->task_statue[i].remaining_time = s1->task_info[i].primary_time;
        s1->task_statue[i].statue = JOB_STATUE_RELEASED;
    }

    return s1;
}


bool availability_for_primary(statue_ptba *s, time_slice_list tsl, task_hrts tid) {
    list_node *tmp = head_of(tsl);
    time_slice *t = NULL;
    task_hrts t_no = task_no(tid);
    task_hrts j_no = job_no(tid);
    time_hrts needed_time;
    time_hrts release_time = s->task_info[t_no].period * job_no(tid);

    if (j_no == s->task_statue[t_no].job)
        needed_time = s->task_statue[t_no].remaining_time;
    else
        needed_time = s->task_info[t_no].primary_time;

    time_hrts count_time = needed_time;
    while (tmp && ts_data(tmp)->task_id != tid) {
        if (ts_data(tmp)->statue & PTBA_FREE) {
            if (ts_data(tmp)->start < release_time) {
                if (ts_data(tmp)->end > release_time)
                    count_time -= ts_data(tmp)->end - release_time;
            } else {
                count_time -= ts_data(tmp)->end - ts_data(tmp)->start;
            }
            if (count_time <= 0) break;
        }
        tmp = tmp->next;
    }
    if (count_time > 0) return false;

    tmp = head_of(tsl);
    while (needed_time > 0) {
        if (ts_data(tmp)->statue & PTBA_FREE) {
            if (ts_data(tmp)->start < release_time) {
                if (ts_data(tmp)->end > release_time) {
                    t = GC_MALLOC_ATOMIC(sizeof(time_slice));
                    t->start = release_time;
                    t->end = ts_data(tmp)->end;
                    t->task_id = tid;
                    t->statue = PTBA_PRIMARY_USED;
                    ts_data(tmp)->end = release_time;
                    insert_after(tsl, tmp, t);
                    tmp = tmp->next;
                    needed_time -= t->end - t->start;
                }
            } else {
                ts_data(tmp)->task_id = tid;
                ts_data(tmp)->statue = PTBA_PRIMARY_USED;
                needed_time -= ts_data(tmp)->end - ts_data(tmp)->start;
            }

            if (needed_time < 0) {
                t = GC_MALLOC_ATOMIC(sizeof(time_slice));
                t->end = ts_data(tmp)->end;
                t->start = t->end + needed_time;
                t->statue = PTBA_FREE;
                t->task_id = -1;
                ts_data(tmp)->end = t->start;
                insert_after(tsl, tmp, t);
                needed_time = 0;
            }
        }
        tmp = tmp->next;
    }
    return true;
}

bool not_terminated(statue_ptba *statue, task_hrts tid) {
    time_hrts t_no = task_no(tid);

    if (job_no(tid) == statue->task_statue[t_no].job &&
            statue->task_statue[t_no].remaining_time == JOB_STATUE_PRIMARY_TERMINATED)
        return false;
    else
        return true;
}

bool predict(statue_ptba *statue, task_hrts tid) {
    time_slice_list tsl = copy_until_r(statue->alternate_ts, captured_by_task, sizeof(time_slice), &tid);
    list_node *tmp = head_of(tsl);

    while (tmp) {
        if (ts_data(tmp)->statue & PTBA_ALTERNATE_START) {
            if (not_terminated(statue, ts_data(tmp)->task_id))
                availability_for_primary(statue, tsl, ts_data(tmp)->task_id);
        }
        tmp = tmp->next;
    }

    if (availability_for_primary(statue, tsl, tid)) {
        statue->predict_table = tsl;
        statue->pt_task = tid;
        return true;
    } else {
        statue->predict_table = NULL;
        statue->pt_task = -1;
        return false;
    }
}

void adjust_released(statue_ptba *statue, time_hrts current_time) {
    time_hrts tmp_release_time;
    for (task_hrts i = 0; i < statue->num_task; i++) {
        if (statue->task_statue[i].statue == JOB_STATUE_FINISHED) {
            tmp_release_time = statue->task_info[i].period * (statue->task_statue[i].job + 1);
            if (tmp_release_time <= current_time) {
                statue->task_statue[i].job++;
                statue->task_statue[i].remaining_time = statue->task_info[i].primary_time;
                statue->task_statue[i].statue = JOB_STATUE_RELEASED;
            }
        }
    }
}

task_hrts find_task_with_earliest_notice_time(statue_ptba *statue) {
    job_statue_ptba job_statue;
    task_hrts tmp_tid;
    list_node *tmp;
    time_hrts least_notice_time = statue->cycle_length + 1;
    task_hrts selected_next_primary = -1;

    for (task_hrts i = 0; i < statue->num_task; i++) {
        job_statue = statue->task_statue[i].statue;
        if (job_statue == JOB_STATUE_RELEASED) {
            tmp_tid = task_id(i, statue->task_statue[i].job);
            tmp = find_node_r(head_of(statue->alternate_ts), captured_by_task, &tmp_tid);
            if (ts_data(tmp)->start <= least_notice_time) {
                least_notice_time = ts_data(tmp)->start;
                selected_next_primary = tmp_tid;
            }
        }
    }

    return selected_next_primary;
}

time_hrts schedule_ptba(statue_ptba *statue, time_hrts current_time, schedule_reason reason,
                        action_type *action) {

    GC_gcollect();

    action->action = ACTION_FINISH;
    action->task_no = -1;



    if (strip_time(statue->alternate_ts, current_time) == 0) {
        statue->current_running = -1;
        return -1;
    }

    if (statue->current_running >= 0) {
        task_statue_ptba *cur_task_statue = statue->task_statue + task_no(statue->current_running);
        if (reason != REASON_PRIMARY_CRASHED) {
            if (cur_task_statue->statue == JOB_STATUE_ALTERNATE_ADV_RUNNING) {
                clean_EIT(statue, current_time);
            } else {
                cur_task_statue->remaining_time -= current_time - statue->last_scheduling_point;
                if (cur_task_statue->remaining_time <= 0) {
                    if (cur_task_statue->statue == JOB_STATUE_PRIMARY_RUNNING) {
                        cancel_n_adjust_alternate(statue->alternate_ts, statue->current_running, statue->task_info);
                    }
                    cur_task_statue->statue = JOB_STATUE_FINISHED;
                }
            }
        }
    }

    adjust_released(statue, current_time);

    if (statue->predict_table)
        return schedule_ptba_PT(statue, current_time, reason, action);
    else
        return schedule_ptba_nonPT(statue, current_time, reason, action);
}

time_hrts schedule_ptba_nonPT(statue_ptba *statue, time_hrts current_time, schedule_reason reason,
                              action_type *action) {
    time_slice *first_slice = first(statue->alternate_ts);

    if (reason == REASON_PRIMARY_CRASHED) {
        task_statue_ptba *cur_task_statue = statue->task_statue + task_no(statue->current_running);
        cur_task_statue->statue = JOB_STATUE_PRIMARY_TERMINATED;
        cur_task_statue->remaining_time = 0;
    }

    if (!(first_slice->statue & PTBA_FREE)) {
        task_statue_ptba *cur_task_statue = statue->task_statue + task_no(first_slice->task_id);
        statue->current_running = first_slice->task_id;
        action->task_no = task_no(first_slice->task_id);

        if (first_slice->statue & PTBA_ALTERNATE_START) {
            action->action |= ACTION_START_ALTERNATE;
            cur_task_statue->remaining_time = statue->task_info[task_no(first_slice->task_id)].alternate_time;
            if (cur_task_statue->statue & JOB_STATUE_PRIMARY_RUNNING)
                action->action |= ACTION_CANCEL_PRIMARY;
            cur_task_statue->statue = JOB_STATUE_ALTERNATE_RUNNING;
        } else
            action->action |= ACTION_RESUME_ALTERNATE;

        statue->last_scheduling_point = current_time;
        return first_slice->end;
    } else {
        task_hrts selected_next_primary;

        while (1) {
            selected_next_primary = find_task_with_earliest_notice_time(statue);
            if (selected_next_primary < 0) break;

            if (predict(statue, selected_next_primary)) {
                statue->pt_task = selected_next_primary;
                return schedule_ptba_PT(statue, current_time, REASON_SCHEDULE_POINT, action);
            } else {
                task_statue_ptba *cur_task_statue = statue->task_statue + task_no(selected_next_primary);
                cur_task_statue->statue = JOB_STATUE_PRIMARY_TERMINATED;
                cur_task_statue->remaining_time = 0;
            }
        }

        try_EIT(statue, action);

        time_hrts next_schedule_time = statue->cycle_length;
        for (task_hrts i = 0; i < statue->num_task; i++) {
            if (statue->task_statue[i].statue == JOB_STATUE_FINISHED) {
                time_hrts tmp_release_time = statue->task_info[i].period * (statue->task_statue[i].job + 1);
                if (tmp_release_time < next_schedule_time)
                    next_schedule_time = tmp_release_time;
            }
        }

        first_slice = first(statue->alternate_ts);
        if (next_schedule_time > first_slice->end)
            next_schedule_time = first_slice->end;

        statue->last_scheduling_point = current_time;
        return next_schedule_time;
    }
}

time_hrts schedule_ptba_PT(statue_ptba *statue, time_hrts current_time, schedule_reason reason,
                           action_type *action) {
    time_slice *first_slice;
    first_slice = strip_time(statue->predict_table, current_time);

    if (reason == REASON_PRIMARY_CRASHED ||
            (statue->current_running == statue->pt_task &&
             statue->task_statue[task_no(statue->current_running)].statue == JOB_STATUE_FINISHED)) {
        statue->pt_task = -1;
        statue->predict_table = NULL;
        return schedule_ptba_nonPT(statue, current_time, reason, action);
    }

    if (first_slice->statue & PTBA_FREE) {
        action->task_no = -1;
        action->action = ACTION_NOTHING;
        statue->current_running = -1;
    } else {
        task_statue_ptba *cur_task_statue = statue->task_statue + task_no(first_slice->task_id);
        if (first_slice->statue & PTBA_PRIMARY_USED) {
            if (cur_task_statue->statue != JOB_STATUE_PRIMARY_RUNNING) {
                cur_task_statue->statue = JOB_STATUE_PRIMARY_RUNNING;
                action->action |= ACTION_START_PRIMARY;
            } else {
                action->action |= ACTION_RESUME_PRIMARY;
            }
        } else {
            if (first_slice->statue & PTBA_ALTERNATE_START) {
                cur_task_statue->remaining_time = statue->task_info[task_no(first_slice->task_id)].alternate_time;
                cur_task_statue->statue = JOB_STATUE_ALTERNATE_RUNNING;
                action->action |= ACTION_START_ALTERNATE;
            } else {
                action->action |= ACTION_RESUME_ALTERNATE;
            }
        }
        action->task_no = task_no(first_slice->task_id);
        statue->current_running = first_slice->task_id;
    }

    statue->last_scheduling_point = current_time;
    return first_slice->end;
}

time_slice *strip_time(time_slice_list tsl, time_hrts current_time) {
    time_slice *tmp_slice = first(tsl);

    while (tmp_slice && tmp_slice->end <= current_time) {
        pop(tsl);
        tmp_slice = first(tsl);
    }

    if (tmp_slice && tmp_slice->start < current_time)
        tmp_slice->start = current_time;

    return tmp_slice;
}

bool try_EIT(statue_ptba *statue, action_type *action) {
    task_hrts tmp = -1;
    task_hrts adv_tid;
    time_hrts tmp_period = 0;
    time_hrts len_orig;
    time_slice *first_ts;
    time_hrts len_first;
    time_slice *tmp_ts;

    for (task_hrts i = 0; i < statue->num_task; i++) {
        if ((statue->task_statue[i].statue == JOB_STATUE_PRIMARY_TERMINATED ||
             statue->task_statue[i].statue == JOB_STATUE_ALTERNATE_RUNNING) &&
            statue->task_info[i].period > tmp_period) {
            tmp_period = statue->task_info[i].period;
            tmp = (task_hrts) i;
        }
    }

    if (tmp < 0) {
        action->task_no = -1;
        action->action = ACTION_NOTHING;
        statue->current_running = -1;
        return false;
    } else {
        adv_tid = task_id(tmp, statue->task_statue[tmp].job);
        statue->alter_orig = find_node_r(head_of(statue->alternate_ts), captured_by_task, &adv_tid);
        action->task_no = tmp;
        if (ts_data(statue->alter_orig)->statue & PTBA_ALTERNATE_START)
            action->action |= ACTION_START_ALTERNATE;
        else
            action->action |= ACTION_RESUME_ALTERNATE;
        statue->current_running = adv_tid;
        if (statue->task_statue[tmp].remaining_time == 0)
            statue->task_statue[tmp].remaining_time = statue->task_info[tmp].alternate_time;
        statue->task_statue[tmp].statue = JOB_STATUE_ALTERNATE_ADV_RUNNING;

        len_orig = ts_data(statue->alter_orig)->end - ts_data(statue->alter_orig)->start;
        first_ts = first(statue->alternate_ts);
        len_first = first_ts->end - first_ts->start;

        if (len_orig < len_first) {
            tmp_ts = GC_MALLOC_ATOMIC(sizeof(time_slice));
            tmp_ts->end = first_ts->end;
            tmp_ts->start = first_ts->start + len_orig;
            tmp_ts->statue = PTBA_FREE;
            tmp_ts->task_id = -1;
            first_ts->end = tmp_ts->start;
            insert_after(statue->alternate_ts, head_of(statue->alternate_ts), tmp_ts);
        }
        return true;
    }

}

void clean_EIT(statue_ptba *statue, time_hrts current_time) {
    task_statue_ptba *cur_task_statue = statue->task_statue + task_no(statue->current_running);
    time_hrts len_orig = ts_data(statue->alter_orig)->end - ts_data(statue->alter_orig)->start;
    time_hrts len_used = current_time - statue->last_scheduling_point;
    time_slice *tmp = NULL;

    cur_task_statue->remaining_time -= len_used;
    if (cur_task_statue->remaining_time <= 0) {
        cur_task_statue->statue = JOB_STATUE_FINISHED;
        cancel_n_adjust_alternate(statue->alternate_ts, statue->current_running, statue->task_info);
    } else {
        cur_task_statue->statue = JOB_STATUE_ALTERNATE_RUNNING;
        if (len_orig > len_used) {
            if (statue->alter_orig->prev && ts_data(statue->alter_orig->prev)->statue & PTBA_FREE) {
                ts_data(statue->alter_orig->prev)->end += len_used;
                ts_data(statue->alter_orig)->start += len_used;
            } else {
                tmp = GC_MALLOC_ATOMIC(sizeof(time_slice));
                tmp->start = ts_data(statue->alter_orig)->start;
                tmp->end = tmp->start + len_used;
                tmp->task_id = -1;
                tmp->statue = PTBA_FREE;
                ts_data(statue->alter_orig)->start = tmp->end;
                insert_before(statue->alternate_ts, statue->alter_orig, tmp);
            }
            ts_data(statue->alter_orig)->statue &= ~PTBA_ALTERNATE_START;
            ts_data(statue->alter_orig)->statue |= PTBA_ALTERNATE_RESUME;
        } else {
            ts_data(statue->alter_orig)->statue = PTBA_FREE;
            ts_data(statue->alter_orig)->task_id = -1;
        }
    }
    statue->alter_orig = NULL;
}

void print_ts(void *tp) {
    time_slice *ts = tp;
    printf("[%ld-%ld][%ld-%ld/%d]", ts->start, ts->end, task_no(ts->task_id), job_no(ts->task_id), ts->statue);
}

void print_statue(statue_ptba *statue, size_t count) {
    printf("AT: ");
    print_list(statue->alternate_ts, print_ts, count);
    printf("PT: ");
    if (statue->predict_table)
        print_list(statue->predict_table, print_ts, count);
    else
        printf("NULL\n");

    printf("TS: ");

    for (task_hrts i = 0; i < statue->num_task; i++) {
        printf("<[%ld]%ld/%ld/%u>", i, statue->task_statue[i].job, statue->task_statue[i].remaining_time,
               statue->task_statue[i].statue);
    }
    printf("\n");
}