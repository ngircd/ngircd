/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2004 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Rendezvous service registration.
 *
 * Supported APIs are:
 *  - Apple Mac OS X
 *  - Howl
 */


#include "portab.h"

#ifdef RENDEZVOUS


static char UNUSED id[] = "$Id: rendezvous.c,v 1.4 2004/12/26 16:48:53 alex Exp $";

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

#ifdef HAVE_RENDEZVOUS_RENDEZVOUS_H
#include <rendezvous/rendezvous.h>
#endif

#include "defines.h"
#include "log.h"

#include "exp.h"
#include "rendezvous.h"


#if defined(HAVE_DNSSERVICEREGISTRATIONCREATE)
#	define APPLE
#elif defined(HAVE_SW_DISCOVERY_INIT)
#	define HOWL
#else
#	error "Can't detect Rendezvous API!?"
#endif


#define MAX_RENDEZVOUS 1000

typedef struct _service
{
	CHAR Desc[CLIENT_ID_LEN];
#ifdef APPLE
	dns_service_discovery_ref Discovery_Ref;
	mach_port_t Mach_Port;
#endif
#ifdef HOWL
	sw_discovery_oid Id;
#endif
} SERVICE;

LOCAL SERVICE My_Rendezvous[MAX_RENDEZVOUS];


LOCAL VOID Unregister( INT Idx );


/* -- Apple API -- */

#ifdef APPLE

#define MAX_MACH_MSG_SIZE 512

LOCAL VOID Registration_Reply_Handler( DNSServiceRegistrationReplyErrorType ErrCode, VOID *Context );

#endif /* Apple */


/* -- Howl API -- */

#ifdef HOWL

LOCAL sw_discovery My_Discovery_Session = NULL;
LOCAL sw_salt My_Salt;

LOCAL sw_result HOWL_API Registration_Reply_Handler( sw_discovery Session, sw_discovery_publish_status Status, sw_discovery_oid Id, sw_opaque Extra );

#endif /* Howl */


GLOBAL VOID Rendezvous_Init( VOID )
{
	/* Initialize structures */

	INT i;

#ifdef HOWL
	if( sw_discovery_init( &My_Discovery_Session ) != SW_OKAY )
	{
		Log( LOG_EMERG, "Can't initialize Rendezvous (Howl): sw_discovery_init() failed!" );
		Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME );
		exit( 1 );
	}

	if( sw_discovery_salt( My_Discovery_Session, &My_Salt ) != SW_OKAY )
	{
		Log( LOG_EMERG, "Can't initialize Rendezvous (Howl): sw_discovery_salt() failed!" );
		Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME );
		exit( 1 );
	}
#endif

	for( i = 0; i < MAX_RENDEZVOUS; i++ ) My_Rendezvous[i].Desc[0] = '\0';
} /* Rendezvous_Init */


GLOBAL VOID Rendezvous_Exit( VOID )
{
	/* Clean up & exit module */

	INT i;

	for( i = 0; i < MAX_RENDEZVOUS; i++ )
	{
		if( My_Rendezvous[i].Desc[0] ) Unregister( i );
	}

#ifdef HOWL
	sw_discovery_fina( My_Discovery_Session );
#endif
} /* Rendezvous_Exit */


