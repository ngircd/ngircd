/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#define CONN_MODULE

#include "portab.h"

/**
 * @file
 * Connection compression using ZLIB
 */

/* Additionan debug messages related to ZIP compression: 0=off / 1=on */
#define DEBUG_ZIP 0

#ifdef ZLIB

#include <assert.h>
#include <string.h>
#include <zlib.h>

#include "conn.h"
#include "conn-func.h"
#include "log.h"
#include "array.h"

#include "conn-zip.h"


GLOBAL bool
Zip_InitConn( CONN_ID Idx )
{
	/* initialize zlib compression on this link */

	assert( Idx > NONE );

	My_Connections[Idx].zip.in.avail_in = 0;
	My_Connections[Idx].zip.in.total_in = 0;
	My_Connections[Idx].zip.in.total_out = 0;
	My_Connections[Idx].zip.in.zalloc = NULL;
	My_Connections[Idx].zip.in.zfree = NULL;
	My_Connections[Idx].zip.in.data_type = Z_ASCII;

	if (inflateInit( &My_Connections[Idx].zip.in ) != Z_OK) {
		Log(LOG_ALERT, "Can't initialize compression on connection %d (zlib inflate)!", Idx);
		return false;
	}

	My_Connections[Idx].zip.out.total_in = 0;
	My_Connections[Idx].zip.out.total_in = 0;
	My_Connections[Idx].zip.out.zalloc = NULL;
	My_Connections[Idx].zip.out.zfree = NULL;
	My_Connections[Idx].zip.out.data_type = Z_ASCII;

	if (deflateInit( &My_Connections[Idx].zip.out, Z_DEFAULT_COMPRESSION ) != Z_OK) {
		Log(LOG_ALERT, "Can't initialize compression on connection %d (zlib deflate)!", Idx);
		return false;
	}

	My_Connections[Idx].zip.bytes_in = My_Connections[Idx].bytes_in;
	My_Connections[Idx].zip.bytes_out = My_Connections[Idx].bytes_out;

	Log(LOG_INFO, "Enabled link compression (zlib) on connection %d.", Idx);
	Conn_OPTION_ADD( &My_Connections[Idx], CONN_ZIP );

	return true;
} /* Zip_InitConn */


/**
 * Copy data to the compression buffer of a connection. We do collect
 * some data there until it's full so that we can achieve better
 * compression ratios.
 * If the (pre-)compression buffer is full, we try to flush it ("actually
 * compress some data") and to add the new (uncompressed) data afterwards.
 * This function closes the connection on error.
 * @param Idx Connection handle.
 * @param Data Pointer to the data.
 * @param Len Length of the data to add.
 * @return true on success, false otherwise.
 */
GLOBAL bool
Zip_Buffer( CONN_ID Idx, const char *Data, size_t Len )
{
	size_t buflen;

	assert( Idx > NONE );
	assert( Data != NULL );
	assert( Len > 0 );

	buflen = array_bytes(&My_Connections[Idx].zip.wbuf);
	if (buflen + Len >= WRITEBUFFER_SLINK_LEN) {
		/* compression buffer is full, flush */
		if( ! Zip_Flush( Idx )) return false;
	}

	/* check again; if zip buf is still too large do not append data:
	 * otherwise the zip wbuf would grow too large */
	buflen = array_bytes(&My_Connections[Idx].zip.wbuf);
	if (buflen + Len >= WRITEBUFFER_SLINK_LEN) {
		Log(LOG_ALERT, "Zip Write buffer space exhausted: %lu bytes", buflen + Len);
		Conn_Close(Idx, "Zip Write buffer space exhausted", NULL, false);
		return false;
	}
	return array_catb(&My_Connections[Idx].zip.wbuf, Data, Len);
} /* Zip_Buffer */


/**
 * Compress data in ZIP buffer and move result to the write buffer of
 * the connection.
 * This function closes the connection on error.
 * @param Idx Connection handle.
 * @return true on success, false otherwise.
 */
GLOBAL bool
Zip_Flush( CONN_ID Idx )
{
	int result;
	unsigned char zipbuf[WRITEBUFFER_SLINK_LEN];
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

#if DEBUG_ZIP
	LogDebug("out->avail_in %d, out->avail_out %d",
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
		Log(LOG_ALERT, "Compression error: buffer overflow!?");
		Conn_Close(Idx, "Compression error!", NULL, false);
		return false;
	}

	assert(out->avail_out <= WRITEBUFFER_SLINK_LEN);

	zipbuf_used = WRITEBUFFER_SLINK_LEN - out->avail_out;
#if DEBUG_ZIP
	LogDebug("zipbuf_used: %d", zipbuf_used);
#endif
	if (!array_catb(&My_Connections[Idx].wbuf,
			(char *)zipbuf, (size_t) zipbuf_used)) {
		Log (LOG_ALERT, "Compression error: can't copy data!?");
		Conn_Close(Idx, "Compression error!", NULL, false);
		return false;
	}

	My_Connections[Idx].bytes_out += zipbuf_used;
	My_Connections[Idx].zip.bytes_out += array_bytes(&My_Connections[Idx].zip.wbuf);
	array_trunc(&My_Connections[Idx].zip.wbuf);

	return true;
} /* Zip_Flush */


/**
 * uncompress data and copy it to read buffer.
 * Returns true if data has been unpacked or no
 * compressed data is currently pending in the zread buffer.
 * This function closes the connection on error.
 * @param Idx Connection handle.
 * @return true on success, false otherwise.
 */
GLOBAL bool
Unzip_Buffer( CONN_ID Idx )
{
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

#if DEBUG_ZIP
	LogDebug("in->avail_in %d, in->avail_out %d",
		in->avail_in, in->avail_out);
#endif
	result = inflate( in, Z_SYNC_FLUSH );
	if( result != Z_OK )
	{
		Log(LOG_ALERT, "Decompression error: %s (code=%d, ni=%d, ai=%d, no=%d, ao=%d)!?", in->msg, result, in->next_in, in->avail_in, in->next_out, in->avail_out);
		Conn_Close(Idx, "Decompression error!", NULL, false);
		return false;
	}

	assert(z_rdatalen >= in->avail_in);
	in_len = z_rdatalen - in->avail_in;
	unzipbuf_used = READBUFFER_LEN - in->avail_out;
#if DEBUG_ZIP
	LogDebug("unzipbuf_used: %d - %d = %d", READBUFFER_LEN,
		in->avail_out, unzipbuf_used);
#endif
	assert(unzipbuf_used <= READBUFFER_LEN);
	if (!array_catb(&My_Connections[Idx].rbuf, (char*) unzipbuf,
			(size_t)unzipbuf_used)) {
		Log (LOG_ALERT, "Decompression error: can't copy data!?");
		Conn_Close(Idx, "Decompression error!", NULL, false);
		return false;
	}
	if( in->avail_in > 0 ) {
		array_moveleft(&My_Connections[Idx].zip.rbuf, 1, in_len );
	} else {
		array_trunc( &My_Connections[Idx].zip.rbuf );
		My_Connections[Idx].zip.bytes_in += unzipbuf_used;
	}

	return true;
} /* Unzip_Buffer */


/**
 * @param Idx Connection handle.
 * @return amount of sent (compressed) bytes
 */
GLOBAL long
Zip_SendBytes( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].zip.bytes_out;
} /* Zip_SendBytes */


/**
 * @param Idx Connection handle.
 * @return amount of received (compressed) bytes
 */
GLOBAL long
Zip_RecvBytes( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].zip.bytes_in;
} /* Zip_RecvBytes */


#endif


/* -eof- */
