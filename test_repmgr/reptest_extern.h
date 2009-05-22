/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1998-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <sys/wait.h>

#include <pthread.h>
#endif

#include "dbinc/db_dispatch.h"
#include "dbinc/db_page.h"
#include "dbinc_auto/crdel_auto.h"
#include "dbinc_auto/db_auto.h"
#include "dbinc_auto/db_ext.h"
#include "dbinc_auto/os_ext.h"

#define	DATABASE1	"am1.db"	/* Database name. */
#define	DATAHOME	"TESTDIR"	/* Environment home. */
#define	DONEFILE	"done"		/* End of workload file sentinel. */
#define	EXITFILE	"exit"		/* End of test file sentinel. */
#define	MARKERDB	"marker.db"	/* Marker file name. */
#define	MAXFAILS	500		/* Maximum DONE failures. */
#define	MAXRETRY	10		/* Maximum deadlock retries. */
#define	MSG_QUIET	3		/* 3 iterations of quiet msgs. */
#define	OUTFILE		"OUTPUT"	/* Output file */
#define	OUTFP		GLOBAL(outfp)
#define	RUNLOG		"RUN_LOG"	/* Log of execution */
#define	SHMKEY_DBS	30
#define	SUBDB1		"subdb1"	/* Subdatabase name. */

#define	MAXDATAREC	512
#define	MAXOPSTXN	1
#define	REP_NTHREADS	4
#define	IS_RECORD_BASED(M)						\
	(GLOBAL(recnum) || (M) == DB_QUEUE || (M) == DB_RECNO)

#define	MY_ID		pthread_self()
struct __dbs_globals {
	int	ackfails;			/* ack failures */
	DB_ENV  *dbenv;				/* Backing environment */
	int	 child_txn;			/* -z: use child txns */
	int	 debug;				/* -d: debug */
	int	 do_throttle;			/* Throttle workload? */
	long	 extentsize;			/* Size of queue extent. */
	char	*home;				/* -h: environment home */
	int	 hostport;			/* host/port is setup */
	int	 inmem;				/* named in-memory database */
	int	 is_master;			/* Am I master? */
	long	 iterations;			/* -i: iterations */
	int	 kill;				/* -k: kill site */
	DB	*markerdb;			/* marker file dbp */
	int	 myid;				/* My id counter */
	DBTYPE	 method;			/* -t: access method */
	long	 nthreads;			/* -c: # of threads */
	FILE	*outfp;				/* Output file FP */
	long	 pagesize;			/* -p: page size */
	char    *progname;			/* Program name. */
	char    *progpath;			/* Full program name. */
	int	 reclen;			/* record length. */
	int	 recnum;			/* use record numbers */
	long	 repth;				/* -m: # of msg threads */
	u_int	 seed;				/* random seed */
	int	 shutdown;			/* We're done. */
	int	 sortdups;			/* -D sorted duplicates */
	time_t	 start_time;			/* Workload start timestamp. */
	int	 subdb;				/* run in subdatabase */
	char	*test;				/* which am test we run. */
	int32_t  testsecs;			/* Seconds to run workload. */
	long	 trickle;			/* -T: trickle thread pct. */
	int	 verbose;			/* -v: verbose messages. */
};

extern struct __dbs_globals __dbs_globals;
#define	GLOBAL(x)	__dbs_globals.x

void	 debug_check(void);

typedef enum {
	HAND_OPEN,
	HAND_CREATE
} handle_mode;

int	 close_db_handle(DB **);
int	 close_env_handle(void);
int	 marker_file(DB**, handle_mode);
int	 open_db_handle(DB**, DBTYPE, handle_mode);
int	 open_env_handle(char *, u_int32_t, handle_mode);
void	 write_file(const char *);

int	 am_dup_compare(DB *, const DBT *, const DBT *);
int	 am_int_compare(DB *, const DBT *, const DBT *);
void	 am_check_err(int32_t, char *, int, DB_TXN *, DBC *, char *);
int32_t	 am_chksum(char *, int);
int	 am_init(void *(*)(void *));
int	 am_shutdown(void);
void	*am_thread(void *);
int	 queue_init(void);
void	*queue_thread(void *);

int	 checkpoint_init(void);
int	 checkpoint_shutdown(void);
void	*checkpoint_thread(void *);

int	 log_init(void);
int	 log_shutdown(void);
void	*log_thread(void *);

int	 trickle_init(long);
int	 trickle_shutdown(void);
void	*trickle_thread(void *);

extern int t_iter[1024];

void	 random_data(char *, size_t);
int	 random_int(int, int);

void	 set_yield(void);

ssize_t	 write_err(int, const void *, size_t);

#define	STROFFSET (2 * sizeof(int32_t))
struct data {
	u_int32_t id;
	int32_t sum;
	char str[MAXDATAREC - STROFFSET];
};

pthread_t *spawn_kids __P((char *, long, void *(*)(void *)));
int	 wait_kids __P((char *, pthread_t *));
int	 wait_procs __P((char *, int *));
