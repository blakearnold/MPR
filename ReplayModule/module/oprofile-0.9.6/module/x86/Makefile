OPROFILE_DIR=/home/blake/module/oprofile-0.9.6
VERSION=0.9.6
KVERS=
KSRC=/lib/modules/2.6.24-26-generic/build

EXTRA_CFLAGS= -DEXPECT_OK -D__NO_VERSION__ \
	-I$(OPROFILE_DIR)/ -I$(OPROFILE_DIR)/libutil -I$(OPROFILE_DIR)/libop -I$(OPROFILE_DIR)/module
 
USE_STANDARD_AS_RULE=1
 
# NB: do not add a file here without adding it to EXTRA_DIST in ../Makefile.am

O_TARGET := arch.o

obj-y := cpu_type.o op_apic.o op_fixmap.o op_rtc.o op_nmi.o \
	op_model_ppro.o op_model_athlon.o op_model_p4.o 

obj-y += oprofile_nmi.o op_syscalls.o

obj-m := $(O_TARGET)
O_OBJS := $(obj-y)
M_OBJS := $(O_TARGET)

TOPDIR := $(KSRC)
CONFIG_SHELL := TOPDIR=$(KSRC) /bin/bash
include $(KSRC)/Rules.make

# FIXME: dependencies 
