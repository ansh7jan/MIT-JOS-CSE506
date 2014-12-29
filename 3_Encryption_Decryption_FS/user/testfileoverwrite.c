#include <inc/lib.h>

#define NEW_SIZE 1
char buf2[NEW_SIZE];

void
umain(int argc, char **argv)
{

	binaryname = "testfileoverwrite";
	int wfdnum, r;
	struct Fd *wfd;

        if ((wfdnum = open("/newfile", O_CREAT | O_RDWR)) < 0) {
                if (wfdnum == -E_NOT_FOUND)
                        panic("error: file /newfile not found\n");
        }

////////////////////////////////////////////////////////////////////////////
//	Write data in 4096 bytes
///////////////////////////////////////////////////////////////////////////
	memset(buf2, 'O', NEW_SIZE);
	if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
		panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'V', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'E', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'R', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'W', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'R', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

        memset(buf2, 'I', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

	memset(buf2, 'T', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

	memset(buf2, 'E', NEW_SIZE);
        if ((r = write(wfdnum, buf2, NEW_SIZE))!=NEW_SIZE)
                panic("error: cannot write %d bytes\n", NEW_SIZE);

   	cprintf("Finished overwriting contents to /newfile file\n");
        struct Stat temp_stat;
        stat("/newfile", &temp_stat);
	close(wfdnum);
}
