/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2006 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Connection compression using ZLIB
 */


#include "portab.h"

#define CONN_MODULE


#ifdef ZLIB

/* enable more zlib related debug messages: */
/* #define DEBUG_ZLIB */

static char UNUSED id[] = "$Id: conn-zip.c,v 1.14 2007/05/17 14:46:14 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <string.h>
#include <zlib.h>

#include "conn.h"
#include "conn-func.h"
#include "log.h"

#include "array.h"
#include "exp.h"
#include "conn-zip.h"


GLOBAL bool
Zip_InitConn( CONN_ID Idx )
{
	/* Kompression fuer Link initialisieren */

	assert( Idx > NONE );

	My_Connections[Idx].zip.in.avail_in = 0;
	My_Connections[Idx].zip.in.total_in = 0;
	My_Connections[Idx].zip.in.total_out = 0;
	My_Connections[Idx].zip.in.zalloc = NULL;
	My_Connections[Idx].zip.in.zfree = NULL;
	My_Connections[Idx].zip.in.data_type = Z_ASCII;

	if( inflateInit( &My_Connections[Idx].zip.in ) != Z_OK )
	{
		/* Fehler! */
		Log( LOG_ALERT, "Can't initialize compression on connection %d (zlib inflate)!", Idx );
		return false;
	}

	My_Connections[Idx].zip.out.total_in = 0;
	My_Connections[Idx].zip.out.total_in = 0;
	My_Connections[Idx].zip.out.zalloc = NULL;
	My_Connections[Idx].zip.out.zfree = NULL;
	My_Connections[Idx].zip.out.data_type = Z_ASCII;

	if( deflateInit( &My_Connections[Idx].zip.out, Z_DEFAULT_COMPRESSION ) != Z_OK )
	{
		/* Fehler! */
		Log( LOG_ALERT, "Can't initialize compression on connection %d (zlib deflate)!", Idx );
		return false;
	}

	My_Connections[Idx].zip.bytes_in = My_Connections[Idx].bytes_in;
	My_Connections[Idx].zip.bytes_out = My_Connections[Idx].bytes_out;

	Log( LOG_INFO, "Enabled link compression (zlib) on connection %d.", Idx );
	Conn_OPTION_ADD( &My_Connections[Idx], CONN_ZIP );

	return true;
} /* Zip_InitConn */


GLOBAL bool
Zip_Buffer( CONN_ID Idx, char *Data, size_t Len )
{
	size_t buflen;

	assert( Idx > NONE );
	assert( Data != NULL );
	assert( Len > 0 );

	buflen = array_bytes(&My_Connections[Idx].zip.wbuf);
	if (buflen >= WRITEBUFFER_LEN) {
		/* compression buffer is full, flush */
		if( ! Zip_Flush( Idx )) return false;
	}

	/* check again; if zip buf is still too large do not append data:
	 * otherwise the zip wbuf would grow too large */
	buflen = array_bytes(&My_Connections[Idx].zip.wbuf);
	if (buflen >= WRITEBUFFER_LEN)
		return false;
	return array_catb(&My_Connections[Idx].zip.wbuf, Data, Len);
} /* Zip_Buffer */


/**
 * Compress data in ZIP buffer and move result to the write buffer of
 * the connection.
 * @param Idx Connection handle.
 * @retrun true on success, false otherwise.
 */
