#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/module.h>
#include <asm/msr.h>
#include <linux/init.h>
#include <linux/list.h>
#include "br_msr.h"
#include "br_record.h"
#include "br_pmcaccess.h"


struct recording recordingList;
long start_record(void){
		printk(KERN_INFO "starting performance counter\n");
		if(probeCPUID() < 0){
			printk(KERN_INFO "cannot start recording\n");
			return -1; //TODO: make more useful error
		}
		INIT_LIST_HEAD(&recordingList.list);
		return 0;

}

long stop_record(void){
		struct recording *tmp;
		struct list_head *pos, *q;


		printk(KERN_INFO "stopping performance counter\n");

		stopCounter(1);
		stopCounter(2);
		//TODO: move data to user space and return it
		list_for_each_safe(pos, q, &recordingList.list){
			tmp= list_entry(pos, struct recording, list);
			list_del(pos);
			kfree(tmp);
	 	}
		return 0;
}
long record_owner(void){
		struct recording *tmp;
		unsigned reg;
		unsigned long long br, br_missed;
		//printk(KERN_INFO "owner: inside the module\n");
		if(!(tmp = kmalloc(sizeof(struct recording),GFP_ATOMIC)))
				return -1; //TODO: make a useful error

		reg = IA32_PMC1; 
		rdmsrl(reg, br);
		reg = IA32_PMC2; 
		rdmsrl(reg, br_missed);
		printk(KERN_INFO "branch address: %llu, missed %llu\n", br, br_missed);
		//record branch, instruction and tid of each thread
		list_add_tail(&tmp->list, &recordingList.list);
		return 0;

}
