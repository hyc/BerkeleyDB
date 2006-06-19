/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: os_spin.c,v 12.5 2006/05/19 19:25:19 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_spin --
 *	Return the number of default spins before blocking.
 */
u_int32_t
__os_spin(dbenv)
	DB_ENV *dbenv;
{
	SYSTEM_INFO SystemInfo;
	u_int32_t tas_spins;

	/* Get the number of processors */
	GetSystemInfo(&SystemInfo);

	/*
	 * Spin 50 times per processor -- we have anecdotal evidence that this
	 * is a reasonable value.
	 */
	if (SystemInfo.dwNumberOfProcessors > 1)
		 tas_spins = 50 * SystemInfo.dwNumberOfProcessors;
	else
		 tas_spins = 1;

	return (tas_spins);
}
