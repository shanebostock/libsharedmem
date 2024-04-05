#include "shmemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#define KEY 123456

key_t get_key(){
	return KEY;
}

int get_id_new(size_t shmem_size){
	int shmid;
	key_t key = get_key();
	if((shmid = shmget(key,shmem_size,IPC_CREAT | 0666)) <0){
 		perror("shmid");
 		exit(1);
 	}
 	return shmid;
}

int get_id(size_t shmem_size){
	int shmid;
	key_t key = get_key();
	if((shmid = shmget(key,shmem_size,0666)) <0){
 		perror("shmget");
 		exit(1);
 	}
 	return shmid;
}

void* create_shared_memory(size_t shmem_size){

	int shmid = get_id_new(shmem_size);
	void *shm;
	
    if ( (shm = shmat(shmid, NULL, 0) ) == (void*) -1) {
        perror("shmat");
        exit(1);
    }
    return shm;
}

void* get_shared_memory(size_t shmem_size){

	int shmid = get_id(shmem_size);
	void *shm;
	
    if ((shm = shmat(shmid, NULL, 0)) == (void*) -1) {
        perror("shmat");
        exit(1);
    }
    return shm;
}
