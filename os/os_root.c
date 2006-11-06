/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999,2006 Oracle.  All rights reserved.
 *
 * $Id: os_root.c,v 12.6 2006/11/01 00:53:40 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_isroot --
 *	Return if user has special permissions.
 *
 * PUBLIC: int __os_isroot __P((void));
 */
int
__os_isroot()
{
#ifdef HAVE_GETUID
	return (getuid() == 0);
#else
	return (0);
#endif
}
