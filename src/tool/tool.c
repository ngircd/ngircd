/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Tool functions
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>

#ifdef SYSLOG
#define SYSLOG_NAMES 1
#include <syslog.h>
#endif

#include "exp.h"
#include "tool.h"


/**
 * Removes all leading and trailing whitespaces of a string.
 * @param String The string to remove whitespaces from.
 */
GLOBAL void
ngt_TrimStr(char *String)
{
	char *start, *end;

	assert(String != NULL);

	start = String;

	/* Remove whitespaces at the beginning of the string ... */
	while (*start == ' ' || *start == '\t' ||
	       *start == '\n' || *start == '\r')
		start++;

	if (!*start) {
		*String = '\0';
		return;
	}

	/* ... and at the end: */
	end = strchr(start, '\0');
	end--;
	while ((*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')
	       && end >= start)
		end--;

	/* New trailing NULL byte */
	*(++end) = '\0';

	memmove(String, start, (size_t)(end - start)+1);
} /* ngt_TrimStr */


/**
 * Convert a string to uppercase letters.
 */
GLOBAL char *
ngt_UpperStr(char *String)
{
	char *ptr;

	assert(String != NULL);

	ptr = String;
	while(*ptr) {
		*ptr = toupper(*ptr);
		ptr++;
	}
	return String;
} /* ngt_UpperStr */


/**
 * Convert a string to lowercase letters.
 */
GLOBAL char *
ngt_LowerStr(char *String)
{
	char *ptr;

	assert(String != NULL);

	ptr = String;
	while(*ptr) {
		*ptr = tolower(*ptr);
		ptr++;
	}
	return String;
} /* ngt_LowerStr */


GLOBAL void
ngt_TrimLastChr( char *String, const char Chr)
{
	/* If last character in the string matches Chr, remove it.
	 * Empty strings are handled correctly. */

	size_t len;

	assert(String != NULL);

	len = strlen(String);
	if(len == 0)
		return;

	len--;

	if(String[len] == Chr)
		String[len] = '\0';
} /* ngt_TrimLastChr */


#ifdef SYSLOG


#ifndef INTERNAL_MARK

#ifndef _code
typedef struct _code {
        char    *c_name;
        int     c_val;
} CODE;
#endif

CODE facilitynames[] = {
#ifdef LOG_AUTH
	{ "auth",	LOG_AUTH },
#endif
#ifdef LOG_AUTHPRIV
	{ "authpriv",	LOG_AUTHPRIV },
#endif
#ifdef LOG_CRON
	{ "cron", 	LOG_CRON },
#endif
#ifdef LOG_DAEMON
	{ "daemon",	LOG_DAEMON },
#endif
#ifdef LOG_FTP
	{ "ftp",	LOG_FTP },
#endif
#ifdef LOG_LPR
	{ "lpr",	LOG_LPR },
#endif
#ifdef LOG_MAIL
	{ "mail",	LOG_MAIL },
#endif
#ifdef LOG_NEWS
	{ "news",	LOG_NEWS },
#endif
#ifdef LOG_UUCP
	{ "uucp",	LOG_UUCP },
#endif
#ifdef LOG_USER
	{ "user",	LOG_USER },
#endif
#ifdef LOG_LOCAL7
	{ "local0",	LOG_LOCAL0 },
	{ "local1",	LOG_LOCAL1 },
	{ "local2",	LOG_LOCAL2 },
	{ "local3",	LOG_LOCAL3 },
	{ "local4",	LOG_LOCAL4 },
	{ "local5",	LOG_LOCAL5 },
	{ "local6",	LOG_LOCAL6 },
	{ "local7",	LOG_LOCAL7 },
#endif
	{ 0,		-1 }
};

#endif


GLOBAL const char
*ngt_SyslogFacilityName(int Facility)
{
	int i = 0;
	while(facilitynames[i].c_name) {
		if (facilitynames[i].c_val == Facility)
			return facilitynames[i].c_name;
		i++;
	}
	return "unknown";
}


GLOBAL int
ngt_SyslogFacilityID(char *Name, int DefaultFacility)
{
	int i = 0;
	while(facilitynames[i].c_name) {
		if (strcasecmp(facilitynames[i].c_name, Name) == 0)
			return facilitynames[i].c_val;
		i++;
	}
	return DefaultFacility;
}


#endif


/* -eof- */
