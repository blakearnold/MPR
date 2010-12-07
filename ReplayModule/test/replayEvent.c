#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int main(int argc, char* argv[]){
	pid_t pid = fork();
	switch(pid){
	case -1: printf("An error occured, could not fork\n");
		exit(1);
		break;
	case 0: //child
		//Start replay
		if(execv(*++argv,argc > 2 ? ++ argv : NULL)< 0)
			printf("An error occured in exec\n");
		break;
	default://parent
		waitpid( pid, NULL, 0);
		//output replay report
		printf("waited in child\n");
		
	}




}



