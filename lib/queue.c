#include "queue.h"

// Creates a queue and returns its address
queue_t* mkqueue(int blocksize) {
    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    queue->length = 0;
    queue->blocksize = blocksize;
    queue->queue = (void**) malloc(sizeof(blocksize));

    if(queue->queue == NULL) {
        fprintf(stderr, "mkqueue couldn't allocate memory...");
        exit(1);
    }

    return queue;
}

// Adds an element to the end of the queue
queue_t* queue_add(queue_t* queue, void* data) {
    queue->length++;
    queue->queue = (void**) realloc(queue->queue, queue->length * queue->blocksize);
    if(queue->queue == NULL) {
        fprintf(stderr, "queue_add couldn't reallocate memory...");
        exit(1);
    }
    queue->queue[queue->length - 1] = data;

    return queue;
}

// Gets an element from an index in the queue
void* queue_get(queue_t* queue, int index) {
    return (index < 0 || queue->length < index+1) ? NULL : queue->queue[index];
}

// Removes and returns the last element of the queue
void* queue_pop(queue_t* queue) {
    return (queue->length <= 0) ? NULL : queue->queue[--queue->length];
}

// Free memory that has been allocated by the queue
void queue_delete(queue_t** queue) {
    while((*queue)->length > 0) free((*queue)->queue[--(*queue)->length]);

    free((*queue)->queue);
    free(*queue);
}
