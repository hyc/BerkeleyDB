/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2005 Oracle.  All rights reserved.
 *
 * $Id: os_sleep.c,v 1.4 2006/11/09 14:31:01 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_sleep --
 *	Yield the processor for a period of time.
 */
void
__os_sleep(dbenv, secs, usecs)
	DB_ENV *dbenv;
	u_long secs, usecs;		/* Seconds and microseconds. */
{
	COMPQUIET(dbenv, NULL);

	/* Don't require that the values be normalized. */
	for (; usecs >= 1000000; usecs -= 1000000)
		++secs;

#ifndef HAVE_BREW_SDK2
	MSLEEP(secs * 1000 + usecs / 1000);
#endif
}
