/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005,2006 Oracle.  All rights reserved.
 *
 * $Id: isalpha.c,v 1.3 2006/11/01 00:52:17 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * isalpha --
 *
 * PUBLIC: #ifndef HAVE_ISALPHA
 * PUBLIC: int isalpha __P((int));
 * PUBLIC: #endif
 */
int
isalpha(c)
	int c;
{
	/*
	 * Depends on ASCII-like character ordering.
	 */
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ? 1 : 0);
}
