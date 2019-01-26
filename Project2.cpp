#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <sstream>
#include <string>
#include <queue>
#include <pthread.h>
#include <semaphore.h> 
#define NUM_GUESTS 10
#define NUM_ROOMS 5
#define SHARED 1

using namespace std;
/*
 To run:
	g++ -pthread Project2.cpp
*/

/* THREADS */
void *Guest(void *);
void *FrontDesk(void *);

/* SEMAPHORES */
sem_t capacity;     // semaphore for # of rooms
sem_t frontDesk;    // semaphore for guest to wait for front desk
sem_t waitingGuest; // semaphore for front desk to wait for a guest
sem_t event1;       // guest goes to front desk/is greeted by desk
sem_t event2;       // guest gets a room key/front desk assigns guest a room
sem_t event3;       // guest returns key/front desk gets key
sem_t event4;       // guest gets balance/front desk gives guest their balance
sem_t event5;       // guest pays/front desk completes checkout

/* SHARED RESOURCES */
int totalServed      = 0;
int guestToServe     = -1;
int currentRoom      = -1;
int rooms [5]        = { -1, -1, -1, -1, -1 }; // if value of roomNumber is -1 then that means that ther is no guest in room
bool checkin         = true;
int balance          = 0;

// -------- MAIN --------------
int main(int argc, char*argv[]) {
	printf("Start main.\n");

	/* INITIALIZE SEMAPHORES */
	sem_init(&capacity, SHARED, 5);
	sem_init(&frontDesk, SHARED, 1);
	sem_init(&waitingGuest, SHARED, 0);
	sem_init(&event1, SHARED, 0);
	sem_init(&event2, SHARED, 0);
	sem_init(&event3, SHARED, 0);
	sem_init(&event4, SHARED, 0);
	sem_init(&event5, SHARED, 0);

	/* CREATE 10 GUEST THREADS */
	pthread_t guestThreads[NUM_GUESTS];
	int index[NUM_GUESTS];
	for (int i = 1; i < NUM_GUESTS + 1; i++) {
		index[i] = i;
		pthread_create(&guestThreads[i], NULL, Guest, (void *)&index[i]);
	}

	/* CREATE FRONT DESK THREAD */
	pthread_t fid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_create(&fid, NULL, FrontDesk, NULL);

	for (int j = 0; j < NUM_GUESTS; j++) {
		pthread_join(guestThreads[j], NULL);
	}

	pthread_join(fid, NULL);

}

//-------- FRONT DESK -----------
void *FrontDesk(void *threadarg) {
	printf("\nStart front desk thread.\n");

	while (totalServed < 20) { 

		sem_wait(&waitingGuest);

		/* CHECK IN */
		if (checkin == true) {

			/* Event 1 */
			sem_wait(&event1);
			printf("\nFront Desk greets guest %d for check in.\n", guestToServe);

			/* Event 2 */
			for (int i = 1; i < (sizeof(rooms) / sizeof(*rooms)) + 1; i++) {
				if (rooms[i] == -1) { // room is vacant
					rooms[i] = guestToServe;
					currentRoom = i;
					printf("\nFront Desk gives guest %d room #%d.\n", guestToServe, i);
					break;
				}
			}
			sem_post(&event2);
		}

		/* CHECK OUT */
		else {

			/* Event 3 */
			sem_wait(&event3);
			for (int i = 1; i < (sizeof(rooms) / sizeof(*rooms)) + 1; i++) {
				if (rooms[i] == guestToServe) { // find the room that the guest was staying in
					printf("\nFront Desk greets guest %d for check out and gets the key for room #%d.\n", guestToServe, i);
					rooms[i] = -1; // reset the value of the room to -1 because it's now vacant
					break;
				}
			}

			/* Event 4 */
			int rand();
			balance = (rand() % 500) + 50;
			printf("\nFront Desk calculates guest %d's balance of $%d.00 and gives guest a receipt.\n", guestToServe, balance);
			sem_post(&event4);

			/* Event 5 */
			sem_wait(&event5);
			printf("\nFront Desk gets guest %d's payment and completes the transaction.\n", guestToServe);
			sem_post(&capacity);
			sem_post(&frontDesk);
		}

		totalServed++;
	}
	
	printf("\nEnd front desk thread.\n");
}

//------------ GUEST ----------------------
void *Guest(void *index) {
	
	int guestIndex = *((int *)index);
	int guestRoom = -1;

	/* CHECK IN */
	sem_wait(&capacity); // wait for an available room
	printf("\nGuest %d enters hotel.\n", guestIndex);

	sem_wait(&frontDesk); // wait for the front desk to be available
	checkin = true;
	sem_post(&waitingGuest);

	/* Event 1 */
	printf("\nGuest %d goes to front desk for check in.\n", guestIndex);
	guestToServe = guestIndex;
	sem_post(&event1);
		
	/* Event 2 */
	sem_wait(&event2);
	guestRoom = currentRoom;
	printf("\nGuest %d gets room #%d from front desk.\n", guestIndex, guestRoom);

	sem_post(&frontDesk);

	/* HOTEL ACTIVITIES */
	int rand();
	int activity = (rand() % 4) + 1; // get a random number between 1 and 4
	int sleepTime = (rand() % 3) + 1; // get a random number between 1 and 3
	if (activity == 1) {
		printf("\nGuest %d goes to hotel swimming pool.\n", guestIndex);
		sleep(sleepTime);
	}
	else if (activity == 2) {
		printf("\nGuest %d goes to hotel restaurant.\n", guestIndex);
		sleep(sleepTime);
	}
	else if (activity == 3) {
		printf("\nGuest %d goes to hotel fitness center.\n", guestIndex);
		sleep(sleepTime);
	}
	else {
		printf("\nGuest %d goes to hotel business center.\n", guestIndex);
		sleep(sleepTime);
	}
	
	/* CHECK OUT */
	sem_wait(&frontDesk);
	checkin = false;
	guestToServe = guestIndex;
	sem_post(&waitingGuest);
	
	/* Event 3 */
	printf("\nGuest %d goes to front desk for checkout.\n", guestIndex);
	printf("\nGuest %d returns key for room #%d.\n", guestIndex, guestRoom);
	sem_post(&event3);

	/* Event 4 */
	sem_wait(&event4);
	printf("\nGuest %d gets the balance of $%d.00.\n", guestIndex, balance);

	/* Event 5 */
	printf("\nGuest %d makes payment.\n", guestIndex);
	sem_post(&event5);

}
	
