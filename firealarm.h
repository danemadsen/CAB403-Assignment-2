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
                        CAB403 Assignment 2 - Fire Alarm Header File
------------------------------------------------------------------------------------------

Group: 88
Team Member: Dane Madsen
Student ID: n10983864
Student Email: n10983864@qut.edu.au

The roles of the fire alarm system:
● Monitor the status of temperature sensors on each car park level

● When a fire is detected, activate alarms on every car park level, open all boom gates
and display an evacuation message on the information signs
*/

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

pthread_t level_threads[LEVELS];

bool alarm_active = false;

void emergency_mode();
void *temperature_monitor(void *arg);
uint16_t median_temperature(volatile uint16_t temperatures[MEDIAN_SAMPLES]);
void print_temperature(uint16_t temperature);