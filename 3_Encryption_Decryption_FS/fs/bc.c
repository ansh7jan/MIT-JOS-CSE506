
#include "fs.h"

// Return the virtual address of this disk block.
void*
diskaddr(uint64_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (DISKMAP + blockno * BLKSIZE);
}

// Fault any disk block that is read in to memory by
// loading it from disk.
static void
bc_pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint64_t blockno = ((uint64_t)addr - DISKMAP) / BLKSIZE;
	int r;

	// Check that the fault was within the block cache region
	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("page fault in FS: eip %08x, va %08x, err %04x",
		      utf->utf_rip, addr, utf->utf_err);

	// Sanity check the block number.
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Allocate a page in the disk map region, read the contents
	// of the block from the disk into that page.
	// Hint: first round addr to page boundary.
	//
	// LAB 5: you code here:
	addr= ROUNDDOWN(addr, BLKSIZE);
	r = sys_page_alloc(thisenv->env_id, addr, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("Panic in bc_pgfault\n");
	if ((blockno == 0) || (blockno == 1) || (blockno == 2))
	r = ide_read(blockno * 8, addr, 8);
	else
		r = decrypt_block(blockno, addr);
        if (r<0)
		panic("Panic in bc_pgfault\n");

}

void
flush_block(void *addr)
{
	uint64_t blockno = ((uint64_t)addr - DISKMAP) / BLKSIZE;
        int r;
        bool ismapped = 0 , isdirty = 0;
        bool flag = 0;

        addr = ROUNDDOWN(addr, BLKSIZE);
        ismapped = (uvpml4e[VPML4E(addr)] & PTE_P) && (uvpde[VPDPE(addr)] & PTE_P) && (uvpd[VPD(addr)] & PTE_P) && (uvpt[PPN(addr)] & PTE_P);        
        if (ismapped)
       {
                   isdirty = (uvpt[PPN(addr)] & PTE_D)!=0;
                    if (isdirty)
                        flag =true;
        }
        if (flag)
        {               
				if ((blockno == 0) || (blockno == 1) || (blockno == 2))
				{
                r = ide_write(8 * blockno, addr, 8);
                if (r<0)
                       panic("write failed");
				}
				else
					encrypt_block(blockno, addr);
                r = sys_page_map(thisenv->env_id, addr, thisenv->env_id, addr, PTE_SYSCALL);
                if (r<0)
                       panic("page map failed while flushing");
        } 
}

void
bc_init(void)
{
	struct Super super;
	set_pgfault_handler(bc_pgfault);

	// cache the super block by reading it once
	memmove(&super, diskaddr(1), sizeof super);
}

