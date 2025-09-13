//========INCLUDES========//
#include "../inc/threadpool.h"

//========FUN DEF=========//
int threadpool_init(threadpool_t* pool)
{
    if (pool == NULL) {
        return -1;
    }
    
    for (int i = 0; i < THREADS; i++)
    {
        pool->task_queue[i].fn = NULL;
        pool->task_queue[i].arg = NULL;
    }

    if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
        return -1;
    }
    
    if (pthread_cond_init(&(pool->notify), NULL) != 0) {
        pthread_mutex_destroy(&(pool->lock));
        return -1;
    }
    
    if (pthread_cond_init(&(pool->not_full), NULL) != 0) {
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
        return -1;
    }

    for (int i = 0; i < THREADS; i++)
    {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_function, pool) != 0)
        {
            // Clean up already created threads
            pool->stop = true;
            pthread_cond_broadcast(&(pool->notify));
            
            for (int j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            
            pthread_mutex_destroy(&(pool->lock));
            pthread_cond_destroy(&(pool->notify));
            pthread_cond_destroy(&(pool->not_full));
            return -1;
        }
    }

    return 0;
}

int threadpool_destroy(threadpool_t* pool, bool immediate)
{
    if (pool == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&(pool->lock));
    
    if (immediate) {
        pool->shutdown_immediate = true;
    }
    
    pool->stop = true;
    pthread_cond_broadcast(&(pool->notify));
    pthread_cond_broadcast(&(pool->not_full));
    pthread_mutex_unlock(&(pool->lock));
    
    for (int i = 0; i < THREADS; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    pthread_cond_destroy(&(pool->not_full));
    
    return 0;
}

void* threadpool_function(void* arg)
{
    threadpool_t* pool = (threadpool_t*)arg;
    
    while (1)
    {
        pthread_mutex_lock(&(pool->lock));
        
        while (pool->queued == 0 && !pool->stop) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }
        
        if (pool->stop && (pool->shutdown_immediate || pool->queued == 0)) {
            pthread_mutex_unlock(&(pool->lock));
            break;
        }
        
        task_t task = pool->task_queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % QUEUE_SIZE;
        pool->queued--;
        
        pthread_cond_signal(&(pool->not_full));
        pthread_mutex_unlock(&(pool->lock));
        
        if (task.fn != NULL) {
            (*(task.fn))(task.arg);
        }
    }
    
    return NULL;
}

int threadpool_add_task(threadpool_t* pool, void (*function)(void*), void* arg)
{
    if (pool == NULL || function == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&(pool->lock));
    
    if (pool->stop) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }
    
    while (pool->queued >= QUEUE_SIZE && !pool->stop) {
        pthread_cond_wait(&(pool->not_full), &(pool->lock));
    }
    
    if (pool->stop) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }
    
    pool->task_queue[pool->queue_back].fn = function;
    pool->task_queue[pool->queue_back].arg = arg;
    pool->queue_back = (pool->queue_back + 1) % QUEUE_SIZE;
    pool->queued++;
    
    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));
    
    return 0;
}

void example_task(void* arg)
{
    if (arg == NULL) {
        return;
    }
    
    int* num = (int*)arg;
    printf("Processing task %d\n", *num);
    sleep(1);
    free(arg);
}