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

int increment = 0;

void get_random_plate_from_file(char *plate) {
    srand((int) time(NULL) + increment);
    increment++;
    // open the file
    FILE* file = fopen("plates.txt", "r");
    // get the number of lines in the file
    int lines = 0;
    char c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            lines++;
        }
    }
    // pick a random line
    int line = rand() % lines;
    // go to the start of the file
    rewind(file);
    // go to the start of the random line
    for (int i = 0; i < line; i++) {
        while((c = fgetc(file)) != '\n');
    }
    fgets(plate, 7, file);
    // close the file
    fclose(file);
};

int main(){
    int shm_fd = shm_open("PARKING", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, 2930);
    void* ptr = mmap(NULL, 2930, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // print the size of pthread_mutex_t
    shm_unlink("PARKING");

    char plate[6];
    for(int i = 0; i < 100; i++){
        get_random_plate_from_file(&plate[0]);
        printf("Plate: %s\n", plate);
        printf("Int: %d\n", plate[0]);
    }
    return 0;
}