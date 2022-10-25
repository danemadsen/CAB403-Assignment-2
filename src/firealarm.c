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

#include "firealarm.h"

int main(void)
{
	int shm_fd = shm_open(SHM_NAME, O_RDWR, SHM_MODE);
	// if shm_open fails, exit
	if(shm_fd != -1) {
		Parking = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		assert(Parking != NULL);

		alarm_active = false;

		//initialise semaphores
		sem_init(&alarm_sem, 1, 0);
		

		pthread_t level_threads[LEVELS];
		for (uint8_t i = 0; i < (uint8_t) LEVELS; i++) {
			Parking->levels[i].alarm = 0;
			pthread_create(&level_threads[i], NULL, temperature_monitor, (void *) (uintptr_t) i);
		}

		while(!alarm_active) {
			sem_wait(&alarm_sem);
		}
		assert(alarm_active);
		emergency_mode();
		for (uint8_t i = 0; i < (uint8_t) LEVELS; i++) {
			pthread_join(level_threads[i], NULL);
		}
		sem_destroy(&alarm_sem);
  	}
	else {
		assert(shm_fd != -1);
	}
	return 0;
}

void emergency_mode(void) {
	assert(alarm_active);
	
	// Handle the alarm system and open boom gates
	// Activate alarms on all levels
	for (uint8_t i = 0; i < (uint8_t) LEVELS; i++) {
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
	assert(!alarm_active);
}

void check_alarm(void) {
	assert(alarm_active);
	for (uint8_t i = 0; i < (uint8_t) LEVELS; i++) {
		if (Parking->levels[i].alarm == (uint8_t) 0) {
			alarm_active = false;
			assert(!alarm_active);
		}
	}
	return;
}

void evacuation_message(void) {
	assert(alarm_active);
	const char evacmessage[9] = "EVACUATE";
	for (uint8_t i = 0; i != (uint8_t) 9; i++) {
		for (uint8_t j = 0; j < (uint8_t) ENTRANCES; j++) {
			pthread_mutex_lock(&Parking->entrances[j].information_sign.mlock);
			Parking->entrances[j].information_sign.display = evacmessage[i];
			assert(Parking->entrances[j].information_sign.display == (char) evacmessage[i]);
			pthread_cond_broadcast(&Parking->entrances[j].information_sign.condition);
			pthread_mutex_unlock(&Parking->entrances[j].information_sign.mlock);
		}
		usleep((useconds_t) 20000 * (useconds_t) TIMESCALE);
	}
}

void open_all_boom_gates(void) {
	assert(alarm_active);
	for(uint8_t i = 0; i < (uint8_t) ENTRANCES; i++) {
		pthread_mutex_lock(&Parking->entrances[i].boom_gate.mlock);
		Parking->entrances[i].boom_gate.status = 'O';
		assert(Parking->entrances[i].boom_gate.status == (char) 'O');
		pthread_cond_broadcast(&Parking->entrances[i].boom_gate.condition);
		pthread_mutex_unlock(&Parking->entrances[i].boom_gate.mlock);
	}
	for(uint8_t i = 0; i < (uint8_t) EXITS; i++) {
		pthread_mutex_lock(&Parking->exits[i].boom_gate.mlock);
		Parking->exits[i].boom_gate.status = 'O';
		assert(Parking->exits[i].boom_gate.status == (char) 'O');
		pthread_cond_broadcast(&Parking->exits[i].boom_gate.condition);
		pthread_mutex_unlock(&Parking->exits[i].boom_gate.mlock);
	}
}

void *temperature_monitor(void *arg) {
	uint8_t level = (uint8_t) (uintptr_t) arg;
	assert(level < (uint8_t) LEVELS);
	volatile uint16_t temperatures[MEDIAN_SAMPLES];
	uint16_t smoothed_temperatures[SMOOTHED_SAMPLES];
	uint8_t under_samples = (uint8_t) SMOOTHED_SAMPLES * (uint8_t) MEDIAN_SAMPLES;
	
	while(!alarm_active) {		
		// Add temperature to beginning of temperatures array
		for(uint8_t i = MEDIAN_SAMPLES - 1; i > (uint8_t) 0; i--) {
			temperatures[i] = temperatures[i - (uint8_t) 1];
		}
		temperatures[0] = Parking->levels[level].temperature;
		
		// Add median temp beginning of smoothed_temperatures array
		for(uint8_t i = SMOOTHED_SAMPLES - 1; i > (uint8_t) 0; i--) {
			smoothed_temperatures[i] = smoothed_temperatures[i - (uint8_t) 1];
		}
		smoothed_temperatures[0] = median_temperature(temperatures);

		if(!under_samples) {
			assert(under_samples == (uint8_t) 0);
			assert(!alarm_active);
			alarm_active = check_fire(smoothed_temperatures);
			if(alarm_active) {
				sem_post(&alarm_sem);
			}
		} 
		else {
			assert(under_samples != (uint8_t) 0);
			under_samples--;
		}

		usleep((useconds_t) 2000 * (useconds_t) TIMESCALE);
	}
	return NULL;
}

uint8_t check_fire(uint16_t smoothed_temperatures[SMOOTHED_SAMPLES]) {
	uint8_t hightemps = 0;
	bool return_val = false;
	for(uint8_t i = 0; i < (uint8_t) SMOOTHED_SAMPLES; i++) {
		if (smoothed_temperatures[i] >= (uint16_t) FIRE_THRESHOLD) {
			hightemps++;
		}
		else {
			continue;
		}
	}

	if (hightemps >= (uint8_t) (SMOOTHED_SAMPLES * 0.9) || (smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1]) >= (uint16_t) 8) {
		return_val = true;
	}
	else {
		assert(hightemps < (uint8_t) (SMOOTHED_SAMPLES * 0.9) || (smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1]) < (uint16_t) 8);
	}
	return return_val;
}

uint16_t median_temperature(volatile uint16_t temperatures[MEDIAN_SAMPLES])
{
	uint16_t median = 0;
	for(uint8_t i = 0; i < (uint8_t) MEDIAN_SAMPLES; i++) {
		median += temperatures[i];
	}
	if(median > (uint16_t) 0) {
		assert(median);
		median = median / (uint16_t) MEDIAN_SAMPLES;
	}
	else {
		assert(!median);
	}
	return median;
}