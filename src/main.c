#include <stdio.h>
#include "threadpool.h"

threadpool_t threadpool;

static void threads_print( void* src ) 
{
    (void)src;
    printf("\n Thread printed \n");
    sleep(100);
    //return NULL;
}

int main(void)
{
    threadpool_init(&threadpool);

    threadpool_add_task( &threadpool, threads_print, NULL );

    sleep(3);
    threadpool_destroy(&threadpool);

    return 0;
}
