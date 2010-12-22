#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/module.h>
#include <asm/msr.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/record.h>
#include <linux/sched.h> // task_struct
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <asm/apicdef.h>
#include "op_apic.h"
#include "br_msr.h"
#include "br_record.h"
#include "br_pmcaccess.h"

#define CCCR_CLEAR_OVF(cccr) ((cccr) &= (~(1ULL << 31)))
static int cur_state = -1;
static struct br_info recordingList;
//static struct recording *userRecList;
/**
 * Deletes all entries from record list.
 */
void deleteList(void){
		struct br_info *tmp;
		struct list_head *pos, *q;
			list_for_each_safe(pos, q, &recordingList.list){
					tmp = list_entry(pos, struct br_info, list);
					list_del(pos);
					kfree(tmp);
					
			}
}
/**
 * Worker function for start_rec system call. Setup performance counters and * starts counting branches. If in replay mode it will also setup Interupt   * code.
 * @param state state of the replay, 0 for record, 1 for replay
 * @param rec recording data structure to be used for replay
 * @return 0 on success and negative value on failure
 */
long start_record(int state, struct recording *rec){
		//TODO: per thread
		struct br_info *tmp;
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
				cur_state = 0;
				break;
			case 1: //replay
					//copy struct to kernel space
					//init interupt handler
					//TODO read from struct
					install_nmi();
					tmp = list_first_entry(&recordingList.list, struct br_info, list);
					list_del(&recordingList.list);
					
					setupCounter(1, BR_INST, BR_UMASK, tmp->rec.branchNum);
		//			if(!access_ok( VERIFY_READ, rec, sizeof(struct recording)))
		//				return -EINVAL;
					kfree(tmp);
				cur_state = 1;
				break;
			default: //user entered out of range state
				return -EINVAL;
		}
		printk(KERN_INFO "starting performance counter\n");
		
		//userRecList = rec;
		return 0;

}

/**
 * Stops recording or replay
 */
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

/**
 * Records owner in linked list, record PC and branch
 */
long record_owner(void){
		struct br_info *tmp;
		unsigned reg;
		unsigned long long br, br_missed;
		unsigned long long eip;
		switch(cur_state){
		case 0: //recording
				//printk(KERN_INFO "owner: inside the module\n");
				if(!(tmp = kmalloc(sizeof(struct br_info),GFP_KERNEL)))
						return -ENOMEM;

				reg = IA32_PMC1;
				rdmsrl(reg, br);
				reg = IA32_PMC2;
				rdmsrl(reg, br_missed);
				//This doesnt work TODO get PC
				asm __volatile__("movl $., %0" : "=r"(eip));
				printk(KERN_INFO "branch address: %llu, missed %llu, eip %llu\n", br, br_missed, eip);
				//record branch, instruction and tid of each thread
				tmp->rec.branchNum = br;
				tmp->rec.threadId = current->pid;
				tmp->rec.instructionOffset = eip;
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
/**
 * NMI handler. Resets counter to be triggered by next branch in list
 */
asmlinkage void br_do_nmi(struct pt_regs * regs)
{
//uint const cpu = get_cpu();
//struct op_msrs const * const msrs = &cpu_msrs[cpu];
//	we can do printk's here!!!
/*
		struct br_info *tmp;
		unsigned long pmcReg, setpmcReg;
		unsigned long long pmc, setpmc;
					tmp = list_first_entry(&recordingList.list, struct br_info, list);
					list_del(&recordingList.list);
					pmcReg = IA32_PMC1;
					setpmcReg = IA32_PERFEVTSEL1;
					rdmsrl(pmcReg, pmc);
					CCCR_CLEAR_OVF(pmc);
					
					rdmsrl(setpmcReg, setpmc);
					//stop counter
					if((setpmc>>22 & 0x1ULL) == 0x1ULL){ //vol. 3 30-4,30-10
						setpmc &=~(0x1ULL<<22);
						wrmsrl(setpmcReg, setpmc);
					}

					pmc = -1*(tmp->rec.branchNum)+1;
					wrmsrl(pmcReg, pmc);
					//reset it
					setpmc |=0x1ULL<<22;
					wrmsrl(setpmcReg, setpmc);
					
					kfree(tmp);
apic_write(APIC_LVTPC, apic_read(APIC_LVTPC) & ~APIC_LVT_MASKED);
*/
}
/**
 *links to sys exit call. used by br_op_nmi.S
 */
asmlinkage long my_sys_exit(int error_code)
{
	do_exit((error_code & 0xff)<<8);
}
