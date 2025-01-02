#ifndef SHMEMORY_H
#define SHMEMORY_H
#include <sys/types.h>
#include <sys/sem.h>
#include <stdint.h>

typedef struct shm_sem {
    int semid;
    int shmid;
} shm_sem_s;

union semun { /* Used in calls to semctl() */
    int                 val;
    struct semid_ds     *buf;
    unsigned short      *array;
    struct seminfo      *__buf;
};

int create_shared_memory(size_t shmem_size,int project_id);
void* get_shared_memory(size_t shmem_size,int project_id);
void* get_shared_memory_by_shmid(int shmid);
key_t get_key(int project_id);
int get_id_new(size_t shmem_size,key_t project_key);
int get_id(size_t shmem_size,key_t project_key);
void clear_shared_memory(void* shm,size_t shmem_size);

int create_sem(uint8_t nsems, int project_id);
#endif /* SHMEMORY_H */