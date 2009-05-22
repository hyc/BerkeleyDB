/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "reptest_extern.h"

static pthread_t *am_threads;		/* Access method threads. */
int		t_iter[1024];		/* Thread array. */
int		children[1024];		/* Proc array. */

#define	TXN_ID(x)	((x) == NULL ? 0 : x->id(x))

static int	amc_get __P((DB *, DB_TXN *, DBC **, DBT *, DBT *, int));
static int	am_del __P((DB *, DB_TXN *, DBT *));
static int	am_get __P((DB *, DB_TXN *, DBT *, DBT *));
static void	am_panic __P((void));
static int	am_put __P((DB *, DB_TXN *, DBT *, DBT *));
static int	check_data __P((DB *, int,
		    DB_TXN *, char *, DBT *, DBT *, int, int, DBC **));
static int 	is_marker_done __P((DB **));

int
am_dup_compare(dbp, dbt1, dbt2)
	DB *dbp;
	const DBT *dbt1, *dbt2;
{
	struct data *data1, *data2;

	dbp = NULL;				/* Quiet the compiler. */

	data1 = dbt1->data;
	data2 = dbt2->data;

	return (memcmp(&data1->id, &data2->id, sizeof(data1->id)));
}

int
am_int_compare (dbp, dbt1, dbt2)
	DB *dbp;
	const DBT *dbt1, *dbt2;
{
	int data1, data2;

	dbp = NULL;				/* Quiet the compiler. */

	memcpy(&data1, dbt1->data, sizeof(data1));
	memcpy(&data2, dbt2->data, sizeof(data2));

	return (data1 - data2);
}

int
am_init(func)
	void *(*func)__P((void *));
{
	if ((u_int)GLOBAL(nthreads) >= sizeof(t_iter) / sizeof(t_iter[0])) {
		GLOBAL(dbenv)->errx(GLOBAL(dbenv),
		    "t_iter array too small, recompile");
		return (1);
	}

	(void)time(&GLOBAL(start_time));
	return ((am_threads = spawn_kids(
	    "access method threads", GLOBAL(nthreads), func)) == NULL ? 1 : 0);
}

int
am_shutdown()
{
	return (wait_kids("am_threads", am_threads));
}

#define	CHECK_ERROR(op)	{						\
	if ((ret == DB_NOTFOUND || ret == DB_KEYEMPTY) &&		\
	    method == DB_QUEUE  && recno == 1 && retry_cnt < MAXRETRY)	\
		goto retry;						\
	am_check_err(id, op, ret, txn, curs, keystr);			\
	if (txn != NULL && txn == ctxn) {				\
		ctxn = NULL;						\
		if (retry_cnt >= 3) {					\
			ptxn->abort(ptxn);				\
			ptxn = NULL;					\
		}							\
	} else								\
		ptxn = NULL;						\
	txn = NULL;							\
	if (ret == EACCES)						\
		continue;						\
	goto retry;							\
}

static int
am_get(dbp, txn, keyp, datap)
	DB *dbp;
	DB_TXN *txn;
	DBT *keyp, *datap;
{
	int ret;

	ret = dbp->get(dbp, txn, keyp, datap, 0);
	return (ret);
}

static int
am_del(dbp, txn, keyp)
	DB *dbp;
	DB_TXN *txn;
	DBT *keyp;
{
	int ret;

	ret = dbp->del(dbp, txn, keyp, 0);
	return (ret);
}

static int
amc_get(dbp, txn, cursp, keyp, datap, mode)
	DB *dbp;
	DB_TXN *txn;
	DBC **cursp;
	DBT *keyp, *datap;
	int mode;
{
	DBC *curs;
	int ret;

	ret = dbp->cursor(dbp, txn, &curs, 0);
	if (ret != 0)
		return (ret);

	ret = curs->get(curs, keyp, datap, mode);

	*cursp = curs;
	if (ret != DB_NOTFOUND && ret != DB_KEYEMPTY)
		return (ret);

	curs->close(curs);
	*cursp = NULL;
	return (ret);
}

