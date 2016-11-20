/*
   Enhanced NCSA Mosaic from Spyglass
   "Guitar"

   Copyright 1994 Spyglass, Inc.
   All Rights Reserved

   Author(s):
   Eric W. Sink eric@spyglass.com
   Jim Seidman	jim@spyglass.com
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
   compare two strings, ignoring case of alpha chars
 */
int GTR_strcmpi(const char *a, const char *b)
{
	int ca, cb;

	while (*a && *b)
	{
		if (isupper(*a))
			ca = tolower(*a);
		else
			ca = *a;

		if (isupper(*b))
			cb = tolower(*b);
		else
			cb = *b;

		if (ca > cb)
			return 1;
		if (cb > ca)
			return -1;
		a++;
		b++;
	}
	if (!*b && !*a)
		return 0;
	else if (!*b)
		return 1;
	else
		return -1;
}

/*
    compare two strings upto given length, ignoring case of alpha chars
*/
int GTR_strncmpi(const char *a, const char *b, int n)
{
 	int ca, cb;
 
 	while ((n>0) && *a && *b)
 	{
 		if (isupper(*a))
 			ca = tolower(*a);
 		else
 			ca = *a;
 
 		if (isupper(*b))
 			cb = tolower(*b);
 		else
 			cb = *b;
 
 		if (ca > cb)
 			return 1;
 		if (cb > ca)
 			return -1;
 		a++;
 		b++;
 		n--;
 	}
 	if (n<=0)
 		return 0;
 	else if (!*b && !*a)
 		return 0;
 	else if (!*b)
 		return 1;
 	else
 		return -1;
}
 
/* Make a copy of a string, allocating space for it */
char *GTR_strdup(const char *a)
{
	int nLen;
	char *p;

	nLen = strlen(a);
	p = malloc(nLen + 1);
	if (p)
	{
		strcpy(p, a);
	}
	return p;
}

/* Like GTR_strdup, but only copy n characters */
char *GTR_strndup(const char *a, int n)
{
	char *p;
	p = malloc(n + 1);
	if (p)
	{
		strncpy(p, a, n);
		p[n] = '\0';
	}
	return p;
}



/******************************************************************************/
/* 			    More String Utilities			      */
/******************************************************************************/
/* dpg */

/*
** return a pointer to the end of a string (i.e. at the 0 char)
*/
static char *
GTR_strend (char *S)
{
    return S + strlen (S);
}

/*
**  Mimic strncpy () except that end 0 char is always written
**   Thus MUST have room for n+1 chars.
**
**  -dpg
*/
char *
GTR_strncpy (char *T, const char *F, int n)
{
    register char *d = T;
 
    if (!T)
	return T;

    if (!F)		/* handled NULL F */
	*T = '\0';
    else
    {
	while (n-- && *F)
	    *d++ = *F++;
	*d = '\0';
    }
    return T;
}

/*
** A cross between strncpy() and strcat().  0 char is guaranteed to 
**  be placed, so make sure there is room for n + 1 bytes.
*/
char *
GTR_strncat (char *T, const char *F, int n)
{
    GTR_strncpy (GTR_strend (T), F, n);
    return T;
}
