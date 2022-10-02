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
#include "simulator.h"

void get_random_plate(char* plate) {
    srand(time(NULL));
    if (rand() % 2 == 0) {
        srand(time(NULL));
        for (int i = 0; i < 3; i++) {
            plate[i] = (char) (rand() % 10 + '0');
        }
        // for the last 3 characters, generate a random capital letter
        for (int i = 3; i < 6; i++) {
            plate[i] = rand() % 26 + 65;
        }
    }
    else {
        srand(time(NULL));
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
            fgets(plate, 7, file);
        }
        // close the file
        fclose(file);
    }
    
};

bool check_plate(char* plate) {
    FILE* file = fopen("plates.txt", "r");
    char c;
    char* file_plate = malloc(7);
    while ((c = fgetc(file)) != EOF) {
        fseek(file, -1, SEEK_CUR);
        fgets(file_plate, 7, file);
        if (strcmp(plate, file_plate) == 0) {
            free(file_plate);
            fclose(file);
            return true;
        }
        memset(file_plate, 0, 7);
    }
    free(file_plate);
    fclose(file);
    return false;
}

int main(){
    //int shm_fd = shm_open("PARKING", O_CREAT | O_RDWR, 0666);
    //ftruncate(shm_fd, 2930);
    //void* ptr = mmap(NULL, 2930, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // print the size of pthread_mutex_t
    char plate[6];
    get_random_plate(plate);

    // use check_plate to check if the plate is in the file and print the result
    if (check_plate(plate)) {
        printf("%s is in the file\n", plate);
    }
    else {
        printf("%s is not in the file\n", plate);
    }
}