static int
am_put(dbp, txn, keyp, datap)
	DB *dbp;
	DB_TXN *txn;
	DBT *keyp, *datap;
{
	return (dbp->put(dbp, txn, keyp, datap, 0));
}

/*
 * am_chksum --
 *	Sum the values in a string.
 *
 * This routine MUST have the property that if the string = sub1 || sub2 that
 * am_chksum(string) = am_chksum(sub1) + am_chksum(sub2).
 */
int32_t
am_chksum(start, len)
	char *start;
	int len;
{
	int32_t sum;

	sum = 0;
	while (len--)
		sum += *start++;

	return (sum);
}

static int
check_data(dbp, id, txn, keystr, keyp, datap, start, stop, cursp)
	DB *dbp;
	int id;
	DB_TXN *txn;
	char *keystr;
	DBT *keyp, *datap;
	int start, stop;
	DBC **cursp;
{
	struct data *data_str;
	DBC *curs;
	int l, ret, sum;

	data_str = (struct data *)datap->data;

	l = start;
	if ((ret = amc_get(dbp, txn, cursp, keyp, datap, DB_SET)) != 0)
		return (ret);
	curs = *cursp;

	do {
		if ((int)data_str->id != l) {
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "[%ld] %s %s %s %ld got %ld",
			    (long)id, "data mismatch for key ",
			    keystr, " expected ", (long)l,
			    (long)data_str->id);
			am_panic();
		}
		sum = am_chksum(data_str->str, datap->size - STROFFSET);
		if (data_str->sum != sum) {
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "[%ld] chksum %d != %d for key %s id %d",
			    id, data_str->sum, sum, keystr, l);
			am_panic();
		}
		l++;
	} while (l <= stop &&
	    (ret = curs->get(curs, keyp, datap, DB_NEXT)) == 0);
	return (ret);
}

