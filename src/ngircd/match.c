/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: match.c,v 1.1 2002/06/26 15:42:58 alex Exp $
 *
 * match.c: Wildcard Pattern Matching
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <string.h>

#include "exp.h"
#include "match.h"


/*
 * Die Pattern-Matching-Funkionen [Matche(), Matche_After_Star()] basieren
 * auf Versionen von J. Kercheval. Die Version 1.1 wurde am 12.03.1991 als
 * "public domain" freigegeben:
 * <http://www.snippets.org/snippets/portable/MATCH+C.php3>
 */


LOCAL INT Matche PARAMS(( REGISTER CHAR *p, REGISTER CHAR *t ));
LOCAL INT Matche_After_Star PARAMS(( REGISTER CHAR *p, REGISTER CHAR *t ));


#define MATCH_PATTERN	6	/* bad pattern */
#define MATCH_LITERAL	5	/* match failure on literal match */
#define MATCH_RANGE	4	/* match failure on [..] construct */
#define MATCH_ABORT	3	/* premature end of text string */
#define MATCH_END	2	/* premature end of pattern string */
#define MATCH_VALID	1	/* valid match */


GLOBAL BOOLEAN
Match( CHAR *Pattern, CHAR *String )
{
	/* Pattern mit String vergleichen */
	if( Matche( Pattern, String ) == MATCH_VALID ) return TRUE;
	else return FALSE;
} /* Match */


LOCAL INT
Matche( REGISTER CHAR *p, REGISTER CHAR *t )
{
	REGISTER CHAR range_start, range_end;
	BOOLEAN invert;
	BOOLEAN member_match;
	BOOLEAN loop;

	for( ; *p; p++, t++ )
	{
		/* if this is the end of the text then this is the end of the match */
		if( ! *t )
		{
			return ( *p == '*' && *++p == '\0' ) ? MATCH_VALID : MATCH_ABORT;
		}

		/* determine and react to pattern type */
		switch( *p )
		{
			case '?':	/* single any character match */
				break;

			case '*':	/* multiple any character match */
				return Matche_After_Star( p, t );

			case '[':	/* [..] construct, single member/exclusion character match */
				/* move to beginning of range */
				p++;

				/* check if this is a member match or exclusion match */
				invert = FALSE;
				if( *p == '!' || *p == '^' )
				{
					invert = TRUE;
					p++;
				}

				/* if closing bracket here or at range start then we have a malformed pattern */
				if ( *p == ']' ) return MATCH_PATTERN;

				member_match = FALSE;
				loop = TRUE;

				while( loop )
				{
					/* if end of construct then loop is done */
					if( *p == ']' )
					{
						loop = FALSE;
						continue;
					}

					/* matching a '!', '^', '-', '\' or a ']' */
					if( *p == '\\' ) range_start = range_end = *++p;
					else  range_start = range_end = *p;

					/* if end of pattern then bad pattern (Missing ']') */
					if( ! *p ) return MATCH_PATTERN;

					/* check for range bar */
					if( *++p == '-' )
					{
						/* get the range end */
						range_end = *++p;

						/* if end of pattern or construct then bad pattern */
						if( range_end == '\0' || range_end == ']' ) return MATCH_PATTERN;

						/* special character range end */
						if( range_end == '\\' )
						{
							range_end = *++p;

							/* if end of text then we have a bad pattern */
							if ( ! range_end ) return MATCH_PATTERN;
						}

						/* move just beyond this range */
						p++;
					}

					/* if the text character is in range then match found. make sure the range
					 * letters have the proper relationship to one another before comparison */
					if( range_start < range_end )
					{
						if( *t >= range_start && *t <= range_end )
						{
							member_match = TRUE;
							loop = FALSE;
						}
					}
					else
					{
						if( *t >= range_end && *t <= range_start )
						{
							member_match = TRUE;
							loop = FALSE;
						}
					}
				}

				/* if there was a match in an exclusion set then no match */
				/* if there was no match in a member set then no match */
				if(( invert && member_match ) || ! ( invert || member_match )) return MATCH_RANGE;

				/* if this is not an exclusion then skip the rest of the [...]
				 * construct that already matched. */
				if( member_match )
				{
					while( *p != ']' )
					{
						/* bad pattern (Missing ']') */
						if( ! *p ) return MATCH_PATTERN;

						/* skip exact match */
						if( *p == '\\' )
						{
							p++;

							/* if end of text then we have a bad pattern */
							if( ! *p ) return MATCH_PATTERN;
						}

						/* move to next pattern char */
						p++;
					}
				}
				break;
			case '\\':	/* next character is quoted and must match exactly */
				/* move pattern pointer to quoted char and fall through */
				p++;

				/* if end of text then we have a bad pattern */
				if( ! *p ) return MATCH_PATTERN;

				/* must match this character exactly */
			default:
				if( *p != *t ) return MATCH_LITERAL;
		}
	}
	/* if end of text not reached then the pattern fails */

	if( *t ) return MATCH_END;
	else return MATCH_VALID;
} /* Matche */


LOCAL INT
Matche_After_Star( REGISTER CHAR *p, REGISTER CHAR *t )
{
	REGISTER INT nextp, match = 0;

	/* pass over existing ? and * in pattern */
	while( *p == '?' || *p == '*' )
	{
		/* take one char for each ? and + */
		if (*p == '?')
		{
			/* if end of text then no match */
			if( ! *t++ ) return MATCH_ABORT;
		}

		/* move to next char in pattern */
		p++;
	}

	/* if end of pattern we have matched regardless of text left */
	if( ! *p ) return MATCH_VALID;

	/* get the next character to match which must be a literal or '[' */
	nextp = *p;
	if( nextp == '\\' )
	{
		nextp = p[1];

		/* if end of text then we have a bad pattern */
		if( ! nextp ) return MATCH_PATTERN;
	}

	/* Continue until we run out of text or definite result seen */
	do
	{
		/* a precondition for matching is that the next character
		 * in the pattern match the next character in the text or that
		 * the next pattern char is the beginning of a range.  Increment
		 * text pointer as we go here */
		if( nextp == *t || nextp == '[' ) match = Matche( p, t );

		/* if the end of text is reached then no match */
		if( ! *t++ ) match = MATCH_ABORT;
	} while( match != MATCH_VALID && match != MATCH_ABORT && match != MATCH_PATTERN );

	/* return result */
	return match;
} /* Matche_After_Star */


/* -eof- */
