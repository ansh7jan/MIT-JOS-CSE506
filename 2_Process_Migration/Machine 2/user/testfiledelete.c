#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	binaryname = "testfiledelete";
	struct Stat temp_stat;
	remove("/newfile");
	stat("/newfile", &temp_stat);
}