void *
am_thread(arg)
	void *arg;
{
	struct data data_str, olddata;
	DB *dbp, *markerdbp;
	DBC *curs;
	DBT data_dbt, key_dbt;
	DBTYPE method;
	DB_TXN *ctxn, *ptxn, *txn;
	db_recno_t recno;
	db_threadid_t thid;
	long id;
	pid_t pid;
	time_t end_time, kill_time, time_now;
	u_int32_t tid;
	int32_t sum;
	int i, inlen, l, move_flag, op, off, outlen;
	int put_flag, ret, retry_cnt, start, stop, xactcnt;
	char in[MAXDATAREC], keystr[8], out[MAXDATAREC], tmpkey[8];

	if (open_db_handle(&dbp, DB_UNKNOWN, HAND_OPEN) != 0)
		return (NULL);
	if (marker_file(&markerdbp, HAND_OPEN) != 0) {
		close_db_handle(&dbp);
		return (NULL);
	}
	(void)dbp->get_type(dbp, &method);
	ptxn = ctxn = txn = NULL;
	id = (long)arg;

	/* Keep warnings away. */
	inlen = off = op = outlen = retry_cnt = 0;

	GLOBAL(dbenv)->errx(GLOBAL(dbenv), "Access method thread: %ld: id %ld",
	    (u_long)MY_ID, id);

	/*
	 * XXX
	 * Solaris gets mad if each thread doesn't get a chance to start,
	 * i.e., the threads proceed sequentially without this call.
	 */
	__os_yield(NULL, 1, 0);

	memset(&key_dbt, 0, sizeof(key_dbt));
	key_dbt.data = keystr;
	key_dbt.ulen = 4;

	xactcnt = 0;
	end_time = GLOBAL(start_time) + GLOBAL(testsecs);
	/*
	 * Kill about 25% of the way through the test to give it a chance
	 * to elect and complete on a new master.
	 */
	if (GLOBAL(kill) == 1)
		kill_time = GLOBAL(start_time) + (GLOBAL(testsecs) / 4);
	for (i = 0, (void)time(&time_now);
	    time_now < end_time;
	    i++, (void)time(&time_now)) {
		/*
		 * If we are ready to kill this master, then do it now.
		 * We want to exit abruptly.
		 */
		if (GLOBAL(kill) && time_now >= kill_time) {
			printf("[%ld: %d] [%lu] KILL.\n",
			    id, i, (u_long)time_now);
			/*
			 * Write out "done" and "exit" files for this site
			 * so that it can be accounted for at the end of test.
			 */
			write_file(DONEFILE);
			write_file(EXITFILE);
			exit(1);
		}

		/*
		 * On each iteration we're going to randomly pick a key,
		 * and do one of the following operations:
		 *
		 * (1) Simply get it and verify its contents.
		 * (2) Find the duplicate that belongs to us and change it.
		 * (3) Find the duplicate that belongs to us, delete it and
		 *     re-add it, or just overwrite it.
		 * (4) Delete all of it and then re-enter it entirely.
		 * (5) Get it and all its duplicates (one per thread).
		 * (6) Find the duplicate that belongs to us, partially
		 *     overwrite it.
		 */
		op = random_int(1, 6);
		/*
		 * If I'm a client, weight the operations toward reads.  Allow
		 * 1/3 of the ops to be update attempts that should fail.  The
		 * read ops existing are #1 and #5, also do a read if #2 or #6.
		 */
		if (GLOBAL(is_master) == 0) {
			if (op == 2)
				op = 1;
			if (op == 6)
				op = 5;
		}
		if (IS_RECORD_BASED(method)) {
			recno = random_int(1, 26 * 26 * GLOBAL(nthreads));
			key_dbt.data = &recno;
			key_dbt.size = sizeof(recno);
			sprintf(keystr, "%d", recno);
		} else {
			random_data(keystr, 4);
			key_dbt.size = 4;
			keystr[0] = 'a';
		}

		/*
		 * If not verbose, print out a message every 100 opts to show
		 * progress, otherwise, detail the operation.
		 */
		if (!GLOBAL(verbose) && i % 100 == 0) {
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "[%ld: %d]", id, i);
		}
		retry_cnt = -1;
retry:
		if (is_marker_done(&markerdbp)) {
			printf(
	"[%ld: %d] [%lu : %lu] premature end due to master marker file.\n",
			    id, i, (u_long)time_now, (u_long)end_time);
			(void)GLOBAL(dbenv)->repmgr_set_ack_policy(
			    GLOBAL(dbenv), DB_REPMGR_ACKS_ALL);
			fflush(stdout);
			GLOBAL(shutdown) = 1;
		}
		if (GLOBAL(shutdown) == 1) {
			if (ptxn != NULL)
				ptxn->abort(ptxn);
			GLOBAL(dbenv)->rep_set_priority(GLOBAL(dbenv), 0);
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			     "[%ld] Shutting down.", id);
			close_db_handle(&dbp);
			markerdbp->close(markerdbp, 0);
			return (NULL);
		}
		if (i % 20 == 0 && GLOBAL(do_throttle))
			__os_yield(NULL, 1, 0);
		memset(&data_dbt, 0, sizeof(data_dbt));
		data_dbt.data = &data_str;
		data_dbt.ulen = GLOBAL(reclen);
		curs = NULL;
		retry_cnt++;
		/* If we are starting with a read, skip the transaction. */
		if (ptxn == NULL && op != 1 && (ret = GLOBAL(dbenv)->
		    txn_begin(GLOBAL(dbenv), NULL, &ptxn, 0)) != 0) {
			GLOBAL(dbenv)->err(GLOBAL(dbenv), ret,
			    "[%ld] txn_begin failed", id);
			close_db_handle(&dbp);
			markerdbp->close(markerdbp, 0);
			return (&GLOBAL(dbenv));
		}

		if (xactcnt == 0)
			xactcnt = random_int(1, MAXOPSTXN);
		txn = ptxn;
		if (ptxn != NULL && GLOBAL(child_txn) == 1) {
			if ((ret = GLOBAL(dbenv)->
			    txn_begin(GLOBAL(dbenv), ptxn, &ctxn, 0)) != 0) {
				GLOBAL(dbenv)->err(GLOBAL(dbenv), ret,
				    "[%ld] txn_begin failed", id);
				close_db_handle(&dbp);
				markerdbp->close(markerdbp, 0);
				return (&GLOBAL(dbenv));
			}
			txn = ctxn;
		}

		if (GLOBAL(verbose)) {
			printf(
"[%ld: %d] [PID %lu : TID %lu] [%lu : %lu] %d key %s (%x) try %d\n",
			    id, i, pid, thid,
			    (u_long)time_now, (u_long)end_time,
			    op, keystr, TXN_ID(txn), retry_cnt);
			fflush(stdout);
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "[%ld: %d] [%lu : %lu] %d key %s (%x)",
			    id, i, (u_long)time_now, (u_long)end_time,
			    op, keystr, TXN_ID(txn));
			fflush(OUTFP);
		}

		key_dbt.flags = DB_DBT_USERMEM;
		if (IS_RECORD_BASED(method))
			key_dbt.data = &recno;
		else
			key_dbt.data = keystr;
		data_dbt.flags = DB_DBT_USERMEM;

		debug_check();
		switch (op) {
		case 1:
			if ((ret =
			    am_get(dbp, txn, &key_dbt, &data_dbt)) != 0) {
				if (ret == DB_NOTFOUND &&
				    txn == NULL && retry_cnt < MAXRETRY)
					goto retry;
				CHECK_ERROR("DB->get");
			}

			if (data_str.id !=
			    (IS_RECORD_BASED(method) ?
			    (recno - 1) % GLOBAL(nthreads) : 0)) {
				GLOBAL(dbenv)->errx(GLOBAL(dbenv),
		"[%ld] data mismatch for key %s: expected %ld got %ld",
				    id, keystr,
				    (long)(IS_RECORD_BASED(method) ?
				    (recno - 1) % GLOBAL(nthreads) : 0),
				    (long)data_str.id);
				am_panic();
			}
			sum = am_chksum(data_str.str,
			    data_dbt.size - STROFFSET);
			if (data_str.sum != sum) {
				GLOBAL(dbenv)->errx(
				    GLOBAL(dbenv), "chksum %d != %d",
				    data_str.sum, sum);
				am_panic();
			}

			break;
		case 2:
		case 3:
		case 5:
		case 6:
			start = 0;
			stop = id;

			if (IS_RECORD_BASED(method))
				start = (recno - 1) % GLOBAL(nthreads);
			else if (op == 5)
				stop = GLOBAL(nthreads) - 1;

			ret = check_data(dbp, id, txn, keystr,
			    &key_dbt, &data_dbt, start, stop, &curs);

			if (ret != 0)
				CHECK_ERROR("check duplicates");

			if (op == 5)
				break;

			key_dbt.flags = DB_DBT_USERMEM;
			data_dbt.flags = 0;

			if (op == 3)
				goto op3;
			if (op == 6)
				goto op6;

			/* Now, change our data. */
			data_dbt.size =
			    random_int(2 * sizeof(int32_t) + 1,
				GLOBAL(reclen) - 1);
			random_data(data_str.str,
			    data_dbt.size - STROFFSET);
			data_str.sum = am_chksum(data_str.str,
			    data_dbt.size - STROFFSET);

			/* Replace */
			if ((ret = curs->put(curs,
			    &key_dbt, &data_dbt, DB_CURRENT)) != 0)
				CHECK_ERROR("DBC->put");
			break;

			/* Delete and reput */
op3:			 if (IS_RECORD_BASED(method)) {
				put_flag = DB_CURRENT;
				goto putit;
			}
			if ((ret = curs->del(curs, 0)) != 0)
				CHECK_ERROR("DBC->del");

			if (id != 0) {
				move_flag = DB_PREV;
				put_flag = GLOBAL(sortdups) ? DB_KEYFIRST
				    : DB_AFTER;
			} else {
				move_flag = DB_NEXT;
				put_flag = GLOBAL(sortdups) ? DB_KEYFIRST
				    : DB_BEFORE;
			}

			/*
			 * Since we don't really want any data on this get,
			 * return 0 bytes.
			 */
			data_dbt.flags = DB_DBT_USERMEM | DB_DBT_PARTIAL;
			data_dbt.dlen = 0;
			key_dbt.flags = DB_DBT_USERMEM;
			key_dbt.data = tmpkey;
			if ((ret = curs->get(curs,
			    &key_dbt, &data_dbt, move_flag)) != 0 &&
			    ret != DB_NOTFOUND && ret != DB_KEYEMPTY)
				CHECK_ERROR("DBC->get");

			/*
			 * I am assuming that if you return 0 bytes, you do
			 * not overwrite the existing buffer, but let's put
			 * our id in just to make sure.
			 */
			data_str.id = id;

putit:			/*
			 * If we are running with one thread, then there is
			 * the possibility that we're now on a new key or
			 * that we have run off the end of the file, in
			 * which case we need to do a regular put.
			 */
			/* Now, do the put. */
			key_dbt.flags = DB_DBT_USERMEM;

			if (0) {
				/*
				 * Partial write.  Generate random offset and
				 * sizes and replace with new random data.
				 * Note checksum math must work here.
				 */
op6:				put_flag = DB_CURRENT;

				memcpy(&olddata, &data_str, sizeof(data_str));
				data_dbt.flags = DB_DBT_PARTIAL;
				data_dbt.doff = random_int(
				    STROFFSET, data_dbt.size - 1);
				data_dbt.dlen = random_int(1,
				    data_dbt.size - data_dbt.doff);
				off = data_dbt.doff;
				outlen = data_dbt.dlen;
				memcpy(out, &data_str.str[off-8], outlen);
				sum = data_str.sum;
				sum -= am_chksum(
				    &((char *)&data_str)[data_dbt.doff],
				    data_dbt.dlen);
				if (method == DB_QUEUE)
					data_dbt.size = data_dbt.dlen;
				else
					data_dbt.size =
					    random_int(1, GLOBAL(reclen) -
					    (data_dbt.size - data_dbt.dlen));
				random_data((char *) &data_str, data_dbt.size);
				sum +=
				    am_chksum((char *)&data_str, data_dbt.size);
				inlen = data_dbt.size;
				memcpy(in, (char*)&data_str, inlen);
			} else {
				data_dbt.flags = 0;
				data_dbt.size = random_int(
				    STROFFSET + 1, GLOBAL(reclen) - 1);

				random_data(data_str.str,
				    data_dbt.size - STROFFSET);

				data_str.sum =
				    am_chksum(data_str.str,
				    data_dbt.size - STROFFSET);
			}

			if (op != 6 && GLOBAL(nthreads) == 1 &&
			    (ret == DB_NOTFOUND ||
			    strcmp(tmpkey, keystr)) != 0) {
				if (IS_RECORD_BASED(method))
					key_dbt.data = &recno;
				else
					key_dbt.data = keystr;
				if ((ret = curs->dbp->put(curs->dbp,
				    txn, &key_dbt, &data_dbt, 0)) != 0)
					CHECK_ERROR("DB->put");
			} else if ((ret = curs->put(curs,
			    &key_dbt, &data_dbt, put_flag)) != 0)
				CHECK_ERROR("DBC->put");

			if (op == 6) {
				/* calculate and write new checksum */
				data_dbt.size = data_dbt.dlen = sizeof(int32_t);
				data_dbt.doff = sizeof(int32_t);
				data_dbt.data = &sum;
				if ((ret = curs->put(curs,
				    &key_dbt, &data_dbt, DB_CURRENT)) != 0)
					CHECK_ERROR("DBC->put");
				data_dbt.data = &data_str;
				data_dbt.flags = DB_DBT_USERMEM;
#define	CHECK
#ifdef CHECK
				/* If checking, read back and verify data */
				if ((ret = curs->get(curs,
				    &key_dbt, &data_dbt, DB_CURRENT)) != 0 &&
				    ret != DB_NOTFOUND && ret != DB_KEYEMPTY)
					CHECK_ERROR("DBC->get");

				sum = am_chksum(data_str.str,
				    data_dbt.size - STROFFSET);
				if (sum != data_str.sum) {
					GLOBAL(dbenv)->errx(GLOBAL(dbenv),
    "%d != %d: inlen %d outlen %d off%d, \n old: %d %s\n out: %s\n in:%s",
					    sum, data_str.sum,
					    inlen, outlen, off,
					    olddata.sum, olddata.str,
					    out, in);
					am_panic();
				}
#endif
			}

			break;
		case 4:
			if ((ret = am_del(dbp, txn, &key_dbt)) != 0)
				CHECK_ERROR("DB->del");

			data_dbt.flags = 0;

			l = IS_RECORD_BASED(method) ?
			    (int) (recno - 1) % GLOBAL(nthreads) : 0;
			for (; l < GLOBAL(nthreads); l++) {
				data_str.id = l;
				data_dbt.size = random_int(
				    STROFFSET + 1, GLOBAL(reclen) - 1);
				random_data(data_str.str,
				    data_dbt.size - STROFFSET);
				data_str.sum = am_chksum(data_str.str,
				    data_dbt.size - STROFFSET);
				if ((ret = am_put(dbp, txn,
				    &key_dbt, &data_dbt)) != 0)
					CHECK_ERROR("DB->put");
				if (IS_RECORD_BASED(method))
					break;
			}
			break;
		}

		/* Check that this key looks OK. */
		if (curs != NULL && GLOBAL(debug)) {
			key_dbt.flags = DB_DBT_USERMEM;
			data_dbt.flags = DB_DBT_USERMEM;
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "[%ld] Debugging key %s", (long)id, keystr);

			l = IS_RECORD_BASED(method)
			    ? (recno - 1) % GLOBAL(nthreads) : 0;
			for (ret =
			    curs->get(curs, &key_dbt, &data_dbt, DB_SET);
			    ret == 0 && l < GLOBAL(nthreads);
			    l++, ret =
			    curs->get(curs, &key_dbt, &data_dbt, DB_NEXT)) {
				key_dbt.flags = DB_DBT_USERMEM;
				if (*((int32_t *)data_dbt.data) != l) {
				    GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			"[%ld] data mismatch for key %s: expected %d got %ld",
					id, keystr, l,
					(long)(*((int32_t *)data_dbt.data)));
				    am_panic();
				}

				if (IS_RECORD_BASED(method))
					break;
				key_dbt.flags = DB_DBT_USERMEM;
			}
		}

		if (curs != NULL)
			if ((ret = curs->close(curs)) != 0) {
				GLOBAL(dbenv)->errx(GLOBAL(dbenv),
				    "[%ld]: c_close failed %s %d", id, ret);
				am_panic();
			}

		if (ctxn != NULL) {
			tid = TXN_ID(ctxn);
			if ((ret = ctxn->commit(ctxn, 0)) != 0) {
				GLOBAL(dbenv)->errx(GLOBAL(dbenv),
				    "[%ld] DB_TXN->commit 0x%x failed %d",
				    id, tid, ret);
				ctxn = NULL;
				goto retry;
			}
			ctxn = NULL;
		}

		/* Now commit the transaction. */
		if (ptxn != NULL && --xactcnt == 0 ) {
			tid = TXN_ID(ptxn);
			if ((ret = ptxn->commit(ptxn, 0)) != 0) {
				GLOBAL(dbenv)->errx(GLOBAL(dbenv),
				    "[%ld] DB_TXN->commit 0x%x failed %d",
				    id, tid, ret);
				ptxn = NULL;
				goto retry;
			}
			ptxn = NULL;
		}
		__os_yield(NULL, 0, 50000);
	}
	if (ptxn != NULL && (ret = ptxn->commit(ptxn, 0)) != 0) {
		GLOBAL(dbenv)->errx(GLOBAL(dbenv),
		    "[%ld] DB_TXN->commit (#2) failed %d", id, ret);
		ptxn = NULL;
		goto retry;
	}
	/*
	 * We could be racing with the master shutting down too, so if
	 * we finish, set our priority to 0 so that we don't become
	 * master at the very end of the test.
	 */
	GLOBAL(dbenv)->rep_set_priority(GLOBAL(dbenv), 0);
	GLOBAL(dbenv)->errx(GLOBAL(dbenv), "[%ld]: exiting cleanly", id);
	close_db_handle(&dbp);
	markerdbp->close(markerdbp, 0);
	return (NULL);
}

