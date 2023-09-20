#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct packet {
	int done;
	int seq;
	char msgbuf[64];
} packet_s;

template <typename T>
class SharedMemory{

private:
size_t mBuffsize;
key_t mKey = 123456789;

size_t get_shm_size(){
	return sizeof(T)*mBuffsize;
} 

key_t get_key(){
	return mKey;
}

int get_id_new(){
	int shmid;
	key_t key = get_key();
	if((shmid = shmget(key,get_shm_size(),IPC_CREAT | 0666)) <0){
 		perror("shmget");
 	}
 	return shmid;
}

int get_id(){
	int shmid;
	key_t key = get_key();
	if((shmid = shmget(key,get_shm_size(),0666)) <0){
 		perror("shmget");
 	}
 	return shmid;
}
public:

SharedMemory(size_t buffsize){
	mBuffsize = buffsize;
}

/*

Creates new shared memory. 

TODO: Should check to see if its the first to create the segment with that key. 

*/

T* create_shared_memory(){

	int shmid = get_id_new();
	T *shm;
	
    if ((shm = (T*)shmat(shmid, NULL, 0)) == (T *) -1) {
        perror("shmat");
    }
    return shm;
}

/*

Returns pointer to pre-exisiting shared memory segment. 

*/

T* get_shared_memory(){

	int shmid = get_id();
	T *shm;
	
    if ((shm = (T*)shmat(shmid, NULL, 0)) == (T *) -1) {
        perror("shmat");
    }
    return shm;
}

read(){

}

write(){

}



};




#endif /* SHAREDMEMORY_H*/