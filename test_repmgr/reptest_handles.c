/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "reptest_extern.h"

static int marker_done();
static int check_files __P((const char *, u_int32_t));

/*
 * open_handles --
 *	Open the DB handles.
 */
int
open_env_handle(prefix, start_state, mode)
	char *prefix;
	u_int32_t start_state;
	handle_mode mode;
{
	u_int32_t flags;
	int ack, ret;

	GLOBAL(dbenv)->set_errfile(GLOBAL(dbenv), OUTFP);
	GLOBAL(dbenv)->set_errpfx(GLOBAL(dbenv), prefix);

	set_yield();

	flags = (DB_THREAD | DB_INIT_LOCK | DB_INIT_LOG |
	    DB_INIT_MPOOL | DB_INIT_REP | DB_INIT_TXN);
	switch (mode) {
	case HAND_OPEN:
		break;
	case HAND_CREATE:
		flags |= (DB_CREATE | DB_RECOVER);
		break;
	}

	if ((ret =
	    GLOBAL(dbenv)->open(GLOBAL(dbenv), GLOBAL(home), flags, 0)) != 0) {
		GLOBAL(dbenv)->err(GLOBAL(dbenv), ret, "%s: open",
		    GLOBAL(home));
		goto err;
	}
	/*
	 * We've read in the DB_CONFIG now that we're open.  If this
	 * test is using the DB_REPMGR_ACKS_NONE policy we want to
	 * throttle the workload so otherwise the clients can end up
	 * way too far behind as the master worker threads blast out
	 * their updates.
	 */
	if ((ret = GLOBAL(dbenv)->repmgr_get_ack_policy(GLOBAL(dbenv),
	    &ack)) != 0)
		goto err;
	if (ack == DB_REPMGR_ACKS_NONE)
		GLOBAL(do_throttle) = 1;
	else
		GLOBAL(do_throttle) = 0;
	/*
	 * Now start replication.
	 */
	if ((ret = GLOBAL(dbenv)->repmgr_start(GLOBAL(dbenv), GLOBAL(repth),
	    start_state)) != 0)
		goto err;
	return (0);

err:
	if (GLOBAL(dbenv) != NULL)
		GLOBAL(dbenv)->close(GLOBAL(dbenv), 0);
	GLOBAL(dbenv) = NULL;
	return (1);
}

