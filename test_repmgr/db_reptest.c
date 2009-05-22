/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "reptest_extern.h"

struct __dbs_globals __dbs_globals;

static void	convert_method();
static void	check_required_state(u_int32_t);
static int	do_init();
static int	driver();
static void	event_callback __P((DB_ENV *, u_int32_t, void *));
static int	init_db(DB *, char *, DBTYPE);
static void	my_printf(int fd, const char *format, ...);
static int	run_test(void);

#define	ERROR_RETURN	1
void	 usage(void);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	FILE *proc;
	char *host, *portstr;
	int ch, eval, ret;
	u_int32_t start_state;
	u_int16_t port;
	char buf[256];


	eval = 0;
	start_state = 0;

	GLOBAL(progpath) = argv[0];
	if ((GLOBAL(progname) = strrchr(argv[0], '/')) == NULL)
		GLOBAL(progname) = argv[0];
	else
		GLOBAL(progname)++;

	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);

	if ((ret = db_env_create(&GLOBAL(dbenv), 0)) != 0) {
		fprintf(stderr, "%s: db_env_create: %s\n",
		    GLOBAL(progname), db_strerror(ret));
		return (1);
	}
	(void)GLOBAL(dbenv)->set_event_notify(GLOBAL(dbenv), event_callback);
	/*
	 * This is used in FILL() below so it must be non-zero.
	 */
	GLOBAL(reclen) = 10;
	GLOBAL(pagesize) = 8192;
	GLOBAL(seed) = time(NULL) ^ getpid() << 7;
	GLOBAL(repth) = REP_NTHREADS;
	GLOBAL(home) = DATAHOME;
	srand(GLOBAL(seed));

	while ((ch = getopt(argc, argv,
	    "Cc:dEf:h:kl:Mm:p:R:r:T:t:vz")) != EOF)
		switch (ch) {
		case 'C':			/* Client (no elections) */
			start_state = DB_REP_CLIENT;
			break;
		case 'c':			/* Concurrency */
			if (__db_getlong(NULL, GLOBAL(progname),
			    optarg, 1, LONG_MAX, &GLOBAL(nthreads)))
				return (EXIT_FAILURE);
			break;
		case 'd':			/* Debug */
			GLOBAL(debug) = 1;
			break;
		case 'E':			/* Client (with elections) */
			start_state = DB_REP_ELECTION;
			break;
		case 'f':                       /* fixed length recordlen. */
			GLOBAL(reclen) = atoi(optarg);
			break;
		case 'h':			/* DBHOME */
			GLOBAL(home) = optarg;
			break;
		case 'k':			/* kill test */
			GLOBAL(kill) = 1;
			break;
		case 'l':
			host = strtok(optarg, ":");
			if ((portstr = strtok(NULL, ":")) == NULL) {
				fprintf(stderr, "Bad host specification.\n");
				goto err;
			}
			port = (unsigned short)atoi(portstr);
			GLOBAL(myid) = port;
			if ((ret = GLOBAL(dbenv)->repmgr_set_local_site(
			    GLOBAL(dbenv), host, port, 0)) != 0) {
				fprintf(stderr,
				    "Could not set listen address (%d).\n",
				    ret);
				goto err;
			}
			GLOBAL(hostport) = 1;
			break;
		case 'M':			/* Master */
			start_state = DB_REP_MASTER;
			break;
		case 'm':			/* Msg processing concurrency */
			if (__db_getlong(NULL, GLOBAL(progname),
			    optarg, 1, LONG_MAX, &GLOBAL(repth)))
				return (EXIT_FAILURE);
			break;
		case 'p':			/* Pagesize. */
			GLOBAL(pagesize) = atoi(optarg);
			break;
		case 'R':
		case 'r':
			host = strtok(optarg, ":");
			if ((portstr = strtok(NULL, ":")) == NULL) {
				fprintf(stderr, "Bad host specification.\n");
				goto err;
			}
			port = (unsigned short)atoi(portstr);
			if ((ret = GLOBAL(dbenv)->repmgr_add_remote_site(
			    GLOBAL(dbenv), host, port, NULL,
			    ch == 'R' ? DB_REPMGR_PEER : 0)) != 0) {
				fprintf(stderr,
				    "Could not set listen address (%d).\n",
				    ret);
				goto err;
			}
			GLOBAL(hostport) = 1;
			break;
		case 'T':			/* Test seconds. */
			GLOBAL(testsecs) = atoi(optarg);
			if (GLOBAL(testsecs) < 0) {
				fprintf(stderr,
				    "Number of secs must be positive (%d).\n",
				    GLOBAL(testsecs));
				goto err;
			}
			break;
		case 't':			/* Access Method. */
			GLOBAL(test) = optarg;
			break;
		case 'v':			/* Verbose output. */
			GLOBAL(verbose) = 1;
			break;
		case 'z':			/* Child txn. */
			GLOBAL(child_txn) = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

	sprintf(buf, "%s/%s", GLOBAL(home), OUTFILE);
	if ((GLOBAL(outfp) = fopen(buf, "a")) == NULL)
		return (ERROR_RETURN);
	setvbuf(OUTFP, NULL, _IOLBF, BUFSIZ);
	/* put stderr and stdout into our file */
	fclose(stderr);
	fclose(stdout);
	sprintf(buf, "%s/OUTPUT", GLOBAL(home));
	if (freopen(buf, "a", stderr) == NULL)
		abort();
	if (freopen(buf, "a", stdout) == NULL)
		abort();
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
	setvbuf(stderr, NULL, _IOLBF, BUFSIZ);

	check_required_state(start_state);
	convert_method();
	if ((ret = open_env_handle(GLOBAL(home), start_state,
	    HAND_CREATE)) != 0)
		goto err;
	if ((proc = fopen("dbs.pid", "w")) != NULL) {
		fprintf(proc, "%d\n", getpid());
		fclose(proc);
	}
	ret = run_test();
	(void)close_env_handle();
	fflush(OUTFP);
	fclose(OUTFP);
err:
	return (ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: %s %s\n\t%s\n", GLOBAL(progname),
"[-dvz] -c threads -C|M  [-f reclen] -h home -l host:port",
"-p pagesize [-r|R host:port] -t method -T seconds");
	exit(EXIT_FAILURE);
}

