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
 * $Id: rendezvous.h,v 1.1 2003/02/23 12:02:26 alex Exp $
 *
 * "Rendezvous" functions (Header)
 */


#ifdef RENDEZVOUS

#ifndef __rdezvous_h__
#define __rdezvous_h__


GLOBAL VOID Rendezvous_Init( VOID );
GLOBAL VOID Rendezvous_Exit( VOID );

GLOBAL BOOLEAN Rendezvous_Register( CHAR *Name, CHAR *Type, UINT Port );

GLOBAL BOOLEAN Rendezvous_Unregister( CHAR *Name );
GLOBAL VOID Rendezvous_UnregisterListeners( VOID );

GLOBAL VOID Rendezvous_Handler( VOID );


#endif	/* __rdezvous_h__ */

#endif	/* RENDEZVOUS */


/* -eof- */
