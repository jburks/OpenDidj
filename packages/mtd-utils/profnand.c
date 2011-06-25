/*
profnand.c: 
    Read the sysfs entry for mtd nand statistics (configure with
    CONFIG_MTD_NAND_LF1000_PROF) and formats them as human readable,
    times in seconds, of min/ave/max read, write, erase operations, as
    well as overall lock time.
R Dowling 3/19/2008
*/
#include <stdio.h> 
#include <stdlib.h>
#define WRITE	"/sys/devices/platform/lf1000-nand/write"
#define DIV	4.593e6
main ()
{
	enum nstype { NS_READ, NS_WRITE, NS_ERASE, NS_LOCK, NS_MAX };
	unsigned long ns_n, ns_min, ns_mean, ns_max;
	char name[10];
	int i;
	FILE *f = fopen (WRITE, "r");
	if (!f)
	{
		printf ("Can't open %s for reading\n", WRITE);
		exit (1);
	}
	for (i=0; i<NS_MAX; i++)
	{
		fscanf (f, "%s N=%ld %ld/%ld/%ld", &name, &ns_n, &ns_min, &ns_mean, &ns_max);
		printf ("%-5s N=%6ld %.6f/%.6f/%.6f Total=%.6f\n", name, ns_n, ns_min/DIV, ns_mean/DIV, ns_max/DIV, 
			ns_mean/DIV*ns_n);
	}
}
