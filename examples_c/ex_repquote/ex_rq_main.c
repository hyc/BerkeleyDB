/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: ex_rq_main.c,v 12.9 2006/06/02 18:36:34 alanb Exp $
 */

#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <db.h>

#define	CACHESIZE	(10 * 1024 * 1024)
#define	DATABASE	"quote.db"
#define	SLEEPTIME	3

const char *progname = "ex_repquote";

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
extern int getopt(int, char * const *, const char *);
#define	sleep(s)		Sleep(1000 * (s))
#endif

typedef struct {
	int is_master;
} APP_DATA;

int doloop __P((DB_ENV *, APP_DATA *));
static void event_callback __P((DB_ENV *, u_int32_t, void *));

/*
 * In this application, we specify all communication via the command line.  In
 * a real application, we would expect that information about the other sites
 * in the system would be maintained in some sort of configuration file.  The
 * critical part of this interface is that we assume at startup that we can
 * find out
 * 	1) what host/port we wish to listen on for connections,
 * 	2) a (possibly empty) list of other sites we should attempt to connect
 * 	to; and
 * 	3) what our Berkeley DB home environment is.
 *
 * These pieces of information are expressed by the following flags.
 * -m host:port (required; m stands for me)
 * -o host:port (optional; o stands for other; any number of these may be
 *	specified)
 * -h home directory
 * -n nsites (optional; number of sites in replication group; defaults to 0
 *	in which case we try to dynamically compute the number of sites in
 *	the replication group)
 * -p priority (optional: defaults to 100)
 */