int
open_db_handle(dbpp, method, mode)
	DB **dbpp;
	DBTYPE method;
	handle_mode mode;
{
	DB *dbp;
	char *dname, *fname;
	int ret;

retry:
	if ((ret = db_create(dbpp, GLOBAL(dbenv), 0)) != 0) {
		GLOBAL(dbenv)->err(GLOBAL(dbenv), ret, "db_create");
		goto err;
	}
	dbp = *dbpp;

	if (mode == HAND_CREATE) {
		if (!IS_RECORD_BASED(method) &&
		    (ret = dbp->set_flags(dbp, DB_DUP)) != 0) {
			dbp->err(dbp, ret, "set_flags: DB_DUP");
			goto err;
		}
		if ((ret = dbp->set_pagesize(dbp, GLOBAL(pagesize))) != 0) {
			dbp->err(dbp, ret, "set_pgsize: %d", GLOBAL(pagesize));
			goto err;
		}

		if (method == DB_QUEUE) {
			if ((ret = dbp->set_re_len(dbp, GLOBAL(reclen))) != 0) {
				dbp->err(dbp, ret,
				    "set_re_len: %d", GLOBAL(reclen));
				goto err;
			}
			if ((ret = dbp->set_re_pad(dbp, 0x0)) != 0) {
				dbp->err(dbp, ret, "set_re_pad: 0x0");
				goto err;
			}
			if (GLOBAL(extentsize) != 0 &&
			    (ret = dbp->set_q_extentsize(
			    dbp, GLOBAL(extentsize))) != 0) {
				dbp->err(dbp, ret, "set_q_extent: 2");
				goto err;
			}
		}
	}
	if (GLOBAL(sortdups)) {
		if ((ret = dbp->set_flags(dbp, DB_DUPSORT)) != 0) {
			dbp->err(dbp, ret, "set_flags");
			goto err;
		}
		if ((ret = dbp->set_dup_compare(dbp, am_dup_compare)) != 0) {
			dbp->err(dbp, ret, "set_dup_compare");
			goto err;
		}
	}

	if (GLOBAL(recnum)) {
		if ((ret = dbp->set_flags(dbp, DB_RECNUM)) != 0) {
			dbp->err(dbp, ret, "set_flags");
			goto err;
		}
		if ((ret = dbp->set_bt_compare(dbp, am_int_compare)) != 0) {
			dbp->err(dbp, ret, "set_bt_compare");
			goto err;
		}
	}

	if (GLOBAL(inmem)) {
		fname = NULL;
		dname = DATABASE1;
	} else if (GLOBAL(subdb)) {
		fname = DATABASE1;
		dname = SUBDB1;
	} else {
		fname = DATABASE1;
		dname = NULL;
	}
	if ((ret = dbp->open(dbp, NULL, fname, dname, method,
	    (mode == HAND_CREATE ? DB_CREATE : 0) | DB_THREAD | DB_AUTO_COMMIT,
	    0644)) == EIO || ret == DB_REP_LEASE_EXPIRED) {
		dbp->close(dbp, 0);
		/*
		 * If the error is expired leases, it could be that the
		 * clients haven't sync'ed yet.  Pause and try again.
		 */
		if (ret == DB_REP_LEASE_EXPIRED)
			__os_yield(NULL, 1, 0);
		goto retry;
	}
	if (ret != 0) {
		dbp->err(dbp, ret, "open: %s", DATABASE1);
		goto err;
	}
	return (0);

err:	if (dbp != NULL) {
		ret = dbp->close(dbp, 0);
		*dbpp = NULL;
	}
	return (1);
}

/*
 * close_handles --
 *	Close the DB handles.
 */
int
close_db_handle(dbpp)
	DB **dbpp;
{
	DB *dbp;
	int ret;

	/*
	 * Make sure we can close the files.  
	 * Do a sync first so there are not too many dirty buffers
	 * on the close.
	 */
	dbp = *dbpp;
	(void)dbp->sync(dbp, 0);
	if ((ret = dbp->close(dbp, 0)) != 0)
		GLOBAL(dbenv)->err(GLOBAL(dbenv), ret, "%s: close", DATABASE1);
	*dbpp = NULL;

	return (ret);
}

int
close_env_handle()
{
	int ret, t_ret;

	ret = marker_done();
	if ((t_ret = GLOBAL(dbenv)->close(GLOBAL(dbenv), 0)) != 0) {
		if (ret == 0)
			ret = t_ret;
		fprintf(OUTFP, "%s: GLOBAL(dbenv)->close: %s\n",
		    GLOBAL(progname), db_strerror(ret));
	}
	fprintf(OUTFP, "%s: ENV CLOSED.  TEST DONE.\n", GLOBAL(progname));
	GLOBAL(dbenv) = NULL;
	return (ret);
}

