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

#pragma once
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <semaphore.h>
#include "common.h"

static bool alarm_active;
static sem_t alarm_sem;

void emergency_mode(void);
void check_alarm(void);
void evacuation_message(void);
void open_all_boom_gates(void);
void *temperature_monitor(void *arg);
uint8_t check_fire(uint16_t smoothed_temperatures[SMOOTHED_SAMPLES]);
uint16_t median_temperature(volatile uint16_t temperatures[MEDIAN_SAMPLES]);