#ifndef BR_RECORD_H
#define BR_RECORD_H
long start_record(void);
long stop_record(void);
long record_owner(void);

struct recording{
		struct list_head list;
		pid_t threadId;
		int instructionOffset;
		int branchNum;
};
#endif/*BR_RECORD_H*/
