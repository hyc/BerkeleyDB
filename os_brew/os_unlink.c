/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2005 Oracle.  All rights reserved.
 *
 * $Id: os_unlink.c,v 1.3 2006/11/01 00:53:41 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_unlink --
 *	Remove a file.
 */
int
__os_unlink(dbenv, path)
	DB_ENV *dbenv;
	const char *path;
{
	IFileMgr *ifmp;
	int ret;

	FILE_MANAGER_CREATE(dbenv, ifmp, ret);
	if (ret != 0)
		return (ret);

	if (IFILEMGR_Remove(ifmp, path) == EFAILED)
		FILE_MANAGER_ERR(dbenv, ifmp, path, "IFILEMGR_Remove", ret);

	IFILEMGR_Release(ifmp);

	return (ret);
}
