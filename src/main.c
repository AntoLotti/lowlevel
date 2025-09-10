#include <stdio.h>
#include "threadpool.h"

void threadpool_wait(threadpool_t* pool);


int main() {
    threadpool_t pool;
    threadpool_init(&pool);

    // Add tasks to the thread pool
    for (int i = 0; i < 100; i++) {
        int* task_num = malloc(sizeof(int));
        *task_num = i;
        threadpool_add_task(&pool, example_task, task_num);
    }

    // Let tasks complete
    threadpool_wait(&pool);
    threadpool_destroy(&pool);

    return 0;
}

void threadpool_wait(threadpool_t* pool) 
{
    pthread_mutex_lock(&(pool->lock));
    while (pool->queued > 0) {
        pthread_mutex_unlock(&(pool->lock));
        usleep(10000);  // Sleep for 10ms
        pthread_mutex_lock(&(pool->lock));
    }
    pthread_mutex_unlock(&(pool->lock));
}