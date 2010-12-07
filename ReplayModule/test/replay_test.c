#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "recordEvent.h"

pthread_spinlock_t lock;
int x = 0;
void *secondThread( void *ptr){

	char *name = (char *) ptr;
	int i;
	for(i = 0; i < 10000; i++){
		rec_owner();		
		//usleep(i*16);
		x++;
		/*
		pthread_spin_lock(&lock);
		printf("%s : %d\n",name, i);
		pthread_spin_unlock(&lock);
		*/
		pthread_yield();
	}

}

void *thirdThread( void *ptr){

	char *name = (char *) ptr;
	int i;
	for(i = 0; i < 10000; i++){
		
		//usleep(i*16);
		int i = x;
		pthread_yield();
		x = i--;
		/*
		pthread_spin_lock(&lock);
		printf("%s : %d\n",name, i);
		pthread_spin_unlock(&lock);
		*/
	}

}

int main(){
	pthread_t thread1, thread2, thread3;
	if(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE)){
		printf("Could not initialize lock");
		exit(1);
	}
	char *name1 = "t1";
	char *name2 = "t2";
	char *name3 = "t3";
	pthread_create( &thread1,  NULL, secondThread, (void *)name1);
	pthread_create( &thread2, NULL, secondThread, (void *)name2);

	pthread_create( &thread3, NULL, thirdThread, (void *)name3);

	pthread_join( thread1, NULL);
	printf("Thread 1 returned: %d\n", x);
	pthread_join(thread2, NULL);
	printf("Thread 2 returned\n");
	pthread_join(thread3, NULL);
	printf("Thread 3 returned: %d\n",x );

exit(0);


}
