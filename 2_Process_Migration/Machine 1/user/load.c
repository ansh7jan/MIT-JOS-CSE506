#include <inc/lib.h>

#define PKTSIZE 1024

int load(void *buffer, void *pp, struct migrate *me, int num, int fd)
{
	memset(buffer, 0, PGSIZE);
	memset(me, 0, sizeof(struct migrate));
	read(fd, buffer, sizeof(struct migrate));
	memcpy(me, (struct migrate *)buffer, sizeof(struct migrate));
	if( num == 0 && me->va == 0)
		return -1;
	memcpy(pp + num * PKTSIZE, (void *)(me->info), PKTSIZE);
	return 0;
}

void
umain(int argc, char **argv)
{
	int r, counter=0, fd;
	envid_t envid;
	uint8_t *start;
    struct Trapframe tf;
	void *buffer = (void *)(UTEMP);
	void *pp = (void *)(UTEMP  + PGSIZE);
	struct migrate *me=(struct migrate *)malloc(sizeof(struct migrate));
	memset(&tf, 0, sizeof(struct Trapframe));
	fd=open(argv[1], O_RDWR);
	r = sys_page_alloc(0, buffer, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("send : panic in sys_page_alloc\n");
	r = sys_page_alloc(0, pp, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("recv : panic in sys_page_alloc\n");
	memset(buffer, 0, PGSIZE);
	memset(pp, 0, PGSIZE);
	read(fd, buffer, sizeof(struct Trapframe));
	memcpy((void *)&tf, buffer, sizeof(struct Trapframe));
	sys_settrapframe(&envid, &tf);
	
	while(true)
	{
		r = load(buffer, pp, me, 0, fd);
		if (r== -1)
			break;
		cprintf("Loading Page : %d  at 0x%08x\n", counter, me->va);
		load(buffer, pp, me, 1, fd);
		load(buffer, pp, me, 2, fd);
		load(buffer, pp, me, 3, fd);
		
		start = (uint8_t *)(me->va);
		sys_pageinsert(envid, (void *)pp, start);
		memset(pp, 0, PGSIZE);
		counter++;
	}
	cprintf("Load succeeded. New environment is : %d\n", envid);
	close(fd);
	return;
}
