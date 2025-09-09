#include <stdio.h>
#include "../inc/threadpool.h"

threadpool_t threadpool;

static void threads_print( void* src ) 
{
    (void)src;
    printf("\n Thread printed \n");
    //return NULL;
}

int main(void)
{
    threadpool_init(&threadpool);

    threadpool_add_task( &threadpool, threads_print, NULL );

    threadpool_destroy(&threadpool);

    return 0;
}
