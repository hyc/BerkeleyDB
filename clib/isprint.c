/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005,2006 Oracle.  All rights reserved.
 *
 * $Id: isprint.c,v 1.3 2006/11/01 00:52:17 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * isprint --
 *
 * PUBLIC: #ifndef HAVE_ISPRINT
 * PUBLIC: int isprint __P((int));
 * PUBLIC: #endif
 */
int
isprint(c)
	int c;
{
	/*
	 * Depends on ASCII character values.
	 */
	return ((c >= ' ' && c <= '~') ? 1 : 0);
}