GLOBAL BOOLEAN Rendezvous_Register( CHAR *Name, CHAR *Type, UINT Port )
{
	/* Register new service */

	INT i;

	/* Search free port structure */
	for( i = 0; i < MAX_RENDEZVOUS; i++ ) if( ! My_Rendezvous[i].Desc[0] ) break;
	if( i >= MAX_RENDEZVOUS )
	{
		Log( LOG_ERR, "Can't register \"%s\" with Rendezvous: limit (%d) reached!", Name, MAX_RENDEZVOUS );
		return FALSE;
	}
	strlcpy( My_Rendezvous[i].Desc, Name, sizeof( My_Rendezvous[i].Desc ));
	
#ifdef APPLE
	/* Register new service */
	My_Rendezvous[i].Discovery_Ref = DNSServiceRegistrationCreate( Name, Type, "", htonl( Port ), "", Registration_Reply_Handler, &My_Rendezvous[i] );
	if( ! My_Rendezvous[i].Discovery_Ref )
	{
		Log( LOG_ERR, "Can't register \"%s\" with Rendezvous: can't register service!", My_Rendezvous[i].Desc );
		My_Rendezvous[i].Desc[0] = '\0';
		return FALSE;
	}
	
	/* Get and save the corresponding Mach Port */
	My_Rendezvous[i].Mach_Port = DNSServiceDiscoveryMachPort( My_Rendezvous[i].Discovery_Ref );
	if( ! My_Rendezvous[i].Mach_Port )
	{
		Log( LOG_ERR, "Can't register \"%s\" with Rendezvous: got no Mach Port!", My_Rendezvous[i].Desc );
		/* Here we actually leek a descriptor :-( */
		My_Rendezvous[i].Discovery_Ref = 0;
		My_Rendezvous[i].Desc[0] = '\0';
		return FALSE;
	}
#endif /* Apple */

#ifdef HOWL
	if( sw_discovery_publish( My_Discovery_Session, 0, Name, Type, NULL, NULL, Port, NULL, 0, Registration_Reply_Handler, &My_Rendezvous[i], &My_Rendezvous[i].Id ) != SW_OKAY )
	{
		Log( LOG_ERR, "Can't register \"%s\" with Rendezvous: can't register service!", My_Rendezvous[i].Desc );
		My_Rendezvous[i].Desc[0] = '\0';
		return FALSE;
	}
#endif /* Howl */

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
		if( My_Rendezvous[i].Desc[0] ) Unregister( i );
	}
} /* Rendezvous_UnregisterListeners */


GLOBAL VOID Rendezvous_Handler( VOID )
{
	/* Handle all Rendezvous stuff; this function must be called
	 * periodically from the run loop of the main program */

#ifdef APPLE
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
#endif /* Debug */
	}
#endif /* Apple */

#ifdef HOWL
	sw_ulong msecs = 10;
	sw_salt_step( My_Salt, &msecs );
#endif
} /* Rendezvous_Handler */


LOCAL VOID Unregister( INT Idx )
{
	/* Unregister service */

#ifdef APPLE
	DNSServiceDiscoveryDeallocate( My_Rendezvous[Idx].Discovery_Ref );
#endif /* Apple */

#ifdef HOWL
	if( sw_discovery_cancel( My_Discovery_Session, My_Rendezvous[Idx].Id ) != SW_OKAY )
	{
		Log( LOG_ERR, "Rendezvous: Failed to unregister \"%s\"!", My_Rendezvous[Idx].Desc );
		return;
	}
#endif /* Howl */
	
	Log( LOG_INFO, "Unregistered \"%s\" from Rendezvous.", My_Rendezvous[Idx].Desc );
	My_Rendezvous[Idx].Desc[0] = '\0';
} /* Unregister */


/* -- Apple API -- */

#ifdef APPLE


LOCAL VOID Registration_Reply_Handler( DNSServiceRegistrationReplyErrorType ErrCode, VOID *Context )
{
	SERVICE *s = (SERVICE *)Context;
	CHAR txt[50];

	if( ErrCode == kDNSServiceDiscoveryNoError )
	{
		/* Success! */
		Log( LOG_INFO, "Successfully registered \"%s\" with Rendezvous.", s->Desc );
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

	Log( LOG_INFO, "Can't register \"%s\" with Rendezvous: %s", s->Desc, txt );
	s->Desc[0] = '\0';
} /* Registration_Reply_Handler */


#endif /* Apple */


/* -- Howl API -- */

#ifdef HOWL


LOCAL sw_result HOWL_API Registration_Reply_Handler( sw_discovery Session, sw_discovery_publish_status Status, UNUSED sw_discovery_oid Id, sw_opaque Extra )
{
	SERVICE *s = (SERVICE *)Extra;
	CHAR txt[50];

	assert( Session == My_Discovery_Session );
	assert( Extra != NULL );

	if( Status == SW_DISCOVERY_PUBLISH_STARTED || Status == SW_DISCOVERY_PUBLISH_STOPPED )
	{
		/* Success! */
		Log( LOG_INFO, "Successfully registered \"%s\" with Rendezvous.", s->Desc );
		return SW_OKAY;
	}
		
	switch( Status )
	{
		case SW_DISCOVERY_PUBLISH_NAME_COLLISION:
			strcpy( txt, "name conflict!" );
			break;
		default:
			sprintf( txt, "error code %ld!", (LONG)Status );
	}

	Log( LOG_INFO, "Can't register \"%s\" with Rendezvous: %s", s->Desc, txt );
	s->Desc[0] = '\0';

	return SW_OKAY;
} /* Registration_Reply_Handler */


#endif /* Howl */


#endif	/* RENDEZVOUS */


/* -eof- */
