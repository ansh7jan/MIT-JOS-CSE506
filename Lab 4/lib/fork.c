// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;
	addr = ROUNDDOWN(addr, PGSIZE);
	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
	if (!((err & FEC_WR) && (uvpt[VPN(addr)] & PTE_COW)) )
		panic("panic due faulting access of non COW page in pgfault");
	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.
	r = sys_page_alloc(0, (void*)PFTEMP, PTE_P|PTE_U|PTE_W);
	if(r<0)
		panic("panic during page allocation in pgfault");
	memmove(PFTEMP, addr, PGSIZE);

	r = sys_page_map(0, (void*)PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W);	
	if(r<0)
                panic("panic during page mapping in pgfault");
	
	// LAB 4: Your code here.

	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	uintptr_t va;

	pte_t entry = uvpt[pn];
	int perm = entry & PTE_SYSCALL;	
	// LAB 4: Your code here.
	va = pn * PGSIZE;
	pte_t pte = uvpt[pn];
	if ( !( (pte & PTE_W) || (pte & PTE_COW) ) )
	{
		r = sys_page_map(0, (void *)va, envid, (void *)va, PTE_P|PTE_U);
		if(r<0)
			panic("page map error in duppage\n");
		return 0;
	}else
	{
		r = sys_page_map(0, (void *)va, envid, (void *)va, PTE_P|PTE_U|PTE_COW);
		if(r<0)
                        panic("page map error in duppage\n");
		r = sys_page_map(0, (void *)va, 0, (void *)va, PTE_P|PTE_U|PTE_COW);
		if(r<0)
                        panic("page map error in duppage\n");
		return 0;
	}
	//panic("duppage not implemented");
	//return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	int r;
	uint64_t start,end;
	envid_t envid;	

	set_pgfault_handler(pgfault);
	envid = sys_exofork();
	if (envid < 0)
		panic("panic in fork");
	if (envid == 0)		
	{
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0; 
	}

	for(start = PPN(UTEXT); start < PPN(UXSTACKTOP-PGSIZE); ) 
	 { 
		if(!(uvpml4e[VPML4E(start)] & PTE_P))
		{ 
                        start += NPTENTRIES; 
                        continue; 
                }
		if(!(uvpde[(start>>18)] & PTE_P))
		{ 
                        start += NPTENTRIES; 
                        continue; 
                }
		if(!(uvpd[(start>>9)] & PTE_P)) 
	 	{ 
	 		start += NPTENTRIES; 
			continue; 
	 	}
		end = start + NPTENTRIES;
		for(; start < end ; start++)
	 	{ 
			if((uvpt[start] & PTE_P)!=PTE_P) 
	 			continue; 

	 		if(start == PPN(UXSTACKTOP - 1)) 
	 			continue;
	 		duppage(envid, start); 
	 	} 
	 } 

/*	for (start = UTEXT; start < (UXSTACKTOP - PGSIZE); start = start + PGSIZE)
	{
		if ( (uvpml4e[VPML4E(start)] & PTE_P) && (uvpde[VPDPE(start)] & PTE_P) && (uvpd[VPD(start)] & PTE_P) && (uvpt[VPN(start)] & PTE_P) )
			duppage(envid, VPN(start));
	}
*/
	r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_U|PTE_W|PTE_P);
	if (r<0)
		panic("panic at allocation of Exception stack in fork");

	extern void _pgfault_upcall(void);
        r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	
	if (r<0)
		panic("panic at setting pgfault upcall");
	r = sys_env_set_status(envid, ENV_RUNNABLE);
	if (r<0)
		panic("panic at setting environment runnable");
	return envid;
	
	panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
