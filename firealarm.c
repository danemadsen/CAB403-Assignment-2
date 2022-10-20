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
                            CAB403 Assignment 2 - Fire Alarm
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
#include <math.h>
#include "firealarm.h"
#include "common.h"

int main()
{
	// wait until a shared memory segment named PARKING is created
	while((shm_fd = shm_open(SHM_NAME, O_RDWR, 0666)) == -1) {
		printf("Waiting for shared memory segment to be created...\n");
		sleep(1);
	}
	Parking = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	for (int i = 0; i < LEVELS; i++) {
		pthread_create(&level_threads[i], NULL, (void *(*)(void *)) temperature_monitor, &Parking->levels[i]);
	}
	
	while(!alarm_active) {
		usleep(1000);
	}
	emergency_mode();
}

void emergency_mode() {
	fprintf(stderr, "*** ALARM ACTIVE ***\n");
	
	// Handle the alarm system and open boom gates
	// Activate alarms on all levels
	for (int i = 0; i < LEVELS; i++) {
		uint8_t *alarm_trigger = &Parking->levels[i].alarm;
		*alarm_trigger = 1;
	}
	
	// Open up all boom gates
	pthread_t boom_gate_threads[ENTRANCES * EXITS];
	for (int i = 0; i < ENTRANCES; i++) {
		BoomGate_t *boomgate = &Parking->entrances[i].boom_gate;
		pthread_create(&boom_gate_threads[i], NULL, open_boom_gate, boomgate);
	}
	for (int i = 0; i < EXITS; i++) {
		BoomGate_t *boomgate = &Parking->exits[i].boom_gate;
		pthread_create(&boom_gate_threads[ENTRANCES - 1 + i], NULL, open_boom_gate, boomgate);
	}
	
	// Show evacuation message on an endless loop
	while(1) {
		char *evacmessage = "EVACUATE ";
		for (char *p = evacmessage; *p != '\0'; p++) {
			for (int i = 0; i < ENTRANCES; i++) {
				pthread_mutex_lock(&Parking->entrances[i].information_sign.mlock);
				Parking->entrances[i].information_sign.display = *p;
				pthread_cond_broadcast(&Parking->entrances[i].information_sign.condition);
				pthread_mutex_unlock(&Parking->entrances[i].information_sign.mlock);
			}
			usleep(20000);
		}
	}
	
	for (int i = 0; i < LEVELS; i++) {
		pthread_join(level_threads[i], NULL);
	}
	
	munmap((void *)Parking, SIZE);
	close(shm_fd);
}

void *temperature_monitor(void *arg) {
	Level_t *level = (Level_t *)arg;
	uint16_t temperatures[MEDIAN_SAMPLES];
	uint16_t smoothed_temperatures[SMOOTHED_SAMPLES];
	uint8_t under_samples = SMOOTHED_SAMPLES, hightemps;
	
	while(1) {
		if(alarm_active) {
			pthread_exit(NULL);
		}
		
		// Add temperature to beginning of temperatures array
		for(int i = MEDIAN_SAMPLES - 1; i > 0; i--) {
			temperatures[i] = temperatures[i - 1];
		}
		temperatures[0] = level->temperature;
		
		// Add median temp beginning of smoothed_temperatures array
		for(int i = SMOOTHED_SAMPLES - 1; i > 0; i--) {
			smoothed_temperatures[i] = smoothed_temperatures[i - 1];
		}
		smoothed_temperatures[0] = median_temperature(temperatures);
		printf("Temp: %d\n", smoothed_temperatures[0]);
		if(!under_samples) {
			hightemps = 0;
			for(int i = 0; i < SMOOTHED_SAMPLES; i++) {
				if (smoothed_temperatures[i] >= 58) {
					hightemps++;
				}
			}

			if (hightemps >= SMOOTHED_SAMPLES* 0.9 || smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1] >= 8) {
				alarm_active = 1;
				printf("High Temps: %d\nSmoothed Temp1: %d\nSmoothed Temp2: %d\n", hightemps, smoothed_temperatures[0], smoothed_temperatures[SMOOTHED_SAMPLES - 1]);
			}
		} 
		else {
			under_samples--;
		}
		
		usleep(2000*TIMESCALE);
	}
}

uint16_t median_temperature(uint16_t temperatures[MEDIAN_SAMPLES])
{
	uint16_t median = 0;
	for(int i = 0; i < MEDIAN_SAMPLES; i++) {
		median += temperatures[i];
	}
	if(median > 0) {
		return floor(median / MEDIAN_SAMPLES);
	}
	else return 0;
}

void *open_boom_gate(void *arg)
{
	struct BoomGate *boomgate = arg;
	pthread_mutex_lock(&boomgate->mlock);
	for (;;) {
		if (boomgate->status == 'C') {
			boomgate->status = 'R';
			pthread_cond_broadcast(&boomgate->condition);
		}
		if (boomgate->status == 'O') {
		}
		pthread_cond_wait(&boomgate->condition, &boomgate->mlock);
	}
	pthread_mutex_unlock(&boomgate->mlock);
}
