#include <stdio.h>
#include <stdlib.h>
#include <linux/record.h>
#include "recordEvent.h"
#include <errno.h>

void replayFunction(void);
int main(int argc, char **argv){
struct recording *rec, *next;
rec = malloc(sizeof(struct recording));
rec->next = NULL;
stop_rec();
printf("Starting to record\n");
//first record
		if(start_rec(rec) < 0){
			perror("start_Rec failed\n");
			//stop_rec();
			exit(1);
		}
replayFunction();
		if(stop_rec() < 0){
			perror("stop_rec failed\n");
		}
printf("Starting to replay\n");
//now replay
		if(start_rep(rec) < 0){
			perror("start_Rep failed\n");
			//stop_rec();
			exit(1);
		}
replayFunction();
		if(stop_rep() < 0){
			perror("stop_rep failed\n");
		}

next = rec;
while((next=next->next)){
	printf("Branch num: %llu, thread id: %u", next->branchNum, next->threadId);
}
return 0;
}



void replayFunction(){
int i;
for(i=0; i<10; i++){
	rec_owner();
	printf("loop: %i\n", i);
	}
printf("what do i do here?\n");



}

void replayed(void){

	printf("hi!");
}
