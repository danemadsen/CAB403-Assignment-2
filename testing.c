#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
//#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

int main(){
    int shm_fd = shm_open("PARKING", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, 2930);
    void* ptr = mmap(NULL, 2930, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // print the size of pthread_mutex_t
    shm_unlink("PARKING");
    return 0;
}