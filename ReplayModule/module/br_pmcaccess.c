
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <asm/msr.h>
#include <asm/apicdef.h>
#include "br_msr.h"
#include "br_pmcaccess.h"

#define CTR_OVERFLOW_P(ctr) (!((ctr) & 0x80000000))
#define CCCR_OVF_P(cccr) ((cccr) & (1U << 31))
#define CCCR_CLEAR_OVF(cccr) ((cccr) &= (~(1ULL << 31)))

void resetCounter(int counter){

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
					default: 
					return;
						break;
				}
		rdmsrl(pmc, high_low);
		CCCR_CLEAR_OVF(high_low);
		rdmsrl(setpmc, high_low);
		setupCounter(counter, high_low & 0xff, high_low>>8 & 0xff, 0);


}
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
				if((high_low>>22 & 0x1ULL) == 0x1ULL){ //vol. 3 30-4,30-10
						printk(KERN_INFO "stopping counter\n");
				
					high_low &=~(0x1ULL<<22);
					wrmsrl(setpmc, high_low);
					printk(KERN_INFO "stopped tracking\n");
				} else{
					printk(KERN_INFO "counter alread disabled");
				}


}
unsigned long long preparePERFEVTSEL(struct perfevesel *sel){


		unsigned long long prepared = 0x0ULL;

		prepared |= sel->event & 0xff;
		prepared |= (sel->mask & 0xff)	<<8;
		prepared |= (sel->_USR & 0x1)	<<16 ;
		prepared |= (sel->_OS & 0x1)		<<17 ;
		prepared |= (sel->_E & 0x1)		<<18 ;
		prepared |= (sel->_PC & 0x1)		<<19 ;
		prepared |= (sel->_INT & 0x1)	<<20 ;
		prepared |= (sel->_ANY & 0x1)	<<21 ;
		prepared |= (sel->_EN & 0x1)		<<22 ;
		prepared |= (sel->_INV & 0x1)	<<23 ;

		return prepared;


}
unsigned long long  prepareRecordSel(struct perfevesel *prep){

			
		prep->_INV = 0x0;	//invert counter mask
		prep->_EN = 0x1;	//enable
		prep->_ANY = 0x1; //tracks threads accross processors
		prep->_INT = 0x0; //interupt, turn on for replaying
		prep->_PC = 0x0;	//pin control
		prep->_E = 0x0;	//edge
		prep->_OS = 0x0;	//OS detection, TODO: do we want system calls tracked?
		prep->_USR = 0x1;	//track user level code
		return preparePERFEVTSEL(prep);

}
unsigned long long  prepareReplaySel(struct perfevesel *prep){

			
		prep->_INV = 0x0;	//invert counter mask
		prep->_EN = 0x1;	//enable
		prep->_ANY = 0x1; //tracks threads accross processors
		prep->_INT = 0x1; //interupt, turn on for replaying
		prep->_PC = 0x0;	//pin control
		prep->_E = 0x0;	//edge
		prep->_OS = 0x0;	//OS detection, TODO: do we want system calls tracked?
		prep->_USR = 0x1;	//track user level code
		return preparePERFEVTSEL(prep);


}

/**
 *@param counter counter number
 *@param event event code
 *@param mask event mask code
 *@param record number to set counter to, if 0 the counter is set to record mode
 */
int setupCounter(int counter,unsigned event, unsigned mask, int record){
				unsigned long long high_low, prepared;
				unsigned pmc, setpmc;
				struct perfevesel branchEvent;
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
					printk(KERN_INFO "stopped tracking\n");
				}

				//zero out counter
				
				printk(KERN_INFO "starting branch tracking!\n");
				branchEvent.event = event;
				branchEvent.mask = mask;
				if(!record){
					high_low = 0x0ULL;
					wrmsrl(pmc, high_low);
					prepared = prepareRecordSel(&branchEvent);
				}else{
					
					high_low = -1*record+1;
					wrmsrl(pmc, high_low);
					prepared = prepareReplaySel(&branchEvent);

				}
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

		asm("cpuid" : "=a" (a), "=b" (b) , "=c" (c), "=d" (d): "0" (0xa) );
		v = a & 0xff;
		printk(KERN_INFO "Version Identifier: %u \n", v);
		

		if(v>0){
			counters = a>>8 & 0xff;  //vol 3. 30-3
			width = a>>16 & 0xff;
			printk(KERN_INFO "Number of counters: %u, width: %u\n", counters, width);

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


				return 0;

	} else{
		printk(KERN_INFO "No performance capablities are on this core\n");
		return -1;
	}
}