void
am_check_err(id, op, op_ret, txn, curs, keystr)
	int32_t id;
	char *op, *keystr;
	int op_ret;
	DB_TXN *txn;
	DBC *curs;
{
	int ret;
	u_int32_t tid;

	GLOBAL(dbenv)->errx(GLOBAL(dbenv), "[%ld] %x: %s: key %s: %s",
	    (long)id, TXN_ID(txn), op, keystr,
	    op_ret == DB_LOCK_DEADLOCK ? "aborting" : db_strerror(op_ret));

	if (op_ret == DB_LOCK_DEADLOCK || op_ret == EIO) {
		if (curs != NULL && (ret = curs->close(curs)) != 0 &&
		    ret != DB_LOCK_DEADLOCK && ret != EIO) {
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "[%ld] %x: %s: key %s: %s",
			    (long)id, TXN_ID(txn), "DBC->close", keystr,
			    db_strerror(ret));
			if (GLOBAL(is_master))
				am_panic();
		}
		tid = TXN_ID(txn);
		if (txn != NULL && (ret = txn->abort(txn)) != 0) {
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "[%ld] %x: DB_TXN->abort: key %s: %s",
			    (long)id, tid, keystr, db_strerror(op_ret));
			am_panic();
		}
	} else {
		GLOBAL(dbenv)->errx(GLOBAL(dbenv), "[%ld] %x: %s: key %s: %s",
		    (long)id, TXN_ID(txn), op, keystr,
		    op_ret == DB_LOCK_DEADLOCK ?
		    "aborting" : db_strerror(op_ret));
		if (GLOBAL(is_master))
			am_panic();
		if (curs != NULL && (ret = curs->close(curs)) != 0 &&
		    ret != DB_LOCK_DEADLOCK && ret != EIO)
			am_panic();
		tid = TXN_ID(txn);
		if (txn != NULL && (ret = txn->abort(txn)) != 0) {
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "[%ld] %x: DB_TXN->abort: key %s: %s",
			    (long)id, tid, keystr, db_strerror(op_ret));
			am_panic();
		}
		if (op_ret == DB_REP_HANDLE_DEAD)
			GLOBAL(shutdown) = 1;
	}
	if (GLOBAL(verbose))
		fflush(OUTFP);
}

