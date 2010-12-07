#include </home/blake/kernel/ubuntu-hardy/include/asm-x86/unistd_32.h>
#include <sys/syscall.h>

long start_rec(void){
	return syscall(__NR_start_rec);
}

long stop_rec(void){
	return syscall(__NR_stop_rec);
}

long rec_owner(void){
	return syscall(__NR_rec_owner);
}
