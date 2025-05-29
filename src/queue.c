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

typedef struct {
    int num;
    int val;

} data_s;

int main(void){
    Queue q;
    data_s data;
    data_s *rb_data;
    init_queue(&q);

    q.num_ele = 3;
    q.ele_size = sizeof(data_s);
    q.data = (data_s*)malloc(q.num_ele*q.ele_size);

    for (int i=0; i<q.num_ele; i++){
        data.num = i;
        data.val = 10+i;
        enqueue(&q, (void*)&data);
    }

    for (int i=0; i<q.num_ele; i++){
        rb_data = dequeue
    }

    free(q.data);
    return 0;
}
void init_queue(Queue* q){
    q->front = 0;
    q->rear = 0;
    q->data = NULL;
}
void enqueue(Queue* q, void* data){
    memcpy(q->data+(q->rear*q->ele_size), data, q->ele_size);
    q->rear++;
    q->count++;
}

void dequeue(Queue* q, void* data){
    memcpy(data,q->data+(q->front*q->ele_size), q->ele_size);
    q->front++;
    q->count--;
}