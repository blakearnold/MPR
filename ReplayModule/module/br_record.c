#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/module.h>
#include <asm/msr.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/record.h>
#include <linux/sched.h> // task_struct
#include <linux/errno.h>
#include <asm/uaccess.h>
#include "op_apic.h"
#include "br_msr.h"
#include "br_record.h"
#include "br_pmcaccess.h"

static int cur_state = -1;
static struct br_info recordingList;
//static struct recording *userRecList;
void deleteList(void){

		struct br_info *tmp;
		//int i;
		struct list_head *pos, *q;
			list_for_each_safe(pos, q, &recordingList.list){
					tmp = list_entry(pos, struct br_info, list);
		//			if(!(rec = kmalloc(sizeof(struct recording),GFP_USER)))
		//				printk(KERN_INFO "could not malloc in userspace");
		//			if(rec && (i = copy_to_user(rec, &tmp->rec, sizeof(struct recording))))
		//				printk(KERN_INFO "failed to copy to user space: %d %d\n", i, sizeof(struct recording));
		//			else{
		//				last->next = rec;
		//				last = rec;
		//			}
					list_del(pos);
					kfree(tmp);
					
			}
}
long start_record(int state, struct recording *rec){
		//TODO: per thread
		if(cur_state != -1)
			return -EBUSY;
		switch(state){
			case 0: //state 0 is recording
				INIT_LIST_HEAD(&recordingList.list);
				deleteList();
				cur_state = 0;
				printk(KERN_INFO "starting branch tracking!\n");
				setupCounter(1, BR_INST, BR_UMASK, 0);
				setupCounter(2, BR_MISS, BR_MISS_UMASK, 0);
		//		if(!access_ok( VERIFY_WRITE, rec, sizeof(struct recording) ))
		//			return -EINVAL;
				cur_state = 0;
				break;
			case 1: //replay
					//copy struct to kernel space
					//init interupt handler
					install_nmi();
					setupCounter(1, BR_INST, BR_UMASK, 16);
					setupCounter(2, BR_MISS, BR_MISS_UMASK, 0);
		//			if(!access_ok( VERIFY_READ, rec, sizeof(struct recording)))
		//				return -EINVAL;
				cur_state = 1;
				break;
			default: //user entered out of range state
				return -EINVAL;
		}
		printk(KERN_INFO "starting performance counter\n");
		
		//userRecList = rec;
		return 0;

}

long stop_record(void){
		//struct recording *rec;
		//struct recording *last;
		//last = userRecList;
		//if(!access_ok(VERIFY_WRITE, userRecList, sizeof(struct recording)))
		//	return -EINVAL;
		switch(cur_state){
		case 0:
			printk(KERN_INFO "stopping recording\n");

			stopCounter(1);
			stopCounter(2);
			//TODO: move data to user space and return it
		//	last->next=NULL;
			break;
		case 1:
			printk(KERN_INFO "stopping replay\n");
			restore_nmi();
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
				if(!(tmp = kmalloc(sizeof(struct br_info),GFP_KERNEL)))
						return -ENOMEM;

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
				reg = IA32_PMC1;
				rdmsrl(reg, br);
				reg = IA32_PMC2;
				rdmsrl(reg, br_missed);
				printk(KERN_INFO "branch address: %llu, missed %llu\n", br, br_missed);
				break;
		default:
			break;
		}
		return 0;

}
