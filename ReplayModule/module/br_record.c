#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/module.h>
#include <asm/msr.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/record.h>
#include <linux/sched.h> // task_struct
#include "br_msr.h"
#include "br_record.h"
#include "br_pmcaccess.h"

int cur_state = -1;
struct br_info recordingList;
long start_record(int state, struct recording *rec){
		//TODO: per thread
		if(cur_state != -1)
				return -1; //TODO: usefull errors
		switch(state){
			case 0: //state 0 is recording
				INIT_LIST_HEAD(&recordingList.list);
				cur_state = 0;
				if(probeCPUID() < 0){
					printk(KERN_INFO "cannot start recording\n");
					return -1; //TODO: make more useful error
				}
				break;
			case 1: //replay
					//copy struct to kernel space
					//init interupt handler
				break;
			default: //user entered out of range state
				return -1;
		}
		printk(KERN_INFO "starting performance counter\n");

		return 0;

}

long stop_record(void){
		struct br_info *tmp;
		struct list_head *pos, *q;

		switch(cur_state){
		case 0:
			printk(KERN_INFO "stopping recording\n");

			stopCounter(1);
			stopCounter(2);
			//TODO: move data to user space and return it
			list_for_each_safe(pos, q, &recordingList.list){
					tmp= list_entry(pos, struct br_info, list);
					list_del(pos);
					kfree(tmp);
			}
		case 1:
			printk(KERN_INFO "stopping replay\n");
			break;
		default: 
			printk(KERN_INFO "nothing to be stopped\n");
		}
		cur_state = -1;
		return 0;
}
long record_owner(void){
		struct br_info *tmp;
		unsigned reg;
		unsigned long long br, br_missed;
		switch(cur_state){
		case 0: //recording
				//printk(KERN_INFO "owner: inside the module\n");
				if(!(tmp = kmalloc(sizeof(struct br_info),GFP_ATOMIC)))
						return -1; //TODO: make a useful error

				reg = IA32_PMC1; 
				rdmsrl(reg, br);
				reg = IA32_PMC2; 
				rdmsrl(reg, br_missed);
				printk(KERN_INFO "branch address: %llu, missed %llu\n", br, br_missed);
				//record branch, instruction and tid of each thread
				tmp->rec.branchNum = br;
				tmp->rec.threadId = current->pid;

				list_add_tail(&tmp->list, &recordingList.list);
				break;
		case 1: //replaying, do nothing? clear list node?
				break;
		default:
			break;
		}
		return 0;

}
