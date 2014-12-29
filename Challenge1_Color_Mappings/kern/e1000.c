#include <kern/e1000.h>

volatile uint32_t *e1000_bar0;

struct tx_desc tx_desc_arr[TX_DESC_COUNT];
char pkt_arr[TX_DESC_COUNT][PKTSIZE];

struct rx_desc rx_desc_arr[RX_DESC_COUNT];
char recv_pkt_arr[RX_DESC_COUNT][RCVPKTSIZE];

int attach_nc(struct pci_func *f)
{
	int i;
	pci_func_enable(f);
		
	e1000_bar0 = mmio_map_region(f->reg_base[0], f->reg_size[0]);
	
	//cprintf("attach_nc %x, dsr--%x \n", *e1000_bar0, e1000_bar0[E1000_STATUS/2]);

	//cprintf(" AAA--   %x \n", PADDR(tx_desc_arr));
	e1000_bar0[E1000_TDBAL/4] = PADDR(tx_desc_arr);
	e1000_bar0[E1000_TDLEN/4] = sizeof(struct tx_desc) * TX_DESC_COUNT;	
	e1000_bar0[E1000_TDH/4] = 0x0;
	e1000_bar0[E1000_TDT/4] = 0x0;
	e1000_bar0[E1000_TCTL/4] =  E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD;

	for (i=0; i< TX_DESC_COUNT; i++)
		tx_desc_arr[i].status = E1000_TXD_STAT_DD;

	cprintf("Anshul---attach %x\n", curenv);
	e1000_bar0[E1000_RA/4] = 0x12005452;
	e1000_bar0[E1000_RA/4 + 1] = 0x5634 | E1000_RAH_AV;
	e1000_bar0[E1000_MTA/4] = 0x0;
	e1000_bar0[E1000_RDBAL/4] = PADDR(rx_desc_arr);
        e1000_bar0[E1000_RDLEN/4] = sizeof(struct rx_desc) * RX_DESC_COUNT;

	for (i=0; i< RX_DESC_COUNT; i++)
		rx_desc_arr[i].addr = PADDR(recv_pkt_arr[i]);
	e1000_bar0[E1000_RDH/4] = 0x0;
	e1000_bar0[E1000_RDT/4] = 0x0;
	e1000_bar0[E1000_RCTL/4] = E1000_RCTL_EN | E1000_RCTL_SECRC ;	

	return 0;
}


int transmit_packet(char *packet, uint32_t len)
{
	bool status;
	int tdt;

	tdt = e1000_bar0[E1000_TDT/4];
	status = tx_desc_arr[tdt].status & E1000_TXD_STAT_DD;
	//cprintf("---AAA--BB, %x", status);
	if(status)
	{
		tx_desc_arr[tdt].addr = PADDR(pkt_arr[tdt]);
		memcpy((void *)pkt_arr[tdt], packet, len);
		tx_desc_arr[tdt].length = len;
		tx_desc_arr[tdt].status = 0;
		tx_desc_arr[tdt].cmd |= 0x08|0x01;
		e1000_bar0[E1000_TDT/4] = (tdt+1) % TX_DESC_COUNT;
		return 0;
	}
	else
		return -1;
}

int receive_packet(char *packet )
{
	bool status;
	int rdt;
	int length=0;
	rdt = e1000_bar0[E1000_RDT/4];
//	cprintf("receive packet rdt-- %x  \n", rdt);
	status = rx_desc_arr[rdt].status & E1000_RXD_STAT_DD;
	length = rx_desc_arr[rdt].length;	
//	cprintf("fsdsdsa %x\n", length);
//	cprintf("receive packet -- %x  \n", status);
	if (status)
	{
//		cprintf("QWERT\n");
		memcpy(packet, recv_pkt_arr[rdt], length);	
//		cprintf("QAZWSXEDC\n");
		rx_desc_arr[rdt].status = 0;
		e1000_bar0[E1000_RDT/4] = (rdt+1) % RX_DESC_COUNT;
//		 cprintf("TTTTTTTTTTTTTTTTT\n");
		return length;
	}
	else
		return -1;


}
