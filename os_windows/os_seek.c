/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: os_seek.c,v 12.6 2006/06/08 14:02:19 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_seek --
 *	Seek to a page/byte offset in the file.
 */
int
__os_seek(dbenv, fhp, pgno, pgsize, relative, isrewind, db_whence)
	DB_ENV *dbenv;
	DB_FH *fhp;
	db_pgno_t pgno;
	u_int32_t pgsize;
	u_int32_t relative;
	int isrewind;
	DB_OS_SEEK db_whence;
{
	/* Yes, this really is how Microsoft designed their API. */
	union {
		__int64 bigint;
		struct {
			unsigned long low;
			long high;
		};
	} offbytes;
	off_t offset;
	int ret;
	DWORD from;

	offset = (off_t)pgsize * pgno + relative;
	if (isrewind)
		offset = -offset;

	switch (db_whence) {
	case DB_OS_SEEK_CUR:
		from = FILE_CURRENT;
		break;
	case DB_OS_SEEK_END:
		from = FILE_END;
		break;
	case DB_OS_SEEK_SET:
		from = FILE_BEGIN;
		break;
	default:
		return (EINVAL);
	}

	offbytes.bigint = offset;
	ret = (SetFilePointer(fhp->handle, offbytes.low,
	    &offbytes.high, from) == (DWORD) - 1) ? __os_get_syserr() : 0;

	if (ret == 0) {
		fhp->pgsize = pgsize;
		fhp->pgno = pgno;
		fhp->offset = relative;
	} else {
		__db_syserr(dbenv, ret, "seek: %lu %d %d",
		    (u_long)pgsize * pgno + relative, isrewind, db_whence);
		ret = __os_posix_err(ret);
	}

	return (ret);
}
