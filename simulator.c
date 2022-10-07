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
#include <fcntl.h>
#include "simulator.h"
#include "common.h"

int main(){
    // Setup the shared memory segement
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SIZE);
    Parking = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    incremental_seed = 0;

    // Initialize the mutexes and conditions
    for (int i = 0; i < ENTRANCES; i++) {
        nums[i] = i;
        Parking->entrances[i].boom_gate.status = 'C';
        pthread_mutex_init(&entrance_queue_lock[i], NULL);
        pthread_cond_init(&entrance_queue_condition[i], NULL);
        pthread_mutex_init(&Parking->entrances[i].LPR.mlock, NULL);
        pthread_cond_init(&Parking->entrances[i].LPR.condition, NULL);
        pthread_mutex_init(&Parking->entrances[i].boom_gate.mlock, NULL);
        pthread_cond_init(&Parking->entrances[i].boom_gate.condition, NULL);
        pthread_mutex_init(&Parking->entrances[i].information_sign.mlock, NULL);
        pthread_cond_init(&Parking->entrances[i].information_sign.condition, NULL);
        pthread_create(&entrance_loop_thread[i], NULL, entrance_loop, &nums[i]);
    }
    for (int i = 0; i < EXITS; i++) {
        nums[i] = i;
        Parking->exits[i].boom_gate.status = 'C';
        pthread_mutex_init(&exit_queue_lock[i], NULL);
        pthread_cond_init(&exit_queue_condition[i], NULL);
        pthread_mutex_init(&Parking->exits[i].LPR.mlock, NULL);
        pthread_cond_init(&Parking->exits[i].LPR.condition, NULL);
        pthread_mutex_init(&Parking->exits[i].boom_gate.mlock, NULL);
        pthread_cond_init(&Parking->exits[i].boom_gate.condition, NULL);
        pthread_create(&exit_loop_threads[i], NULL, exit_loop, &nums[i]);
    }
    for (int i = 0; i < LEVELS; i++) {
        Parking->levels[i].temperature = 20;
        pthread_mutex_init(&Parking->levels[i].LPR.mlock, NULL);
        pthread_cond_init(&Parking->levels[i].LPR.condition, NULL);
    }

    // Initialise thread of temperature loop function
    pthread_create(&temperature_loop_thread, NULL, temperature_loop, NULL);
    // wait 1 seconds before starting the car loop
    sleep(1);
    car_generator_loop();
    return 0;
};

struct Car move_queue(struct Car Queue[LEVEL_CAPACITY], pthread_mutex_t *lock) {
    pthread_mutex_lock(lock);
    struct Car Auto = Queue[0];
    for (int i = 1; i < LEVEL_CAPACITY; i++) {
        if (Queue[i].plate == NULL) {
            break;
        }
        else {
            Queue[i - 1] = Queue[i];
        }
    }
    pthread_mutex_unlock(lock);
    return Auto;
};

void enter_car(int entry) {
    // Get the first car in the queue
    struct Car Auto = move_queue(entrance_queue[entry], &entrance_queue_lock[entry]);
    if (Auto.plate == NULL) return;
    // wait 2ms
    usleep(2000);
    // send the plate to the LPR
    send_plate(Auto.plate, &Parking->entrances[entry].LPR);
    // wait for a digital sign signal before proceeding
    Auto.level = get_display(Parking->entrances[entry].information_sign);
    // check if Auto.level is an approved char
    if (Auto.level < '1' || Auto.level > '5') return;
    open_boom_gate(&Parking->entrances[entry].boom_gate);
    // wait 10ms
    usleep(10000);
    send_plate(Auto.plate, &Parking->levels[((int) Auto.level) - 1].LPR);
    srand(get_seed());
    Auto.departure_time = ((int) get_seed()) + (rand() % 9901 + 100);
    close_boom_gate(&Parking->entrances[entry].boom_gate);
    // Find the first available car_threads pthread_t
    for (int i = 0; i < LEVELS*LEVEL_CAPACITY; i++) {
        if (car_threads[i] == 0) {
            // Create a new car thread
            pthread_create(&car_threads[i], NULL, car_instance, &Auto);
            break;
        }
    }
    printf("Car %s entered the parking lot at entrance %d and is parked on level %c\n", Auto.plate, entry, Auto.level);
};

void exit_car(int ext) {
    printf("exit_car(%d)", ext);
    struct Car Auto = move_queue(exit_queue[ext], &exit_queue_lock[ext]);
    if (Auto.plate == NULL) return;
    // wait 10ms
    usleep(10000);
    // send the plate to the LPR
    send_plate(Auto.plate, &Parking->exits[ext].LPR);
    open_boom_gate(&Parking->exits[ext].boom_gate);
    close_boom_gate(&Parking->exits[ext].boom_gate);
    printf("Car %s has left the parking lot\n", Auto.plate);
};

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

