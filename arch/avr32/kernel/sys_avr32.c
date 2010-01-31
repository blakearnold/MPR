/*
 * Copyright (C) 2004-2006 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/unistd.h>

asmlinkage int sys_pipe(unsigned long __user *filedes)
{
	int fd[2];
	int error;

	error = do_pipe(fd);
	if (!error) {
		if (copy_to_user(filedes, fd, sizeof(fd)))
			error = -EFAULT;
	}
	return error;
}

int kernel_execve(const char *file, char **argv, char **envp)
{
	register long scno asm("r8") = __NR_execve;
	register long sc1 asm("r12") = (long)file;
	register long sc2 asm("r11") = (long)argv;
	register long sc3 asm("r10") = (long)envp;

	asm volatile("scall"
		     : "=r"(sc1)
		     : "r"(scno), "0"(sc1), "r"(sc2), "r"(sc3)
		     : "cc", "memory");
	return sc1;
}
