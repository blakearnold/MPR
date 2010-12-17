#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "recordEvent.h"

int main(int argc, char* argv[]){
	pid_t pid = fork();
	switch(pid){
	case -1: printf("An error occured, could not fork\n");
		exit(1);
		break;
	case 0: //child
		if(start_rec() < 0){
			printf("start_Rec failed\n");
			exit(1);
		}
		if(execv(*++argv,argc > 2 ? ++ argv : NULL)< 0)
			printf("An error occured in exec\n");
		break;
	default://parent
		waitpid( pid, NULL, 0);
		if(stop_rec() < 0){
			printf("stop_rec failed\n");
		}
		printf("waited in child\n");
		
	}
return 0;



}



