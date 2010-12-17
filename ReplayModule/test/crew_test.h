#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>

long start_rec(void){
	return syscall(__NR_start_rec);
}

long stop_rec(void){
	return syscall(__NR_stop_rec);
}

long rec_owner(void){
	return syscall(__NR_rec_owner);
}
