#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

#define PKTSIZE 1024

void save (void *buffer, void *pp, uint8_t *start, int num, int fd)
{
	struct migrate *me = (struct migrate *)malloc(sizeof(struct migrate));
	memset(buffer, 0, PGSIZE);
	memset(me, 0, sizeof(struct migrate));
	memcpy(me->info, (char *)(pp + num*PKTSIZE), PKTSIZE);
	me->va = (uint32_t *)start;
	memcpy(buffer, (void *)me, sizeof(struct migrate));
	write(fd, buffer, sizeof(struct migrate));
}

void
umain(int argc, char **argv)
{
	if(argc != 3)
	{
		cprintf("Correct format - save <envid> <filename> \n");
		return;
	}

	int fd, i, r, flag, counter =0;
	envid_t envid;
	uint8_t *start;
	struct Trapframe tf;
	void *buffer = (void *)(UTEMP);
	void *pp = (void *)(UTEMP  + PGSIZE);

	envid = strtol(argv[1], 0, 0);
	for(i=0; i<NENV; i++)
		if(envs[i].env_id == envid)
			break;
						     
	if(envs[i].env_status == ENV_RUNNABLE)
	{
		cprintf("Pause the process first\n");
		return;
	}
	fd = open(argv[2], O_RDWR|O_CREAT);	
	memset(&tf, 0, sizeof(struct Trapframe));
	r = sys_page_alloc(0, buffer, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("send : panic in sys_page_alloc\n");
	r = sys_page_alloc(0, pp, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("send : panic in sys_page_alloc\n");
	memset(buffer, 0, PGSIZE);
	memset(pp, 0, PGSIZE);
	tf = envs[i].env_tf;
	memcpy(buffer, (void *)&tf, sizeof(struct Trapframe));
	write(fd, buffer, sizeof(struct Trapframe));
	
	for(start = (uint8_t*)UTEXT; start<(uint8_t*)USTACKTOP; start = start + PGSIZE)
	{
		sys_pageinfo(envid, (uint32_t*)start, pp, &flag);
		if(flag == 1)
		{
			cprintf("Saving Page : %d  at 0x%08x\n", counter, (uint32_t *)start);
			
			save(buffer, pp, start, 0, fd);
			save(buffer, pp, start, 1, fd);
			save(buffer, pp, start, 2, fd);
			save(buffer, pp, start, 3, fd);
			counter++;
		}
		memset(pp, 0, PGSIZE);
	}
	start = 0;
	save(buffer, pp, start, 0, fd);
	cprintf("Successfully saved %d pages of env %d on disk to file %s\n", counter, envid, argv[2]);	
	close(fd);
	return;
}
