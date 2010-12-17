#include <stdio.h>
#include <stdlib.h>
#include "crew_test.h"

int main(int argc, char **argv){
int i;
		if(start_rec() < 0){
			printf("start_Rec failed\n");
			//stop_rec();
			exit(1);
		}
for(i=0; i<10; i++)
	rec_owner();
		if(stop_rec() < 0){
			printf("stop_rec failed\n");
		}
printf("what do i do here?\n");

unsigned long long high, low;
high= 0xffffffffffffffffULL;
low = 0x0;
high &= ~(0x1ULL<<22);

printf("wahoo %llx, %llx\n", high, low);

return 0;












}
