// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/dwarf.h>
#include <kern/kdebug.h>
#include <kern/dwarf_api.h>
#include <kern/trap.h>

#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line



struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display Stack Backtrace", mon_backtrace },
	{"setcolor", "Choose color to set - red, green, yellow, blue, magenta, cyan, white ", mon_color },
	{"showmappings", "Enter range to show mappings", mon_showmappings },
	{"dumpmemory", "Enter the range for which to dump the memory", mon_dumpmemory },
	{"step", "Single step one instruction if the kernel monitor was invoked via breakpoint exception", mon_step },
	{"continue", "Continue if the kernel monitor was invoked via breakpoint exception", mon_continue }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	int i;
	struct Ripdebuginfo *info=NULL;
	uint64_t *x= (uint64_t *)read_rbp();
	uintptr_t rip;
	read_rip(rip);
    debuginfo_rip(rip, info);
    uint64_t y = rip;
    uint64_t z  = info->rip_fn_addr;
    uint64_t count = info->rip_fn_narg;
    cprintf("rbp %016x rip %016x",rip-1, rip);
    cprintf("\n%s:%d:  %s+%016x args:%d ", info->rip_file, info->rip_line, info->rip_fn_name, y-z, info->rip_fn_narg);	
    i=0;
    while(count--) {
        cprintf("%016x ", (rip+3+(i++))>>32);
    }
    cprintf("\n");
    
	x= (uint64_t *)read_rbp();
	while (x) {	
	    debuginfo_rip(*(x+1), info);
	    cprintf("rbp %016x rip %016x",*x, *(x+1));
	    y = *(x+1);
	    z  = info->rip_fn_addr;
	    cprintf("\n%s:%d:  %s+%016x args:%d ", info->rip_file, info->rip_line, info->rip_fn_name, y-z, info->rip_fn_narg);
	    i=0;
	    count = info->rip_fn_narg;
	    while(count--) {
	        cprintf("%016x ", *(x+3+(i++))>>32);
	    }
	    cprintf("\n");
//	    if(strstr(info->rip_fn_name,"i386_init")!=NULL)
//	        return 0;
//	    else
    	    x= (uint64_t *)* x;
    }
	return 0;
}

int
mon_color(int argc, char **argv, struct Trapframe *tf)
{
	char *str = argv[1];
	if (strcmp(str, "red") == 0)
                cprintf("\x1b[31m");
	else if (strcmp(str, "green") == 0)
                cprintf("\x1b[32m");
	else if (strcmp(str, "yellow") == 0)
                cprintf("\x1b[33m"); 
	else if (strcmp(str, "blue") == 0)
		cprintf("\x1b[34m");
	else if (strcmp(str, "magenta") == 0)
		cprintf("\x1b[35m");
	else if (strcmp(str, "cyan") == 0)    
                cprintf("\x1b[36m");
	else
                cprintf("\x1b[0m");
	return 0;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	pte_t *pte;
	int set=0;
	uint64_t start = (uint64_t)strtol(argv[1], NULL, 0);
	uint64_t end = (uint64_t)strtol(argv[2], NULL, 0);

	cprintf("start %x, end %x\n",start, end );
	
	if (start % PGSIZE !=0)
		start =start - (start % PGSIZE);
	if (end % PGSIZE !=0)
		end = end + (PGSIZE - (end % PGSIZE));
	cprintf("start %x, end %x\n",start, end );

	//cprintf("boot_pml4e values : %x\n",boot_pml4e);
	for (; end >=start; start = start+PGSIZE)
	{
		pte = pml4e_walk(boot_pml4e, (void *)start, 0);
		if (pte)
		{
			cprintf("0x%x  0x%x  ", start, PTE_ADDR(*pte));
			if (*pte & PTE_P)
				set = 1;
			cprintf("PTE_P %d  ", set);
			set = 0;
			if (*pte & PTE_U)
                                set = 1;
                        cprintf("PTE_U %d  ", set);
                        set = 0;
			if (*pte & PTE_P)
                                set = 1;
                        cprintf("PTE_W %d\n", set);
			set = 0;
		}
		else
			cprintf("0x%x  0x%x  PTE_P 0  PTE_U 0  PTE_W 0", start, PTE_ADDR(*pte));
	}
	return 0;
}

int
mon_dumpmemory(int argc, char **argv, struct Trapframe *tf)
{
	uint64_t *mem;
	uint64_t start = (uint64_t)strtol(argv[1], NULL, 0);
        uint64_t end = (uint64_t)strtol(argv[2], NULL, 0);

        cprintf("start %x, end %x\n",start, end );
	mem = (uint64_t *)start;

	for(; mem < (uint64_t *)end; mem=mem+4)
	{
		cprintf("0x%08x \t 0x%08x \t 0x%08x \t 0x%08x\n", mem[0], mem[1], mem[2], mem[3]);
	}
	return 0;
}

int
mon_step(int argc, char **argv, struct Trapframe *tf)
{
	if (tf->tf_trapno == T_BRKPT)
	{
		tf->tf_eflags = tf->tf_eflags|FL_TF;	
	}
	else
		cprintf("Not a breakpoint exception. Can't step into\n");
	return -1;
}

int
mon_continue(int argc, char **argv, struct Trapframe *tf)
{
        if (tf->tf_trapno == T_BRKPT)
        {
		tf->tf_eflags = tf->tf_eflags & ~FL_TF;
                tf->tf_eflags = tf->tf_eflags | FL_RF; 
        }
        else
                cprintf("Not a breakpoint exception. Can't continue\n");
        return -1;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
