#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	int r;
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int length;
	length =2048;
	while (true)
	{
		//int length=0;
		char buf[2048];
		while (( r = sys_receive_packet(buf, &length)) <0)
			sys_yield();

		while ((r = sys_page_alloc(0, &nsipcbuf, PTE_P|PTE_U|PTE_W)) < 0);
		nsipcbuf.pkt.jp_len = length;
//		cprintf("---input-- %x  %x", r, length);
		memmove(nsipcbuf.pkt.jp_data, buf, length);
//		cprintf("---BBBBB--");
		while (( r = sys_ipc_try_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P|PTE_U|PTE_W)) <0 );
	
       
	}



}
