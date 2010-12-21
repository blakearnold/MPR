#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/record.h>

long start_rec(struct recording *rec){
	return syscall(__NR_start_rec, 0, rec);
}

long start_rep(struct recording *rec){
	return syscall(__NR_start_rec, 1, rec);
}

long stop_rec(void){
	return syscall(__NR_stop_rec);
}
//its the same thing
long stop_rep(void){
	return syscall(__NR_stop_rec);
}

long rec_owner(void){
	return syscall(__NR_rec_owner);
}
