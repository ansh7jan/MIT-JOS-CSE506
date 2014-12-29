// System call stubs.

#include <inc/syscall.h>
#include <inc/lib.h>

static inline int64_t
syscall(int num, int check, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	int64_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");

	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint64_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

void
sys_yield(void)
{
	syscall(SYS_yield, 0, 0, 0, 0, 0, 0);
}

int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	return syscall(SYS_page_alloc, 1, envid, (uint64_t) va, perm, 0, 0);
}

int
sys_page_map(envid_t srcenv, void *srcva, envid_t dstenv, void *dstva, int perm)
{
	return syscall(SYS_page_map, 1, srcenv, (uint64_t) srcva, dstenv, (uint64_t) dstva, perm);
}

int
sys_page_unmap(envid_t envid, void *va)
{
	return syscall(SYS_page_unmap, 1, envid, (uint64_t) va, 0, 0, 0);
}

// sys_exofork is inlined in lib.h

int
sys_env_set_status(envid_t envid, int status)
{
	return syscall(SYS_env_set_status, 1, envid, status, 0, 0, 0);
}

int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	return syscall(SYS_env_set_trapframe, 1, envid, (uint64_t) tf, 0, 0, 0);
}

int
sys_env_set_pgfault_upcall(envid_t envid, void *upcall)
{
	return syscall(SYS_env_set_pgfault_upcall, 1, envid, (uint64_t) upcall, 0, 0, 0);
}

int
sys_ipc_try_send(envid_t envid, uint64_t value, void *srcva, int perm)
{
	return syscall(SYS_ipc_try_send, 0, envid, value, (uint64_t) srcva, perm, 0);
}

int
sys_ipc_recv(void *dstva)
{
	return syscall(SYS_ipc_recv, 1, (uint64_t)dstva, 0, 0, 0, 0);
}

unsigned int
sys_time_msec(void)
{
	return (unsigned int) syscall(SYS_time_msec, 0, 0, 0, 0, 0, 0);
}

int
sys_transmit_packet(char *packet, uint32_t len)
{
	return syscall(SYS_transmit_packet, 0, (uint64_t ) packet, (uint64_t) len, 0, 0, 0);
}

int
sys_receive_packet(char *packet, int *len)
{
        return syscall(SYS_receive_packet, 0, (uint64_t ) packet, (uint64_t)len, 0, 0, 0);
}

int
setstatus(int psid, int status)
{
        return syscall(SYS_setstatus, 0, (uint64_t ) psid, (uint64_t)status, 0, 0, 0);
}

int
sys_pageinfo(envid_t env_id, uint32_t *addr, void *temp_mem, int *status)
{
		return syscall(SYS_pageinfo, 1, (uint64_t)env_id, (uint64_t)addr, (uint64_t)temp_mem, (uint64_t)status, 0);
}

int 
sys_settrapframe(envid_t *env_id, struct Trapframe *tf)
{
	return syscall(SYS_settrapframe, 1, (uint64_t)env_id, (uint64_t)tf, 0, 0, 0);
}

int 
sys_pageinsert(envid_t env_id, void *temp_mem, uint8_t *addr)
{
	return syscall(SYS_pageinsert, 1, (uint64_t)env_id, (uint64_t)temp_mem, (uint64_t)addr, 0, 0);
}