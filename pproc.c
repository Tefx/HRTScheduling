//
// Created by tefx on 9/8/15.
//

#define _POSIX_C_SOURCE 199309L

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

void busy_sleep(char *s, long ns) {
    struct timespec req;
    long i = 0;

    while (1) {
        req.tv_sec = 0;

        if (i + 50 > ns) {
            req.tv_nsec = (ns - i) * 100 + 50;
            i = ns;
        } else {
            req.tv_nsec = 5e8L;
            i += 50;
        }
        while (nanosleep(&req, &req) == -1);
        //printf("\t[%s] %.2f seconds passed. (%.2fs total)\n", s, (double)i/100, (double)ns/100);

        if (i == ns) break;
    }


}

int main(int argc, char *argv[]) {
    long runtime = (long) (atof(argv[1]) * 100);
    double fail_prob = atof(argv[2]);

    if (runtime >= 300)
        runtime -= 150;
    else
        runtime -= 100;

    srand((unsigned int) time(NULL));
    printf("\t[%s] started.\n", argv[0]);
    if (rand() % 100 <= fail_prob * 100) {
        runtime = rand() % runtime;
        busy_sleep(argv[0], runtime);
        printf("\t[%s] failed (after %.2fs).\n", argv[0], (double) runtime / 100);
        exit(EXIT_FAILURE);
    } else {
        busy_sleep(argv[0], runtime);
        printf("\t[%s] succeed.\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
}