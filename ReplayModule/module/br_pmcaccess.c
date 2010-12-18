
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <asm/msr.h>
#include "br_msr.h"
#include "br_pmcaccess.h"

void stopCounter(int counter){
	unsigned long long high_low;
				unsigned pmc, setpmc;
				switch(counter){
					case 0: pmc = IA32_PMC0;
							setpmc = IA32_PERFEVTSEL0;
							break;

					case 1: pmc = IA32_PMC1;
							setpmc = IA32_PERFEVTSEL1;
							break;
					case 2: pmc = IA32_PMC2;
							setpmc = IA32_PERFEVTSEL2;
							break;
					case 3: pmc = IA32_PMC3;
							setpmc = IA32_PERFEVTSEL3;
							break;
					default: printk(KERN_INFO "Unkown counter #%d", counter);
					return;
						break;
				}
		rdmsrl(setpmc, high_low);
		if((high_low>>22 & 0x1ULL) == 0x1ULL){
				//counter is enabled
				//turn off
				high_low &= ~(0x1ULL<<22);
				wrmsrl(setpmc, high_low);	
				printk(KERN_INFO "stopped tracking\n");
		} else {
				printk(KERN_INFO "tracking alread stopped?!\n");
		}


}
unsigned long long preparePERFEVTSEL(unsigned event, unsigned mask){

		int _INV = 0x0;	//invert counter mask
		int _EN = 0x1;	//enable
		int _ANY = 0x1; //tracks threads accross processors
		int _INT = 0x0; //interupt, turn on for replaying
		int _PC = 0x0;	//pin control
		int _E = 0x0;	//edge
		int _OS = 0x0;	//OS detection, TODO: do we want system calls tracked?
		int _USR = 0x1;	//track user level code

		unsigned long long prepared = 0x0ULL;

		prepared |= event & 0xff;
		prepared |= (mask & 0xff)	<<8;
		prepared |= (_USR & 0x1)	<<16 ;
		prepared |= (_OS & 0x1)		<<17 ;
		prepared |= (_E & 0x1)		<<18 ;
		prepared |= (_PC & 0x1)		<<19 ;
		prepared |= (_INT & 0x1)	<<20 ;
		prepared |= (_ANY & 0x1)	<<21 ;
		prepared |= (_EN & 0x1)		<<22 ;
		prepared |= (_INV & 0x1)	<<23 ;

		return prepared;


}
int setupCounter(int counter,unsigned event, unsigned mask){
				unsigned long long high_low, prepared;
				unsigned pmc, setpmc;
				switch(counter){
					case 0: pmc = IA32_PMC0;
							setpmc = IA32_PERFEVTSEL0;
							break;

					case 1: pmc = IA32_PMC1;
							setpmc = IA32_PERFEVTSEL1;
							break;
					case 2: pmc = IA32_PMC2;
							setpmc = IA32_PERFEVTSEL2;
							break;
					case 3: pmc = IA32_PMC3;
							setpmc = IA32_PERFEVTSEL3;
							break;
					default: printk(KERN_INFO "Unkown counter #%d", counter);
						return -1;
						break;
				}
				rdmsrl(setpmc, high_low);
				if((high_low>>22 & 0x1ULL) == 0x1ULL){ //vol. 3 30-4,30-10
						printk(KERN_INFO "restarting perf counter\n");
				
					//counter is already enabled
					//turn off for now TODO: go to next counter
					high_low &=~(0x1ULL<<22);
					wrmsrl(setpmc, high_low);
					printk(KERN_INFO "stoped tracking\n");
				}

				//zero out counter
				high_low = 0x0ULL;
				wrmsrl(pmc, high_low);
				
				printk(KERN_INFO "starting branch tracking!\n");
				prepared = preparePERFEVTSEL(event, mask);
				rdmsrl(setpmc, high_low);
				high_low &= 0xffffffff00000000ULL;
				prepared |= high_low;
				printk("about to wrmsrl %llx\n", prepared);
				wrmsrl(setpmc, prepared);
				return 0;
}


int probeCPUID(){
		unsigned a, b, c, d, v, reg, freeze, counters, width;
		unsigned long long high_low;	
		unsigned br_inst, br_miss, br_miss_umask, br_umask;

		asm("cpuid" : "=a" (a), "=b" (b) , "=c" (c), "=d" (d): "0" (0xa) );
		v = a & 0xff;
		printk(KERN_INFO "Version Identifier: %u \n", v);
		

		if(v>0){
			counters = a>>8 & 0xff;  //vol 3. 30-3
			printk(KERN_INFO "Number of counters: %u\n", counters);
			width = a>>16 & 0xff;

		} else {
			return -1; //no version
		}
		if((b>>5&0x1) == 0x1){ //Vol 3. 30-13
			printk(KERN_INFO "branch instruction retired performance counter not available\n");
			return -1;
		}
		switch (v){
			case 3: 
			case 2:
			case 1:
			break;	
			default: printk(KERN_INFO "unknown version %u\n", v);
		}	


		asm("cpuid" 
						: "=a" (a), "=b" (b) , "=c" (c), "=d" (d)
						: "0" (0x1) );

		printk(KERN_INFO "type: %x\n", a>>12 & 0xc); //cpuinfo manual
		printk(KERN_INFO "family: %x\n", (a >>8 & 0xf) + (a>>20 & 0xff));
		printk(KERN_INFO "model: %x\n", (a >>4 & 0xf) + ((a>>16 & 0xf)<<4));
		if((c>>15 & 0x1) == 0x1){ //vol 3. 30-81
				printk(KERN_INFO "perf capabilities enabled\n");
				reg = IA32_PERF_CAPABILITIES;
				rdmsrl(reg, high_low);
				freeze = (high_low>>12 & 0x1ULL); //vol. 3 30-81
				if(freeze == 0x1){
						reg = IA32_DEBUGCTL;
						rdmsrl(reg, high_low);
						if((high_low>>14 & 0x1ULL) == 0x0ULL){ //vol. 3 30-81
							high_low &= ~(0x1ULL <<14);
							printk(KERN_INFO "freeze while smm enabled\n");

						} else {
						printk(KERN_INFO "freeze while smm already enabled\n");
						}
				
				}

				br_inst = 0xc4;
				br_umask = 0x00;
				br_miss = 0xc5;
				br_miss_umask = 0x0;
				printk(KERN_INFO "starting branch tracking!\n");
				setupCounter(1, br_inst, br_umask);
				setupCounter(2, br_miss, br_miss_umask);

				return 0;

	} else{
		printk(KERN_INFO "No performance capablities are on this core\n");
		return -1;
	}
}


