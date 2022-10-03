#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "simulator.h"
#include "common.h"

int main() {
    // Setup the shared memory segement
    shm_fd = shm_open("PARKING", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SIZE);
    Parking = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Initialize the mutexes and conditions
    for (int i = 0; i < ENTRANCES; i++) {
        pthread_mutex_init(&Parking->entrances[i].LPR.mlock, NULL);
        pthread_cond_init(&Parking->entrances[i].LPR.condition, NULL);
        pthread_mutex_init(&Parking->entrances[i].boom_gate.mlock, NULL);
        pthread_cond_init(&Parking->entrances[i].boom_gate.condition, NULL);
        pthread_mutex_init(&Parking->entrances[i].information_sign.mlock, NULL);
        pthread_cond_init(&Parking->entrances[i].information_sign.condition, NULL);
    }
    for (int i = 0; i < EXITS; i++) {
        pthread_mutex_init(&Parking->exits[i].LPR.mlock, NULL);
        pthread_cond_init(&Parking->exits[i].LPR.condition, NULL);
        pthread_mutex_init(&Parking->exits[i].boom_gate.mlock, NULL);
        pthread_cond_init(&Parking->exits[i].boom_gate.condition, NULL);
    }
    for (int i = 0; i < LEVELS; i++) {
        pthread_mutex_init(&Parking->levels[i].LPR.mlock, NULL);
        pthread_cond_init(&Parking->levels[i].LPR.condition, NULL);
    }
    
    return 0;
};

void new_car() {
    struct Car Auto;
    get_random_plate(Auto.plate);
    send_to_random_entrance(Auto);
};

void generate_plate(char *plate) {
    srand(time(NULL));
    for (int i = 0; i < 3; i++) {
        plate[i] = (char) (rand() % 10 + '0');
    }
    // for the last 3 characters, generate a random capital letter
    for (int i = 3; i < 6; i++) {
        plate[i] = rand() % 26 + 65;
    }
};

void get_random_plate_from_file(char *plate) {
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
};

void get_random_plate(char* plate) {
    srand(time(NULL));
    if (rand() % 2 == 0) {
        generate_plate(plate);
    }
    else {
        get_random_plate_from_file(plate);
    }
};

char get_display(int entry) {
    pthread_mutex_lock(&Parking->entrances[entry].information_sign.mlock);
    pthread_cond_wait(&Parking->entrances[entry].information_sign.condition, &Parking->entrances[entry].information_sign.mlock);
    char display = Parking->entrances[entry].information_sign.display;
    pthread_mutex_unlock(&Parking->entrances[entry].information_sign.mlock);
    return display;
};

void send_to_random_entrance(struct Car Auto) {
    srand(time(NULL));
    int random_entrance = rand() % ENTRANCES;
    for (int i = 0; i < ENTRANCES; i++) {
        if (entrance_queue[random_entrance][i].plate == NULL) {
            entrance_queue[random_entrance][i] = Auto;
            break;
        }
    }
};

void send_plate(char plate[6], int entry) {
    pthread_mutex_lock(&Parking->entrances[entry].LPR.mlock);
    strcpy(Parking->entrances[entry].LPR.plate, plate);
    pthread_cond_signal(&Parking->entrances[entry].LPR.condition);
    pthread_mutex_unlock(&Parking->entrances[entry].LPR.mlock);
};

void send_car(int entry) {
    // read the plate
    char plate[6];
    struct Car Auto = entrance_queue[entry][0];
    strcpy(plate, Auto.plate);
    // move the queue forward
    for (int i = 1; i < LEVEL_CAPACITY; i++) {
        if (entrance_queue[entry][i].plate == NULL) {
            break;
        }
        else {
            entrance_queue[entry][i - 1] = entrance_queue[entry][i];
        }
    }
    // send the plate to the LPR
    send_plate(plate, entry);
    // wait for a digital sign signal before proceeding
    Auto.level = get_display(entry);
};