int
marker_file(dbpp, mode)
	DB **dbpp;
	handle_mode mode;
{
	DB *dbp;
	DBT key, data;
	int ret;

	/*
	 * We're marking that the initialization is complete.  Force
	 * out a checkpoint to push all the initial data to the disk.
	 */
	(void)GLOBAL(dbenv)->txn_checkpoint(GLOBAL(dbenv), 0, 0, DB_FORCE);
retry:
	if ((ret = db_create(dbpp, GLOBAL(dbenv), 0)) != 0) {
		GLOBAL(dbenv)->err(GLOBAL(dbenv), ret, "db_create");
		return (ret);
	}
	dbp = *dbpp;
	if ((ret = dbp->open(dbp, NULL, MARKERDB, NULL, DB_BTREE,
	    (mode == HAND_CREATE ? DB_CREATE : 0) | DB_AUTO_COMMIT,
	    0644)) == EIO) {
		dbp->close(dbp, 0);
		goto retry;
	}
	if (ret != 0) {
		dbp->err(dbp, ret, "open: %s", MARKERDB);
		(void)dbp->close(dbp, 0);
	} else if (mode == HAND_CREATE) {
		memset(&key, 0, sizeof(key));
		memset(&data, 0, sizeof(data));
		key.data = "BEGIN";
		key.size = (u_int32_t)strlen("BEGIN");
		data.data = &ret;
		data.ulen = data.size = sizeof(ret);
		if ((ret = dbp->put(dbp, NULL,
		    &key, &data, DB_AUTO_COMMIT)) != 0) {
			dbp->err(dbp, ret, "DB->put");
			return (ret);
		}
	}
	return (ret);
}

static int
marker_done()
{
	DB *dbp;
	DBT key, data;
	u_int32_t nsites;
	int count, done, fails, max, ret;

	/*
	 * Coordinate shutdown among the master and all the clients.
	 * After the workload is complete, the master will signal the
	 * DONE key in the marker file, and that will tell all clients
	 * to stop their workload, if it isn't yet complete.  
	 * The master will set the ack policy to ALL and loop writing
	 * the DONE key until all clients acknowledge that txn, which
	 * means they are all caught up.  The master will then write
	 * the EXIT key and all sites can exit.
	 */
retry:
	done = 0;
	max = ret = 0;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	dbp = GLOBAL(markerdb);
	if ((ret = GLOBAL(dbenv)->repmgr_set_ack_policy(GLOBAL(dbenv),
	    DB_REPMGR_ACKS_ALL)) != 0) {
		GLOBAL(dbenv)->err(GLOBAL(dbenv), ret, "set_ack_policy");
		goto err;
	}
	if (GLOBAL(is_master)) {
		GLOBAL(ackfails) = fails = 0;
		if ((ret = GLOBAL(dbenv)->rep_get_nsites(GLOBAL(dbenv),
		    &nsites)) != 0) {
			GLOBAL(dbenv)->err(GLOBAL(dbenv), ret,
			    "rep_get_nsites");
			goto err;
		}
		/*
		 * We want to do a read operation here so that we can
		 * force leases to be updated without having it done
		 * in a txn.  A txn will hold pages write locked and
		 * might prevent progress on a site that is in internal
		 * init.
		 */
		key.data = "BEGIN";
		key.size = (u_int32_t)strlen("BEGIN");
		data.data = &fails;
		data.ulen = data.size = sizeof(fails);
		data.flags = DB_DBT_USERMEM;
		ret = dbp->get(dbp, NULL, &key, &data, 0);
		if (ret != 0 && ret != DB_NOTFOUND &&
		    ret != DB_LOCK_DEADLOCK) {
			dbp->err(dbp, ret, "DB->get");
			goto err;
		}
		key.data = "DONE";
		key.size = (u_int32_t)strlen("DONE");
		/*
		 * Subtract one because we don't count ourselves.
		 */
		nsites--;
		while (!done) {
			__os_yield(NULL, 1, 0);
			data.data = &GLOBAL(ackfails);
			data.size = sizeof(GLOBAL(ackfails));
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "Done: iteration %d", GLOBAL(ackfails));
			if (GLOBAL(ackfails) > MAXFAILS) {
				GLOBAL(dbenv)->errx(GLOBAL(dbenv),
				    "Done: MAX reached.");
				ret = 0;
				max = 1;
				break;
			}
			if ((ret = dbp->put(dbp, NULL,
			    &key, &data, DB_AUTO_COMMIT)) != 0) {
				dbp->err(dbp, ret, "DB->put");
				goto err;
			}
			(void) GLOBAL(dbenv)->txn_checkpoint(
			    GLOBAL(dbenv), 0, 0, 0);
			done = check_files(DONEFILE, nsites);
		}
		__os_yield(NULL, 3, 0);
		GLOBAL(dbenv)->errx(GLOBAL(dbenv),
		    "Done: (master) ready to exit");
		key.data = "EXIT";
		key.size = (u_int32_t)strlen("EXIT");
		if ((ret = dbp->put(dbp, NULL,
		    &key, &data, DB_AUTO_COMMIT)) != 0) {
			dbp->err(dbp, ret, "DB->put");
			goto err;
		}
		__os_yield(NULL, 3, 0);
		/*
		 * If we're the master and we've reached our maximum, we
		 * only check exit files once and then we're done.  If the
		 * client didn't respond to finishing it could be that it
		 * crashed or died unexpectedly.  We don't want the master
		 * to loop forever (waiting for Tcl to kill it) in that case.
		 */
		do {
			done = check_files(EXITFILE, nsites);
		} while (!done && !max);
	} else {
		/*
		 * We're done with our workload.  Wait for the master to
		 * tell us it is time to exit.
		 */
		count = 1;
		write_file(DONEFILE);
		key.data = "EXIT";
		key.size = (u_int32_t)strlen("EXIT");
		data.data = &fails;
		data.ulen = data.size = sizeof(fails);
		data.flags = DB_DBT_USERMEM;
		while (!done) {
			/*
			 * If we've waited the maximum, something must be
			 * wrong with the group.  Abort so we have something
			 * to investigate.
			 */
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
		    	    "Done %d: (client) Looking for %s",count,key.data);
			if (count > MAXFAILS) {
				GLOBAL(dbenv)->errx(GLOBAL(dbenv),
				    "Done: (client) MAX reached.");
				ret = 0;
				goto err;
			}
			ret = dbp->get(dbp, NULL, &key, &data, 0);
			if (ret != 0 && ret != DB_NOTFOUND &&
			    ret != DB_LOCK_DEADLOCK) {
				dbp->err(dbp, ret, "DB->get");
				goto err;
			}
			count++;
			if (ret == 0) {
				done = 1;
				GLOBAL(dbenv)->errx(GLOBAL(dbenv),
		    		    "Done: (client) ready to exit");
				break;
			}
			__os_yield(NULL, 1, 0);
		}
		write_file(EXITFILE);
	}
