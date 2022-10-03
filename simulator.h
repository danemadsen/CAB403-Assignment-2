#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

int shm_fd;
struct CarPark* Parking;
struct Car entrance_queue[ENTRANCES][LEVEL_CAPACITY];
struct Car parked_cars[LEVELS*LEVEL_CAPACITY];

pthread_mutex_t parked_cars_mlock;
pthread_cond_t parked_cars_condition;

void new_car();
void get_random_plate(char* plate);
void send_to_random_entrance(struct Car Auto);
