#include "ns.h"
#include <inc/lib.h>

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	int r;
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	
	while(true)
	{
		r = sys_ipc_recv(&nsipcbuf);
		if (r<0)
			return ;
		while((r = sys_transmit_packet(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) < 0);
			//sys_yield();

	}

}
