//
// Created by tefx on 9/8/15.
//
#define _POSIX_SOURCE

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gc/gc.h>
#include <stdio.h>
#include <time.h>
#include "process.h"

static task_record *TR;
time_t timer = 0;

void set_signals() {
    signal(SIGCHLD, sighandler_SIGCHLD);
    signal(SIGALRM, sighandler_SIGALRM);
}

void unset_signals() {
    signal(SIGCHLD, SIG_DFL);
    signal(SIGALRM, SIG_DFL);
}

void start_scheduling(task_info *tis, task_hrts n) {
    TR = GC_MALLOC(sizeof(task_record));
    TR->info = tis;
    TR->num = n;
    TR->pids = GC_MALLOC(sizeof(pid_t) * n);
    TR->job_count = GC_MALLOC(sizeof(task_hrts) * n);
    TR->schedule_statue = prepare_statue_ptba(TR->info, TR->num);
    TR->last_point = 0;
    TR->running = -1;
    for (task_hrts i = 0; i < n; i++) {
        TR->pids[i] = 0;
        TR->job_count[i] = 0;
    }
    timer = time(NULL);
    schedule(REASON_SCHEDULE_POINT);
    set_signals();
    pause();
}

int wait_for_exit(task_hrts tid) {
    int statue;
    do {
        waitpid(TR->pids[tid], &statue, 0);
    } while (!WIFEXITED(statue));
    TR->pids[tid] = 0;
    TR->running = -1;
    return WEXITSTATUS(statue);
}

void start_primary(task_hrts tid) {
    pid_t pid;
    if ((pid = fork())) {
        TR->pids[tid] = pid;
        TR->running = tid;
    } else {
        execv(TR->info[tid].primary_path, TR->info[tid].primary_argv);
    }
}

void start_alternate(task_hrts tid) {
    pid_t pid = fork();
    if (pid) {
        TR->pids[tid] = pid;
        TR->running = tid;
    } else {
        execv(TR->info[tid].alternate_path, TR->info[tid].alternate_argv);
    }
}

int task_action(task_hrts tid, int sig) {
    return kill(TR->pids[tid], sig);
}

bool schedule(schedule_reason reason) {

    //print_statue(TR->schedule_statue, 10);

    time_t new_time = time(NULL);
    time_hrts current_time = TR->last_point + new_time - timer;
    time_hrts next_time;
    action_type *act = GC_MALLOC(sizeof(action_type));

    printf("[%lds]>>> ", current_time);

    if (TR->running >= 0) {
        printf("Pause Task %ld Job %ld & ", TR->running, TR->job_count[TR->running]);
        task_action(TR->running, SIGSTOP);
        TR->running = -1;
    }

    TR->last_point = current_time;
    next_time = schedule_ptba(TR->schedule_statue, current_time, reason, act);

    if (act->action == ACTION_FINISH) {
        printf("Scheduling finished.\n");
        return false;
    }

    if (act->action & ACTION_CANCEL_PRIMARY) {
        task_action(act->task_no, SIGKILL);
        TR->running = -1;
        TR->pids[act->task_no] = 0;
    }

    if (act->action & ACTION_START_OR_RESUME_PRIMARY) {
        if (TR->pids[act->task_no] > 0) {
            task_action(act->task_no, SIGCONT);
            TR->running = act->task_no;
            printf("Resume Task %ld Job %ld (primary).\n", TR->running, TR->job_count[TR->running]);
        } else {
            start_primary(act->task_no);
            printf("Start Task %ld Job %ld (primary).\n", TR->running, TR->job_count[TR->running]);
        }
    } else if (act->action & ACTION_START_OR_RESUME_ALTERNATE) {
        if (TR->pids[act->task_no] > 0) {
            task_action(act->task_no, SIGCONT);
            TR->running = act->task_no;
            printf("Resume Task %ld Job %ld (alternate).\n", TR->running, TR->job_count[TR->running]);
        } else {
            start_alternate(act->task_no);
            printf("Start Task %ld Job %ld (alternate).\n", TR->running, TR->job_count[TR->running]);
        }
    } else if (act->action & ACTION_NOTHING) {
        printf("Idle.\n");
    }

    timer = new_time;
    alarm((unsigned int) (next_time - current_time));
    return true;
}

void sighandler_SIGCHLD(int sig) {
    int statue;
    task_hrts tmp;
    unset_signals();
    waitpid(TR->pids[TR->running], &statue, WNOHANG);

    if (WIFEXITED(statue)) {
        tmp = TR->running;
        TR->pids[TR->running] = 0;
        TR->running = -1;
        if (WEXITSTATUS(statue)) {
            if (!schedule(REASON_PRIMARY_CRASHED))
                return;
        } else {
            TR->job_count[tmp]++;
        }
    }
    set_signals();
    pause();
}

void sighandler_SIGALRM(int sig) {
    unset_signals();
    if (!schedule(REASON_SCHEDULE_POINT))
        return;
    set_signals();
    pause();
}