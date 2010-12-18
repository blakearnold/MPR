#ifndef RECORD_H
#define RECORD_H
#include <linux/types.h>
struct recording{
		struct recording *next;
		pid_t threadId;
		long long unsigned instructionOffset;
		long long unsigned branchNum;
};

#endif /*RECORD_H*/
