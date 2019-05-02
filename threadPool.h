#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <sys/param.h>
#include "osqueue.h"

typedef struct thread_pool
{
    OSQueue* queue;
    int maxThreads;
    int workingThreads;
    int finishThreads;

    pthread_mutex_t mutex;
    pthread_cond_t conditionMutex;

    pthread_t *threads;

}ThreadPool;

typedef struct {
    void (*function)(void *);
    void * param;
} Task;

typedef enum {
    invalid = -1, failed_lock = -2, no_more_place = -3,
    finish = -4, fail = -5
} errors;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
