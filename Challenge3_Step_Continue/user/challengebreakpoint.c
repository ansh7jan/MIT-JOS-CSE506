// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int i=0;
	asm volatile("int $3");
	i =i+1;
	cprintf("Anshul\n");
	asm volatile("int $3");
	cprintf("Ankit\n");
}

