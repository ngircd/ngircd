#include "portab.h"
#include <string.h>

#ifndef HAVE_STRTOK_R

char *
strtok_r(char *str, const char *delim, char **saveptr)
{
	char *tmp;

	if (!str)
		str = *saveptr;
	str += strspn(str, delim);
	if (*str == 0)
		return NULL;

	tmp = str + strcspn(str, delim); /* get end of token */
	if (*tmp) { /* another delimiter */
		*tmp = 0;
		tmp++;
	}
	*saveptr = tmp;
	return str;
}

#endif

