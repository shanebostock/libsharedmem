#include <shmemory.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define RB_ELEMS 1
#define NUM_SEMS 2
#define RB_ID 1312
#define RB_SEM_ID 1337

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



void p(shm_sem_s* ids){
    struct sembuf  sop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if (semop(ids->semid, &sop, 1) == -1){
        perror("semop");
    }
}

void v(shm_sem_s* ids){
    struct sembuf  sop;
    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if (semop(ids->semid, &sop, 1) == -1){
        perror("semop");
    }
}

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

int32_t msleep(uint32_t ms){
    struct timespec ts;
    int32_t res;
    
    ts.tv_sec = ms/1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    do {
        res = nanosleep(&ts,&ts);
    } while (res && errno == EINTR);

    return res;
}

void* writer_thread0(void* _ids){
    // setup writer thread
    shm_sem_s *ids=(shm_sem_s *)_ids;
    struct sembuf sop;
    sop.sem_num = WRITER_E;
    sop.sem_op = 0; // try wait for zero
    sop.sem_flg = 0;
    union semun arg;
    packet_s new_packet;
    packet_s *p_new_packet = &new_packet;

    packet_s *rb_addr = (packet_s*)get_shared_memory_by_shmid(ids->shmid);
    packet_s *writer = rb_addr;
    
    int num_packets_written = 0;
    uint8_t next_seq = 0;
    
    do {
        //aquire semaphore
        if(semop(ids->semid,&sop,1) == -1){
            perror("semop, try_wait, writer");
            exit(1);
        }
        // recv packets
        generate_packet(p_new_packet,next_seq);
        next_seq+=1;
        // write packets
        memcpy(writer,p_new_packet,packet_size);
        if(packet_size != rb_mem_size){
            // increment the pointers to write next
        }
        num_packets_written+=1;
        // release semaphore
        arg.val=1;
        if(semctl(ids->semid,WRITER_E,SETVAL,arg)==-1){
            perror("semctl");
            exit(1);
        }
        // signal reader
        arg.val = 0;
        if(semctl(ids->semid,READER_E,SETVAL,arg)==-1){
            perror("semctl");
            exit(1);
        }        
    } while(num_packets_written < TEST_SIZE);
    pthread_exit(NULL);
}

void* reader_thread1(void* _ids){
    // setup
    shm_sem_s *ids=(shm_sem_s *)_ids;
    struct sembuf sop;
    sop.sem_num = READER_E;
    sop.sem_op = 0; // try wait for zero
    sop.sem_flg = 0;
    union semun arg;
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
        if(semop(ids->semid,&sop,1) == -1){
            perror("semop, try_wait, writer");
            exit(1);
        }
        // read packets
        if(reader->done != 0){
            writer_fails+=1;
            perror("writer incomplete");

            // release semaphore
            arg.val=1;
            if(semctl(ids->semid,READER_E,SETVAL,arg)==-1){
                perror("semctl");
                exit(1);
            }
            // signal writer
            arg.val = 0;
            if(semctl(ids->semid,WRITER_E,SETVAL,arg)==-1){
                perror("semctl");
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
        // release semaphore
        arg.val=1;
        if(semctl(ids->semid,READER_E,SETVAL,arg)==-1){
            perror("semctl");
            exit(1);
        }
        
        // signal writer
        arg.val = 0;
        if(semctl(ids->semid,WRITER_E,SETVAL,arg)==-1){
            perror("semctl");
            exit(1);
        }        
    } while((num_packets_read < TEST_SIZE) && (writer_fails < 2));
    printf("total errors: seq: %d, writer_fails: %d\n", err_count, writer_fails);
    pthread_exit(NULL);
}

void impl(void){
    shm_sem_s ids;
    
    // id for the ring buffer
    ids.shmid = create_shared_memory(rb_mem_size,RB_ID);
    // id for the semaphore set
    ids.semid = create_sem(NUM_SEMS, RB_SEM_ID);
    int err;
    pthread_t r_thread, w_thread;
    union semun arg, dummy;
    
    // initialize the reader thread semaphore to 1
    arg.val = 1;
    if(semctl(ids.semid,READER_E,SETVAL,arg)==-1){
        perror("semctl");
        exit(1);
    }
    // initialize the writer thread semaphore to 0
    arg.val=0;
    if(semctl(ids.semid,WRITER_E,SETVAL,arg)==-1){
        perror("semctl");
        exit(1);
    }

    err = pthread_create(&w_thread,NULL,writer_thread0,(void*)&ids);
    if (err !=0){
        perror("could not create writer thread");
    }
    err = pthread_create(&r_thread,NULL,reader_thread1,(void*)&ids);
    if (err !=0){
        perror("could not create reader thread");
    }
    
    err = pthread_join(w_thread,NULL);
    if (err !=0){
        perror("could not join writer thread");
    }
    err = pthread_join(r_thread,NULL);
    if (err !=0){
        perror("could not join reader thread");
    }
    /* Remove shared memory and semaphore set. */

    if (shmctl(ids.shmid, IPC_RMID, NULL) == -1)
        perror("shmctl");
    /* when IPC_RMID is called - it removes the whole set the 2nd arg (semnum) is ignored.*/
    if (semctl(ids.semid, 0, IPC_RMID, dummy) == -1)
        perror("semctl");
}

int main(int argc, char** argv){

    impl();

    return 0;
}