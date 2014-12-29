#include <inc/lib.h>
#include <inc/env.h>

void 
umain(int argc, char **argv)
{
	int count;
	cprintf("PID\tStatus\n");
	for(count=0; count<NENV; count++)
	{
		if(envs[count].env_status != ENV_FREE)
		{
			if(envs[count].env_status == ENV_RUNNING)
				cprintf("%d\tRunning\n",envs[count].env_id);
			if(envs[count].env_status == ENV_RUNNABLE)
				cprintf("%d\tRunnable\n",envs[count].env_id);
			if(envs[count].env_status == ENV_NOT_RUNNABLE)
				cprintf("%d\tNot Runnable\n",envs[count].env_id);
			}
		}
	cprintf("\n");
}