static void
am_panic()
{
	abort();
	/* NOTREACHED */
}

static int
is_marker_done(dbpp)
	DB **dbpp;
{
	DB *dbp;
	DBT key, data;
	int dataval, done, ret, retry_cnt;

	if (GLOBAL(is_master))
		return (0);
	ret = 0;
	retry_cnt = 0;
	dbp = *dbpp;
retry:
	done = 0;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = "DONE";
	key.size = (u_int32_t)strlen("DONE");
	data.data = &dataval;
	data.ulen = data.size = sizeof(dataval);
	data.flags = DB_DBT_USERMEM;
	/*
	 * The master writes 2 keys, DONE and then EXIT.  
	 */
	ret = dbp->get(dbp, NULL, &key, &data, 0);
	if (ret == 0)
		done = 1;
	if (ret == DB_LOCK_DEADLOCK) {
		printf("is_marker_done: deadlock, try again\n");
		__os_yield(NULL, 1, 0);
		retry_cnt++;
		if (retry_cnt < MAXRETRY)
			goto retry;
	}
	if (ret == DB_REP_HANDLE_DEAD) {
		printf("is_marker_done: handle dead, reopen & try again\n");
		dbp->close(dbp, 0);
		if (marker_file(dbpp, HAND_OPEN) != 0) {
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "is_marker_done: Dead handle. Could not reopen.");
			am_panic();
		}
		dbp = *dbpp;
		retry_cnt++;
		if (retry_cnt < MAXRETRY)
			goto retry;
	}
	return (done);
}