static void
usage()
{
	fprintf(stderr, "usage: %s ", progname);
	fprintf(stderr, "[-h home][-o host:port][-m host:port]%s",
	    "[-n nsites][-p priority]\n");
	exit(EXIT_FAILURE);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	DB_ENV *dbenv;
	const char *home;
	char ch, *host, *portstr;
	int ret, totalsites, t_ret, verbose, got_listen_address, friend;
	u_int16_t port;
	APP_DATA my_app_data;
	u_int32_t start_policy;
	int priority;

	my_app_data.is_master = 0; /* assume I start out as client */
	dbenv = NULL;
	ret = verbose = got_listen_address = 0;
	home = "TESTDIR";

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr, "Error creating env handle: %s\n",
		    db_strerror(ret));
		goto err;
	}
	dbenv->app_private = &my_app_data;

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);
	dbenv->set_event_notify(dbenv, event_callback);

	start_policy = DB_REP_ELECTION;	/* default */
	priority = 100;		/* default */

	while ((ch = getopt(argc, argv, "CFf:h:Mm:n:o:p:v")) != EOF) {
		friend = 0;
		switch (ch) {
		case 'C':
			start_policy = DB_REP_CLIENT;
			break;
		case 'F':
			start_policy = DB_REP_FULL_ELECTION;
			break;
		case 'h':
			home = optarg;
			break;
		case 'M':
			start_policy = DB_REP_MASTER;
			break;
		case 'm':
			host = strtok(optarg, ":");
			if ((portstr = strtok(NULL, ":")) == NULL) {
				fprintf(stderr, "Bad host specification.\n");
				goto err;
			}
			port = (unsigned short)atoi(portstr);
			if ((ret = dbenv->repmgr_set_local_site(dbenv, host, port, 0))
			    != 0) {
				fprintf(stderr,
				    "Could not set listen address (%d).\n", ret);
				goto err;
			}
			got_listen_address = 1;
			break;
		case 'n':
			totalsites = atoi(optarg);
			if ((ret = dbenv->rep_set_nsites(dbenv, totalsites)) != 0)
				dbenv->err(dbenv, ret, "set_nsites");
			break;
		case 'f':
			friend = 1; /* FALLTHROUGH */
		case 'o':
			host = strtok(optarg, ":");
			if ((portstr = strtok(NULL, ":")) == NULL) {
				fprintf(stderr, "Bad host specification.\n");
				goto err;
			}
			port = (unsigned short)atoi(portstr);
			if ((ret = dbenv->repmgr_add_remote_site(dbenv, host,
			    port, friend ? DB_REPMGR_PEER : 0)) != 0) {
				dbenv->err(dbenv, ret,
				    "Could not add site %s:%d", host,
				    (int)port);
				goto err;
			}
			break;
		case 'p':
			priority = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	
	/* Error check command line. */
	if ((!got_listen_address) || home == NULL)
		usage();

	dbenv->rep_set_priority(dbenv, priority);

	/*
	 * We can now open our environment, although we're not ready to
	 * begin replicating.  However, we want to have a dbenv around
	 * so that we can send it into any of our message handlers.
	 */
	(void)dbenv->set_cachesize(dbenv, 0, CACHESIZE, 0);
	(void)dbenv->set_flags(dbenv, DB_TXN_NOSYNC, 1);

	if ((ret = dbenv->open(dbenv, home, DB_CREATE | DB_RECOVER | DB_THREAD |
	    DB_INIT_REP | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL |
	    DB_INIT_TXN, 0)) != 0)
		goto err;

	if (verbose &&
	    (ret = dbenv->set_verbose(dbenv, DB_VERB_REPLICATION, 1)) != 0)
		goto err;
	
	if ((ret = dbenv->repmgr_start(dbenv, 3, start_policy)) != 0)
		goto err;

	/* Sleep to give ourselves time to find a master. */
	sleep(5);

	if ((ret = doloop(dbenv, &my_app_data)) != 0) {
		dbenv->err(dbenv, ret, "Client failed");
		goto err;
	}

err:	
	if (dbenv != NULL &&
	    (t_ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "failure closing env: %s (%d)\n",
		    db_strerror(t_ret), t_ret);
		if (ret == 0)
			ret = t_ret;
	}

	return (ret);
}

static void
event_callback(dbenv, which, info)
	DB_ENV *dbenv;
	u_int32_t which;
	void *info;
{
	APP_DATA *app = dbenv->app_private;

	info = NULL;				/* Currently unused. */

	switch (which) {
	case DB_EVENT_REP_MASTER:
		app->is_master = 1;
		break;

	case DB_EVENT_REP_CLIENT:
		app->is_master = 0;
		break;

	case DB_EVENT_REP_NEWMASTER:
		/* I don't care about this one, for now. */
		break;

	default:
		dbenv->errx(dbenv, "ignoring event %d", which);
	}
}

static int
print_stocks(dbc)
	DBC *dbc;
{
	DBT key, data;
#define	MAXKEYSIZE	10
#define	MAXDATASIZE	20
	char keybuf[MAXKEYSIZE + 1], databuf[MAXDATASIZE + 1];
	int ret;
	u_int32_t keysize, datasize;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	printf("\tSymbol\tPrice\n");
	printf("\t======\t=====\n");

	for (ret = dbc->c_get(dbc, &key, &data, DB_FIRST);
	    ret == 0;
	    ret = dbc->c_get(dbc, &key, &data, DB_NEXT)) {
		keysize = key.size > MAXKEYSIZE ? MAXKEYSIZE : key.size;
		memcpy(keybuf, key.data, keysize);
		keybuf[keysize] = '\0';

		datasize = data.size >= MAXDATASIZE ? MAXDATASIZE : data.size;
		memcpy(databuf, data.data, datasize);
		databuf[datasize] = '\0';

		printf("\t%s\t%s\n", keybuf, databuf);
	}
	printf("\n");
	fflush(stdout);
	return (ret == DB_NOTFOUND ? 0 : ret);
}

#define	BUFSIZE 1024

int
doloop(dbenv, app_data)
	DB_ENV *dbenv;
	APP_DATA *app_data;
{
	DB *dbp;
	DBC *dbc;
	DBT key, data;
	char buf[BUFSIZE], *rbuf;
	int ret, t_ret, was_master;

	dbp = NULL;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	ret = was_master = 0;

	for (;;) {
		/*
		 * Check whether I've changed from client to master or the
		 * other way around.
		 */
		if (app_data->is_master != was_master) {
			if (dbp != NULL)
				(void)dbp->close(dbp, DB_NOSYNC);
			dbp = NULL;
			was_master = app_data->is_master;
		}
		
		if (dbp == NULL) {
			if ((ret = db_create(&dbp, dbenv, 0)) != 0)
				return (ret);

			/* Set page size small so page allocation is cheap. */
			if ((ret = dbp->set_pagesize(dbp, 512)) != 0)
				goto err;
		
			if ((ret = dbp->open(dbp, NULL, DATABASE,
			    NULL, DB_BTREE,
			    app_data->is_master ? DB_CREATE | DB_AUTO_COMMIT :
			    DB_RDONLY, 0)) != 0) {
				if (ret == ENOENT) {
					printf(
					  "No stock database yet available.\n");
					if ((ret = dbp->close(dbp, 0)) != 0) {
						dbenv->err(dbenv, ret,
						    "DB->close");
						goto err;
					}
					dbp = NULL;
					sleep(SLEEPTIME);
					continue;
				}
				dbenv->err(dbenv, ret, "DB->open");
				goto err;
			}
		}

		if (app_data->is_master) {
			printf("QUOTESERVER> ");
			fflush(stdout);
		
			if (fgets(buf, sizeof(buf), stdin) == NULL)
				break;
			(void)strtok(&buf[0], " \t\n");
			rbuf = strtok(NULL, " \t\n");
			if (rbuf == NULL || rbuf[0] == '\0') {
				if (strncmp(buf, "exit", 4) == 0 ||
				    strncmp(buf, "quit", 4) == 0)
					break;
				dbenv->errx(dbenv, "Format: TICKER VALUE");
				continue;
			}
		
			key.data = buf;
			key.size = (u_int32_t)strlen(buf);
		
			data.data = rbuf;
			data.size = (u_int32_t)strlen(rbuf);

			if ((ret = dbp->put(dbp, NULL, &key, &data,
			    DB_AUTO_COMMIT)) != 0)
			{
				dbp->err(dbp, ret, "DB->put");
				if (ret != DB_KEYEXIST)
					goto err;
			}
		} else {
			sleep(SLEEPTIME);

			dbc = NULL;
			if ((ret = dbp->cursor(dbp, NULL, &dbc, 0)) != 0 ||
			    (ret = print_stocks(dbc)) != 0) {
				dbenv->err(dbenv, ret, "Error traversing data");
				goto err;
			}
			if (dbc != NULL &&
			    (t_ret = dbc->c_close(dbc)) != 0 && ret == 0)
				ret = t_ret;

			switch (ret) {
			case 0:
			case DB_LOCK_DEADLOCK:
				continue;
			case DB_REP_HANDLE_DEAD:
				if (dbp != NULL) {
					(void)dbp->close(dbp, DB_NOSYNC);
					dbp = NULL;
				}
				continue;
			default:
				goto err;
			}
		}
	}

err:	if (dbp != NULL)
		(void)dbp->close(dbp, DB_NOSYNC);

	return (ret);
}
