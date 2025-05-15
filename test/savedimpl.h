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

typedef struct packet {
	uint8_t done;
	uint8_t seq;
	char msgbuf[64];
} packet_s;

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

static void usage(const char *pname) {
    fprintf(stderr, "Usage: %s [-cx] pathname proj-id num-sems\n",
            pname);
    fprintf(stderr, "    -c           Use IPC_CREAT flag\n");
    fprintf(stderr, "    -x           Use IPC_EXCL flag\n");
    exit(EXIT_FAILURE);
}



int sem_shm_test(){
    void *tret;
    int32_t err;
    pthread_t r_thread, w_thread;
    thread_data_s wdata,rdata;
    shm_sem_s *shmp_p;

    // shmp_p->shm = create_shared_memory(get_size(),RB_ID);
    // clear_shared_memory(shmp->shm, get_size());

    if (sem_init(shmp_p->sem1, 1, 0) == -1){
        perror("sem_init");
        exit(1);
    }
    if (sem_init(shmp_p->sem2, 1, 0) == -1){
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

int posix_shm_test(){
    int fd;
    //char* shmpath;
    shm_sem_s *shmp;
    char buffer[PATH_BUFF_SIZE];
	memset((char*)buffer,0,PATH_BUFF_SIZE); // clear buffer
    readlink("/proc/self/exe", buffer, PATH_BUFF_SIZE); // gets file path of the calling proc
	const char* p_buf = buffer; // need a const char*

    fd = shm_open(p_buf,O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd ==-1){
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(fd,sizeof(shm_sem_s)) ==-1){
        perror("ftruncate");
        exit(1);
    }

    shmp = mmap(NULL,sizeof(*shmp),PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED){
        perror("mmap");
        exit(1);
    }
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

int sem_test(){
    return 0;
}
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
void basic_io_test(){
    char buf[MEM_SIZE];
    char *p_buf = buf;
    char end_msg[] = "goodbye\n";
    
    do{
        printf("please write a message:  ");
        fgets(p_buf,MEM_SIZE,stdin);
        printf("You Wrote: %s\n",buf);
    } while (strcmp(buf,end_msg)!=0);
    printf("end of test\n");
}

void* string_read_once(void* _ids){
    shm_sem_s *ids=(shm_sem_s *)_ids;
    char           *addr;
    union semun arg;
    struct sembuf  sop;

    /* Attach shared memory into our address space. */

    addr = shmat(ids->shmid, NULL, SHM_RDONLY);
    if (addr == (void *) -1)
        perror("shmat");

    /* Initialize semaphore 0 in set with value 1. */

    arg.val = 1;
    if (semctl(ids->semid, 0, SETVAL, arg) == -1)
        perror("semctl");

    printf("shmid = %d; semid = %d\n", ids->shmid, ids->semid);

    /* Wait for semaphore value to become 0. */

    sop.sem_num = 0;
    sop.sem_op = 0;
    sop.sem_flg = 0;

    if (semop(ids->semid, &sop, 1) == -1)
        perror("semop");

    /* Print the string from shared memory. */

    printf("%s\n", addr);
    pthread_exit(NULL);
}

void* string_write_once(void* _ids){
    shm_sem_s *ids=(shm_sem_s *)_ids;
    char *addr;
    size_t len;
    struct sembuf sop;
    char* msg = "Hello, from the writer\n";

    addr = shmat(ids->shmid, NULL, 0);
    if (addr == (void *) -1)
        perror("shmat");
    len = strlen(msg) +1;
    
    memcpy(addr, msg, len);
    msleep(1000); // wait 1s to see that the reader is waiting
    /* Decrement semaphore to 0. */

    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;

    if (semop(ids->semid, &sop, 1) == -1)
        perror("semop");

    pthread_exit(NULL);
}

void basic_threaded_test(void){

    int err;
    shm_sem_s ids;
    pthread_t r_thread, w_thread;
    union semun dummy;

    ids.shmid = shmget(IPC_PRIVATE, MEM_SIZE, IPC_CREAT | 0600);
    if (ids.shmid == -1)
        perror("shmget");

    ids.semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (ids.semid == -1)
        perror("semget");

    err = pthread_create(&r_thread,NULL,string_read_once,(void*)&ids);
    if (err !=0){
        perror("could not create thread");
    }
    err = pthread_create(&w_thread,NULL,string_write_once,(void*)&ids);
    if (err !=0){
        perror("could not create thread");
    }
    err = pthread_join(w_thread,NULL);
    if (err !=0){
        perror("could not join thread");
    }
    err = pthread_join(r_thread,NULL);
    if (err !=0){
        perror("could not join thread");
    }

    /* Remove shared memory and semaphore set. */

    if (shmctl(ids.shmid, IPC_RMID, NULL) == -1)
        perror("shmctl");
    if (semctl(ids.semid, 0, IPC_RMID, dummy) == -1)
        perror("semctl");
}

void* string_read_multi(void* _ids){
    shm_sem_s *ids=(shm_sem_s *)_ids;
    //semun_u arg;
    struct sembuf sop;
    char end_msg[] = "goodbye\n";
    void* p_shm;
    char* reader;
    /* Attach shared memory into our address space. */

    p_shm = shmat(ids->shmid, NULL, SHM_RDONLY);
    if (p_shm == (void *) -1){
        perror("shmat");
    }
        

        /*init, increment */
    union semun arg;
    arg.val = 1;
    
    if (semctl(ids->semid, 0, SETVAL, arg) == -1){
        perror("semctl");
        
    }
        // printf("shmid = %d; semid = %d\n", ids->shmid, ids->semid);
        sop.sem_num = 0;
        sop.sem_op = 0;
        sop.sem_flg = 0;
    
    do{
        /* Wait for semaphore value to become 0. */
        //wait    
        if (semop(ids->semid, &sop, 1) == -1)
            perror("semop");

        /* Print the string from shared memory. */
        reader = p_shm;
        printf("I read: ");
        printf("%s\n", reader);
        /* increment */
        arg.val = 1;
        if (semctl(ids->semid, 0, SETVAL, arg) == -1)
            perror("semctl");
        sop.sem_num = 0;
        sop.sem_op = 0;
        sop.sem_flg = 0;
    } while (strcmp(reader,end_msg)!=0);
    
    printf("reader exiting\n");
    pthread_exit(NULL);
}

void* string_write_multi(void* _ids){
    shm_sem_s *ids=(shm_sem_s *)_ids;
    char end_msg[] = "goodbye\n";
    struct sembuf  sop;
    char msg[MEM_SIZE];
    char *p_msg;
    p_msg = msg;
    void* p_shm;
    char* writer;
    

    p_shm = shmat(ids->shmid, NULL, 0);
    if (p_shm == (void *) -1)
        perror("shmat");
    
    do {
        /* set writer to start of shm */
        writer = p_shm;
        printf("Type a msg. When you are done type 'goodbye':   ");
           
        if(fgets(p_msg,MEM_SIZE,stdin) == NULL){
            perror("fgets");
            exit(1);
        }
        memcpy(writer, p_msg, 64);


        sop.sem_num = 0;
        sop.sem_op = -1;
        sop.sem_flg = 0;

        if (semop(ids->semid, &sop, 1) == -1)
            perror("semop");
        /* As soon as this sleep is removed there is deadlock created by the race condition */   
        msleep(500); 
        /* Decrement semaphore to 0. */
    } while(strcmp(msg,end_msg)!=0);
    printf("writer exiting\n");

    pthread_exit(NULL);    
}

void multi_line_threaded_test(){
    int err;
    shm_sem_s ids;
    pthread_t r_thread, w_thread;
    union semun dummy;

    ids.shmid = shmget(IPC_PRIVATE, MEM_SIZE, IPC_CREAT | 0600);
    if (ids.shmid == -1)
        perror("shmget");
    ids.semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (ids.semid == -1)
        perror("semget");

    err = pthread_create(&r_thread,NULL,string_read_multi,(void*)&ids);
    if (err !=0){
        perror("could not create thread");
    }
    err = pthread_create(&w_thread,NULL,string_write_multi,(void*)&ids);
    if (err !=0){
        perror("could not create thread");
    }
    err = pthread_join(w_thread,NULL);
    if (err !=0){
        perror("could not join thread");
    }
    err = pthread_join(r_thread,NULL);
    if (err !=0){
        perror("could not join thread");
    }
    /* Remove shared memory and semaphore set. */

    if (shmctl(ids.shmid, IPC_RMID, NULL) == -1)
        perror("shmctl");
    if (semctl(ids.semid, 0, IPC_RMID, dummy) == -1)
        perror("semctl");
}

void* string_read_multi_b(void* _ids){
    shm_sem_s *ids=(shm_sem_s *)_ids;
    //semun_u arg;
    struct sembuf sop;
    char end_msg[] = "goodbye\n";
    void* p_shm;
    char* reader;
    union semun arg; 
    /* Attach shared memory into our address space. */

    p_shm = shmat(ids->shmid, NULL, SHM_RDONLY);
    if (p_shm == (void *) -1){
        perror("shmat");
    }
        /*init, increment */

        // printf("shmid = %d; semid = %d\n", ids->shmid, ids->semid);
        sop.sem_num = 0;
        sop.sem_op = 0;
        sop.sem_flg = 0;
    
    do{
        /* Wait for semaphore value to become 0. */
        //wait    
        if (semop(ids->semid, &sop, 1) == -1)
            perror("semop");

        /* Print the string from shared memory. */
        reader = p_shm;
        printf("I read: ");
        printf("%s\n", reader);
        /* increment */
        arg.val = 1;
        if (semctl(ids->semid, 0, SETVAL, arg) == -1)
            perror("semctl");
        sop.sem_num = 0;
        sop.sem_op = 0;
        sop.sem_flg = 0;
    } while (strcmp(reader,end_msg)!=0);
    
    printf("reader exiting\n");
    pthread_exit(NULL);
}

// void* string_write_multi_b(void* _ids){
//     shm_sem_s *ids=(shm_sem_s *)_ids;
//     char end_msg[] = "goodbye\n";
//     struct sembuf  sop;
//     char msg[MEM_SIZE];
//     char *p_msg;
//     p_msg = msg;
//     void* p_shm;
//     char* writer;
    

//     p_shm = shmat(ids->shmid, NULL, 0);
//     if (p_shm == (void *) -1)
//         perror("shmat");
    
//     do {
//         /* set writer to start of shm */
//         writer = p_shm;
//         printf("Type a msg. When you are done type 'goodbye':   ");
           
//         if(fgets(p_msg,MEM_SIZE,stdin) == NULL){
//             perror("fgets");
//             exit(1);
//         }
//         memcpy(writer, p_msg, 64);


//         sop.sem_num = 0;
//         sop.sem_op = -1;
//         sop.sem_flg = 0;

//         if (semop(ids->semid, &sop, 1) == -1)
//             perror("semop");
//         /* As soon as this sleep is removed there is deadlock created by the race condition */   
//         msleep(500); 
//         /* Decrement semaphore to 0. */
//     } while(strcmp(msg,end_msg)!=0);
//     printf("writer exiting\n");

//     pthread_exit(NULL);    
// }

// void muli_line_threaded_b_test(void* _ids){
//     int err;
//     shm_sem_s *ids=(shm_sem_s *)_ids;
//     pthread_t r_thread, w_thread;
//     union semun arg, dummy;

//     ids->shmid = shmget(IPC_PRIVATE, MEM_SIZE, IPC_CREAT | 0600);
//     if (ids->shmid == -1)
//         perror("shmget");

//     ids->semid = semget(IPC_PRIVATE, 0, IPC_CREAT | 0600);
//     if (ids->semid == -1)
//         perror("semget");

//     arg.val = 1;
    
//     if (semctl(ids->semid, 0, SETVAL, arg) == -1){
//         perror("semctl");        
//     }
    
//     err = pthread_create(&r_thread,NULL,string_read_multi_b,(void*)&ids);
//     if (err !=0){
//         perror("could not create thread");
//     }
//     err = pthread_create(&w_thread,NULL,string_write_multi_b,(void*)&ids);
//     if (err !=0){
//         perror("could not create thread");
//     }
//     err = pthread_join(w_thread,NULL);
//     if (err !=0){
//         perror("could not join thread");
//     }
//     err = pthread_join(r_thread,NULL);
//     if (err !=0){
//         perror("could not join thread");
//     }
//     /* Remove shared memory and semaphore set. */

//     if (shmctl(ids->shmid, IPC_RMID, NULL) == -1)
//         perror("shmctl");
//     if (semctl(ids->semid, 0, IPC_RMID, dummy) == -1)
//         perror("semctl");    
// }