#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	system("poweroff");
	printf("%d\n",errno);
	return(0);
}
