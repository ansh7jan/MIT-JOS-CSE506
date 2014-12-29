#include <inc/lib.h>

#define NEW_SIZE 4096
char buf2[NEW_SIZE];

void
umain(int argc, char **argv)
{

	binaryname = "testfilewrite2";
	int wfdnum, r;
	struct Fd *wfd;

        if ((wfdnum = open("/filesarang", O_CREAT | O_RDWR)) < 0) {
                if (wfdnum == -E_NOT_FOUND)
                        panic("error: file /filesarang not found\n");
        }

////////////////////////////////////////////////////////////////////////////
//	Write data in 4096 bytes
///////////////////////////////////////////////////////////////////////////
	memset(buf2, 'S', NEW_SIZE);
	if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
		panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'A', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'R', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'A', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'N', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'G', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

   	cprintf("Finished writing contents S-A-R-A-N-G to /filesarang file\n");
        struct Stat temp_stat;
        stat("/filesarang", &temp_stat);
	close(wfdnum);
}