GLOBAL bool
Zip_Flush( CONN_ID Idx )
{
	int result;
	unsigned char zipbuf[WRITEBUFFER_LEN];
	int zipbuf_used = 0;
	z_stream *out;

	out = &My_Connections[Idx].zip.out;

	out->avail_in = (uInt)array_bytes(&My_Connections[Idx].zip.wbuf);
	if (!out->avail_in)
		return true;	/* nothing to do. */

	out->next_in = array_start(&My_Connections[Idx].zip.wbuf);
	assert(out->next_in != NULL);

	out->next_out = zipbuf;
	out->avail_out = (uInt)sizeof zipbuf;

#ifdef DEBUG_ZIP
	Log(LOG_DEBUG, "out->avail_in %d, out->avail_out %d",
		out->avail_in, out->avail_out);
#endif
	result = deflate( out, Z_SYNC_FLUSH );
	if(( result != Z_OK ) || ( out->avail_in > 0 ))
	{
		Log( LOG_ALERT, "Compression error: code %d!?", result );
		Conn_Close( Idx, "Compression error!", NULL, false );
		return false;
	}

	if (out->avail_out <= 0) {
		/* Not all data was compressed, because data became
		 * bigger while compressing it. */
		Log (LOG_ALERT, "Compression error: buffer overvlow!?");
		Conn_Close(Idx, "Compression error!", NULL, false);
		return false;
	}

	assert(out->avail_out <= WRITEBUFFER_LEN);

	zipbuf_used = WRITEBUFFER_LEN - out->avail_out;
#ifdef DEBUG_ZIP
	Log(LOG_DEBUG, "zipbuf_used: %d", zipbuf_used);
#endif
	if (!array_catb(&My_Connections[Idx].wbuf,
			(char *)zipbuf, (size_t) zipbuf_used))
		return false;

	My_Connections[Idx].bytes_out += zipbuf_used;
	My_Connections[Idx].zip.bytes_out += array_bytes(&My_Connections[Idx].zip.wbuf); 
	array_trunc(&My_Connections[Idx].zip.wbuf);

	return true;
} /* Zip_Flush */


GLOBAL bool
Unzip_Buffer( CONN_ID Idx )
{
	/* Daten entpacken und in Lesepuffer kopieren. Bei Fehlern
	* wird false geliefert, ansonsten true. Der Fall, dass keine
	* Daten mehr zu entpacken sind, ist _kein_ Fehler! */

	int result;
	unsigned char unzipbuf[READBUFFER_LEN];
	int unzipbuf_used = 0;
	unsigned int z_rdatalen;
	unsigned int in_len;
	
	z_stream *in;

	assert( Idx > NONE );

	z_rdatalen = (unsigned int)array_bytes(&My_Connections[Idx].zip.rbuf);
	if (z_rdatalen == 0)
		return true;

	in = &My_Connections[Idx].zip.in;

	in->next_in = array_start(&My_Connections[Idx].zip.rbuf);
	assert(in->next_in != NULL);

	in->avail_in = z_rdatalen;
	in->next_out = unzipbuf;
	in->avail_out = (uInt)sizeof unzipbuf;

#ifdef DEBUG_ZIP
	Log(LOG_DEBUG, "in->avail_in %d, in->avail_out %d",
		in->avail_in, in->avail_out);
#endif
	result = inflate( in, Z_SYNC_FLUSH );
	if( result != Z_OK )
	{
		Log( LOG_ALERT, "Decompression error: %s (code=%d, ni=%d, ai=%d, no=%d, ao=%d)!?", in->msg, result, in->next_in, in->avail_in, in->next_out, in->avail_out );
		Conn_Close( Idx, "Decompression error!", NULL, false );
		return false;
	}

	assert(z_rdatalen >= in->avail_in);
	in_len = z_rdatalen - in->avail_in;
	unzipbuf_used = READBUFFER_LEN - in->avail_out;
#ifdef DEBUG_ZIP
	Log(LOG_DEBUG, "unzipbuf_used: %d - %d = %d", READBUFFER_LEN,
		in->avail_out, unzipbuf_used);
#endif
	assert(unzipbuf_used <= READBUFFER_LEN);
	if (!array_catb(&My_Connections[Idx].rbuf, (char*) unzipbuf,
			(size_t)unzipbuf_used))
		return false;

	if( in->avail_in > 0 ) {
		array_moveleft(&My_Connections[Idx].zip.rbuf, 1, in_len );
	} else {
		array_trunc( &My_Connections[Idx].zip.rbuf );
		My_Connections[Idx].zip.bytes_in += unzipbuf_used;
	}

	return true;
} /* Unzip_Buffer */


GLOBAL long
Zip_SendBytes( CONN_ID Idx )
{
	/* Anzahl gesendeter Bytes (komprimiert!) liefern */

	assert( Idx > NONE );
	return My_Connections[Idx].zip.bytes_out;
} /* Zip_SendBytes */


GLOBAL long
Zip_RecvBytes( CONN_ID Idx )
{
	/* Anzahl gesendeter Bytes (komprimiert!) liefern */

	assert( Idx > NONE );
	return My_Connections[Idx].zip.bytes_in;
} /* Zip_RecvBytes */


#endif


/* -eof- */
