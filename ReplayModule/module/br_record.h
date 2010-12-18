#ifndef BR_RECORD_H
#define BR_RECORD_H
#include <linux/record.h>
long start_record(int state, struct recording *rec);
long stop_record(void);
long record_owner(void);

struct br_info{
	struct list_head list;
	struct recording rec;
};

#endif/*BR_RECORD_H*/
