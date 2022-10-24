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
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "firealarm.h"
#include "common.h"

int main()
{
	// wait until a shared memory segment named PARKING is created
	if((shm_fd = shm_open(SHM_NAME, O_RDWR, 0666)) == -1) {
		printf("Shared memory segment doesnt exist\n");
    	return(1);
  	}
	Parking = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	assert(Parking != NULL);

	alarm_active = 0;

	//initialise mutex and condition variables
	pthread_mutex_init(&alarm_lock, NULL);
	pthread_cond_init(&alarm_cond, NULL);
	
	for (int i = 0; i < LEVELS; i++) {
		pthread_create(&level_threads[i], NULL, temperature_monitor, &Parking->levels[i]);
	}
	
	while(!alarm_active) {
		pthread_mutex_lock(&alarm_lock);
		pthread_cond_wait(&alarm_cond, &alarm_lock);
		pthread_mutex_unlock(&alarm_lock);
	}
	assert(alarm_active);
	emergency_mode();
	for (int i = 0; i < LEVELS; i++) {
		pthread_join(level_threads[i], NULL);
	}
	return 0;
}

void emergency_mode() {
	printf("*** ALARM ACTIVE ***\n");
	assert(alarm_active);
	
	// Handle the alarm system and open boom gates
	// Activate alarms on all levels
	for (int i = 0; i < LEVELS; i++) {
		Parking->levels[i].alarm = 1;
	}
	// Open all boom gates
	open_all_boom_gates();
	
	// Show evacuation message while alarm is active
	while(alarm_active) {
		// Display evacuation message on information signs
		evacuation_message();

		// check if the alarm is still active
		check_alarm();
	}
	assert(alarm_active == false);
}

void check_alarm() {
	assert(alarm_active);
	for (int i = 0; i < LEVELS; i++) {
		if (Parking->levels[i].alarm == 0) {
			pthread_mutex_lock(&alarm_lock);
			alarm_active = 0;
			pthread_cond_broadcast(&alarm_cond);
			pthread_mutex_unlock(&alarm_lock);
			assert(alarm_active == 0);
			return;
		}
	}
}

void evacuation_message() {
	assert(alarm_active);
	char *evacmessage = "EVACUATE";
	for (char *p = evacmessage; *p != '\0'; p++) {
		for (int i = 0; i < ENTRANCES; i++) {
			pthread_mutex_lock(&Parking->entrances[i].information_sign.mlock);
			Parking->entrances[i].information_sign.display = *p;
			pthread_cond_broadcast(&Parking->entrances[i].information_sign.condition);
			pthread_mutex_unlock(&Parking->entrances[i].information_sign.mlock);
		}
		usleep(20000*TIMESCALE);
	}
}

void open_all_boom_gates() {
	for(int i = 0; i < ENTRANCES; i++) {
		pthread_mutex_lock(&Parking->entrances[i].boom_gate.mlock);
		Parking->entrances[i].boom_gate.status = 'O';
		pthread_cond_broadcast(&Parking->entrances[i].boom_gate.condition);
		pthread_mutex_unlock(&Parking->entrances[i].boom_gate.mlock);
	}
	for(int i = 0; i < EXITS; i++) {
		pthread_mutex_lock(&Parking->exits[i].boom_gate.mlock);
		Parking->exits[i].boom_gate.status = 'O';
		pthread_cond_broadcast(&Parking->exits[i].boom_gate.condition);
		pthread_mutex_unlock(&Parking->exits[i].boom_gate.mlock);
	}
}

void *temperature_monitor(void *arg) {
	Level_t *level = (Level_t *)arg;
	volatile uint16_t temperatures[MEDIAN_SAMPLES];
	uint16_t smoothed_temperatures[SMOOTHED_SAMPLES];
	uint8_t under_samples = SMOOTHED_SAMPLES*MEDIAN_SAMPLES;
	
	while(!alarm_active) {		
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

		if(!under_samples) {
			assert(under_samples == 0);
			pthread_mutex_lock(&alarm_lock);
			assert(alarm_active == 0);
			alarm_active = check_fire(smoothed_temperatures);
			pthread_cond_broadcast(&alarm_cond);
			pthread_mutex_unlock(&alarm_lock);
		} 
		else {
			assert(under_samples != 0);
			under_samples--;
		}

		print_temperature(smoothed_temperatures[0]);
		usleep(2000*TIMESCALE);
	}
	return NULL;
}

uint8_t check_fire(uint16_t smoothed_temperatures[SMOOTHED_SAMPLES]) {
	uint8_t hightemps = 0;
	for(int i = 0; i < SMOOTHED_SAMPLES; i++) {
		if (smoothed_temperatures[i] >= FIRE_THRESHOLD) {
			hightemps++;
		}
		else continue;
	}

	if (hightemps >= SMOOTHED_SAMPLES * 0.9 || smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1] >= 8) {
		if(hightemps >= SMOOTHED_SAMPLES * 0.9) return 1;
		else return 2;
		return true;
	}
	else {
		assert(hightemps < SMOOTHED_SAMPLES * 0.9 || smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1] < 8);
		return 0;
	}
}

uint16_t median_temperature(volatile uint16_t temperatures[MEDIAN_SAMPLES])
{
	uint16_t median = 0;
	for(int i = 0; i < MEDIAN_SAMPLES; i++) {
		assert(temperatures[i] <= MAX_TEMP);
		median += temperatures[i];
	}
	if(median > 0) {
		return floor(median / MEDIAN_SAMPLES);
	}
	else{
		assert(median == 0);
		return 0;
	}
}

void print_temperature(uint16_t temperature) {
	if(temperature <= BASE_TEMP + MAX_TEMP_CHANGE) {
		printf("\033[42mNORMAL =>\033[0m Temperature: %d\n", temperature);
	}
	else if(temperature >= FIRE_THRESHOLD) {
		printf("\033[41mFIRE   =>\033[0m Temperature: %d\n", temperature);
	}
	else {
		assert(temperature > BASE_TEMP + MAX_TEMP_CHANGE);
		assert(temperature < FIRE_THRESHOLD);
		printf("\033[43mRISING =>\033[0m Temperature: %d\n", temperature);
	}

	if(alarm_active) {
		if(alarm_active == 1){
			printf("\033[45mFIRE DETECTED =>\033[0m Inferno\n");
		} else if(alarm_active == 2){
			printf("\033[45mFIRE DETECTED =>\033[0m Explosion\n");
		} else {
			assert(alarm_active == 1 || alarm_active == 2);
		}
	}
}