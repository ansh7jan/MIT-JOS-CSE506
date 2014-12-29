#include <inc/lib.h>
#include <inc/env.h>

int atoi(char *str)
{
	int i=0;
	int num=0;
	
	while (str[i]!=0)
	{
		num = num*10 + str[i] - '0';
		i++;
	}
	return num;
}

void 
umain(int argc, char **argv)
{
	int processId = atoi(argv[1]);

	int r = setstatus(processId, ENV_NOT_RUNNABLE);
	if (r<0)
		cprintf("Failed to pause process\n");
	else
		cprintf("Process %d Pause\n", processId);
}
