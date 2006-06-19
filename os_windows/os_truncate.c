/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: os_truncate.c,v 12.7 2006/06/08 13:34:03 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_truncate --
 *	Truncate the file.
 */
int
__os_truncate(dbenv, fhp, pgno, pgsize)
	DB_ENV *dbenv;
	DB_FH *fhp;
	db_pgno_t pgno;
	u_int32_t pgsize;
{
	/* Yes, this really is how Microsoft have designed their API */
	union {
		__int64 bigint;
		struct {
			unsigned long low;
			long high;
		};
	} off;
	off_t offset;
	HANDLE dup_handle;
	int ret;

	ret = 0;
	offset = (off_t)pgsize * pgno;

#ifdef HAVE_FILESYSTEM_NOTZERO
	/*
	 * If the filesystem doesn't zero fill, it isn't safe to extend the
	 * file, or we end up with junk blocks.  Just return in that case.
	 */
	if (__os_fs_notzero()) {
		off_t stat_offset;
		u_int32_t mbytes, bytes;

		/* Stat the file. */
		if ((ret =
		    __os_ioinfo(dbenv, NULL, fhp, &mbytes, &bytes, NULL)) != 0)
			return (ret);
		stat_offset = (off_t)mbytes * MEGABYTE + bytes;

		if (offset > stat_offset)
			return (0);
	}
#endif

	/*
	 * Windows doesn't provide truncate directly.  Instead, it has
	 * SetEndOfFile, which truncates to the current position.  To
	 * deal with that, we first duplicate the file handle, then
	 * seek and set the end of file.  This is necessary to avoid
	 * races with {Read,Write}File in other threads.
	 */
	if (!DuplicateHandle(GetCurrentProcess(), fhp->handle,
	    GetCurrentProcess(), &dup_handle, 0, FALSE,
	    DUPLICATE_SAME_ACCESS)) {
		ret = __os_get_syserr();
		goto err;
	}

	off.bigint = (__int64)pgsize * pgno;
	RETRY_CHK((SetFilePointer(dup_handle,
	    off.low, &off.high, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
	    GetLastError() != NO_ERROR) ||
	    !SetEndOfFile(dup_handle), ret);

	if (!CloseHandle(dup_handle) && ret == 0)
		ret = __os_get_syserr();

err:	if (ret != 0) {
		__db_syserr(dbenv, ret, "SetFilePointer: %lu", pgno * pgsize);
		ret = __os_posix_err(ret);
	}

	return (ret);
}
