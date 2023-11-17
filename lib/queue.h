#include <stdio.h>
#include <stdlib.h>

// "Queue" implementation
typedef struct queue_t {
    void** queue;
    int length;
    int blocksize;
} queue_t;

queue_t* mkqueue(int blocksize);
queue_t* queue_add(queue_t* queue, void* data);
void* queue_get(queue_t* queue, int index);
void* queue_pop(queue_t* queue);
void* queue_remove(queue_t* queue, int index);
void queue_delete(queue_t** queue);
