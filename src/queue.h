#ifndef QUEUE_H
#define QUEUE_H


typedef struct {
    uint32_t num_ele;
    size_t ele_size;
    uint32_t count;
    uint32_t rear;
    uint32_t front;
    void* data; 
} Queue;

void init_queue(Queue* q);
void enqueue(Queue* q, void* data);
void* dequeue(Queue* q);

#endif /*QUEUE_H*/