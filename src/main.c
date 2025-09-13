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

    // Wait for tasks to complete
    //threadpool_wait(&pool);
    sleep(1);
    threadpool_destroy(&pool);

    return 0;
}

/*
void threadpool_wait(threadpool_t* pool)
{
    // Wait until all tasks are completed and all threads are idle
    while (1)
    {
        pthread_mutex_lock(&(pool->lock));
        int queued = pool->queued;
        int active = pool->active_threads;
        pthread_mutex_unlock(&(pool->lock));
        
        if (queued == 0 && active == 0) break;
        
        usleep(10000);  // Sleep for 10ms
    }
}
*/