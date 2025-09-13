//========INCLUDES========//
#include "../inc/threadpool.h"

//========FUN DEF=========//
static bool creator_pthreads(threadpool_t* src)
{
    for (int i = 0; i < THREADS; i++)
    {
        if (pthread_create(&(src->threads[i]), NULL, threadpool_function, src) != 0)
        {
            perror("Error creating the threads");
            return false;
        }
    }
    return true;
}

static bool destructor_pthreads(threadpool_t* src)
{
    for (int i = 0; i < THREADS; i++)
    {
        if (pthread_join(src->threads[i], NULL) != 0)
        {
            perror("Error joining the threads");
            return false;
        }
    }
    return true;
}

void threadpool_init(threadpool_t* src)
{
    src->queued = 0;
    src->queue_front = 0;
    src->queue_back = 0;
    src->stop = false;
    src->active_threads = 0;

    // Initialize task queue to zeros
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        src->task_queue[i].fn = NULL;
        src->task_queue[i].arg = NULL;
    }

    pthread_mutex_init(&(src->lock), NULL);
    pthread_cond_init(&(src->notify), NULL);

    if (!creator_pthreads(src))
    {
        fprintf(stderr, "Failed to create threads\n");
        exit(EXIT_FAILURE);
    }
}

void threadpool_destroy(threadpool_t* src)
{
    pthread_mutex_lock(&(src->lock));
    src->stop = true;
    pthread_cond_broadcast(&(src->notify));
    pthread_mutex_unlock(&(src->lock));
    
    destructor_pthreads(src);

    pthread_mutex_destroy(&(src->lock));
    pthread_cond_destroy(&(src->notify));
}

void* threadpool_function(void* src)
{
    threadpool_t* thpool = (threadpool_t*)src;
    
    pthread_mutex_lock(&(thpool->lock));
    thpool->active_threads++;
    pthread_mutex_unlock(&(thpool->lock));

    while (1)
    {
        pthread_mutex_lock(&(thpool->lock));
        
        while (thpool->queued == 0 && !thpool->stop)
        {
            pthread_cond_wait(&(thpool->notify), &(thpool->lock));
        }

        if (thpool->stop && thpool->queued == 0)
        {
            thpool->active_threads--;
            pthread_mutex_unlock(&(thpool->lock));
            pthread_exit(NULL);
        }
        
        task_t task = thpool->task_queue[thpool->queue_front];
        thpool->queue_front = (thpool->queue_front + 1) % QUEUE_SIZE;
        thpool->queued--;

        pthread_mutex_unlock(&(thpool->lock));

        if (task.fn != NULL)
        {
            (*(task.fn))(task.arg);
        }
    }
    
    return NULL;
}

void threadpool_add_task(threadpool_t* pool, void (*function)(void*), void* arg)
{
    pthread_mutex_lock(&(pool->lock));

    if( function == NULL )
    {
        fprintf(stderr, "NULL function pointer\n");
        pthread_mutex_unlock(&(pool->lock));
        return;
    }

    if (pool->queued == QUEUE_SIZE)
    {
        fprintf(stderr, "Queue is full, dropping task\n");
        pthread_mutex_unlock(&(pool->lock));
        return;
    }

    pool->task_queue[pool->queue_back].fn = function;
    pool->task_queue[pool->queue_back].arg = arg;
    pool->queue_back = (pool->queue_back + 1) % QUEUE_SIZE;
    pool->queued++;

    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));
}

void example_task(void* arg)
{
    int* num = (int*)arg;
    printf("Processing task %d\n", *num);
    sleep(1);  // Simulate task work
    free(arg);
}