obj-m += replay.o
replay-objs := branchInfo.o br_pmcaccess.o br_record.o br_op_nmi.o op_apic.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
