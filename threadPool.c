#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define ERROR_TYPE "Error in system call\n"
#define ERROR_LEN 255

void errorFunc(ThreadPool* threadPool){
    write(2, ERROR_TYPE, ERROR_LEN);
    tpDestroy(threadPool, 0);
}

static void* threadPoolForCreate(void *funcTreadPool){

}

ThreadPool* tpCreate(int numOfThreads){
    ThreadPool *threadPool;

    threadPool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (threadPool == NULL){
        //couldn't creat ThreadPool struct
        errorFunc(threadPool);
        return NULL;
    }

    //Initialize//
    threadPool->maxThreads = 0;
    threadPool->workingThreads = 0;
    threadPool->finishThreads = 0;

    threadPool->queue = osCreateQueue();

    threadPool->threads = (pthread_t *)malloc(sizeof(pthread_t)*numOfThreads);
    if (threadPool->threads == NULL){
        errorFunc(threadPool);
        return NULL;
    }

    if ((pthread_mutex_init(&(threadPool->mutex), NULL) != 0)
    || (pthread_cond_init(&(threadPool->conditionMutex), NULL) !=0)){
        errorFunc(threadPool);
        return NULL;
    }

    //Create Threads//
    int i = 0;
    for (; i < numOfThreads; i++){
        int err = pthread_create(&(threadPool->threads[i]), NULL, threadPoolForCreate, (void*)threadPool);
        if (err != 0){
            errorFunc(threadPool);
            return NULL;
        }
        threadPool->maxThreads++;
        threadPool->workingThreads++;
    }

    return threadPool;
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param){

    if ((threadPool == NULL) || (computeFunc == NULL)){
        return invalid;
    }

    if(pthread_mutex_lock(&(threadPool->mutex)) != 0){
        return failed_lock;
    }

    Task* newTask;

    newTask = (Task *)malloc(sizeof(Task));

    if (newTask == NULL){
        errorFunc(threadPool);
        return NULL;
    }

    //Initialize//
    newTask->function = computeFunc;
    newTask->param = param;

    //insert new task to queue
    osEnqueue(threadPool->queue, newTask);

    if (pthread_mutex_unlock(&(threadPool->mutex)) != 0){
        return failed_lock;
    }

}
