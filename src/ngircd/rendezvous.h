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
 * $Id: rendezvous.h,v 1.2 2005/03/19 18:43:49 fw Exp $
 *
 * "Rendezvous" functions (Header)
 */


#ifdef RENDEZVOUS

#ifndef __rdezvous_h__
#define __rdezvous_h__


GLOBAL void Rendezvous_Init( void );
GLOBAL void Rendezvous_Exit( void );

GLOBAL bool Rendezvous_Register( char *Name, char *Type, unsigned int Port );

GLOBAL bool Rendezvous_Unregister( char *Name );
GLOBAL void Rendezvous_UnregisterListeners( void );

GLOBAL void Rendezvous_Handler( void );


#endif	/* __rdezvous_h__ */

#endif	/* RENDEZVOUS */


/* -eof- */
