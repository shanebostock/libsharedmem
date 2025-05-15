#include "ring_buffer.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

ringbuffer_s create_ring_buffer(int numele, size_t sizeele, int numsems){
    ringbuffer_s rb;
    rb.numele = numele;
    rb.sizeele = sizeele;
    rb.numsems = numsems; 
    rb.ids.shmid = create_shared_memory(rb.sizeele*rb.numele,IPC_PRIVATE);
    rb.ids.semid = create_sem(rb.numsems,IPC_PRIVATE);
    rb.pos = 0;
    rb.start = get_shared_memory_by_shmid(rb.ids.shmid);
    rb.curr = get_shared_memory_by_shmid(rb.ids.shmid);
    return rb;
}

void destroy_ring_buffer(ringbuffer_s* rb){
    /* Remove shared memory and semaphore set. */
    if (free_shared_memory(rb->ids.shmid) == -1){
        perror("free_shared_memory");
    }
    if (remove_sem_set(rb->ids.semid) == -1){
        perror("remove_sem_set");
    }
    /* delete the details of the ringbuffer */
    rb->numele = 0;
    rb->sizeele = 0;
    rb->numsems = 0;
    rb->ids.shmid = 0;
    rb->ids.semid = 0;
    rb->pos = 0;
    rb->start = NULL;
    rb->curr = NULL;
}

int write_next(ringbuffer_s* rb, void* element){
    
    void* dest = memcpy(rb->curr,element,rb->sizeele);
    if(dest != rb->curr){
        return -1;
    }
    /* update curr and pos*/
    int r_pos = rb->pos += 1;
    if(rb->pos < rb->numele){
        rb->curr = rb->start + (rb->sizeele * rb->pos); 
    }
    else{
        rb->curr = rb->start;
        rb->pos = 0;
    }
    return r_pos;
}
int release_and_signal(ringbuffer_s *rb){
    return 0;
}