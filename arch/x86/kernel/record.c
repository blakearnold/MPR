#include <linux/errno.h>
#include <linux/module.h>
#include <linux/record.h>
//Create mock syscalls to use module


long (*start_rec)(int state, struct recording *rec) = NULL;
EXPORT_SYMBOL(start_rec);

asmlinkage long sys_start_rec(int state, struct recording *rec){
    return start_rec ? start_rec(state, rec) : -ENOSYS;
}

long (*stop_rec)(void) = NULL;
EXPORT_SYMBOL(stop_rec);

asmlinkage long sys_stop_rec(void){
    return stop_rec ? stop_rec() : -ENOSYS;
}

long (*rec_owner)(void) = NULL;
EXPORT_SYMBOL(rec_owner);

asmlinkage long sys_rec_owner(void){
    return rec_owner ? rec_owner() : -ENOSYS;
}

