#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "easy_pthreads.h"

#define MAX_THREADS 0x10000


pj_t pjs[MAX_THREADS] = {0};

void *pj_exec(void *pthread_attrs) {
    pj_t *job = NULL;
    pthread_t tid = pthread_self();

    // Find this job
    int i = 0;
    for(i = 0; i < MAX_THREADS; i++) {
        if(pthread_equal(pjs[i].tid, tid)) {
            job = &(pjs[i]);
        }
    }

    if(job == NULL) {
        fprintf(stderr, "Could not find job.\n");
        exit(EXIT_FAILURE);
    }

    job->func(job);

    return NULL;
}

pj_t *pj_create(void **args, void *ret, void (*func)(pj_t *)) {
    pj_t *job = NULL;

    // Find the first empty job.
    int i = 0;
    for(i = 0; i < MAX_THREADS; i++) {
        if(pjs[i].tid == 0) {
            job = &(pjs[i]);
            // Minimize the chance of two threads grabbing the same pthread_job
            job->tid = -1;
            break;
        }
    }

    if(job == NULL) {
        // If you have 65000 threads, there's probably a problem.
        fprintf(stderr, "Too many threads (%i), could not create more.\n", i);
        exit(EXIT_FAILURE);
    }

    // Two args, a and b
    job->args = args;
    // The address we are returning to.
    job->ret = ret;
    // What function are we calling?
    job->func = func;
    job->done = 0;

    pthread_create(&(job->tid), NULL, pj_exec, NULL);
    return job;
}

void pj_join(pj_t *job) {
    pthread_join(job->tid, NULL);
}
