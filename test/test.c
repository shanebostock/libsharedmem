#include <shmemory.h>
#include <shmemaphore.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#include <ctype.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BUFFERSIZE 3
#define TESTSIZE 3
#define RB_ID 1312
#define RB_SEM_ID 1337
#define BUF_SIZE 1024

typedef struct packet {
	uint8_t done;
	uint8_t seq;
	char msgbuf[64];
} packet_s;


typedef struct shm_sem {
    sem_t  sem1;            /* POSIX unnamed semaphore */
    sem_t  sem2;            /* POSIX unnamed semaphore */
    uint32_t rb_slots;      /* Number of "slots" in the ring buffer */
    void* shm;               /* ptr to shared memory */
} shm_sem_s;

typedef struct thread_data{
    uint32_t test_size;
    uint32_t count;
    uint32_t err_count;
    shm_sem_s* shmp;
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
int write_packets(thread_data_s *wdata){
    packet_s *pkt = get_shared_memory(get_size(),RB_ID); //start of shmem
    packet_s *writer = pkt; // set writer to start of shmem
    packet_s msg;
    packet_s *m_ptr = &msg;
    uint32_t write_count = 0;

    while(write_count < wdata->test_size){
        msleep(100);
        generate_packet(m_ptr,write_count);    
        memcpy(writer,m_ptr,sizeof(packet_s));
        writer++;
        write_count++;
    }
    wdata->count=write_count;
    return 0;
}

int read_packets(thread_data_s *rdata){
    packet_s *pkt = get_shared_memory(get_size(),RB_ID);
    packet_s *reader = pkt;
    packet_s msg;
    packet_s *m_ptr = &msg;
    uint32_t err_count = 0;
    uint32_t read_count = 0;  
    uint32_t cont_count = 0;
    uint8_t buf_pos = 0;
    int8_t prev_seq = -1;
    
    while (read_count < rdata->test_size){
        msleep(100);
        if(cont_count > BUFFERSIZE){
            break;
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
    return 0;
}

void* t_write_packets (void *arg){
    thread_data_s *wdata=(thread_data_s *)arg;
    //wdata->shm = get_shared_memory(get_size(),RB_ID); //start of shmem
    packet_s *writer = wdata->shmp->shm; // set writer to start of shmem
    uint32_t write_count = 0;
    uint32_t cont_count = 0;  
    uint8_t buf_pos = 0;
    packet_s msg;
    packet_s *m_ptr = &msg;

    while(write_count < wdata->test_size){
        msleep(100);
        generate_packet(m_ptr,write_count);    

        if (buf_pos >= BUFFERSIZE){
            buf_pos = 0;
            writer = wdata->shmp->shm;
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
    pthread_exit(NULL);
    
}

void* t_read_packets(void *arg){
    thread_data_s *rdata=(thread_data_s *)arg;
    //rdata->shm = get_shared_memory(get_size(),RB_ID); // start of buffer
    packet_s *reader = rdata->shmp->shm; //local point
    packet_s msg;
    packet_s *m_ptr = &msg;
    uint32_t err_count = 0;
    uint32_t read_count = 0;
    uint32_t cont_count = 0;  
    uint8_t buf_pos = 0;
    int8_t prev_seq = -1;
    
    while (read_count < rdata->test_size){
        msleep(100);
        if(cont_count > BUFFERSIZE){
            break;
        }

        if (buf_pos >= BUFFERSIZE) {
            buf_pos = 0;
            reader = rdata->shmp->shm;
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
    pthread_exit(NULL);
}


/*
This test case needs locks on the critical section and better synchronization. 

The case passes sometimes and fails sometimes as their is a race condition b/w reader and writer
*/

int shm_test(){

    thread_data_s wdata,rdata;
    /* set to size of buffer to prevent looping back to start on RB*/
    wdata.test_size = BUFFERSIZE;
    rdata.test_size = BUFFERSIZE;

    void *shm = create_shared_memory(get_size(),RB_ID);
    clear_shared_memory(shm, get_size());
    write_packets((void*)&wdata);
    read_packets((void*)&rdata);
    // printf("write_packets - write count: %d\n",wdata.count);
    // printf("read_packets - read count: %d, error count: %d \n",rdata.count,rdata.err_count);
    if (rdata.err_count == 0){
        return 0;
    }
    else{
        return 1;
    }
}

int sem_test(){
    return 0;
}

int sem_shm_test(){
    void *tret;
    int32_t err;
    pthread_t r_thread, w_thread;
    thread_data_s wdata,rdata;
    shm_sem_s *shmp;
    
    shmp->shm = create_shared_memory(get_size(),RB_ID);
    clear_shared_memory(shmp->shm, get_size());

    if (sem_init(&shmp->sem1, 1, 0) == -1){
        perror("sem_init");
        exit(1);
    }
    if (sem_init(&shmp->sem2, 1, 0) == -1){
        perror("sem_init");
        exit(1);
    }

    wdata.test_size = TESTSIZE;
    rdata.test_size = TESTSIZE;
    wdata.shmp = &shmp;
    rdata.shmp = &shmp;
    
    err = pthread_create(&w_thread,NULL,t_write_packets,(void*)&wdata);
    if (err !=0){
        perror("could not create thread");
    }
    err = pthread_create(&r_thread,NULL,t_read_packets,(void*)&rdata);
    if (err !=0){
        perror("could not create thread");
    }

    err = pthread_join(w_thread,&tret);
    printf("w_thread exited with code: %d, write count: %d\n",err,wdata.count);
    err = pthread_join(r_thread,&tret);
    printf("r_thread exited with code: %d, read count: %d, error count: %d\n",err,rdata.count,rdata.err_count);
    return 0;
}

// int posix_shm_test(){
//     int fd;
//     //char* shmpath;
//     shm_sem_s *shmp;
//     char buffer[PATH_BUFF_SIZE];
// 	memset((char*)buffer,0,PATH_BUFF_SIZE); // clear buffer
//     readlink("/proc/self/exe", buffer, PATH_BUFF_SIZE); // gets file path of the calling proc
// 	const char* p_buf = buffer; // need a const char*

//     fd = shm_open(p_buf,O_CREAT | O_EXCL | O_RDWR, 0600);
//     if (fd ==-1){
//         perror("shm_open");
//         exit(1);
//     }
//     if (ftruncate(fd,sizeof(shm_sem_s)) ==-1){
//         perror("ftruncate");
//         exit(1);
//     }

//     shmp = mmap(NULL,sizeof(*shmp),PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
//     if (shmp == MAP_FAILED){
//         perror("mmap");
//         exit(1);
//     }


//     return 0;
// }

int main(int argc, char** argv){
    // int ret_val = 0;
    // ret_val = shm_test();
    // if (ret_val == 0){
    //     printf("basic shm tests passed\n");
    // }
    // ret_val = sem_test();
    // ret_val = posix_shm_test();
    sem_shm_test();
    
    return 0;
}