/* fast readlink command 'rl' for /proc/pid/fd */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main (int c, char **v)
{
	char buf[1024];
	int i;
	for (i=1; i<c; i++)
	{
		int x = readlink (v[i], buf, 1023);
		if (x>0)
		{
			int n = 0;
			buf[x] = 0;
			// Search for /proc/###
			if (strlen(v[i])>6)
				n = atoi (&v[i][6]);
			printf ("%d\t%s\n", n, buf);
		}
	}
	return 0;
}
