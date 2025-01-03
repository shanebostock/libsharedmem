#ifndef SHMEMORY_H
#define SHMEMORY_H
#include <sys/types.h>
#include <sys/sem.h>
#include <stdint.h>

typedef struct shm_sem {
    int semid;
    int shmid;
} shm_sem_s;

// create a new shared memory segment
int create_shared_memory(size_t shmem_size,int project_id);
// get the shared memory by size & project id
/* TODO: THIS FUNCTION NEEDS A RETHINK */
void* get_shared_memory(size_t shmem_size,int project_id);
// get the shared memory segment by shmid
void* get_shared_memory_by_shmid(int shmid);
// clear the shared memory
/* TODO: THIS FUNCTION NEEDS A RETHINK */
void clear_shared_memory(void* shm,size_t shmem_size);

//create a new sem set
int create_sem(uint8_t nsems, int project_id);
// increment
int release_sem(int semid, int semnum);
// decrement
int signal_sem(int semid, int semnum);
// wait for signal
int try_wait(int semid,int semnum);

#endif /* SHMEMORY_H */