static void
#ifdef STDC_HEADERS
my_printf(int fd, const char *format, ...)
#else
my_printf(format, va_alist)
	int fd;
	const char *format;
	va_dcl
#endif
{
	char buf[1024];
	va_list ap;
#ifdef STDC_HEADERS
	va_start(ap, format);
#else
	va_start(ap);
#endif
	vsprintf(buf, format, ap);
	write(fd, buf, strlen(buf));

	va_end(ap);
}

static void
convert_method()
{
	if (GLOBAL(test) == NULL)
		usage();
	else if (strcmp(GLOBAL(test), "btree") == 0)
		GLOBAL(method) = DB_BTREE;
	else if (strcmp(GLOBAL(test), "dbtree") == 0) {
		GLOBAL(method) = DB_BTREE;
		GLOBAL(sortdups) = 1;
	} else if (strcmp(GLOBAL(test), "ibtree") == 0) {
		GLOBAL(method) = DB_BTREE;
		GLOBAL(inmem) = 1;
	} else if (strcmp(GLOBAL(test), "rbtree") == 0) {
		GLOBAL(method) = DB_BTREE;
		GLOBAL(recnum) = 1;
	} else if (strcmp(GLOBAL(test), "sbtree") == 0) {
		GLOBAL(method) = DB_BTREE;
		GLOBAL(subdb) = 1;
	} else if (strcmp(GLOBAL(test), "hash") == 0)
		GLOBAL(method) = DB_HASH;
	else if (strcmp(GLOBAL(test), "queue") == 0)
		GLOBAL(method) = DB_QUEUE;
	else if (strcmp(GLOBAL(test), "queueext") == 0) {
		GLOBAL(method) = DB_QUEUE;
		GLOBAL(extentsize) = GLOBAL(pagesize) * 3;
	} else if (strcmp(GLOBAL(test), "iqueue") == 0) {
		GLOBAL(method) = DB_QUEUE;
		GLOBAL(inmem) = 1;
	} else if (strcmp(GLOBAL(test), "recno") == 0)
		GLOBAL(method) = DB_RECNO;
	else if (strcmp(GLOBAL(test), "frecno") == 0) {
		GLOBAL(method) = DB_RECNO;
		if (GLOBAL(reclen) == 0)
			usage();
	} else
		usage();
	return;
}

