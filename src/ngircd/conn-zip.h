/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: conn-zip.h,v 1.1 2002/12/30 16:07:23 alex Exp $
 *
 * Connection compression using ZLIB (header)
 */


#ifdef USE_ZLIB

#ifndef __conn_zip_h__
#define __conn_zip_h__


GLOBAL BOOLEAN Zip_InitConn PARAMS(( CONN_ID Idx ));

GLOBAL BOOLEAN Zip_Buffer PARAMS(( CONN_ID Idx, CHAR *Data, INT Len ));
GLOBAL BOOLEAN Zip_Flush PARAMS(( CONN_ID Idx ));
GLOBAL BOOLEAN Unzip_Buffer PARAMS(( CONN_ID Idx ));

GLOBAL LONG Zip_SendBytes PARAMS(( CONN_ID Idx ));
GLOBAL LONG Zip_RecvBytes PARAMS(( CONN_ID Idx ));


#endif /* __conn_zip_h__ */

#endif /* USE_ZLIB */


/* -eof- */
