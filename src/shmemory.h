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
/* mark shared memory for deletion */
int free_shared_memory(int shmid);

//create a new sem set
int create_sem(uint8_t nsems, int project_id);
// increment
int release_sem(int semid, int semnum);
// decrement
int signal_sem(int semid, int semnum);
// wait for signal
int try_wait(int semid,int semnum);
/* delete semaphores*/
int remove_sem_set(int shmid);
#endif /* SHMEMORY_H */