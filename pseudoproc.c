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

        if (i > ns) {
            req.tv_nsec = ns - i + 5e8L;
        } else
            req.tv_nsec = 5e8L;
        while (nanosleep(&req, &req) == -1);
        //printf("\t[%s] %.2f second passed.\n", s, ((double)i/1e9));

        if (i > ns)
            break;
        else
            i += 5e8;
    }


}

int main(int argc, char *argv[]) {
    long runtime = (long) (atof(argv[1]) * 1e9L);
    double fail_prob = atof(argv[2]);

    if (runtime >= 3e9L)
        runtime -= 1.5e9L;
    else
        runtime -= 1e9L;

    srand((unsigned int) time(NULL));
    printf("\t[%s] started.\n", argv[0]);
    if (rand() % 100 <= fail_prob * 100) {
        runtime = rand() % (runtime / (long) 1e6) * 1e6;
        busy_sleep(argv[0], runtime);
        printf("\t[%s] failed (after %.2fs).\n", argv[0], runtime / 1e9);
        exit(EXIT_FAILURE);
    } else {
        busy_sleep(argv[0], runtime);
        printf("\t[%s] succeed.\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
}