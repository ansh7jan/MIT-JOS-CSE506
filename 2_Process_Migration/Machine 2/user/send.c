#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

#define PKTSIZE 1024
void sleep()
{
	sys_yield();
	sys_yield();
    sys_yield();
    sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
}
static void
die(char *m)
{
	cprintf("%s\n", m);
	exit();
}

void sendpacket (void *buffer, void *pp, uint8_t *start, int num, int sockfd)
{
	struct migrate *me = (struct migrate *)malloc(sizeof(struct migrate));
	memset(buffer, 0, PGSIZE);
	memset(me, 0, sizeof(struct migrate));
	memcpy(me->info, (char *)(pp + num*PKTSIZE), PKTSIZE);
	me->va = (uint32_t *)start;
	memcpy(buffer, (void *)me, sizeof(struct migrate));
	write(sockfd, buffer, sizeof(struct migrate));
	sleep();
}

void
umain(int argc, char **argv)
{
	if(argc != 5)
	{
		cprintf("migrate_send_normal: argument error!\n");
		cprintf("migrate_send_normal \"env_id\" \"destination_ip\" \"destination_port\" \"local_ip\"\n");
		return;
	}

	int sockfd, i, r, flag, counter =0;
	envid_t envid;
	uint8_t *start;
	struct Trapframe tf;
	struct sockaddr_in servaddr;
	void *buffer = (void *)(UTEMP);
	void *pp = (void *)(UTEMP  + PGSIZE);

	envid = strtol(argv[1], 0, 0);
	for(i=0; i<NENV; i++)
		if(envs[i].env_id == envid)
			break;
						     
	if(envs[i].env_status == ENV_RUNNABLE)
		die("Pause the process first\n");
		
	memset(&tf, 0, sizeof(struct Trapframe));
	
	if((sockfd=socket(PF_INET,SOCK_STREAM, IPPROTO_UDP)) < 0)
		die("Failed to create socket");;

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[2]);
	servaddr.sin_port=htons(strtol(argv[3], 0, 0));

	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		die("Failed to connect");

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
	write(sockfd, buffer, sizeof(struct Trapframe));
	
	for(start = (uint8_t*)UTEXT; start<(uint8_t*)USTACKTOP; start = start + PGSIZE)
	{
		sys_pageinfo(envid, (uint32_t*)start, pp, &flag);
		if(flag == 1)
		{
			cprintf("Sending Page : %d  at 0x%08x\n", counter, (uint32_t *)start);
			
			sendpacket(buffer, pp, start, 0, sockfd);
			sendpacket(buffer, pp, start, 1, sockfd);
			sendpacket(buffer, pp, start, 2, sockfd);
			sendpacket(buffer, pp, start, 3, sockfd);
			counter++;
		}
		memset(pp, 0, PGSIZE);
	}
	start = 0;
	sendpacket(buffer, pp, start, 0, sockfd);
	cprintf("Sending successful\n");	
	close(sockfd);
	return;
}
