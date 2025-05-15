#include <shmemory.h>
#include <ring_buffer.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <errno.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define RB_ELEMS 1
#define NUM_SEMS 2
#define RB_ID 1312
#define RB_SEM_ID 1337
#define SET_VAL 0
#define TEST_SIZE 100

enum rw_thread {
    READER_E = 0,
    WRITER_E = 1,

    LAST_E,
};

typedef struct packet {
	uint8_t done;
	uint8_t seq;
	char msgbuf[64];
} packet_s;

const size_t packet_size = sizeof(packet_s);
const size_t rb_mem_size = sizeof(packet_s)*RB_ELEMS;

void generate_packet(packet_s *ptr, uint8_t seq){
    ptr->done = 0;
    ptr->seq = seq;
    sprintf(ptr->msgbuf,"Message %d.\n",seq);
}


int test_packet(uint8_t recv_seq, uint8_t expected_seq){
    if (recv_seq == expected_seq){
        return 0;
    }
    else{
        return 1;
    }
}

void* writer_thread0(void* _rb){
    // setup writer thread
    ringbuffer_s *rb=(ringbuffer_s *)_rb;

    packet_s new_packet;
    // packet_s *p_new_packet = &new_packet;

    // packet_s *rb_addr = (packet_s*)get_shared_memory_by_shmid(ids->shmid);
    // packet_s *writer = rb_addr;
    
    int num_packets_written = 0;
    uint8_t next_seq = 0;
    int retval = 0;
    
    do {
        //aquire semaphore
        if(try_wait(rb->ids.semid,WRITER_E) == -1){
            perror("try_wait");
            exit(1);
        }
        // recv packets
        generate_packet(&new_packet,next_seq);
        next_seq+=1;
        // write packets
        retval = write_next(rb, (void*)&new_packet);
        if (retval == -1){
            perror("write_next");
        }

        if(retval%rb->numele == 0){
            /* release the writer sem */
            if(release_sem(rb->ids.semid,WRITER_E)==-1){
                perror("release_sem");
                exit(1);
            }
            /* signal the reader sem */
            if(signal_sem(rb->ids.semid,READER_E)==-1){
                perror("signal_sem");
                exit(1);
            }      
        }
        num_packets_written+=1;
 
    } while(num_packets_written < TEST_SIZE);
    pthread_exit(NULL);
}

#if 0
void* reader_thread1(void* _rb){
    // setup
    ringbuffer_s *rb=(ringbuffer_s *)_rb;
    packet_s new_packet;
    packet_s *p_new_packet = &new_packet;

    packet_s *rb_addr = (packet_s*)get_shared_memory_by_shmid(ids->shmid);
    packet_s *reader = rb_addr;

    int num_packets_read = 0;
    uint8_t recv_seq = 0;
    uint8_t expected_seq = 0;
    uint8_t err_count = 0;
    uint8_t writer_fails = 0;


    do {
        //aquire semaphore
        if(try_wait(ids->semid,READER_E) == -1){
            perror("try_wait");
            exit(1);
        }
        // read packets
        if(reader->done != 0){
            writer_fails+=1;
            perror("writer incomplete");
            /*release the reader sem */
            if(release_sem(ids->semid,READER_E)==-1){
                perror("release_sem");
                exit(1);
            }
            /* signal the writer sem */
            if(signal_sem(ids->semid,WRITER_E)==-1){
                perror("signal_sem");
                exit(1);
            }
            continue;  
        }
        else {
            memcpy(p_new_packet,reader,packet_size);
            num_packets_read+=1;
        }

        // test packets
        recv_seq = new_packet.seq;
        err_count += test_packet(recv_seq,expected_seq);
        expected_seq = recv_seq +1;
        // print packet msg
        printf("%s",new_packet.msgbuf);
        // set the done bit
        reader->done=1;
        if(packet_size != rb_mem_size){
            // increment the pointers to read next
        }
        /*release the reader sem */
        if(release_sem(ids->semid,READER_E)==-1){
            perror("release_sem");
            exit(1);
        }
        /* signal the writer sem */
        if(signal_sem(ids->semid,WRITER_E)==-1){
            perror("signal_sem");
            exit(1);
        }
      
    } while((num_packets_read < TEST_SIZE) && (writer_fails < 2));
    printf("total errors: seq: %d, writer_fails: %d\n", err_count, writer_fails);
    pthread_exit(NULL);
}
#endif

void impl(void){
    /* 
       This example will use two threads ( a reader and a writer )
       during the "define" step we need to establish which semaphore 
       will signal which thread to operate on the critical section
    */

    /*setup*/ 
    size_t packet_size = sizeof(packet_s);
    int numpackets = 3;
    int numsems = 2; /* one for reader, one for writer */
    ringbuffer_s rb = create_ring_buffer(numpackets,packet_size,numsems);
    int err;
    pthread_t w_thread;
    // pthread_t r_thread;


    /* define  */
    /* Enum above defines READER_E = 0, and WRITER_E = 0 
       These are used for the Threads to know which semaphore to listen for
       and which semaphore to set.    
    */

    /* release the reader semaphore 
       this sets the READER Semaphore to 1
       making the READER thread start in a waiting state
    */ 
    if(release_sem(rb.ids.semid,READER_E)==-1){
        perror("release_sem");
        exit(1);
    }
    /* signal the writer semaphore 
       this sets the WRITER Semaphore to 0
       allowing the WRTIER thread to proceed immediately
    */ 
    if(signal_sem(rb.ids.semid,WRITER_E)==-1){
        perror("signal_sem");
        exit(1);
    }

    /* start the two threads working on the ring buffer 
       both threads start with a "try_wait" call - where the semaphore
       waits for the semaphore to be 0. Because the WRITER
       thread's semaphore has been init to 0 it can proceed immediately.
       the READER thread has been init to 1, so when it calls "try_wait"
       it will be paused until its semaphore is set to 0. 
    */

    err = pthread_create(&w_thread,NULL,writer_thread0,(void*)&rb);
    if (err !=0){
        perror("could not create writer thread");
    }
    // err = pthread_create(&r_thread,NULL,reader_thread1,(void*)&rb);
    // if (err !=0){
    //     perror("could not create reader thread");
    // }
    
    /* try to joing threads when they are done. */    
    err = pthread_join(w_thread,NULL);
    if (err !=0){
        perror("could not join writer thread");
    }
    // err = pthread_join(r_thread,NULL);
    // if (err !=0){
    //     perror("could not join reader thread");
    // }
    /* take down */
    destroy_ring_buffer(&rb);

}

int main(int argc, char** argv){

    impl();

    return 0;
}