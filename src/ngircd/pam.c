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

#include "portab.h"

#ifdef PAM

/**
 * @file
 * PAM User Authentication
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SECURITY_PAM_APPL_H
# include <security/pam_appl.h>
#endif
#ifdef HAVE_PAM_PAM_APPL_H
# include <pam/pam_appl.h>
#endif

#include "defines.h"
#include "log.h"
#include "conn.h"
#include "client.h"
#include "conf.h"

#include "pam.h"

static char *password;

/**
 * PAM "conversation function".
 * This is a callback function used by the PAM library to get the password.
 * Please see the PAM documentation for details :-)
 */
static int
password_conversation(int num_msg, const struct pam_message **msg,
		      struct pam_response **resp, void *appdata_ptr) {
	LogDebug("PAM: conv(%d, %d, '%s', '%s')",
		 num_msg, msg[0]->msg_style, msg[0]->msg, appdata_ptr);

	/* Can we deal with this request? */
	if (num_msg != 1 || msg[0]->msg_style != PAM_PROMPT_ECHO_OFF) {
		Log(LOG_ERR, "PAM: Unexpected PAM conversation '%d:%s'!",
		    msg[0]->msg_style, msg[0]->msg);
		return PAM_CONV_ERR;
	}

	if (!appdata_ptr) {
		/* Sometimes appdata_ptr gets lost!? */
		appdata_ptr = password;
	}

	/* Duplicate password ("application data") for the PAM library */
	*resp = calloc(num_msg, sizeof(struct pam_response));
	if (!*resp) {
		Log(LOG_ERR, "PAM: Out of memory!");
		return PAM_CONV_ERR;
	}

	(*resp)[0].resp = strdup((char *)appdata_ptr);
	(*resp)[0].resp_retcode = 0;

	return ((*resp)[0].resp ? PAM_SUCCESS : PAM_CONV_ERR);
}

/**
 * PAM "conversation" structure.
 */
static struct pam_conv conv = {
	&password_conversation,
	NULL
};

/**
 * Authenticate a connecting client using PAM.
 * @param Client The client to authenticate.
 * @return true when authentication succeeded, false otherwise.
 */
GLOBAL bool
PAM_Authenticate(CLIENT *Client) {
	pam_handle_t *pam;
	int retval = PAM_SUCCESS;

	LogDebug("PAM: Authenticate \"%s\" (%s) ...",
		 Client_OrigUser(Client), Client_Mask(Client));

	/* Set supplied client password */
	if (password)
		free(password);
	password = strdup(Conn_Password(Client_Conn(Client)));
	conv.appdata_ptr = Conn_Password(Client_Conn(Client));

	/* Initialize PAM */
	retval = pam_start(Conf_PAMServiceName, Client_OrigUser(Client), &conv, &pam);
	if (retval != PAM_SUCCESS) {
		Log(LOG_ERR, "PAM: Failed to create authenticator! (%d)", retval);
		return false;
	}

	pam_set_item(pam, PAM_RUSER, Client_User(Client));
	pam_set_item(pam, PAM_RHOST, Client_Hostname(Client));
#if defined(HAVE_PAM_FAIL_DELAY) && !defined(NO_PAM_FAIL_DELAY)
	pam_fail_delay(pam, 0);
#endif

	/* PAM authentication ... */
	retval = pam_authenticate(pam, 0);

	/* Success? */
	if (retval == PAM_SUCCESS)
		Log(LOG_INFO, "PAM: Authenticated \"%s\" (%s).",
		    Client_OrigUser(Client), Client_Mask(Client));
	else
		Log(LOG_ERR, "PAM: Error on \"%s\" (%s): %s",
		    Client_OrigUser(Client), Client_Mask(Client),
		    pam_strerror(pam, retval));

	/* Free PAM structures */
	if (pam_end(pam, retval) != PAM_SUCCESS)
		Log(LOG_ERR, "PAM: Failed to release authenticator!");

	return (retval == PAM_SUCCESS);
}

#endif /* PAM */

/* -eof- */
