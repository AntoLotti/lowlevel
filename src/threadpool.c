//========INCLUDES========//
#include "../inc/threadpool.h"

//========FUN DEF=========//
static bool creator_pthreads( threadpool_t* src )
{
	pthread_mutex_lock( &(src->lock) );
    for (int i = 0; i < THREADS; i++)
    {
        if ( pthread_create( &(src->threads[ i ]), NULL, threadpool_function, src ) != 0  )
        {
            perror("Error creating the threads");
            return false;
        }
    }

	pthread_mutex_unlock( &(src->lock) );
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
    src->stop       = 0;    // Initialization of the index 

    // Initialize task queue to zeros
    for (int i = 0; i < QUEUE_SIZE; i++) 
    {
        src->task_queue[i].fn = NULL;
        src->task_queue[i].arg = NULL;
    }

    pthread_mutex_init( &(src->lock), NULL );   //Initialization of the mutex
    pthread_cond_init( &(src->notify), NULL );  //Initialization of the condition variable

     if (!creator_pthreads(src))
    {
        fprintf(stderr, "Failed to create threads\n");
        exit(EXIT_FAILURE);
    }

}

void threadpool_destroy( threadpool_t* src )
{
    pthread_mutex_lock( &(src->lock) );         // Make sure that only one thread access this function
    while (src->queued > 0) 
    {
        pthread_mutex_unlock(&(src->lock));
        usleep(10000);  // Sleep for 10ms
        pthread_mutex_lock(&(src->lock));
    }
    src->stop = 1;                           // Ensured the condition
    pthread_cond_broadcast( &(src->notify) );   // Ensure that all the threads get the notification
    pthread_mutex_unlock( &(src->lock) );       // Release the mutex
    
    destructor_pthreads( src );                 //Join the threads

    pthread_mutex_destroy( &(src->lock) );      // Destroy the mutex
    pthread_cond_destroy( &(src->notify) );     // Destroy the condition variable

}

void* threadpool_function( void* src )
{
    threadpool_t* thpool = (threadpool_t*)src ;

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
            pthread_exit( NULL );   
            //return NULL;
        }
        
        // Assigne a task 
        task_t task = thpool->task_queue[ thpool->queue_front ];

        // Move the task in the queue (Circular Queues)
        thpool->queue_front = (thpool->queue_front + 1) % QUEUE_SIZE;
        thpool->queued--;

        // Free the mutex
        pthread_mutex_unlock( &(thpool->lock) );

        // Execute the task
        if (task.fn != NULL) 
        {
            (*(task.fn))(task.arg);
        }
    }
    
    return NULL;
}

void threadpool_add_task(threadpool_t* pool, void (*function)(void*), void* arg)
{
    pthread_mutex_lock( &(pool->lock) );

    if (pool->queued >= QUEUE_SIZE) {
        // Handle queue full situation
        fprintf(stderr, "Queue is full, dropping task\n");
        pthread_mutex_unlock(&(pool->lock));
        return;
    }

    // Check the length of the queue
    if ( pool->queued < QUEUE_SIZE )
    {   
        // Add the task to the queue 
        pool->task_queue[ pool->queued ].arg = arg;
        pool->task_queue[ pool->queued ].fn = function;

        // Find the next position where to put the task (Circular Queues)
        pool->queue_back = (pool->queue_back + 1) % QUEUE_SIZE;
        pool->queued++;

        // Notified  all threads that there is a task in the queue
        pthread_cond_signal(&(pool->notify) );
        //pthread_cond_broadcast( &(pool->notify) ); 
    }
    
    pthread_mutex_unlock( &(pool->lock) );
}

void example_task(void* arg) 
{
    int* num = (int*)arg;
    printf("Processing task %d\n", *num);
    sleep(1);  // Simulate task work
    free(arg);
}
