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
 * $Id: irc-mode.c,v 1.5 2002/05/21 00:10:16 alex Exp $
 *
 * irc-mode.c: IRC-Befehle zur Mode-Aenderung (MODE, AWAY, ...)
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <string.h>

#include "channel.h"
#include "defines.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"

#include "exp.h"
#include "irc-mode.h"


GLOBAL BOOLEAN IRC_MODE( CLIENT *Client, REQUEST *Req )
{
	CHAR *mode_ptr, the_modes[CLIENT_MODE_LEN], x[2];
	CLIENT *cl, *chan_cl, *prefix;
	BOOLEAN set, ok;
	CHANNEL *chan;
	
	assert( Client != NULL );
	assert( Req != NULL );

	cl = chan_cl = prefix = NULL;
	chan = NULL;

	/* Valider Client? */
	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Keine Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Ziel suchen: Client bzw. Channel */
	if( Client_IsValidNick( Req->argv[0] )) cl = Client_Search( Req->argv[0] );
	if( Channel_IsValidName( Req->argv[0] )) chan = Channel_Search( Req->argv[0] );

	/* Kein Ziel gefunden? */
	if(( ! cl ) && ( ! chan )) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[0] );

	assert(( cl && chan ) != TRUE );

	/* Falsche Anzahl Parameter? */
	if(( cl ) && ( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
	if(( chan ) && ( Req->argc > 3 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Client ermitteln, wenn bei Channel-Modes mit 3 Parametern */
	if(( chan ) && (Req->argc == 3 ))
	{
		chan_cl = Client_Search( Req->argv[2] );
		if( ! chan_cl ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[0] );
	}
	
	/* Wenn Anfragender ein User ist: Zugriff erlaubt? */
	if( Client_Type( Client ) == CLIENT_USER )
	{
		if( cl )
		{
			/* MODE ist nur fuer sich selber zulaessig! */
			if( cl != Client ) return IRC_WriteStrClient( Client, ERR_USERSDONTMATCH_MSG, Client_ID( Client ));
		}
		if( chan )
		{
			/* Darf der User die Channel-Modes ermitteln? */
		}
	}

	/* Werden die Modes "nur" erfragt? */
	if(( cl ) && ( Req->argc == 1 )) return IRC_WriteStrClient( Client, RPL_UMODEIS_MSG, Client_ID( Client ), Client_Modes( cl ));
	if(( chan ) && ( Req->argc == 1 )) return IRC_WriteStrClient( Client, RPL_CHANNELMODEIS_MSG, Client_ID( Client ), Channel_Name( chan ), Channel_Modes( chan ));

	mode_ptr = Req->argv[1];

	/* Sollen Modes gesetzt oder geloescht werden? */
	if( cl )
	{
		if( *mode_ptr == '+' ) set = TRUE;
		else if( *mode_ptr == '-' ) set = FALSE;
		else return IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Client ));
		mode_ptr++;
	}
	else
	{
		if( *mode_ptr == '-' ) set = FALSE;
		else set = TRUE;
		if(( *mode_ptr == '-' ) || ( *mode_ptr == '+' )) mode_ptr++;
	}
	
	/* Prefix fuer Antworten etc. ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER )
	{
		prefix = Client_Search( Req->prefix );
		if( ! prefix ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	}
	else prefix = Client;

	/* Reply-String mit Aenderungen vorbereiten */
	if( set ) strcpy( the_modes, "+" );
	else strcpy( the_modes, "-" );

	ok = TRUE;
	x[1] = '\0';
	while( *mode_ptr )
	{
		x[0] = '\0';
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Befehl kommt von einem Server, daher
			 * trauen wir ihm "unbesehen" ... */
			x[0] = *mode_ptr;
		}
		else
		{
			/* Modes validieren */
			if( cl )
			{
				/* User-Modes */
				switch( *mode_ptr )
				{
					case 'i':
						/* invisible */
						x[0] = 'i';
						break;
					case 'r':
						/* restricted (kann nur gesetzt werden) */
						if( set ) x[0] = 'r';
						else ok = IRC_WriteStrClient( Client, ERR_RESTRICTED_MSG, Client_ID( Client ));
						break;
					case 'o':
						/* operator (kann nur geloescht werden) */
						if( ! set )
						{
							Client_SetOperByMe( Client, FALSE );
							x[0] = 'o';
						}
						else ok = IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Client ));
						break;
					case 's':
						/* server messages */
						x[0] = 's';
						break;
					default:
						Log( LOG_DEBUG, "Unknown mode \"%c%c\" from \"%s\"!?", set ? '+' : '-', *mode_ptr, Client_ID( Client ));
						ok = IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG2_MSG, Client_ID( Client ), set ? '+' : '-', *mode_ptr );
						x[0] = '\0';
				}
			}
			if( chan )
			{
				/* Ist der User ein Channel Operator? */
				if( ! strchr( Channel_UserModes( chan, Client ), 'o' ))
				{
					Log( LOG_DEBUG, "Can't change modes: \"%s\" is not operator on %s!", Client_ID( Client ), Channel_Name( chan ));
					ok = IRC_WriteStrClient( Client, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Client ), Channel_Name( chan ));
					break;
				}
				
				/* Channel-Modes oder Channel-User-Modes */
				if( chan_cl )
				{
					/* Channel-User-Modes */
					switch( *mode_ptr )
					{
						case 'o':
							/* Channel Operator */
							x[0] = 'o';
							break;
						case 'v':
							/* Voice */
							x[0] = 'v';
							break;
						default:
							Log( LOG_DEBUG, "Unknown channel-user-mode \"%c%c\" from \"%s\" on \"%s\" at %s!?", set ? '+' : '-', *mode_ptr, Client_ID( Client ), Client_ID( chan_cl ), Channel_Name( chan ));
							ok = IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG2_MSG, Client_ID( Client ), set ? '+' : '-', *mode_ptr );
							x[0] = '\0';
					}
				}
				else
				{
					/* Channel-Modes */
					switch( *mode_ptr )
					{
						case 'a':
							/* Anonymous */
							x[0] = 'a';
							break;
						case 'm':
							/* Moderated */
							x[0] = 'm';
							break;
						case 'n':
							/* kein Schreiben in den Channel von aussen */
							x[0] = 'n';
							break;
						case 'p':
							/* Private */
							x[0] = 'p';
							break;
						case 'q':
							/* Quiet */
							x[0] = 'q';
							break;
						case 's':
							/* Secret */
							x[0] = 's';
							break;
						case 't':
							/* Topic Lock */
							x[0] = 't';
							break;
						case 'P':
							/* Persistent */
							x[0] = 'P';
							break;
						default:
							Log( LOG_DEBUG, "Unknown channel-mode \"%c%c\" from \"%s\" at %s!?", set ? '+' : '-', *mode_ptr, Client_ID( Client ), Channel_Name( chan ));
							ok = IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG2_MSG, Client_ID( Client ), set ? '+' : '-', *mode_ptr );
							x[0] = '\0';
					}
				}
			}
		}
		if( ! ok ) break;
		
		mode_ptr++;
		if( ! x[0] ) continue;

		/* Okay, gueltigen Mode gefunden */
		if( cl )
		{
			/* Es geht um User-Modes */
			if( set )
			{
				/* Mode setzen. Wenn der Client ihn noch nicht hatte: merken */
				if( Client_ModeAdd( cl, x[0] )) strcat( the_modes, x );
				
			}
			else
			{
				/* Modes geloescht. Wenn der Client ihn hatte: merken */
				if( Client_ModeDel( cl, x[0] )) strcat( the_modes, x );
			}

			/* "nachbearbeiten" */
			if( x[0] == 'a' )
			{
				/* away */
				if( set ) Client_SetAway( cl, DEFAULT_AWAY_MSG );
				else Client_SetAway( cl, NULL );
			}
		}
		if( chan )
		{
			/* Es geht um Channel-Modes oder Channel-User-Modes */
			if( chan_cl )
			{
				/* Channel-User-Modes */
				if( set )
				{
					/* Mode setzen. Wenn der Channel ihn noch nicht hatte: merken */
					if( Channel_UserModeAdd( chan, chan_cl, x[0] )) strcat( the_modes, x );
				}
				else
				{
					/* Mode setzen. Wenn der Channel ihn noch nicht hatte: merken */
					if( Channel_UserModeDel( chan, chan_cl, x[0] )) strcat( the_modes, x );
				}
			}
			else
			{
				/* Channel-Mode */
				if( set )
				{
					/* Mode setzen. Wenn der Channel ihn noch nicht hatte: merken */
					if( Channel_ModeAdd( chan, x[0] )) strcat( the_modes, x );
				}
				else
				{
					/* Mode setzen. Wenn der Channel ihn noch nicht hatte: merken */
					if( Channel_ModeDel( chan, x[0] )) strcat( the_modes, x );
				}
			}
		}
	}

	/* Wurden Modes geaendert? */
	if( the_modes[1] )
	{
		if( cl )
		{
			/* Client-Mode */
			if( Client_Type( Client ) == CLIENT_SERVER )
			{
				/* Modes an andere Server forwarden */
				IRC_WriteStrServersPrefix( Client, prefix, "MODE %s :%s", Client_ID( cl ), the_modes );
			}
			else
			{
				/* Bestaetigung an Client schicken & andere Server informieren */
				ok = IRC_WriteStrClientPrefix( Client, prefix, "MODE %s %s", Client_ID( cl ), the_modes );
				IRC_WriteStrServersPrefix( Client, prefix, "MODE %s :%s", Client_ID( cl ), the_modes );
			}
			Log( LOG_DEBUG, "User \"%s\": Mode change, now \"%s\".", Client_Mask( cl ), Client_Modes( cl ));
		}
		if( chan )
		{
			/* Channel-Modes oder Channel-User-Mode */
			if( chan_cl )
			{
				/* Channel-User-Mode */
				if( Client_Type( Client ) == CLIENT_SERVER )
				{
					/* Modes an andere Server und Channel-User forwarden */
					IRC_WriteStrServersPrefix( Client, prefix, "MODE %s %s :%s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
					IRC_WriteStrChannelPrefix( Client, chan, prefix, FALSE, "MODE %s %s %s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
				}
				else
				{
					/* Bestaetigung an Client schicken & andere Server sowie Channel-User informieren */
					ok = IRC_WriteStrClientPrefix( Client, prefix, "MODE %s %s %s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
					IRC_WriteStrServersPrefix( Client, prefix, "MODE %s %s :%s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
					IRC_WriteStrChannelPrefix( Client, chan, prefix, FALSE, "MODE %s %s %s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
				}
				Log( LOG_DEBUG, "User \"%s\" on %s: Mode change, now \"%s\".", Client_Mask( chan_cl), Channel_Name( chan ), Channel_UserModes( chan, chan_cl ));
			}
			else
			{
				/* Channel-Mode */
				if( Client_Type( Client ) == CLIENT_SERVER )
				{
					/* Modes an andere Server und Channel-User forwarden */
					IRC_WriteStrServersPrefix( Client, prefix, "MODE %s :%s", Channel_Name( chan ), the_modes );
					IRC_WriteStrChannelPrefix( Client, chan, prefix, FALSE, "MODE %s %s", Channel_Name( chan ), the_modes );
				}
				else
				{
					/* Bestaetigung an Client schicken & andere Server sowie Channel-User informieren */
					ok = IRC_WriteStrClientPrefix( Client, prefix, "MODE %s %s", Channel_Name( chan ), the_modes );
					IRC_WriteStrServersPrefix( Client, prefix, "MODE %s :%s", Channel_Name( chan ), the_modes );
					IRC_WriteStrChannelPrefix( Client, chan, prefix, FALSE, "MODE %s %s", Channel_Name( chan ), the_modes );
				}
				Log( LOG_DEBUG, "Channel \"%s\": Mode change, now \"%s\".", Channel_Name( chan ), Channel_Modes( chan ));
			}
		}
	}

	return ok;
} /* IRC_MODE */


GLOBAL BOOLEAN IRC_AWAY( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( Req->argc == 1 ) && (Req->argv[0][0] ))
	{
		/* AWAY setzen */
		Client_SetAway( Client, Req->argv[0] );
		IRC_WriteStrServersPrefix( Client, Client, "MODE %s :+a", Client_ID( Client ));
		return IRC_WriteStrClient( Client, RPL_NOWAWAY_MSG, Client_ID( Client ));
	}
	else
	{
		/* AWAY loeschen */
		Client_SetAway( Client, NULL );
		IRC_WriteStrServersPrefix( Client, Client, "MODE %s :-a", Client_ID( Client ));
		return IRC_WriteStrClient( Client, RPL_UNAWAY_MSG, Client_ID( Client ));
	}
} /* IRC_AWAY */


/* -eof- */
