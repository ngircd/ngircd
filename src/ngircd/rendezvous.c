/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2003 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Rendezvous service registration (using Mach Ports, e.g. Mac OS X)
 */


#include "portab.h"

#ifdef RENDEZVOUS


static char UNUSED id[] = "$Id: rendezvous.c,v 1.1 2003/02/23 12:02:26 alex Exp $";

#include "imp.h"
#include <assert.h>

#include <stdio.h>
#include <string.h>

#ifdef HAVE_MACH_PORT_H
#include "mach/port.h"
#include "mach/message.h"
#endif

#ifdef HAVE_DNSSERVICEDISCOVERY_DNSSERVICEDISCOVERY_H
#include <DNSServiceDiscovery/DNSServiceDiscovery.h>
#endif

#include "defines.h"
#include "log.h"

#include "exp.h"
#include "rendezvous.h"


typedef struct _service
{
	dns_service_discovery_ref Discovery_Ref;
	mach_port_t Mach_Port;
	CHAR Desc[CLIENT_ID_LEN];
} SERVICE;


LOCAL VOID Registration_Reply_Handler( DNSServiceRegistrationReplyErrorType ErrCode, VOID *Context );
LOCAL VOID Unregister( INT Idx );


#define MAX_RENDEZVOUS 1000
#define MAX_MACH_MSG_SIZE 512


LOCAL SERVICE My_Rendezvous[MAX_RENDEZVOUS];


GLOBAL VOID Rendezvous_Init( VOID )
{
	/* Initialize structures */

	INT i;

	for( i = 0; i < MAX_RENDEZVOUS; i++ )
	{
		My_Rendezvous[i].Discovery_Ref = 0;
		My_Rendezvous[i].Mach_Port = 0;
	}
} /* Rendezvous_Init */


GLOBAL VOID Rendezvous_Exit( VOID )
{
	/* Clean up & exit module */

	INT i;

	for( i = 0; i < MAX_RENDEZVOUS; i++ )
	{
		if( My_Rendezvous[i].Discovery_Ref ) Unregister( i );
	}
} /* Rendezvous_Exit */


GLOBAL BOOLEAN Rendezvous_Register( CHAR *Name, CHAR *Type, UINT Port )
{
	/* Register new service */

	INT i;

	/* Search free port structure */
	for( i = 0; i < MAX_RENDEZVOUS; i++ ) if( ! My_Rendezvous[i].Discovery_Ref ) break;
	if( i >= MAX_RENDEZVOUS )
	{
		Log( LOG_ERR, "Can't register \"%s\" with Rendezvous: limit (%d) reached!", Name, MAX_RENDEZVOUS );
		return FALSE;
	}
	strlcpy( My_Rendezvous[i].Desc, Name, sizeof( My_Rendezvous[i].Desc ));
	
	/* Register new service */
	My_Rendezvous[i].Discovery_Ref = DNSServiceRegistrationCreate( Name, Type, "", htonl( Port ), "", Registration_Reply_Handler, My_Rendezvous[i].Desc );
	if( ! My_Rendezvous[i].Discovery_Ref )
	{
		Log( LOG_ERR, "Can't register \"%s\" with Rendezvous: can't register service!", My_Rendezvous[i].Desc );
		return FALSE;
	}
	
	/* Get and save the corresponding Mach Port */
	My_Rendezvous[i].Mach_Port = DNSServiceDiscoveryMachPort( My_Rendezvous[i].Discovery_Ref );
	if( ! My_Rendezvous[i].Mach_Port )
	{
		Log( LOG_ERR, "Can't register \"%s\" with Rendezvous: got no Mach Port!", My_Rendezvous[i].Desc );
		/* Here we actually leek a descriptor :-( */
		My_Rendezvous[i].Discovery_Ref = 0;
		return FALSE;
	}

	Log( LOG_DEBUG, "Rendezvous: Registering \"%s\" ...", My_Rendezvous[i].Desc );
	return TRUE;
} /* Rendezvous_Register */


GLOBAL BOOLEAN Rendezvous_Unregister( CHAR *Name )
{
	/* Unregister service from rendezvous */

	INT i;
	BOOLEAN ok;

	ok = FALSE;
	for( i = 0; i < MAX_RENDEZVOUS; i++ )
	{
		if( strcmp( Name, My_Rendezvous[i].Desc ) == 0 )
		{
			Unregister( i );
			ok = TRUE;
		}
	}

	return ok;
} /* Rendezvous_Unregister */


GLOBAL VOID Rendezvous_UnregisterListeners( VOID )
{
	/* Unregister all our listening sockets from Rendezvous */

	INT i;

	for( i = 0; i < MAX_RENDEZVOUS; i++ )
	{
		if( strchr( My_Rendezvous[i].Desc, '.' )) Unregister( i );
	}
} /* Rendezvous_UnregisterListeners */


GLOBAL VOID Rendezvous_Handler( VOID )
{
	/* Handle all Rendezvous stuff; this function must be called
	 * periodically from the run loop of the main program */

	INT i;
	CHAR buffer[MAX_MACH_MSG_SIZE];
	mach_msg_return_t result;
	mach_msg_header_t *msg;

	for( i = 0; i < MAX_RENDEZVOUS; i++ )
	{
		if( ! My_Rendezvous[i].Discovery_Ref ) continue;

		/* Read message from Mach Port */
		msg = (mach_msg_header_t *)buffer;
		result = mach_msg( msg, MACH_RCV_MSG|MACH_RCV_TIMEOUT, 0, MAX_MACH_MSG_SIZE, My_Rendezvous[i].Mach_Port, 1, 0 );

		/* Handle message */
		if( result == MACH_MSG_SUCCESS ) DNSServiceDiscovery_handleReply( msg );
#ifdef DEBUG
		else if( result != MACH_RCV_TIMED_OUT ) Log( LOG_DEBUG, "mach_msg(): %ld", (LONG)result );
#endif
	}
} /* Rendezvous_Handler */


LOCAL VOID Registration_Reply_Handler( DNSServiceRegistrationReplyErrorType ErrCode, VOID *Context )
{
	CHAR txt[50];

	if( ErrCode == kDNSServiceDiscoveryNoError )
	{
		/* Success! */
		Log( LOG_INFO, "Successfully registered \"%s\" with Rendezvous.", Context ? Context : "NULL" );
		return;
	}

	switch( ErrCode )
	{
		case kDNSServiceDiscoveryAlreadyRegistered:
			strcpy( txt, "name already registered!" );
			break;
		case kDNSServiceDiscoveryNameConflict:
			strcpy( txt, "name conflict!" );
			break;
		default:
			sprintf( txt, "error code %ld!", (LONG)ErrCode );
	}

	Log( LOG_INFO, "Can't register \"%s\" with Rendezvous: %s", Context ? Context : "NULL", txt );
} /* Registration_Reply_Handler */


LOCAL VOID Unregister( INT Idx )
{
	/* Unregister service */

	DNSServiceDiscoveryDeallocate( My_Rendezvous[Idx].Discovery_Ref );
	Log( LOG_INFO, "Unregistered \"%s\" from Rendezvous.", My_Rendezvous[Idx].Desc );
	My_Rendezvous[Idx].Discovery_Ref = 0;
} /* Unregister */


#endif	/* RENDEZVOUS */


/* -eof- */
