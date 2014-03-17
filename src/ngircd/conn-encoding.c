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

#define __conn_encoding_c__

#define CONN_MODULE

#include "portab.h"

/**
 * @file
 * Functions to deal with character encodings and conversions
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "conn.h"

#ifdef ICONV

#include "log.h"
#include "conn-encoding.h"

char Encoding_Buffer[COMMAND_LEN];
char *Convert_Message PARAMS((iconv_t Handle, char *Message));

/**
 * Set client character encoding on a connection.
 *
 * @param Conn Connection identifier.
 * @param ClientEnc Client encoding (for example "ASCII", "MacRoman", ...).
 * @return true on success, false otherwise.
 */
GLOBAL bool
Conn_SetEncoding(CONN_ID Conn, const char *ClientEnc)
{
	char client_enc[25], server_enc[25];

	assert(Conn > NONE);
	assert(ClientEnc != NULL);

	Conn_UnsetEncoding(Conn);

	/* Is the client character set identical to server character set? */
	if (strcasecmp(ClientEnc, "UTF-8") == 0)
		return true;

	snprintf(client_enc, sizeof(client_enc), "%s//TRANSLIT", ClientEnc);
	snprintf(server_enc, sizeof(server_enc), "%s//TRANSLIT", "UTF-8");

	My_Connections[Conn].iconv_from = iconv_open(server_enc, client_enc);
	if (My_Connections[Conn].iconv_from == (iconv_t)(-1)) {
		Conn_UnsetEncoding(Conn);
		return false;
	}
	My_Connections[Conn].iconv_to = iconv_open(client_enc, server_enc);
	if (My_Connections[Conn].iconv_to == (iconv_t)(-1)) {
		Conn_UnsetEncoding(Conn);
		return false;
	}

	LogDebug("Set client character set of connection \"%d\" to \"%s\".",
		 Conn, client_enc);
	return true;
}

/**
 * Remove client character encoding conversion on a connection.
 *
 * @param Conn Connection identifier.
 */
GLOBAL void
Conn_UnsetEncoding(CONN_ID Conn)
{
	assert(Conn > NONE);

	if (My_Connections[Conn].iconv_from != (iconv_t)(-1))
		iconv_close(My_Connections[Conn].iconv_from);
	if (My_Connections[Conn].iconv_to != (iconv_t)(-1))
		iconv_close(My_Connections[Conn].iconv_to);

	My_Connections[Conn].iconv_from = (iconv_t)(-1);
	My_Connections[Conn].iconv_to = (iconv_t)(-1);

	LogDebug("Unset character conversion of connection %d.", Conn);
}

/**
 * Convert the encoding of a given message.
 *
 * This function uses a static buffer for the result of the encoding
 * conversion which is overwritten by subsequent calls to this function!
 *
 * @param Handle libiconv handle.
 * @param Message The message to convert.
 * @return Pointer to the result.
 */
char *
Convert_Message(iconv_t Handle, char *Message)
{
	size_t in_left, out_left;
	char *out = Encoding_Buffer;

	assert (Handle != (iconv_t)(-1));
	assert (Message != NULL);

	in_left = strlen(Message);
	out_left = sizeof(Encoding_Buffer) - 1;

	if (iconv(Handle, &Message, &in_left, &out, &out_left) == (size_t)(-1)) {
		/* An error occurred! */
		LogDebug("Error converting message encoding!");
		strlcpy(out, Message, sizeof(Encoding_Buffer));
		iconv(Handle, NULL, NULL, NULL, NULL);
	} else
		*out = '\0';

	return Encoding_Buffer;
}

#endif /* ICONV */

/**
 * Convert encoding of a message received from a connection.
 *
 * Note 1: If no conversion is required, this function returns the original
 * pointer to the message.
 *
 * Note 2: This function uses Convert_Message(), so subsequent calls to this
 * function will overwrite the earlier results.
 *
 * @param Conn Connection identifier.
 * @param Message The message to convert.
 * @return Pointer to the result.
 * @see Convert_Message
 */
GLOBAL char *
Conn_EncodingFrom(UNUSED CONN_ID Conn, char *Message)
{
	assert(Conn > NONE);
	assert (Message != NULL);

#ifdef ICONV
	if (My_Connections[Conn].iconv_from != (iconv_t)(-1))
		return Convert_Message(My_Connections[Conn].iconv_from, Message);
#endif
	return Message;
}

/**
 * Convert encoding of a message for sending on a connection.
 *
 * Note 1: If no conversion is required, this function returns the original
 * pointer to the message.
 *
 * Note 2: This function uses Convert_Message(), so subsequent calls to this
 * function will overwrite the earlier results.
 *
 * @param Conn Connection identifier.
 * @param Message The message to convert.
 * @return Pointer to the result.
 * @see Convert_Message
 */
GLOBAL char *
Conn_EncodingTo(UNUSED CONN_ID Conn, char *Message)
{
	assert(Conn > NONE);
	assert (Message != NULL);

#ifdef ICONV
	if (My_Connections[Conn].iconv_to != (iconv_t)(-1))
		return Convert_Message(My_Connections[Conn].iconv_to, Message);
#endif
	return Message;
}

/* -eof- */
