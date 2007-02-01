/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006
 *	Oracle Corp.  All rights reserved.
 *
 * $Id: ce_getcwd.c,v 12.1 2007/01/22 06:12:19 alexg Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * getcwd --
 */
char *
getcwd(buf, size)
	char *buf;
	size_t size;
{
	/*
	 * Taken from msdn:
	 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/cpref/html/frlrfsystemiodirectoryclassgetcurrentdirectorytopic.asp
	 * The .NET Compact Framework does not support GetCurrentDirectory
	 * because current directory functionality is not used in devices
	 * running WinCE .NET.
	 */
	__os_set_errno(ENOENT);
	return (NULL);
}
