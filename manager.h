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
                        CAB403 Assignment 2 - Manager Header File
------------------------------------------------------------------------------------------

Group: 88
Team Member: Dane Madsen
Student ID: n10983864
Student Email: n10983864@qut.edu.au

The roles of the manager:
● Monitor the status of the LPR sensors and keep track of where each car is in the car
park

● Tell the boom gates when to open and when to close (the boom gates are a simple
piece of hardware that can only be told to open or close, so the job of automatically
closing the boom gates after they have been open for a little while is up to the
manager)

● Control what is displayed on the information signs at each entrance

● As the manager knows where each car is, it is the manager’s job to ensure that there
is room in the car park before allowing new vehicles in (number of cars < number of
levels * the number of cars per level). The manager also needs to keep track of how
full the individual levels are and direct new cars to a level that is not fully occupied

● Keep track of how long each car has been in the parking lot and produce a bill once
the car leaves.

● Display the current status of the parking lot on a frequently-updating screen, showing
how full each level is, the current status of the boom gates, signs, temperature
sensors and alarms, as well as how much revenue the car park has brought in so far.

Manager timings:
● After a boom gate has been fully opened, it will start to close 20ms later. Cars
entering the car park will just drive in if the boom gate is fully open after they have
been directed to a level (however, if the car arrives just as the boom gate starts to
close, it will have to wait for the boom gate to fully close, then fully open again.)

● Cars are billed based on how long they spend in the car park (see the Billing section
for more information.)
*/

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"

#define RATE 0.05

int shm_fd = -1;
CarPark_t* Parking;
double revenue;
int int_buffer;
char entrance_boom_gate_status[ENTRANCES];
char exit_boom_gate_status[EXITS];
char char_buffer;

Car_t parked_cars[LEVELS][LEVEL_CAPACITY];
int parked_cars_count[LEVELS];
pthread_mutex_t parked_cars_mlock;
pthread_cond_t parked_cars_condition;
pthread_mutex_t revenue_lock;

pthread_t entrance_threads[ENTRANCES];
pthread_t level_threads[LEVELS];
pthread_t exit_threads[EXITS];

bool check_plate(char *plate);
bool check_unique(char *plate);
bool check_space();
char get_level();

void *entrance_loop(void *arg);
void *level_loop(void *arg);
void *exit_loop(void *arg);
void display_loop();