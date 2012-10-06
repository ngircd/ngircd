/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2012 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __conn_encoding_h__
#define __conn_encoding_h__

/**
 * @file
 * Functions to deal with character encodings and conversions (header)
 */

#ifdef ICONV

GLOBAL bool Conn_SetEncoding PARAMS((CONN_ID Idx, const char *ClientEnc));
GLOBAL void Conn_UnsetEncoding PARAMS((CONN_ID Idx));

#endif /* ICONV */

GLOBAL char* Conn_EncodingFrom PARAMS((CONN_ID Idx, char *Message));
GLOBAL char* Conn_EncodingTo PARAMS((CONN_ID Idx, char *Message));

#endif
