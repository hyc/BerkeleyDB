/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: abort.c,v 1.1 2006/07/12 14:23:56 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * abort --
 *
 * PUBLIC: #ifndef HAVE_ABORT
 * PUBLIC: void abort __P((void));
 * PUBLIC: #endif
 */
void
abort()
{
	exit(1);
	/* NOTREACHED */
}
