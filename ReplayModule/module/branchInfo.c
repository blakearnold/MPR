/*  
 *  branchInfo.c -- recording replaying instructions
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <asm/msr.h>
#include "br_msr.h"
#include "br_record.h"
#include "br_pmcaccess.h"

#define DRIVER_AUTHOR	"Blake Arnold <bablake@gmail.com>"
#define DRIVER_DESC	"Branch Record module for modified kernel"

extern long (*start_rec)(int state, struct recording *rec);
extern long (*stop_rec)(void);
extern long (*rec_owner)(void);


static int __init init_hello(void)
{
		//TODO: check cpuinfo
		printk(KERN_INFO "Loading Branch recording module\n");
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
		//TODO: stop performance counter
		printk(KERN_INFO "Unloading branch recording module\n");
		start_rec = NULL;
		stop_rec = NULL;
		rec_owner = NULL;
}



module_init(init_hello);
module_exit(cleanup_hello);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);	/* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

MODULE_SUPPORTED_DEVICE("testdevice");
