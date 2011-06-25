// Quickly compare two version strings (with possible garbage) for = or >
// R Dowling 2/1/2008

// This version of ver converts version strings to single "ints" so is only
// suitable for those types of versions.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

help ()
{
	printf ("Usage\n");
	printf ("  ver VER1 -op VER2 [-v]\n");
	printf ("Exits with 0 for true, 1 for false, -1 for error.\n");
	printf ("Handles version numbers with or without dots.\n");
	printf ("-op is one of -eq, -gt\n");
	printf ("Optional -v at end will display test to stdout\n");
	printf ("  ver VER1\n");
	printf ("Extracts version from VER1 and prints to stdout\n");
	exit (1);
}

// Convert a version number string to a single integer
//	12.34.56 -> 56
//	"12.34" -> 34
//	19" -> 19
int parse (char *str)
{
	int len;
	char *t, *s;
	if (!str)
		exit (-1);
	len = strlen (str);
	if (len < 1)
		exit (-1);
	// Start at end: scan backwards for end of number
	for (t = str + len - 1; t >= str; t--)
		if (*t >= '0' && *t <= '9')
			break;
	// Move back to start of number
	for (s=t, t=t-1; t >= str; s=t--)
		if (!(*t >= '0' && *t <= '9'))
			break;
	// Failed?
	if (s < str)
	{
		printf ("Can't find a number in %s\n", str);
		exit (-1);
	}
	// printf ("str='%s'=%x t='%s'=%x=%d...\n", str, str, s, s, s-str);

	// Now s... is a number
	long l = strtol (s, NULL, 10);
	// printf ("return=%d\n", l);
	return l;
}

main (int c, char **v)
{
	int v1, v2;
	if (c<2)
		help ();
	v1 = parse (v[1]);
	if (c==2)
	{
		printf ("%d\n", v1);
		return 0;
	}
	if (c<4)
		help ();
	v2 = parse (v[3]);
	if (c==5)
		printf ("%d %s %d?  Check $?\n", v1, v[2], v2);
	if (v[2] && !strcmp (v[2], "-eq"))
		return ! (v1 == v2);
	else if (v[2] && !strcmp (v[2], "-gt"))
		return ! (v1 > v2);
	else
		help ();
	return 0;
}
