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

Simulator timings:
● Every 1-100ms*, a new car will be generated by the simulator with a random license
plate, and will start moving towards a random entrance.

● Once a car reaches the front of the queue, it will wait 2ms before triggering the
entrance LPR.

● Boom gates take 10ms to fully open and 10ms to fully close.

● After the boom gate is open, the car takes another 10ms to drive its parking space
(triggering the level LPR for the first time).

● Once parked, the car will wait 100-10000ms before departing the level (and triggering
the level LPR for the second time).

● It then takes the car a further 10ms to drive to a random exit and trigger the exit LPR.

● Every 1-5ms, the temperature on each level will change to a random value.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include "simulator.h"
#include "common.h"

int main(){
    // Ensure no shared memory segemnt already exists
    shm_unlink(SHM_NAME);
    // Setup the shared memory segement
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SIZE);
    Parking = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    incremental_seed = 0;
    // initialise ParkedCars to all have plates of '000000'
    for (int i = 0; i < LEVELS*LEVEL_CAPACITY; i++){
        strcpy(ParkedCars[i].plate, "000000");
        ParkedCars[i].departure_time = 0;
    }

    // initialise the mutexes and conditions to be PTHREAD_PROCESS_SHARED
    pthread_mutexattr_init(&shared_mutex_attr);
    pthread_mutexattr_setpshared(&shared_mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&shared_cond_attr);
    pthread_condattr_setpshared(&shared_cond_attr, PTHREAD_PROCESS_SHARED);

    // Initialize the mutexes and conditions
    for (int i = 0; i < ENTRANCES; i++) {
        nums[i] = i;
        Parking->entrances[i].boom_gate.status = 'C';
        pthread_mutex_init(&entrance_lock[i], &shared_mutex_attr);
        pthread_cond_init(&entrance_condition[i], &shared_cond_attr);
        pthread_mutex_init(&Parking->entrances[i].LPR.mlock, &shared_mutex_attr);
        pthread_cond_init(&Parking->entrances[i].LPR.condition, &shared_cond_attr);
        pthread_mutex_init(&Parking->entrances[i].boom_gate.mlock, &shared_mutex_attr);
        pthread_cond_init(&Parking->entrances[i].boom_gate.condition, &shared_cond_attr);
        pthread_mutex_init(&Parking->entrances[i].information_sign.mlock, &shared_mutex_attr);
        pthread_cond_init(&Parking->entrances[i].information_sign.condition, &shared_cond_attr);
    }
    for (int i = 0; i < EXITS; i++) {
        nums[i] = i;
        Parking->exits[i].boom_gate.status = 'C';
        pthread_mutex_init(&exit_lock[i], &shared_mutex_attr);
        pthread_cond_init(&exit_condition[i], &shared_cond_attr);
        pthread_mutex_init(&Parking->exits[i].LPR.mlock, &shared_mutex_attr);
        pthread_cond_init(&Parking->exits[i].LPR.condition, &shared_cond_attr);
        pthread_mutex_init(&Parking->exits[i].boom_gate.mlock, &shared_mutex_attr);
        pthread_cond_init(&Parking->exits[i].boom_gate.condition, &shared_cond_attr);
    }
    for (int i = 0; i < LEVELS; i++) {
        Parking->levels[i].temperature = 20;
        pthread_mutex_init(&Parking->levels[i].LPR.mlock, &shared_mutex_attr);
        pthread_cond_init(&Parking->levels[i].LPR.condition, &shared_cond_attr);
    }

    pthread_create(&entrance_loop_thread, NULL, entrance_loop, NULL);
    pthread_create(&exit_loop_thread, NULL, exit_loop, NULL);
    //temperature_loop();
    while(1);
    return 0;
};

