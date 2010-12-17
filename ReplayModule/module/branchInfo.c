/*  
 *  hello-1.c - The simplest kernel module.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>
#include <linux/list.h>
#include <asm/msr.h>
#include "br_msr.h"

#define DRIVER_AUTHOR	"Blake Arnold <bablake@gmail.com>"
#define DRIVER_DESC	"A Sample driver"

#define MISC_PMC_ENABLED_P(x) ((x) & 1 << 7)
extern long (*start_rec)(void);
long start_record(void);
int probeCPUID(void);

extern long (*stop_rec)(void);
long stop_record(void);

extern long (*rec_owner)(void);
long record_owner(void);

struct recording{
		struct list_head list;
		pid_t threadId;
		int instructionOffset;
		int branchNum;
};


struct recording recordingList;

static int __init init_hello(void)
{
		printk(KERN_INFO "Hello world 1.\n");
		/* 
		 * A non 0 return means init_module failed; module can't be loaded. 
		 */
		start_rec = &start_record;
		stop_rec = &stop_record;
		rec_owner = &record_owner;
		return 0;
}

static void __exit cleanup_hello(void)
{
		printk(KERN_INFO "Goodbye world 1.\n");
}

long start_record(void){
		int low, high;
		printk(KERN_INFO "starting inside the module\n");
		if(probeCPUID() < 0){
			printk(KERN_INFO "cannot start recording");
			return -1; //TODO: make more useful error
		}
		INIT_LIST_HEAD(&recordingList.list);
		//setup data structure
		return 0;

}

long stop_record(void){
		unsigned a, d, reg;
		struct recording *tmp;
		struct list_head *pos, *q;


		printk(KERN_INFO "stopping inside the module");

		reg = IA32_PERFEVTSEL0;
		asm("rdmsr" 
						: "=a" (a), "=d" (d) 
						: "c" (reg));
		if((a>>22 & 0x1) == 0x1){
				//counter is already enabled
				//turn off for now TODO: go to next counter
								a = a& ( ~(0x1<<22));
								asm("wrmsr"
								: 
								: "a" (a));
								
				printk(KERN_INFO "stopped tracking");
		}
		list_for_each_safe(pos, q, &recordingList.list){
			tmp= list_entry(pos, struct recording, list);
			list_del(pos);
			kfree(tmp);
	 	}

		//return data structure
		return 0;

}
long record_owner(void){
		struct recording *tmp;
		unsigned reg, a, d;
		//printk(KERN_INFO "owner: inside the module\n");
		if(!(tmp = kmalloc(sizeof(struct recording),GFP_ATOMIC)))
				return -1; //TODO: make a useful error

		reg = IA32_PMC0; 
		asm("rdmsr" 
						: "=a" (a), "=d" (d) 
						: "c" (reg));
		printk(KERN_INFO "branch address: %u_%u", d, a);
		//record branch, instruction and tid of each thread
		list_add_tail(&tmp->list, &recordingList.list);
		return 0;

}


module_init(init_hello);
module_exit(cleanup_hello);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);	/* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

MODULE_SUPPORTED_DEVICE("testdevice");

unsigned preparePERFEVTSEL(unsigned event, unsigned mask){

		int _INV = 0x0;	//invert counter mask
		int _EN = 0x1;	//enable
		int _ANY = 0x1; //tracks threads accross processors
		int _INT = 0x0; //interupt, turn on for replaying
		int _PC = 0x0;	//pin control
		int _E = 0x0;	//edge
		int _OS = 0x0;	//OS detection, TODO: do we want system calls tracked?
		int _USR = 0x1;	//track user level code

		unsigned prepared = 0x0;

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

int probeCPUID(){
		unsigned a, b, c, d, v, reg, freeze, counters, width;
		
		unsigned br_inst;
		unsigned br_umask;
		unsigned event;

		asm("cpuid" : "=a" (a), "=b" (b) , "=c" (c), "=d" (d): "0" (0xa) );
		v = a & 0xff;
		printk(KERN_INFO "Version Identifier: %u \n", v);
		

		if(v>0){
			counters = a>>8 & 0xff;  //vol 3. 30-3
			printk(KERN_INFO "Number of counters: %u", counters);
			width = a>>16 & 0xff;

		}else{
			return -1; //no version
		}
		if((b>>5&0x1) == 0x1){ //Vol 3. 30-13
			printk(KERN_INFO "branch instruction retired performance counter not available");
			return -1;
		}
		switch (v){
			case 3: 
			case 2:
			case 1:
			break;	
			default: printk(KERN_INFO "unknown version %u", v);
		}	


		asm("cpuid" 
						: "=a" (a), "=b" (b) , "=c" (c), "=d" (d)
						: "0" (0x1) );

		printk(KERN_INFO "type: %x\n", a>>12 & 0xc); //cpuinfo manual
		printk(KERN_INFO "family: %x\n", (a >>8 & 0xf) + (a>>20 & 0xff));
		printk(KERN_INFO "model: %x\n", (a >>4 & 0xf) + ((a>>16 & 0xf)<<4));
		if((c>>15 & 0x1) == 0x1){
				printk(KERN_INFO "perf capabilities enabled\n");
				reg = IA32_PERF_CAPABILITIES;
				asm("rdmsr" 
								: "=a" (a), "=d" (d) 
								: "c" (reg));
				freeze = (a>>12 & 0x1); //vol. 3 30-81
				if(freeze == 0x1)
						printk(KERN_INFO "freeze while smm enambled");


				reg = IA32_PERFEVTSEL0;
				br_inst = 0xc4;
				br_umask = 0x00;
			
				asm("rdmsr" 
								: "=a" (a), "=d" (d) 
								: "c" (reg));
				if((a>>22 & 0x1) == 0x1){ //vol. 3 30-4,30-10
						printk(KERN_INFO "restarting perf counter\n");
				
					//counter is already enabled
					//turn off for now TODO: go to next counter
					a &=~(0x1<<22);
					asm("wrmsr"
					: 
					: "a" (a));
					printk(KERN_INFO "stoped tracking\n");
				}

				//zero out counter
				d = 0x0;
				a = 0x0;
				reg = IA32_PMC0; 
				asm("wrmsr"
				: 
				: "a" (a), "d" (d), "c" (reg));

				printk(KERN_INFO "starting branch tracking!");
				event = preparePERFEVTSEL(br_inst, br_umask);
				reg = IA32_PERFEVTSEL0;
				asm("wrmsr"
				: 
				: "a" (event), "c" (reg));
				printk(KERN_INFO "starting branch tracking!");


				return 0;

	} else{
		printk(KERN_INFO "No performance capablities are on this core");
		return -1;
}



}


