#include <inc/lib.h>

void divideByZero()
{
	cprintf("In user env: divide by zero occured..exiting\n");
	exit();
}

void
umain(int argc, char **argv)
{
	sys_env_set_divide_upcall(0, divideByZero);
 	int a = 100;
	int b =0;
	int c = a/b;
	cprintf("main program \n");
}
