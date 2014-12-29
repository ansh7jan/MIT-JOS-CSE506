#include <inc/lib.h>
#include <inc/env.h>

void umain(int argc, char **argv)
{
	int envid ;
//	envid = sys_getenvid();
//	cprintf("envid   \n%d", envid);
	
	int counter = 0;
	int now;
	int end;
	
	while(true)
	{
		envid = sys_getenvid();
		cprintf("envid %d   counter %d\n", envid, counter);
			counter++;

		now = sys_time_msec();
		end = sys_time_msec() + 10000;
		while(end > sys_time_msec())
			sys_yield();
	}
}