void get_random_plate(char* plate) {
    srand(get_seed());
    if (rand() % 2 == 0) {
        generate_plate(plate);
    }
    else {
        get_random_plate_from_file(plate);
    }
};

void send_plate(char plate[6], struct LicencePlateRecognition *LPR) {
    pthread_mutex_lock(&LPR->mlock);
    strcpy(LPR->plate, plate);
    pthread_cond_signal(&LPR->condition);
    pthread_mutex_unlock(&LPR->mlock);
};

char get_display(struct InformationSign sign) {
    pthread_mutex_lock(&sign.mlock);
    pthread_cond_wait(&sign.condition, &sign.mlock);
    char display = sign.display;
    pthread_mutex_unlock(&sign.mlock);
    return display;
};

void open_boom_gate(struct BoomGate *boom_gate) {
    pthread_mutex_lock(&boom_gate->mlock);
    while (boom_gate->status != 'R') {
        if(boom_gate->status == 'L') {
            boom_gate->status = 'C';
        }
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
        if (boom_gate->status == 'R') {
            boom_gate->status = 'O';
        }
        pthread_cond_wait(&boom_gate->condition, &boom_gate->mlock);
    }
    // wait 10ms
    usleep(10000);
    boom_gate->status = 'C';
    pthread_cond_signal(&boom_gate->condition);
    pthread_mutex_unlock(&boom_gate->mlock);
};

void send_to_random_entrance(struct Car Auto) {
    srand(get_seed());
    int random_entrance = rand() % ENTRANCES;
    pthread_mutex_lock(&entrance_queue_lock[random_entrance]);
    for (int i = 0; i < ENTRANCES; i++) {
        if (entrance_queue[random_entrance][i].plate == NULL) {
            entrance_queue[random_entrance][i] = Auto;
            break;
        }
    }
    pthread_cond_signal(&entrance_queue_condition[random_entrance]);
    pthread_mutex_unlock(&entrance_queue_lock[random_entrance]);
};

void send_to_random_exit(struct Car Auto) {
    srand(get_seed());
    int random_exit = rand() % EXITS;
    pthread_mutex_lock(&exit_queue_lock[random_exit]);
    for (int i = 0; i < EXITS; i++) {
        if (exit_queue[random_exit][i].plate == NULL) {
            exit_queue[random_exit][i] = Auto;
            break;
        }
    }
    pthread_cond_signal(&exit_queue_condition[random_exit]);
    pthread_mutex_unlock(&exit_queue_lock[random_exit]);
};

void set_random_temperature(int lvl){
    srand(get_seed()*(lvl+1));
    // add a ramdom value between -3 and 3 to the current temperature
    Parking->levels[lvl].temperature += rand() % 7 - 3;
};

void car_generator_loop() {
    while (1) {
        srand(get_seed());
        // wait 1-100ms
        usleep((rand() % 100000) + 1000);
        // create a new car
        struct Car Auto;
        get_random_plate(Auto.plate);
        send_to_random_entrance(Auto);
    }
};

int get_seed() {
    pthread_mutex_lock(&seed_lock);
    int seed = incremental_seed * time(NULL);
    incremental_seed++;
    pthread_mutex_unlock(&seed_lock);
    return seed;
};

void *temperature_loop(void *arg) {
    while (1) {
        srand(get_seed());
        // wait 1-5ms
        usleep((rand() % 5000) + 1000);
        for (int i = 0; i < LEVELS; i++) {
            set_random_temperature(i);
        }
    }
};

void *entrance_loop(void *arg) {
    int entry = *((int *) arg);
    printf("Entrance loop %d started\n", entry);
    while (1) {
        printf("Entrance loop %d entered\n", entry);
        pthread_mutex_lock(&entrance_queue_lock[entry]);
        while (entrance_queue[entry][0].plate == NULL) {
            pthread_cond_wait(&entrance_queue_condition[entry], &entrance_queue_lock[entry]);
        }
        pthread_mutex_unlock(&entrance_queue_lock[entry]);
        enter_car(entry);
        printf("Entrance loop %d passed\n", entry);
    }
    printf("Entrance loop %d ended\n", entry);
};

void *exit_loop(void *arg) {
    int ext = *((int *) arg);
    while (1) {
        pthread_mutex_lock(&exit_queue_lock[ext]);
        while (exit_queue[ext][0].plate == NULL) {
            pthread_cond_wait(&exit_queue_condition[ext], &exit_queue_lock[ext]);
        }
        pthread_mutex_unlock(&exit_queue_lock[ext]);
        exit_car(ext);
    }
};

void *car_instance(void *arg) {
    struct Car Auto = *((struct Car *) arg);
    srand(get_seed());
    // wait 1-100ms
    usleep((rand() % 100000) + 1000);
    // send the car to a random exit
    send_to_random_exit(Auto);
};