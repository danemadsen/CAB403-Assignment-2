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
#include <string.h>
#include "common.h"

volatile double revenue;
bool alarm_active = false;

Car_t parked_cars[LEVELS][LEVEL_CAPACITY];
Car_t arriving_cars[LEVELS*LEVEL_CAPACITY];
Car_t departing_cars[LEVELS*LEVEL_CAPACITY];
int parked_cars_count[LEVELS];
pthread_mutex_t parked_cars_mlock;
pthread_mutex_t arriving_cars_mlock;
pthread_mutex_t departing_cars_mlock;
pthread_mutex_t revenue_lock;

pthread_t entrance_threads[ENTRANCES];
pthread_t level_threads[LEVELS];
pthread_t exit_threads[EXITS];

void charge_car(Car_t *Auto);
void add_parked_car(Car_t Auto);
void add_arriving_car(Car_t Auto);
void add_departing_car(Car_t Auto);
void get_car(Car_t *Auto);
bool check_plate(char *plate);
bool check_unique(char *plate);
bool check_space();
bool check_alarm();
char get_level();
void raise_boom_gate(BoomGate_t *boom_gate);
void lower_boom_gate(BoomGate_t *boom_gate);
char get_boom_gate_status(BoomGate_t *boom_gate);
void set_sign(Sign_t *sign, char signal);
int get_level_index(Level_t *lvl);
int get_level_count(int level);
int get_total_count();

void *entrance_loop(void *arg);
void *level_loop(void *arg);
void *exit_loop(void *arg);
void display_loop();