#include <linux/errno.h>
#include <linux/module.h>
//Create mock syscalls to use module


long (*start_rec)(void) = NULL;
EXPORT_SYMBOL(start_rec);

asmlinkage long sys_start_rec(void){
    printk("start recording\n");
    return start_rec ? start_rec() : -ENOSYS;
}

long (*stop_rec)(void) = NULL;
EXPORT_SYMBOL(stop_rec);

asmlinkage long sys_stop_rec(void){
    printk("stopping recording\n");
    return stop_rec ? stop_rec() : -ENOSYS;
}

long (*rec_owner)(void) = NULL;
EXPORT_SYMBOL(rec_owner);

asmlinkage long sys_rec_owner(void){
    printk("becocming owner\n");
    return rec_owner ? rec_owner() : -ENOSYS;
}

