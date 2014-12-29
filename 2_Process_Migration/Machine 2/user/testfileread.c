#include <inc/lib.h>

#define NEW_SIZE 4096
char buf[NEW_SIZE];

void
umain(int argc, char **argv)
{

	binaryname = "testfileread";
	int rfdnum, r, i;

        struct Stat temp_stat;
        stat("/newfile", &temp_stat);   	

	cprintf("\nReading contents of file /newfile\n");
        rfdnum = open("/newfile", O_RDONLY);
	
        if (rfdnum == -E_NOT_FOUND)
		cprintf("error: file /newfile not found\n");
      	else {
		////////////////////////////////////////////////////////////////////////////////
		//	Read 10 blocks in multiple of 4096 bytes
		/////////////////////////////////////////////////////////////////////////////////

		for (i=0; i<10; i++) {
			if ((r = read(rfdnum, buf, NEW_SIZE)) < 0)
				panic("error: cannot read %d bytes\n", NEW_SIZE);
			if (r == 0)
				break;
			else
				cprintf("\nBlock %d:  Length:%d  buf=%c\n",i,r,((char*)buf)[0]);
		}

		close(rfdnum);
	}
	cprintf("\nDone reading contents of file /newfile\n");
}
