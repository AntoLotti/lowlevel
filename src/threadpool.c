//========INCLUDES========//
#include <stdbool.h>
#include <errno.h>
#include "../inc/threadpool.h"


//========FUN DEF=========//
static bool creator_pthreads( threadpool_t* src )
{
    for (int i = 0; i < THREADS; i++)
    {
        if ( pthread_create( &(src->threads[ i ]), NULL, &threadpool_assigner, src ) != 0  )
        {
            perror("Error creating the threads");
            return false;
        }
    }

    return true;
}

static bool destructor_pthreads( threadpool_t* src )
{
    for (int i = 0; i < THREADS; i++)
    {
        if ( pthread_join( src->threads[ i ], NULL ) != 0  )
        {
            perror("Error joining the threads");
            return false;
        }
    }

    return true;
}

void threadpool_init( threadpool_t* src )
{
    src->queued   = 0;        // Initialization of the index
    src->queue_front  = 0;        // Initialization of the index 
    src->queue_back = 0;        // Initialization of the index
    src->stop       = false;    // Initialization of the index 

    pthread_mutex_init( &(src->lock), NULL );   //Initialization of the mutex
    pthread_cond_init( &(src->notify), NULL );  //Initialization of the condition variable

    creator_pthreads( src );   //Creation of the threads

}

void threadpool_add_task( threadpool_t* dst, void* (*fun)( void* arg ), void* arg )
{
    pthread_mutex_lock( &(dst->lock) );

    // Find the next position where to put the task (Circular Queues)
    int nextTaskpos = (dst->queue_back + 1) % MAX_TASKS;

    // Check the length of the queue
    if ( dst->queued < MAX_TASKS )
    {   
        // Add the task to the queue 
        dst->task_queue[ nextTaskpos ].arg = arg;
        dst->task_queue[ nextTaskpos ].fn = fun;
        dst->queue_back = nextTaskpos;
        dst->queued++;

        // Notified  all threads that there is a task in the queue
        pthread_cond_broadcast( &(dst->notify) ); 
    }
    
    pthread_mutex_unlock( &(dst->lock) );
}

void threadpool_destroy( threadpool_t* src )
{
    pthread_mutex_lock( &(src->lock) );         // Make sure that only one thread access this function
    src->stop = true;                           // Ensured the condition
    pthread_cond_broadcast( &(src->notify) );   // Ensure that all the threads get the notification
    pthread_mutex_unlock( &(src->lock) );       // Release the mutex
    
    destructor_pthreads( src );                 //Join the threads

    pthread_mutex_destroy( &(src->lock) );      // Destroy the mutex
    pthread_cond_destroy( &(src->notify) );     // Destroy the condition variable

}

void* threadpool_assigner( void* src )
{
    threadPool_t* thpool = (threadPool_t*)src ;

    while ( 1 )
    {
        // Lock the mutex so only one thread can modify the queue
        pthread_mutex_lock( &(thpool->lock) );
        
        // Check if the Queue is empty, if it's wait untile there are some task
        while ( thpool->queued == 0 && !(thpool->stop) )
        {
            pthread_cond_wait( &(thpool->notify), &(thpool->lock) );
        }

        // Check if the threadpool must stop
        if ( thpool->stop == true )
        {
            pthread_mutex_unlock( &(thpool->lock) );
            pthread_exit( &(thpool->lock) );   
            //return NULL;
        }
        
        // Assigne a task 
        task_t taskTh = thpool->taskQueue[ thpool->queue_front ];

        // Move the task in the queue (Circular Queues)
        thpool->queue_front = (thpool->queue_front + 1) % MAX_TASKS;
        thpool->queued--;

        // Free the mutex
        pthread_mutex_unlock( &(thpool->lock) );

        // Ejecute the task
        (*(taskTh.taskAction))( taskTh.arg );
    }
    
    return NULL;
}