/*
                             888888888               888888888     
                           88:::::::::88           88:::::::::88   
                         88:::::::::::::88       88:::::::::::::88 
                        8::::::88888::::::8     8::::::88888::::::8
                        8:::::8     8:::::8     8:::::8     8:::::8
                        8:::::8     8:::::8     8:::::8     8:::::8
                         8:::::88888:::::8       8:::::88888:::::8 
                          8:::::::::::::8         8:::::::::::::8  
                         8:::::88888:::::8       8:::::88888:::::8 
                        8:::::8     8:::::8     8:::::8     8:::::8
                        8:::::8     8:::::8     8:::::8     8:::::8
                        8:::::8     8:::::8     8:::::8     8:::::8
                        8::::::88888::::::8     8::::::88888::::::8
                         88:::::::::::::88       88:::::::::::::88 
                           88:::::::::88           88:::::::::88   
                             888888888               888888888   

------------------------------------------------------------------------------------------
                            CAB403 Assignment 2 - Simulator
------------------------------------------------------------------------------------------

Group: 88
Team Member: Dane Madsen
Student ID: n10983864
Student Email: n10983864@qut.edu.au

The roles of the simulator:
● Simulate cars:
    ○ A simulated car receives a random license plate (sometimes on the list,
    sometimes not) and queues up at a random entrance to the car park,
    triggering an LPR when it reaches the front of the queue.

    ○ After triggering the LPR, the simulated car will watch the digital sign. If the
    sign contains a number, it will keep note of that number (the level where the
    car has been instructed to park) and then wait for the boom gate to open. If
    the sign contains any other character, the simulated car will just leave the
    queue and drive off, disappearing from the simulation.

    ○ After the boom gate opens, the car will drive to the level it was instructed to
    drive to, triggering the level LPR in the process.

    ○ The car will then park for a random amount of time.

    ○ After the car has finished parking, it will leave, setting off the level LPR again.
    It will then drive towards a random exit. Upon reaching that exit, it will set off
    the exit LPR and wait for the boom gate to open. Once the boom gate is
    open, it will leave the car park and disappear from the simulation.

● Simulate boom gates:
    ○ Boom gates take a certain amount of time to open and close. Once the
    manager has instructed a closed boom gate to open or an open boom gate to
    close, the simulator’s job is to wait for a small amount of time before putting
    the boom gate into the open/closed state.

● Simulate temperature:
    ○ Each level of the car park has a temperature sensor, sending back the current
    temperature (in degrees celsius). The simulator will frequently update these
    values with reasonable random values. The simulator should also be able to
    simulate a fire by generating higher values, in order to test / demonstrate the
    fire alarm system.
*/

// Language: c
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
    pthread_mutex_init(&entrance_queue_lock, NULL);
    pthread_cond_init(&entrance_queue_condition, NULL);
    pthread_mutex_init(&parked_cars_mlock, NULL);
    pthread_cond_init(&parked_cars_condition, NULL);
    
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

char get_display(struct InformationSign sign) {
    pthread_mutex_lock(&sign.mlock);
    pthread_cond_wait(&sign.condition, &sign.mlock);
    char display = sign.display;
    pthread_mutex_unlock(&sign.mlock);
    return display;
};

struct Car move_queue(struct Car Queue[LEVEL_CAPACITY]) {
    pthread_mutex_lock(&entrance_queue_lock);
    struct Car Auto = Queue[0];
    for (int i = 1; i < LEVEL_CAPACITY; i++) {
        if (Queue[i].plate == NULL) {
            break;
        }
        else {
            Queue[i - 1] = Queue[i];
        }
    }
    pthread_mutex_unlock(&entrance_queue_lock);
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
    pthread_mutex_unlock(&parked_cars_mlock);
};

// sort cars in parked_cars by lowest to highest departure_time
void sort_parked_cars() {
    pthread_mutex_lock(&parked_cars_mlock);
    for (int i = 0; i < LEVELS*LEVEL_CAPACITY; i++) {
        for (int j = i + 1; j < LEVELS*LEVEL_CAPACITY; j++) {
            if (parked_cars[i].departure_time > parked_cars[j].departure_time) {
                struct Car temp = parked_cars[i];
                parked_cars[i] = parked_cars[j];
                parked_cars[j] = temp;
            }
        }
    }
    pthread_mutex_unlock(&parked_cars_mlock);
};

void get_next_car(struct Car *Auto) {
    pthread_mutex_lock(&parked_cars_mlock);
    int i = 0;
    while(parked_cars[i].plate == NULL && i < LEVELS*LEVEL_CAPACITY) {
        i++;
    }
    *Auto = parked_cars[i];
    if(Auto->departure_time < ((intptr_t) time(NULL))) {
        *parked_cars[i].plate = '\0';
    }
    else {
        *Auto->plate = '\0';
    }
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
    pthread_mutex_lock(&entrance_queue_lock);
    for (int i = 0; i < ENTRANCES; i++) {
        if (entrance_queue[random_entrance][i].plate == NULL) {
            entrance_queue[random_entrance][i] = Auto;
            break;
        }
    }
    pthread_mutex_unlock(&entrance_queue_lock);
};

void send_to_random_exit(struct Car Auto) {
    srand(time(NULL));
    int random_exit = rand() % EXITS;
    pthread_mutex_lock(&exit_queue_lock);
    for (int i = 0; i < EXITS; i++) {
        if (exit_queue[random_exit][i].plate == NULL) {
            exit_queue[random_exit][i] = Auto;
            break;
        }
    }
    pthread_mutex_unlock(&exit_queue_lock);
};

void send_plate(char plate[6], struct LicencePlateRecognition *LPR) {
    pthread_mutex_lock(&LPR->mlock);
    strcpy(LPR->plate, plate);
    pthread_cond_signal(&LPR->condition);
    pthread_mutex_unlock(&LPR->mlock);
};

void enter_car(int entry) {
    // Get the first car in the queue
    struct Car Auto = move_queue(entrance_queue[entry]);
    // wait 2ms
    usleep(2000);
    // send the plate to the LPR
    send_plate(Auto.plate, &Parking->entrances[entry].LPR);
    // wait for a digital sign signal before proceeding
    Auto.level = get_display(Parking->entrances[entry].information_sign);
    // check if Auto.level is an approved char
    if (Auto.level < '1' || Auto.level > '5') return;
    open_boom_gate(&Parking->entrances[entry].boom_gate);
    // Log the time in unix millis to the car arrival_time
    Auto.arrival_time = (int) time(NULL);
    // wait 10ms
    usleep(10000);
    send_plate(Auto.plate, &Parking->levels[((int) Auto.level) - 1].LPR);
    srand(time(NULL));
    Auto.departure_time = Auto.arrival_time + (rand() % 9901 + 100);
    close_boom_gate(&Parking->entrances[entry].boom_gate);
    add_car(Auto);
};

void exit_car(int ext) {
    struct Car Auto;
    get_next_car(&Auto);
    // wait 2ms
    usleep(2000);
    // send the plate to the LPR
    srand(time(NULL));
    int random_exit = rand() % EXITS;
    send_plate(Auto.plate, &Parking->exits[random_exit].LPR); // Here Dick head 8===========================================================================D
    // check if Auto.level is an approved char
    open_boom_gate(&Parking->exits[ext].boom_gate);
    // wait 10ms
    usleep(10000);
    send_plate(Auto.plate, &Parking->levels[((int) Auto.level) - 1].LPR);
    close_boom_gate(&Parking->exits[ext].boom_gate);
};
