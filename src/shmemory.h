#ifndef SHMEMORY_H
#define SHMEMORY_H
#include <sys/types.h>
key_t get_key();
int get_id_new(size_t shmem_size);
int get_id(size_t shmem_size);
void* create_shared_memory(size_t shmem_size);
void* get_shared_memory(size_t shmem_size);


#endif /* SHMEMORY_H */