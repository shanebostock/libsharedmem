#include "shmemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PATH_BUFF_SIZE 4096
/**********************/
/* INTNERAL DECS*/
/* types */
union semun { /* Used in calls to semctl() */
    int                 val;
    struct semid_ds     *buf;
    unsigned short      *array;
    struct seminfo      *__buf;
};

/* helper funcs */
key_t get_key(int project_id);
int32_t msleep(uint32_t ms);
void clear_shared_memory(void* shm, size_t shmem_size);

/* shared memory wrappers */
int get_id_new(size_t shmem_size, key_t project_key);
int get_id(size_t shmem_size, key_t project_key);

/* semaphore wrappers */
int update_sem(int semid, int semnum, union semun arg);


/**********************/
/* SHARED MEMORY EXTERNAL */
void* get_shared_memory(size_t shmem_size, int project_id){

	key_t project_key = get_key(project_id);
	int shmid = get_id(shmem_size,project_key);
	void *shm;
	
    if ((shm = shmat(shmid, NULL, 0)) == (void*) -1) {
        perror("shmat");
        exit(1);
    }
    return shm;
}

void* get_shared_memory_by_shmid(int shmid){
	
	void *shm;
    if ((shm = shmat(shmid, NULL, 0)) == (void*) -1) {
        perror("shmat");
        exit(1);
    }
    return shm;
}

int create_shared_memory(size_t shmem_size, int project_id){
	
	key_t project_key = get_key(project_id);
	int shmid = get_id_new(shmem_size,project_key);
	void *shm;
	
    if ( (shm = shmat(shmid, NULL, 0) ) == (void*) -1) {
        perror("shmat");
        exit(1);
    }
	clear_shared_memory(shm, shmem_size);
    return shmid;
}

int free_shared_memory(int shmid){

	    if (shmctl(shmid, IPC_RMID, NULL) == -1){
			perror("shmctl");
			return -1;
		}
        return 0;
}

/**********************/
/* SEMAPHORE EXTERNAL */
int create_sem(uint8_t nsems, int project_id){
    int id = 0;
    key_t project_key = get_key(project_id);
    id = semget(project_key, nsems, IPC_CREAT | 0600);
    if (id == -1){
        perror("semget");
    }
    return id;
}

/* release */
int release_sem(int semid, int semnum){
	union semun arg;
	arg.val = 1; /* increment */
	return update_sem(semid,semnum,arg);
}

/* signal */ 
int signal_sem(int semid, int semnum){
	union semun arg;
	arg.val = 0; /* decrement */
	return update_sem(semid,semnum,arg);
}


/* wait for signal */ 
int try_wait(int semid,int semnum){
    struct sembuf sop;
    sop.sem_num = semnum;
    sop.sem_op = 0; // try wait for zero
    sop.sem_flg = 0;
	if (semop(semid,&sop,1)==-1){
		perror("semop");
		return -1;
	}
	return 0;
}

int remove_sem_set(int semid){

	union semun dummy;
    /* when IPC_RMID is called - it removes the whole set the 2nd arg (semnum) is ignored.*/
    if (semctl(semid, 0, IPC_RMID, dummy) == -1){
		perror("semctl");
		return -1;
	}
	return 0;
        
}


/* INTNERAL FUNCS*/
/* helper funcs */
key_t get_key(int project_id){

	char buffer[PATH_BUFF_SIZE];
	memset((char*)buffer,0,PATH_BUFF_SIZE); /* clear buffer */ 
    readlink("/proc/self/exe", buffer, PATH_BUFF_SIZE); /* gets file path of the calling proc */ 
	const char* p_buf = buffer; /* need a const char* for ftok */ 
	return ftok(p_buf,project_id);
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

void clear_shared_memory(void* shm, size_t shmem_size){
	memset(shm,0,shmem_size);
}

/* shared memory wrappers */
int get_id_new(size_t shmem_size, key_t project_key){
	int shmid;
	int flags = 0;

	if((shmid = shmget(project_key,shmem_size,flags | IPC_CREAT | 0666)) <0){
 		perror("shmid");
 		exit(1);
 	}
 	return shmid;
}

int get_id(size_t shmem_size, key_t project_key){
	int shmid;
	int flags = 0;

	if((shmid = shmget(project_key,shmem_size,flags | 0666)) <0){
 		perror("shmget");
 		exit(1);
 	}
 	return shmid;
}
/* semaphore wrappers */
int update_sem(int semid, int semnum, union semun arg){

    if(semctl(semid,semnum,SETVAL,arg)==-1){
        perror("semctl");
        return -1;
    }
	return 0;	
}