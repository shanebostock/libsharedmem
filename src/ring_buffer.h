#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <shmemory.h>
#include <sys/types.h>
typedef struct ringbuffer {
    int numele;
    size_t sizeele;
    int numsems;
    shm_sem_s ids;
    int pos;
    void* start;
    void* curr;
} ringbuffer_s;

ringbuffer_s create_ring_buffer(int numele, size_t sizeele, int numsems);
void destroy_ring_buffer(ringbuffer_s* rb);
int write_next(ringbuffer_s* rb, void* element);
#endif /*RING_BUFFER_H*/