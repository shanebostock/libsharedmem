#include <shmemory.h>
#include "queue.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <errno.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>




void init_queue(Queue* q){
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    q->data = NULL;
}

void enqueue(Queue* q, void* data){
    if (q->count >= q->num_ele) {
        fprintf(stderr, "Queue is full, cannot enqueue.\n");
        return;
    }

    memcpy(q->data+(q->rear*q->ele_size), data, q->ele_size);
    q->rear++;
    q->count++;

    if (q->rear >= q->num_ele) {
        q->rear = 0; // wrap around if we reach the end
    }
}

void dequeue(Queue* q, void* data){
    if (q->count <= 0) {
        fprintf(stderr, "Queue is empty, cannot dequeue.\n");
        return;
    }
    memcpy(data,q->data+(q->front*q->ele_size), q->ele_size);
    q->front++;
    q->count--;

    if (q->front >= q->num_ele) {
        q->front = 0; // wrap around if we reach the end
    }
}
