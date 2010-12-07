/*  
 *  hello-1.c - The simplest kernel module.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>
#include <linux/list.h>
#include <asm/msr.h>
#define DRIVER_AUTHOR	"Blake Arnold <bablake@gmail.com>"
#define DRIVER_DESC	"A Sample driver"

#define MISC_PMC_ENABLED_P(x) ((x) & 1 << 7)
extern long (*start_rec)(void);
long start_record(void);

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
	printk(KERN_INFO "starting inside the module");
	rdmsr(MSR_IA32_MISC_ENABLE, low, high);
	if (! MISC_PMC_ENABLED_P(low)) {
		printk(KERN_ERR "oprofile: P4 PMC not available\n");
		return -1;
	}
    	INIT_LIST_HEAD(&recordingList.list);
	//setup data structure
	return 0;

}

long stop_record(void){
	printk(KERN_INFO "stopping inside the module");
	//return data structure
	return 0;

}
long record_owner(void){
	struct recording *tmp;
	printk(KERN_INFO "owner: inside the module");
	if(!(tmp = kmalloc(sizeof(struct recording),GFP_ATOMIC)))
		return -1; //TODO: make a useful error
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