static int
run_test()
{
	time_t now;
	int err_fd;
	char time_buf[CTIME_BUFLEN];

	time(&now);
	err_fd = dup(STDERR_FILENO);
	fprintf(OUTFP, "%s %ld threads: %s",
	    GLOBAL(test), GLOBAL(nthreads), __os_ctime(&now, time_buf));
	my_printf(err_fd, "%s %ld threads: %s",
	    GLOBAL(test), GLOBAL(nthreads), __os_ctime(&now, time_buf));
	GLOBAL(shutdown) = 0;

	if (do_init() != 0)
		return (ERROR_RETURN);

	if (driver() != 0)
		return (ERROR_RETURN);
	return (0);
}

static int
do_init()
{
	DB *dbp;
	int init_done;

	init_done = 0;
	while (!init_done) {
		/*
		 * If I am the master, then create the initial database.
		 */
		if (GLOBAL(is_master)) {
			if (open_db_handle(&dbp, GLOBAL(method),
			    HAND_CREATE) != 0)
				return (1);
			if (init_db(dbp, GLOBAL(test), GLOBAL(method)) != 0)
				return (1);
			if (close_db_handle(&dbp) != 0)
				return (1);
			if (marker_file(&GLOBAL(markerdb), HAND_CREATE) != 0)
				return (1);
			init_done = 1;
			__os_yield(NULL, 5, 0);
		} else {
			/*
			 * If I am the client, wait for the marker file
			 * to exist so that I know that my client has the
			 * complete initialized database too.
			 */
			if (marker_file(&GLOBAL(markerdb), HAND_OPEN) == 0)
				init_done = 1;
			__os_yield(NULL, 1, 0);
		}
		/*
		 * If we haven't found our initial marker, pause so that
		 * it can get created or possibly an election happen.
		 */
	}
	return (0);
}

static int
driver()
{
	/* Start supporting threads. */
	if (trickle_init(1) != 0)
		return (1);
	if (checkpoint_init() != 0)
		return (1);
	if (log_init() != 0)
		return (1);

	/* Start access method tests and wait for them. */
	if (am_init(am_thread) != 0 || am_shutdown() != 0)
		return (1);

	/* Tell remaining threads to exit; wait for them. */
	GLOBAL(shutdown) = 1;
	GLOBAL(dbenv)->errx(GLOBAL(dbenv),
	    "Waiting for support threads to wake up...");

	if (log_shutdown() != 0)
		return (1);
	if (trickle_shutdown() != 0)
		return (1);
	if (checkpoint_shutdown() != 0)
		return (1);

	GLOBAL(dbenv)->txn_checkpoint(GLOBAL(dbenv), 0, 0, DB_FORCE);
	return (0);
}

