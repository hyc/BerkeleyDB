/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: os_seek.c,v 12.8 2006/06/08 13:33:59 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_seek --
 *	Seek to a page/byte offset in the file.
 *
 * PUBLIC: int __os_seek __P((DB_ENV *,
 * PUBLIC:      DB_FH *, db_pgno_t, u_int32_t, u_int32_t, int, DB_OS_SEEK));
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
	off_t offset;
	int ret, whence;

	/* Check for illegal usage. */
	DB_ASSERT(dbenv, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);

	switch (db_whence) {
	case DB_OS_SEEK_CUR:
		whence = SEEK_CUR;
		break;
	case DB_OS_SEEK_END:
		whence = SEEK_END;
		break;
	case DB_OS_SEEK_SET:
		whence = SEEK_SET;
		break;
	default:
		return (EINVAL);
	}

	offset = (off_t)pgsize * pgno + relative;
	if (isrewind)
		offset = -offset;

	if (DB_GLOBAL(j_seek) != NULL)
		ret = DB_GLOBAL(j_seek)(fhp->fd, offset, whence);
	else
		RETRY_CHK((lseek(fhp->fd, offset, whence) == -1 ? 1 : 0), ret);

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
