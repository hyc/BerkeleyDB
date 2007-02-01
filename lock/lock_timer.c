/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996,2006 Oracle.  All rights reserved.
 *
 * $Id: lock_timer.c,v 12.9 2006/11/29 20:08:47 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/lock.h"

/*
 * __lock_set_timeout --
 *	Set timeout values in shared memory.
 *
 * This is called from the transaction system.  We either set the time that
 * this transaction expires or the amount of time a lock for this transaction
 * is permitted to wait.
 *
 * PUBLIC: int __lock_set_timeout __P(( DB_ENV *,
 * PUBLIC:      u_int32_t, db_timeout_t, u_int32_t));
 */
int
__lock_set_timeout(dbenv, locker, timeout, op)
	DB_ENV *dbenv;
	u_int32_t locker;
	db_timeout_t timeout;
	u_int32_t op;
{
	int ret;

	LOCK_SYSTEM_LOCK(dbenv);
	ret = __lock_set_timeout_internal(dbenv, locker, timeout, op);
	LOCK_SYSTEM_UNLOCK(dbenv);
	return (ret);
}

/*
 * __lock_set_timeout_internal
 *		-- set timeout values in shared memory.
 *
 * This is the internal version called from the lock system.  We either set
 * the time that this transaction expires or the amount of time that a lock
 * for this transaction is permitted to wait.
 *
 * PUBLIC: int __lock_set_timeout_internal
 * PUBLIC:     __P((DB_ENV *, u_int32_t, db_timeout_t, u_int32_t));
 */
int
__lock_set_timeout_internal(dbenv, locker, timeout, op)
	DB_ENV *dbenv;
	u_int32_t locker;
	db_timeout_t timeout;
	u_int32_t op;
{
	DB_LOCKER *sh_locker;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	u_int32_t locker_ndx;
	int ret;

	lt = dbenv->lk_handle;
	region = lt->reginfo.primary;

	LOCKER_LOCK(lt, region, locker, locker_ndx);
	ret = __lock_getlocker(lt, locker, locker_ndx, 1, &sh_locker);

	if (ret != 0)
		return (ret);

	if (op == DB_SET_TXN_TIMEOUT) {
		if (timeout == 0)
			timespecclear(&sh_locker->tx_expire);
		else
			__lock_expires(dbenv, &sh_locker->tx_expire, timeout);
	} else if (op == DB_SET_LOCK_TIMEOUT) {
		sh_locker->lk_timeout = timeout;
		F_SET(sh_locker, DB_LOCKER_TIMEOUT);
	} else if (op == DB_SET_TXN_NOW) {
		timespecclear(&sh_locker->tx_expire);
		__lock_expires(dbenv, &sh_locker->tx_expire, 0);
		sh_locker->lk_expire = sh_locker->tx_expire;
		if (!timespecisset(&region->next_timeout) ||
		    timespeccmp(
			&region->next_timeout, &sh_locker->lk_expire, >))
			region->next_timeout = sh_locker->lk_expire;
	} else
		return (EINVAL);

	return (0);
}

/*
 * __lock_inherit_timeout
 *		-- inherit timeout values from parent locker.
 * This is called from the transaction system.  This will
 * return EINVAL if the parent does not exist or did not
 * have a current txn timeout set.
 *
 * PUBLIC: int __lock_inherit_timeout __P(( DB_ENV *, u_int32_t, u_int32_t));
 */
int
__lock_inherit_timeout(dbenv, parent, locker)
	DB_ENV *dbenv;
	u_int32_t parent, locker;
{
	DB_LOCKER *parent_locker, *sh_locker;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	u_int32_t locker_ndx;
	int ret;

	lt = dbenv->lk_handle;
	region = lt->reginfo.primary;
	ret = 0;
	LOCK_SYSTEM_LOCK(dbenv);

	/* If the parent does not exist, we are done. */
	LOCKER_LOCK(lt, region, parent, locker_ndx);
	if ((ret = __lock_getlocker(lt,
	    parent, locker_ndx, 0, &parent_locker)) != 0)
		goto err;

	/*
	 * If the parent is not there yet, thats ok.  If it
	 * does not have any timouts set, then avoid creating
	 * the child locker at this point.
	 */
	if (parent_locker == NULL ||
	    (timespecisset(&parent_locker->tx_expire) &&
	    !F_ISSET(parent_locker, DB_LOCKER_TIMEOUT))) {
		ret = EINVAL;
		goto done;
	}

	LOCKER_LOCK(lt, region, locker, locker_ndx);
	if ((ret = __lock_getlocker(lt,
	    locker, locker_ndx, 1, &sh_locker)) != 0)
		goto err;

	sh_locker->tx_expire = parent_locker->tx_expire;

	if (F_ISSET(parent_locker, DB_LOCKER_TIMEOUT)) {
		sh_locker->lk_timeout = parent_locker->lk_timeout;
		F_SET(sh_locker, DB_LOCKER_TIMEOUT);
		if (!timespecisset(&parent_locker->tx_expire))
			ret = EINVAL;
	}

done:
err:	LOCK_SYSTEM_UNLOCK(dbenv);
	return (ret);
}

/*
 * __lock_expires --
 *	Set the expire time given the time to live.
 *
 * PUBLIC: void __lock_expires __P((DB_ENV *, db_timespec *, db_timeout_t));
 */
void
__lock_expires(dbenv, timespecp, timeout)
	DB_ENV *dbenv;
	db_timespec *timespecp;
	db_timeout_t timeout;
{
	db_timespec v;

	/*
	 * If timespecp is set then it contains "now".  This avoids repeated
	 * system calls to get the time.
	 */
	if (!timespecisset(timespecp))
		__os_gettime(dbenv, timespecp);

	/* Convert the microsecond timeout argument to a timespec. */
	DB_TIMEOUT_TO_TIMESPEC(timeout, &v);

	/* Add the timeout to "now". */
	timespecadd(timespecp, &v);
}

/*
 * __lock_expired -- determine if a lock has expired.
 *
 * PUBLIC: int __lock_expired __P((DB_ENV *, db_timespec *, db_timespec *));
 */
int
__lock_expired(dbenv, now, timespecp)
	DB_ENV *dbenv;
	db_timespec *now, *timespecp;
{
	if (!timespecisset(timespecp))
		return (0);

	if (!timespecisset(now))
		__os_gettime(dbenv, now);

	return (timespeccmp(now, timespecp, >=));
}
