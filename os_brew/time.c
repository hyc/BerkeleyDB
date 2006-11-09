/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006
 *	Oracle Corp.  All rights reserved.
 *
 * $Id: time.c,v 1.6 2006/11/09 14:31:38 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * time --
 *
 * PUBLIC: #ifndef HAVE_TIME
 * PUBLIC: time_t time __P((time_t *));
 * PUBLIC: #endif
 */
time_t
time(timer)
	time_t *timer;
{
	time_t now;

	/*
	 * Berkeley DB uses POSIX time values internally; convert a BREW time
	 * value into a POSIX time value.
	 */
#ifdef HAVE_BREW_SDK2
	now = (time_t)GETTIMESECONDS() + BREW_EPOCH_OFFSET;
#else
	now = (time_t)GETUTCSECONDS() + BREW_EPOCH_OFFSET;
#endif

	if (timer != NULL)
		*timer = now;
	return (now);
}
