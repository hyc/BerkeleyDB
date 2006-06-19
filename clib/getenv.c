/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: getenv.c,v 1.1 2006/05/12 16:19:25 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * getenv --
 *
 * PUBLIC: #ifndef HAVE_GETENV
 * PUBLIC: char *getenv __P((const char *));
 * PUBLIC: #endif
 */
char *
getenv(name)
	const char *name;
{
	COMPQUIET(name, NULL);

	/*
	 * If we don't have getenv(3), ignore the environment.
	 */
	return (NULL);
}
