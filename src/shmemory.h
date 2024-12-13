#ifndef SHMEMORY_H
#define SHMEMORY_H
#include <sys/types.h>
#include <sys/sem.h>
#define PATH_BUFF_SIZE 4096
void* create_shared_memory(size_t shmem_size,int project_id);
void* get_shared_memory(size_t shmem_size,int project_id);
key_t get_key(int project_id);
int get_id_new(size_t shmem_size,key_t project_key);
int get_id(size_t shmem_size,key_t project_key);
void clear_shared_memory(void* shm,size_t shmem_size);
#endif /* SHMEMORY_H */