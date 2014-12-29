#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/syscall.h>

#include <inc/string.h>

#define PKTSIZE 1518
#define RCVPKTSIZE 2048
#define E1000_STATUS   0x00008
#define TX_DESC_COUNT 64
#define RX_DESC_COUNT 64

#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */

#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */

#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */

#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */

#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */

#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */

#define E1000_RA       0x05400  /* Receive Address - RW Array */
#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */
#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */
#define E1000_RCTL     0x00100  /* RX Control - RW */
#define E1000_RCTL_EN             0x00000002    /* enable */
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */

struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};

struct rx_desc
{
	uint64_t addr;
	uint16_t length;
	uint16_t checksum;
	uint8_t status;
	uint8_t error;
	uint16_t special;
};

int attach_nc(struct pci_func *f);
int transmit_packet(char *packet, uint32_t len);
int receive_packet(char *packet);

#endif	// JOS_KERN_E1000_H
