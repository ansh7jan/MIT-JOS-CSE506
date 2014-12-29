//
// Simple inline assembly example
// 
// For JOS lab 1 exercise 1
//

#include <stdio.h>

int main(int argc, char **argv)
{
  int x = 1;
  printf("Hello x = %d\n", x);
  
  
      __asm__ ("movl %1, %0\n\t"
      	       "addl $1, %0"
	       : "=r" (x)			/* output operands */
	       : "r" (x)            /* input operands */
	       : "0");				/* clobbered operands */

  printf("Hello x = %d after increment\n", x);

  if(x == 2){
    printf("OK\n");
  }
  else{
    printf("ERROR\n");
  }
}
