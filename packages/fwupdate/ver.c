// Quickly compare two version strings (with possible garbage) for = or >
// R Dowling 2/1/2008

// This version of ver compares versions component by component

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_COMPONENTS	6

#define VERNUM(x)	strchr("0123456789.-",x)
#define DIGIT(x)	strchr("0123456789",x)
#define PUNCT(x)	strchr(".-",x)

#define NO	1
#define YES	0

// #define DEBUG

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

void show (int *v, int n)
{
	int i;
	printf ("%d:{", n);
	for (i=0; i<n; i++)
	{
		if (i>0)
			printf (",");
		printf ("%d", v[i]);
	}
	printf ("}");
}

// Convert a version number string to an array of integers
//	12.34.56 -> 56
//	"12.34" -> 34
//	19" -> 19
int parse (char *str, int *result)
{
	int len, i;
	char *t, *s, *e, *u;
	if (!str)
		exit (-1);
	len = strlen (str);
	if (len < 1)
		return 0;
	// Start at end: scan backwards for end of number
	e = str + len;
	for (t = e - 1; t >= str; t--)
		if (DIGIT(*t))
			break;
	// Failed?
	if (t < str)
	{
		printf ("Can't find a number in %s\n", str);
		exit (-1);
	}
	
	// Now scan back further for start of number, but allow punctuation.
	for (s=t, t=t-1; t >= str; s=t--)
		if (!VERNUM(*t))
			break;

#ifdef DEBUG
	printf ("So far, e=%s t=%s s=%s\n", e, t, s);
#endif

	// If first char is not a digit, move forward: ex: joe-1.2 would stop
	// on - between joe and 1.   Move forward over it
	for (;s<e && PUNCT(*s); s++)
		;

#ifdef DEBUG
	printf ("finally str='%s' s='%s'\n", str, s);
#endif

	// Now, pull them out, one by one
	for (u=s, i=0; i<MAX_COMPONENTS; i++)
	{
		char *v = u;
		long l = strtol (v, &u, 10);
		if (u==v)
		{
			// No more chars we can parse
			break;
		}
		// Accept number
		result[i] = l;
#ifdef DEBUG
		printf ("r[%d]=%d\n", i, result[i]);
#endif
		// Skip over punctuation
		for (u++; u<e && PUNCT(*u); u++)
			;
	}
	return i;
}

main (int c, char **v)
{
	int v1[MAX_COMPONENTS], v2[MAX_COMPONENTS];
	int n1, n2, i;
	if (c<2)
		help ();
	n1 = parse (v[1], &v1[0]);
	if (c==2)
	{
		show(v1, n1);
		printf ("\n");
		return YES;
	}
	if (c<4)
		help ();
	n2 = parse (v[3],  &v2[0]);
	if (c==5)
	{
		show(v1, n1); printf (" %s ", v[2]);
		show(v2, n2); printf ("?\n");
	}
	if (v[2] && !strcmp (v[2], "-eq"))
	{
		if (n1 != n2)
			return NO;
		for (i=0; i<n1; i++)
			if (v1[i] != v2[i])
				return NO;
		return YES;
	}
	else if (v[2] && !strcmp (v[2], "-gt"))
	{
		for (i=0; i<n1 && i<n2; i++)
		{
			if (v1[i] < v2[i])
				return NO;
			if (v1[i] > v2[i])
				return YES;
		}
		if (n1<=n2)
			return NO;
		return YES;
	}
	else
		help ();
	return YES;
}
