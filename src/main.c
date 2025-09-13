#include <stdio.h>
#include "threadpool.h"

int main() {
    threadpool_t pool;
    
    if (threadpool_init(&pool) != 0) {
        fprintf(stderr, "Failed to initialize thread pool\n");
        return 1;
    }

    // Add tasks to the thread pool
    for (int i = 0; i < 100; i++) {
        int* task_num = malloc(sizeof(int));
        if (task_num == NULL) {
            fprintf(stderr, "Failed to allocate memory for task %d\n", i);
            continue;
        }
        *task_num = i;
        
        if (threadpool_add_task(&pool, example_task, task_num) != 0) {
            fprintf(stderr, "Failed to add task %d\n", i);
            free(task_num);
        }
    }

    // Wait for tasks to complete
    sleep(5);
    
    if (threadpool_destroy(&pool, false) != 0) {
        fprintf(stderr, "Failed to destroy thread pool\n");
        return 1;
    }

    return 0;
}