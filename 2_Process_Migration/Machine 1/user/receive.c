#include <inc/lib.h>
#include <lwip/sockets.h>

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

int recvpacket(void *buffer, void *pp, struct migrate *me, int num, int clientsock)
{
	memset(buffer, 0, PGSIZE);
	memset(me, 0, sizeof(struct migrate));
	read(clientsock, buffer, sizeof(struct migrate));
	memcpy(me, (struct migrate *)buffer, sizeof(struct migrate));
	if( num == 0 && me->va == 0)
		return -1;
	memcpy(pp + num * PKTSIZE, (void *)(me->info), PKTSIZE);			      
	sleep();
	return 0;
}

void
umain(int argc, char **argv)
{
	int serversock;
	int clientsock;
	struct sockaddr_in server;
	struct sockaddr_in client;

	int r, counter;
	envid_t envid;
	uint8_t *start;
    struct Trapframe tf;
	void *buffer = (void *)(UTEMP);
	void *pp = (void *)(UTEMP  + PGSIZE);
	struct migrate *me=(struct migrate *)malloc(sizeof(struct migrate));
	memset(&tf, 0, sizeof(struct Trapframe));
	
	if((serversock=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		die("Failed to create socket\n");
		 
	memset(&server, 0, sizeof(server));     
	server.sin_family=AF_INET;            
	server.sin_addr.s_addr=htonl(INADDR_ANY); 
	server.sin_port=htons(50001);          	
	if(bind(serversock, (struct sockaddr *)&server, sizeof(server)) < 0)
		die("Failed to bind\n");

	if(listen(serversock, 10) < 0)
		die("Failed to listen\n");
				
	cprintf("Waiting for connections\n");

	unsigned int clientlen=sizeof(client);
	if((clientsock=accept(serversock, (struct sockaddr *)&client, &clientlen)) < 0)
		die("Failed to accept client connection\n");
	
	r = sys_page_alloc(0, buffer, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("send : panic in sys_page_alloc\n");
	r = sys_page_alloc(0, pp, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("recv : panic in sys_page_alloc\n");
	memset(buffer, 0, PGSIZE);
	memset(pp, 0, PGSIZE);
	read(clientsock, buffer, sizeof(struct Trapframe));
	memcpy((void *)&tf, buffer, sizeof(struct Trapframe));
	sys_settrapframe(&envid, &tf);
	
	while(true)
	{
		r = recvpacket(buffer, pp, me, 0, clientsock);
		if (r== -1)
			break;
		cprintf("Receiving Page : %d  at 0x%08x\n", counter, me->va);
		recvpacket(buffer, pp, me, 1, clientsock);
		recvpacket(buffer, pp, me, 2, clientsock);
		recvpacket(buffer, pp, me, 3, clientsock);
		
		start = (uint8_t *)(me->va);
		sys_pageinsert(envid, (void *)pp, start);
		memset(pp, 0, PGSIZE);
		counter++;
	}
	cprintf("Process migrated. Resume : %d\n", envid);
	close(clientsock);
	return;
}
