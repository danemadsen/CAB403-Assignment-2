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
    
    // Initialise the boom_gates to have the status 'C'
    for (int i = 0; i < ENTRANCES; i++) {
        Parking->entrances[i].boom_gate.status = 'C';
    }
    for (int i = 0; i < EXITS; i++) {
        Parking->exits[i].boom_gate.status = 'C';
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

struct Car* move_queue(struct Car *Queue[LEVEL_CAPACITY]) {
    struct Car *Auto = Queue[0];
    for (int i = 1; i < LEVEL_CAPACITY; i++) {
        if (Queue[i]->plate == NULL) {
            break;
        }
        else {
            Queue[i - 1] = Queue[i];
        }
    }
    return Auto;
};

void add_car(struct Car Auto){
    pthread_mutex_lock(&parked_cars_mlock);
    for (int i = 0; i < LEVELS*LEVEL_CAPACITY; i++) {
        if (parked_cars[i].plate == NULL) {
            parked_cars[i] = Auto;
            break;
        }
    }
    pthread_cond_signal(&parked_cars_condition);
    pthread_mutex_unlock(&parked_cars_mlock);
};

void open_boom_gate(struct BoomGate *boom_gate) {
    pthread_mutex_lock(&boom_gate->mlock);
    while (boom_gate->status != 'R') {
        pthread_cond_wait(&boom_gate->condition, &boom_gate->mlock);
    }
    // wait 10ms
    usleep(10000);
    boom_gate->status = 'O';
    pthread_cond_signal(&boom_gate->condition);
    pthread_mutex_unlock(&boom_gate->mlock);
};

void close_boom_gate(struct BoomGate *boom_gate) {
    pthread_mutex_lock(&boom_gate->mlock);
    while (boom_gate->status != 'L') {
        pthread_cond_wait(&boom_gate->condition, &boom_gate->mlock);
    }
    // wait 10ms
    usleep(10000);
    boom_gate->status = 'C';
    pthread_cond_signal(&boom_gate->condition);
    pthread_mutex_unlock(&boom_gate->mlock);
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

void send_plate(char plate[6], struct LicencePlateRecognition *LPR) {
    pthread_mutex_lock(&LPR->mlock);
    strcpy(LPR->plate, plate);
    pthread_cond_signal(&LPR->condition);
    pthread_mutex_unlock(&LPR->mlock);
};

void enter_car(int entry) {
    // Get the first car in the queue
    struct Car *Auto = move_queue(entrance_queue[entry]);
    // wait 2ms
    usleep(2000);
    // send the plate to the LPR
    send_plate(Auto->plate, &Parking->entrances[entry].LPR);
    // wait for a digital sign signal before proceeding
    Auto->level = get_display(entry);
    // check if Auto.level is an approved char
    if (Auto->level < '1' || Auto->level > '5') return;
    open_boom_gate(&Parking->entrances[entry].boom_gate);
    // Log the time in unix millis to the car arrival_time
    Auto->arrival_time = get_time();
    // wait 10ms
    usleep(10000);
    send_plate(Auto->plate, &Parking->levels[((int) Auto->level) - 1].LPR);
    srand(time(NULL));
    Auto->departure_time = Auto->arrival_time + (rand() % 9901 + 100);
    close_boom_gate(&Parking->entrances[entry].boom_gate);
    add_car(*Auto);
};