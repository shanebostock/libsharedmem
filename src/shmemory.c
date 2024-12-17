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



/**************************************************************/
/*

The main entry points to the sharedmem library.

*/
void* get_shared_memory(size_t shmem_size,int project_id){

	key_t project_key = get_key(project_id);
	int shmid = get_id(shmem_size,project_key);
	void *shm;
	
    if ((shm = shmat(shmid, NULL, 0)) == (void*) -1) {
        perror("shmat");
        exit(1);
    }
    return shm;
}

void* create_shared_memory(size_t shmem_size, int project_id){
	
	key_t project_key = get_key(project_id);
	int shmid = get_id_new(shmem_size,project_key);
	void *shm;
	
    if ( (shm = shmat(shmid, NULL, 0) ) == (void*) -1) {
        perror("shmat");
        exit(1);
    }
    return shm;
}
/**************************************************************/

key_t get_key(int project_id){

	char buffer[PATH_BUFF_SIZE];
	memset((char*)buffer,0,PATH_BUFF_SIZE); // clear buffer
    readlink("/proc/self/exe", buffer, PATH_BUFF_SIZE); // gets file path of the calling proc
	const char* p_buf = buffer; // need a const char* for ftok
	return ftok(p_buf,project_id);
}

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


void clear_shared_memory(void* shm, size_t shmem_size){
	memset(shm,0,shmem_size);
}