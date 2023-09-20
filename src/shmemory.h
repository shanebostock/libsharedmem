#ifndef SHMEMORY_H
#define SHMEMORY_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define KEY 1234565


typedef struct packet {
	int done;
	int seq;
	char msgbuf[64];
} packet_s;

size_t get_shm_size(){
	return sizeof(packet_s)*10;
} 
//fix the variable name from shmsize to function call

key_t get_key(){
	return KEY;
}

int get_id_new(){
	int shmid;
	key_t key = get_key();
	if((shmid = shmget(key,get_shm_size(),IPC_CREAT | 0666)) <0){
 		perror("shmid");
 		exit(0);
 	}
 	return shmid;
}

int get_id(){
	int shmid;
	key_t key = get_key();
	if((shmid = shmget(key,get_shm_size(),0666)) <0){
 		perror("shmget");
 		exit(0);
 	}
 	return shmid;
}

packet_s* create_shared_memory(){

	int shmid = get_id_new();
	packet_s *shm;
	
    if ((shm = (packet_s*) shmat(shmid, NULL, 0)) == (packet_s *) -1) {
        perror("shmat");
        exit(0);
    }
    return shm;
}

packet_s* get_shared_memory(){

	int shmid = get_id();
	packet_s *shm;
	
    if ((shm = (packet_s*) shmat(shmid, NULL, 0)) == (packet_s *) -1) {
        perror("shmat");
        exit(0);
    }
    return shm;
}

#endif /* SHMEMORY_H */