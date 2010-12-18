#include <linux/unistd.h>
#include <sys/syscall.h>
#include <linux/record.h>

long start_rec(void){
	return syscall(__NR_start_rec, 0, NULL);
}

long start_replay(struct recording *rec){
	return syscall(__NR_start_rec, 1, rec);
}

long stop_rec(void){
	return syscall(__NR_stop_rec);
}

long rec_owner(void){
	return syscall(__NR_rec_owner);
}
