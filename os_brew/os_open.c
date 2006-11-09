/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2005 Oracle.  All rights reserved.
 *
 * $Id: os_open.c,v 1.3 2006/11/01 00:53:41 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_open --
 *	Open a file descriptor.
 */
int
__os_open(dbenv, name, flags, mode, fhpp)
	DB_ENV *dbenv;
	const char *name;
	u_int32_t flags;
	int mode;
	DB_FH **fhpp;
{
	return (__os_open_extend(dbenv, name, 0, flags, mode, fhpp));
}

/*
 * __os_open_extend --
 *	Open a file descriptor (including page size and log size information).
 */
int
__os_open_extend(dbenv, name, page_size, flags, mode, fhpp)
	DB_ENV *dbenv;
	const char *name;
	u_int32_t page_size, flags;
	int mode;
	DB_FH **fhpp;
{
	OpenFileMode oflags;
	int ret;

	COMPQUIET(page_size, 0);

#define	OKFLAGS								\
	(DB_OSO_ABSMODE | DB_OSO_CREATE | DB_OSO_DIRECT | DB_OSO_DSYNC |\
	DB_OSO_EXCL | DB_OSO_RDONLY | DB_OSO_REGION |	DB_OSO_SEQ |	\
	DB_OSO_TEMP | DB_OSO_TRUNC)
	if ((ret = __db_fchk(dbenv, "__os_open", flags, OKFLAGS)) != 0)
		return (ret);

	oflags = 0;
	if (LF_ISSET(DB_OSO_CREATE))
		oflags |= _OFM_CREATE;

	if (LF_ISSET(DB_OSO_RDONLY))
		oflags |= _OFM_READ;
	else
		oflags |= _OFM_READWRITE;

	return (__os_openhandle(dbenv, name, oflags, mode, fhpp));
}
