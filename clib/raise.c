/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2006 Oracle.  All rights reserved.
 *
 * $Id: raise.c,v 12.6 2006/11/01 00:52:17 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * raise --
 *	Send a signal to the current process.
 *
 * PUBLIC: #ifndef HAVE_RAISE
 * PUBLIC: int raise __P((int));
 * PUBLIC: #endif
 */
int
raise(s)
	int s;
{
	return (kill(getpid(), s));
}
