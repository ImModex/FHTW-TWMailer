#include <stdio.h>
#include <stdlib.h>

// Queue implementation
typedef struct queue_t {
    void** queue;
    unsigned int length;
    unsigned int blocksize;
} queue_t;

queue_t* mkqueue(unsigned int blocksize);
queue_t* queue_add(queue_t* queue, void* data);
void* queue_get(queue_t* queue, int index);
void* queue_pop(queue_t* queue);
void queue_delete(queue_t** queue);
