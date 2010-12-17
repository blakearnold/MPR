#include <stdio.h>
#include <stdlib.h>
#include "br_msr.h"
#include "perfTest.h"


int probeCPUID(){
	unsigned a, b, c, d, v, reg;
        asm("cpuid" : "=a" (a), "=b" (b) , "=c" (c), "=d" (d): "0" (0xa) );
	v = a & 0xff;
        printf("Version Identifier: %u \n", v);


printf("-----------------\n");	
	switch (v){
		case 3: 

printf("-------v3----------\n");	
			



		
		case 2:
printf("------v2-----------\n");	
		printf("number of fixed counters: %u\n", d & 0xf);
		printf("bit width of fixed counters: %u\n", d >>5 & 0xff);
		
		case 1:
printf("--------v1---------\n");	
		printf("number of counters: %u\n", a>>8 & 0xff);
		printf("bit width of counter: %u\n", a>>16 & 0xff);
		break;	
		default: printf("unknown version");
	}	
        asm("cpuid" : "=a" (a), "=b" (b) , "=c" (c), "=d" (d): "0" (0x1) );

		printf("type: %x\n", a>>12 & 0xc);
		printf("family: %x\n", (a >>8 & 0xf) + (a>>20 & 0xff));
		printf("model: %x\n", (a >>4 & 0xf) + ((a>>16 & 0xf)<<4));
		if((c>>15 & 0x1) == 0x1){
			printf("perf capabilities enabled\n");
			reg = IA32_PERF_CAPABILITIES;
			asm("rdmsr" : "=a" (a), "=d" (d) : "c" (reg));
		if((a>>12 & 0x1) == 0x1)
			printf("freeze while smm enambled");
		}	

	return 0;

}

int main(int argc, char **argv){
	probeCPUID();
	return 0;

}

