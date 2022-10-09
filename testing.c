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

    char plate;
    for(int i = 0; i < 100; i++){
        generate_plate(&plate);
        printf("%s\n", plate);
    }
    return 0;
}

void generate_plate(char *plate) {
    srand(get_seed());
    for (int i = 0; i < 3; i++) {
        plate[i] = (char) (rand() % 10 + '0');
    }
    // for the last 3 characters, generate a random capital letter
    for (int i = 3; i < 6; i++) {
        plate[i] = rand() % 26 + 65;
    }
};