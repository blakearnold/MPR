#ifndef BR_PMCACCESS_H
#define BR_PMCACCESS_H

#define BR_INST 		0xc4
#define BR_UMASK		0x00
#define BR_MISS 		0xc5
#define BR_MISS_UMASK 	0x0

struct perfevesel{



		unsigned event,
		mask,
		_INV :1,	//invert counter mask
		_EN : 1,	//enable
		_ANY : 1, //tracks threads accross processors
		_INT : 1, //interupt, turn on for replaying
		_PC : 1, 	//pin control
		_E : 1,	//edge
		_OS : 1,	//OS detection, TODO: do we want system calls tracked?
		_USR : 1;	//track user level code


};
int probeCPUID(void);
void stopCounter(int counter);
int setupCounter(int counter,unsigned event, unsigned mask,int record);
void resetCounter(int counter);
unsigned long long  prepareRecordSel(struct perfevesel *prep);
unsigned long long  prepareReplaySel(struct perfevesel *prep);

#endif /*BR_PMCACCESS_H*/