Car_t get_departing(){
    while(1){
        for (int i = 0; i < LEVELS*LEVEL_CAPACITY; i++){
            if (ParkedCars[i].departure_time < ((int) time(NULL)) && ParkedCars[i].departure_time != 0){
                return ParkedCars[i];
            }
        }
    }
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

void get_random_plate_from_file(char *plate) {
    srand(get_seed());
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
};

void get_random_plate(char* plate) {
    srand(get_seed());
    if (rand() % 2 == 0) {
        generate_plate(plate);
    }
    else {
        get_random_plate_from_file(plate);
    }
    printf("Plate: %s\n", plate);
};

void send_plate(char plate[6], LPR_t *lpr) {
    pthread_mutex_lock(&lpr->mlock);
    strcpy(lpr->plate, plate);
    pthread_mutex_unlock(&lpr->mlock);
    pthread_cond_signal(&lpr->condition);
};

char get_display(Sign_t *sign) {
    pthread_mutex_lock(&sign->mlock);
    printf("Parking: %d\n", Parking->entrances[0].information_sign.display);
    printf("Parking: %d\n", Parking->entrances[1].information_sign.display);
    while(sign->display == '\0') {
        printf("get_display: sign->display = %d\n", sign->display);
        pthread_cond_wait(&sign->condition, &sign->mlock);
    }
    printf("passed");
    char display = sign->display;
    sign->display = '\0';
    pthread_mutex_unlock(&sign->mlock);
    return display;
};

void open_boom_gate(BoomGate_t *boom_gate) {
    pthread_mutex_lock(&boom_gate->mlock);
    if(boom_gate->status == 'L') boom_gate->status = 'C';
    while (boom_gate->status != 'R') {
        pthread_cond_wait(&boom_gate->condition, &boom_gate->mlock);
    }
    // wait 10ms
    usleep(10000);
    boom_gate->status = 'O';
    pthread_mutex_unlock(&boom_gate->mlock);
    pthread_cond_signal(&boom_gate->condition);
};

void close_boom_gate(BoomGate_t *boom_gate) {
    pthread_mutex_lock(&boom_gate->mlock);
    if (boom_gate->status == 'R') boom_gate->status = 'O';
    while (boom_gate->status != 'L') {
        pthread_cond_wait(&boom_gate->condition, &boom_gate->mlock);
    }
    // wait 10ms
    usleep(10000);
    boom_gate->status = 'C';
    pthread_mutex_unlock(&boom_gate->mlock);
    pthread_cond_signal(&boom_gate->condition);
};

int get_seed() {
    pthread_mutex_lock(&seed_lock);
    int seed = incremental_seed * time(NULL);
    incremental_seed++;
    pthread_mutex_unlock(&seed_lock);
    return seed;
};

void *entrance_loop(void *arg) {
    while (1) {
        srand(get_seed());
        // wait 1-100ms
        usleep((rand() % 100000) + 1000);
        // create a new car
        for (int i = 0; i < LEVELS*LEVEL_CAPACITY; i++) {
            if (car_threads[i] == 0) {
                // Create a new car thread
                pthread_create(&car_threads[i], NULL, car_entry, NULL);
                break;
            }
        }
    }
};

void *car_entry(void *arg) {
    Car_t Auto;
    get_random_plate(Auto.plate);
    
    srand(get_seed());
    int random_entrance = rand() % ENTRANCES;
    pthread_mutex_lock(&entrance_lock[random_entrance]);
    
    // wait 2ms
    usleep(2000);
    
    // Send the car to the entrance LPR
    send_plate(Auto.plate, &Parking->entrances[random_entrance].LPR);
    
    // Get the level from the information sign
    Auto.level = get_display(&Parking->entrances[random_entrance].information_sign);

    if (Auto.level >= '1' && Auto.level <= '5') {
        open_boom_gate(&Parking->entrances[random_entrance].boom_gate);
        // wait 10ms
        usleep(10000);
        send_plate(Auto.plate, &Parking->levels[((int) Auto.level) - 49].LPR);
        Auto.departure_time = ((int) time(NULL)) + (rand() % 9901 + 100);
        close_boom_gate(&Parking->entrances[random_entrance].boom_gate);
        for(int i = 0; i < LEVEL_CAPACITY; i++) {
            if (ParkedCars[i].plate == "000000") {
                ParkedCars[i] = Auto;
                break;
            }
        }
    }
    pthread_mutex_unlock(&entrance_lock[random_entrance]);
    pthread_cond_signal(&entrance_condition[random_entrance]);
};

void *exit_loop(void *arg) {
    while (1) {
        Car_t Auto = get_departing();
        for (int i = 0; i < LEVELS*LEVEL_CAPACITY; i++) {
            if (car_threads[i] == 0) {
                // Create a new car thread
                pthread_create(&car_threads[i], NULL, car_exit, &Auto);
                break;
            }
        }
    }
};

void *car_exit(void *arg) {
    Car_t Auto = *((Car_t *) arg);
    srand(get_seed());
    int random_exit = rand() % EXITS;
    pthread_mutex_lock(&exit_lock[random_exit]);
    
    // wait 10ms
    usleep(10000);
    
    // Send the car to the exit LPR
    send_plate(Auto.plate, &Parking->exits[random_exit].LPR);
    open_boom_gate(&Parking->exits[random_exit].boom_gate);
    // wait 10ms
    usleep(10000);
    close_boom_gate(&Parking->exits[random_exit].boom_gate);
    pthread_mutex_unlock(&exit_lock[random_exit]);
    pthread_cond_signal(&exit_condition[random_exit]);
};

void temperature_loop() {
    while (1) {
        srand(get_seed());
        // wait 1-5ms
        usleep((rand() % 5000) + 1000);
        for (int i = 0; i < LEVELS; i++) {
            srand(get_seed()*(i+1));
            Parking->levels[i].temperature += rand() % 7 - 3;
        }
    }
};