static int
init_db(dbp, m_name, method)
	DB *dbp;
	char *m_name;
	DBTYPE method;
{
	struct data data;
	DB_TXN *txn;
	DBT data_dbt, key_dbt;
	db_recno_t recno;
	int j, k, l, nrecs, ret;
	int save_i, save_j, save_k, save_l, save_nrecs;
	char keystr[4];

	/*
	 * Initialize the database.
	 * Our goal is to store 26^2 keys each with "nthreads" duplicates.
	 * Since record-based methods do not contain duplicates, we simply
	 * generate nthreads * 26^2 unique records with record number keys.
	 * Non record-number keys consist of a three-character string of
	 * lower-case alphabetic characters.  The data fields are 4 bytes of
	 * ID with a random string attached.
	 */
#define	FILL(L) {							\
	data.id = (L);							\
	data_dbt.size =							\
	    random_int(STROFFSET + 1, GLOBAL(reclen) - 1);		\
	random_data(data.str , data_dbt.size - STROFFSET);		\
	data.sum = am_chksum((char *)data_dbt.data +			\
	    STROFFSET, data_dbt.size - STROFFSET);			\
	ret = dbp->put(dbp, txn, &key_dbt, &data_dbt, 0);		\
	if (ret == EIO) {						\
		if ((ret = txn->abort(txn)) == 0)			\
			goto aborted;					\
	}								\
	if (ret != 0) {							\
		dbp->err(dbp, ret, "%s: put", m_name);			\
		return (1);						\
	}								\
	nrecs++;							\
}

	if ((ret =
	    GLOBAL(dbenv)->txn_begin(GLOBAL(dbenv), NULL, &txn, 0)) != 0) {
		GLOBAL(dbenv)->err(GLOBAL(dbenv), ret, "txn_begin failed");
		return (1);
	}
	keystr[3] = '\0';
	memset(&key_dbt, 0, sizeof(key_dbt));
	key_dbt.flags = DB_DBT_USERMEM;
	if (IS_RECORD_BASED(method)) {
		key_dbt.data = &recno;
		key_dbt.size = sizeof(recno);
	} else {
		key_dbt.data = keystr;
		key_dbt.size = 4;
	}
	memset(&data_dbt, 0, sizeof(data_dbt));
	data_dbt.data = &data;
	save_i = save_j = save_k = save_l = save_nrecs = 0;
	nrecs = 0;
	keystr[0] = 'a';
	for (j = 0; j < 26; j++) {
		keystr[2] = 'a' + j;
		for (k = 0; k < 26; k++) {
			keystr[1] = 'a' + k;
			for (l = 0; l < GLOBAL(nthreads); l++) {
				if (IS_RECORD_BASED(method))
					recno = nrecs + 1;
				FILL(l);
				if ((nrecs % 1000) == 0) {
					if ((ret =
					    txn->commit(txn, 0)) != 0) {
						GLOBAL(dbenv)->err(
						    GLOBAL(dbenv), ret,
					    "DB_TXN->commit failed");
aborted:					j = save_j;
						k = save_k;
						l = save_l - 1;
						nrecs = save_nrecs;
						keystr[0] = 'a';
						keystr[1] = 'a' + k;
						keystr[2] = 'a' + j;
					}
					if ((ret = GLOBAL(dbenv)->
					    txn_begin(GLOBAL(dbenv),
					    NULL, &txn, 0)) != 0) {
						GLOBAL(dbenv)->err(
						    GLOBAL(dbenv), ret,
						    "txn_begin failed");
						return (1);
					}
					save_nrecs = nrecs;
					save_j = j;
					save_k = k;
					save_l = l + 1;
				}
			}
		}
	}
	GLOBAL(dbenv)->errx(GLOBAL(dbenv), "%d records done", nrecs);
	/* Now commit the last transaction. */
	if ((ret = txn->commit(txn, 0)) != 0) {
		GLOBAL(dbenv)->
		    err(GLOBAL(dbenv), ret, "DB_TXN->commit failed\n");
		goto aborted;
	}
	txn = NULL;
	return (0);
}

static void
check_required_state(start_state)
	u_int32_t start_state;
{
	if (GLOBAL(home) != NULL &&
	    GLOBAL(hostport) != 0 &&
	    GLOBAL(nthreads) != 0 &&
	    start_state != 0 &&
	    GLOBAL(test) != NULL &&
	    GLOBAL(testsecs) != 0)
		return;
	else
		usage();
	/* NOTREACHED */
}

static void
event_callback(dbenv, which, info)
	DB_ENV *dbenv;
	u_int32_t which;
	void *info;
{
	COMPQUIET(info, NULL);
	switch (which) {
	case DB_EVENT_PANIC:
		printf("received panic event\n");
		abort();
		/* NOTREACHED */

	case DB_EVENT_REP_CLIENT:
		GLOBAL(is_master) = 0;
		break;

	case DB_EVENT_REP_MASTER:
		GLOBAL(is_master) = 1;
		break;

	case DB_EVENT_REP_PERM_FAILED:
		printf("insufficient acks\n");
		GLOBAL(ackfails)++;
		break;

	case DB_EVENT_REP_STARTUPDONE: /* FALLTHROUGH */
	case DB_EVENT_REP_NEWMASTER:
		/* I don't care about these, for now. */
		break;

	default:
		dbenv->errx(dbenv, "ignoring event %d", which);
	}
}