err:
	(void)GLOBAL(markerdb)->close(GLOBAL(markerdb), 0);
	if (ret == DB_REP_HANDLE_DEAD) {
		marker_file(&GLOBAL(markerdb), HAND_OPEN);
		GLOBAL(dbenv)->errx(GLOBAL(dbenv),
		    "Done: dead handle.  Reopen and retry");
		goto retry;
	}
	return (ret);
}

static int
check_files(file_prefix, nsites)
	const char *file_prefix;
	u_int32_t nsites;
{
	int cnt, i, len, ret;
	u_int32_t sites;
	char **names;

	if ((ret = __os_dirlist(NULL, DATAHOME, 0, &names, &cnt)) != 0)
		return (0);
	sites = 0;
	len = strlen(file_prefix);
	for (i = 0; i < cnt; i++) {
		if (strncmp(names[i], file_prefix, len) == 0)
			sites++;
	}
	__os_dirfree(NULL, names, cnt);
	if (sites == nsites)
		return (1);
	else
		return (0);
}

void
write_file(file_prefix)
	const char *file_prefix;
{
	int fd;
	char filebuf[32];

	sprintf(filebuf, "%s/%s.%d", DATAHOME, file_prefix, GLOBAL(myid));
	/*
	 * If an error occurs and the file is not created, that should be
	 * okay.  Clients and the master will all loop some maximum number
	 * of attempts to synchronize the end of the test.  Eventually they
	 * will exit and the verification step will take over.
	 */
	if ((fd = open(filebuf, O_CREAT | O_RDWR, 0777)) >= 0)
		close(fd);
}
