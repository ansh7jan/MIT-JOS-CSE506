/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>
#include <kern/macro.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushq $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushq $0;							\
	pushq $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
   Find the PUSHA,POPA macros in kern/macro.h. Since amd64 doesn't support
   pusha,popa so you will find these macros handy.
 */

TRAPHANDLER_NOEC(xtrpx_divide, T_DIVIDE)
TRAPHANDLER_NOEC(xtrpx_debug, T_DEBUG)
TRAPHANDLER_NOEC(xtrpx_nmi, T_NMI)
TRAPHANDLER_NOEC(xtrpx_brkpt, T_BRKPT)
TRAPHANDLER_NOEC(xtrpx_oflow, T_OFLOW)
TRAPHANDLER_NOEC(xtrpx_bound, T_BOUND)
TRAPHANDLER_NOEC(xtrpx_illop, T_ILLOP)
TRAPHANDLER_NOEC(xtrpx_device, T_DEVICE)
TRAPHANDLER(xtrpx_dblflt, T_DBLFLT)
TRAPHANDLER(xtrpx_tss, T_TSS)
TRAPHANDLER(xtrpx_segnp, T_SEGNP)
TRAPHANDLER(xtrpx_stack, T_STACK)
TRAPHANDLER(xtrpx_gpflt, T_GPFLT)
TRAPHANDLER(xtrpx_pgflt, T_PGFLT)
TRAPHANDLER_NOEC(xtrpx_fperr, T_FPERR)
TRAPHANDLER_NOEC(xtrpx_align, T_ALIGN)
TRAPHANDLER_NOEC(xtrpx_mchk, T_MCHK)
TRAPHANDLER_NOEC(xtrpx_simderr, T_SIMDERR)
TRAPHANDLER_NOEC(xtrpx_syscall, T_SYSCALL)



/*
 * Lab 3: Your code here for _alltraps
 *
 * Hint: Be sure to review the x64 calling convention from lab1
 *       for how to pass a pointer to the trapframe.
 */

_alltraps:
movw %ds, %rbx
pushq %rbx
movw %es, %rbx
pushq %rbx
PUSHA
movw $GD_KD, %rbx
movw %rbx, %ds
movw %rbx, %es
movq %rsp, %rdi
call trap
POPA_
popq %rax
movw %rax, %es
popq %rax
movw %rax, %es
iret
