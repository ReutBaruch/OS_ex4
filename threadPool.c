#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define ERROR_TYPE "Error in system call\n"
#define ERROR_LEN 255

void errorFunc(){
    write(2, ERROR_TYPE, ERROR_LEN);
}

static void* threadPoolForCreate(void *tempThreadPool){

    ThreadPool *threadPool;
    threadPool = (ThreadPool *)tempThreadPool;

    while (true) {

        if (pthread_mutex_lock(&(threadPool->mutex)) != 0) {
            errorFunc();
            return (void *) -1;
            //return (void *) invalid;
        }

        while ((osIsQueueEmpty(threadPool->queue)) && (!threadPool->destroyStarted)) {
            pthread_cond_wait(&(threadPool->conditionMutex), &(threadPool->mutex));
        }

        if ((threadPool->destroyStarted) && ((threadPool->shouldFinishTasks == 0) ||
                                             ((threadPool->shouldFinishTasks == 1) &&
                                              (osIsQueueEmpty(threadPool->queue))))) {
            if (pthread_mutex_unlock(&(threadPool->mutex)) != 0) {
                errorFunc();
            }
            break;
        }
        //  }

        /* if (threadPool->destroyStarted){
             if (((threadPool->shouldFinishTasks) && osIsQueueEmpty(threadPool->queue)) || (threadPool->shouldFinishTasks == 0)){
                 if(pthread_mutex_unlock(&(threadPool->mutex)) != 0){
                     errorFunc();
                 }
                 break;
             }
         }*/


        //   pthread_mutex_unlock(&(threadPool->mutex));
        Tasks *newTask;
        newTask = osDequeue(threadPool->queue);
        pthread_mutex_unlock(&(threadPool->mutex));
        (*(newTask->function))(newTask->param);
    }//end of while true

    pthread_mutex_unlock(&(threadPool->mutex));



  //  }
    return 0;
}

ThreadPool* tpCreate(int numOfThreads){
    ThreadPool *threadPool;

    threadPool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (threadPool == NULL){
        //couldn't creat ThreadPool struct
        errorFunc();
        return NULL;
    }

    //Initialize//
    threadPool->maxThreads = 0;
    threadPool->workingThreads = 0;
   // threadPool->finishThreads = 0;
    threadPool->destroyStarted = false;
    threadPool->shouldFinishTasks = -1;

    threadPool->queue = osCreateQueue();

    threadPool->threads = (pthread_t *)malloc((sizeof(pthread_t))*numOfThreads);
    if (threadPool->threads == NULL){
        errorFunc();
        return NULL;
    }

  /*  threadPool->task = (Tasks *)malloc(sizeof(Tasks)*numOfThreads);
    if (threadPool->task == NULL){
        errorFunc();
    }
*/
    if ((pthread_mutex_init(&(threadPool->mutex), NULL) != 0)
    || (pthread_cond_init(&(threadPool->conditionMutex), NULL) !=0)){
        errorFunc();
        return NULL;
    }

    //Create Threads//
    int i = 0;
    for (; i < numOfThreads; i++){

        int err = pthread_create(&(threadPool->threads[i]), NULL, threadPoolForCreate, (void*)threadPool);
        if (err != 0){
            errorFunc();
            return NULL;
        }
        threadPool->maxThreads++;
        threadPool->workingThreads++;
    }
    return threadPool;
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param){
    //if we called Destroy we don't want to add any more tasks
    if (!threadPool->destroyStarted) {

        if ((threadPool == NULL) || (computeFunc == NULL)) {
            return invalid;
        }

        if (pthread_mutex_lock(&(threadPool->mutex)) != 0) {
            return failed_lock;
        }

        Tasks *newTask;

        newTask = (Tasks *) malloc(sizeof(Tasks));

        if (newTask == NULL) {
            errorFunc();
            return -1;
            //return invalid;
        }

        //Initialize//
        newTask->function = computeFunc;
        newTask->param = param;

        //insert new task to queue
        osEnqueue(threadPool->queue, newTask);

        if (pthread_cond_signal(&(threadPool->conditionMutex)) != 0) {
            errorFunc();
            return -1;
            // return invalid;
        }

        if (pthread_mutex_unlock(&(threadPool->mutex)) != 0) {
            return -1;
            //return failed_lock;
        }

        return 0;
    }

    return -1;
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){

    if (pthread_mutex_lock(&(threadPool->mutex)) != 0){
        errorFunc();
    }

    threadPool->destroyStarted = true;
    threadPool->shouldFinishTasks = shouldWaitForTasks;

    if (pthread_cond_broadcast(&(threadPool->conditionMutex)) != 0){
        errorFunc();
    }

    if (pthread_mutex_unlock(&(threadPool->mutex)) != 0){
        errorFunc();
    }

    int i = 0;
    for (; i < threadPool->maxThreads; i++){
        if (pthread_join(threadPool->threads[i], NULL) != 0){
            errorFunc();
        }
    }

    free(threadPool->threads);
    osDestroyQueue(threadPool->queue);
    pthread_mutex_destroy(&(threadPool->mutex));
    pthread_cond_destroy(&(threadPool->conditionMutex));
    free(threadPool);
}