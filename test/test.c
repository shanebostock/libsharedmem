#include <shmemory.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define BUFFERSIZE 3
#define TESTSIZE 100
#define RB_ID 1312

typedef struct packet {
	uint8_t done;
	uint8_t seq;
	char msgbuf[64];
} packet_s;

typedef struct thread_data{
    uint32_t count;
    uint32_t err_count;
    uint32_t cont_count;
} thread_data_s;

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

uint8_t test_packet(packet_s msg, int8_t prev_seq){
    //printf("done: %d, seq: %d, msg: %s",msg.done, msg.seq,msg.msgbuf);
    if (prev_seq +1 != msg.seq){
        printf("Expected seq %d, got seq %d\n.", prev_seq +1, msg.seq);
        return 1;
    }
    return 0;

}

size_t get_size(){
    size_t mem_size = sizeof(packet_s) * BUFFERSIZE;
    return mem_size;
}

void generate_packet(packet_s *ptr, uint8_t seq){
    ptr->done = 0;
    ptr->seq = seq;
    sprintf(ptr->msgbuf,"Message %d.\n",seq);
}

void* write_packets(void *arg){
    thread_data_s *wdata=(thread_data_s *)arg;
    packet_s *pkt = get_shared_memory(get_size(),RB_ID); //start of shmem
    packet_s *writer = pkt; // set writer to start of shmem
    uint32_t write_count = 0;
    uint32_t cont_count = 0;  
    uint8_t buf_pos = 0;
    packet_s msg;
    packet_s *m_ptr = &msg;

    while(write_count < TESTSIZE){
        msleep(100);
        generate_packet(m_ptr,write_count);    

        if (buf_pos >= BUFFERSIZE){
            buf_pos = 0;
            writer = pkt;
        }
        //aquire lock
        if(write_count < BUFFERSIZE){
            memcpy(writer,m_ptr,sizeof(packet_s));
            buf_pos++;
            writer++;
            write_count++;
        }
        else if(writer->done == 1){
            memcpy(writer,m_ptr,sizeof(packet_s));
            buf_pos++;
            writer++;
            write_count++;
        }
        else{
            cont_count++;
            continue;

        }
        // release lock
        
    }
    wdata->count=write_count;
    wdata->cont_count=cont_count;
    pthread_exit(NULL);
    
}

void* read_packets(void *arg){
    thread_data_s *rdata=(thread_data_s *)arg;
    packet_s *pkt = get_shared_memory(get_size(),RB_ID);
    packet_s *reader = pkt;
    packet_s msg;
    packet_s *m_ptr = &msg;
    uint32_t cont_count = 0;
    uint32_t err_count = 0;
    uint32_t read_count = 0;  
    uint8_t buf_pos = 0;
    int8_t prev_seq = -1;
    
    while (1){
        msleep(100);
        if(cont_count > BUFFERSIZE){
            break;
        }

        if (buf_pos >= BUFFERSIZE) {
            buf_pos = 0;
            reader = pkt;
        }
        // aquire lock
        if(reader->done == 0){
            memcpy(m_ptr,reader,sizeof(packet_s));
            reader->done = 1;
            buf_pos++;
            reader++;
            read_count++;
            cont_count = 0;
        }
        else{
            cont_count++;
            continue;
        }
        // release lock
        /* Do something with the data */
        if (test_packet(msg,prev_seq) ==1){
            err_count++;
        }
        prev_seq = msg.seq;
          
    }

    rdata->count=read_count;
    rdata->err_count=err_count;
    rdata->cont_count=cont_count;
    pthread_exit(NULL);
}


/*
This test case needs locks on the critical section and better synchronization. 

The case passes sometimes and fails sometimes as their is a race condition b/w reader and writer
*/
int main(int argc, char** argv){
    void *tret;
    int32_t err;
    pthread_t r_thread, w_thread;
    thread_data_s wdata,rdata;

    void *shm = create_shared_memory(get_size(),RB_ID);
    clear_shared_memory(shm, get_size());

    err = pthread_create(&w_thread,NULL,write_packets,(void*)&wdata);
    if (err !=0){
        perror("could not create thread");
    }
    err = pthread_create(&r_thread,NULL,read_packets,(void*)&rdata);
    if (err !=0){
        perror("could not create thread");
    }

    err = pthread_join(w_thread,&tret);
    printf("w_thread exited with code: %d, write count: %d cont_count: %d\n",err,wdata.count, wdata.cont_count);
    err = pthread_join(r_thread,&tret);
    printf("r_thread exited with code: %d, read count: %d, error count: %d cont_count: %d \n",err,rdata.count,rdata.err_count,rdata.cont_count);
    
    return 0;
}