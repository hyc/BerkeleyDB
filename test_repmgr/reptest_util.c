/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "reptest_extern.h"

int	debug_on;				/* Enable checking. */
int	debug_print;				/* Display current counter. */
int	debug_stop;				/* Stop on each iteration. */
int	debug_test;				/* Stop on iteration N. */
static pthread_t *log_threads;			/* Log threads. */
static pthread_t *checkpoint_threads;		/* Checkpoint threads. */
static pthread_t *trickle_threads;		/* Trickle threads. */
static long	  percent;			/* Trickle percent. */

static void reptest_yield __P((u_long));
int log_pid;

/*
 * debug_check --
 *	Convenient way to set breakpoints.
 */
void
debug_check()
{
	if (debug_on == 0)
		return;

	if (debug_print != 0) {
		(void)fprintf(OUTFP, "\r%6d:", debug_on);
		fflush(OUTFP);
	}

	if (debug_on++ == debug_test || debug_stop)
		__db_loadme();
}

/*
 * random_int --
 *	Return a random integer.
 */
int
random_int(lo, hi)
	int lo, hi;
{
#ifndef	RAND_MAX
#define	RAND_MAX	0x7fffffff
#endif
	int ret, t;

	t = rand();
	if (t > RAND_MAX) {
		fprintf(OUTFP, "Max random is higher than %d\n", RAND_MAX);
		exit(EXIT_FAILURE);
	}
	ret = (int)(((double)t / ((double)(RAND_MAX) + 1)) * (hi - lo + 1));
	ret += lo;
	return (ret);
}

/*
 * random_data --
 *	Return random printable characters.
 */
void
random_data(p, len)
	char *p;
	size_t len;
{
	/*
	 * Generate a string of length len-1 and then set
	 * the trailing NUL in the last byte.
	 */
	while (--len > 0)
		*p++ = 'a' + random_int(0, 25);
	*p = '\0';
}

/*
 * set_yield --
 *	Deliberately yield the processor on every page request to maximize
 *	concurrency (and minimize throughput).
 */
void
set_yield()
{
	(void)GLOBAL(dbenv)->set_flags(GLOBAL(dbenv), DB_YIELDCPU, 1);
}

int
checkpoint_init()
{
	return ((checkpoint_threads = spawn_kids(
	    "checkpoint threads", 1, checkpoint_thread)) == NULL ? 1 : 0);
}

int
checkpoint_shutdown()
{
	return (wait_kids("checkpoint_threads", checkpoint_threads));
}

void *
checkpoint_thread(arg)
	void *arg;
{
	int ret;

	arg = 0;				/* UNUSED. */

	GLOBAL(dbenv)->errx(GLOBAL(dbenv), "Checkpoint thread: %lu",
	    (u_long)MY_ID);

	for (;;) {
		if ((ret = GLOBAL(dbenv)->txn_checkpoint(
		    GLOBAL(dbenv), 0, 0, 0)) != 0 && ret != EIO) {
			GLOBAL(dbenv)->err(GLOBAL(dbenv), ret,
			    "checkpoint thread: %s", db_strerror(ret));
			GLOBAL(shutdown) = 1;
			return (NULL);
		}

		if (GLOBAL(shutdown) == 1)
			return (NULL);

		GLOBAL(dbenv)->errx(GLOBAL(dbenv),
		    "checkpoint thread: %s",
		    ret == 0 ? "complete" : db_strerror(ret));

		/* 
		 * Don't have the support threads convoy on each other.
		 * Have them randomly sleep some period rather than
		 * a fixed amount of time.
		 */
		reptest_yield(random_int(5, 60));
		if (GLOBAL(shutdown) == 1)
			return (NULL);
	}
	/* NOTREACHED */
}

int
log_init()
{
	return ((log_threads =
	    spawn_kids("log threads", 1, log_thread)) == NULL ? 1 : 0);
}

int
log_shutdown()
{
	return (wait_kids("log threads", log_threads));
}

void *
log_thread(arg)
	void *arg;
{
	int save, ret;
	char **begin, **list, buf[4096];

	arg = 0;				/* UNUSED. */

	GLOBAL(dbenv)->errx(GLOBAL(dbenv), "Log cleaning thread: %lu",
	    (u_long)MY_ID);

	save = 0;
	for (;;) {
		if (GLOBAL(shutdown))
			return ((void *) 0);

		/* Get the list of log files. */
		if ((ret = GLOBAL(dbenv)->
		    log_archive(GLOBAL(dbenv), &list, 0)) != 0) {
			GLOBAL(dbenv)->err(GLOBAL(dbenv),
			    ret, "DB_ENV->log_archive list");
			ret = 1;
			goto out;
		}
		if (list != NULL) {
			for (begin = list; *list != NULL; ++list) {
				(void)snprintf(buf,
				    sizeof(buf), "%s/%s", GLOBAL(home), *list);
				if ((ret = unlink(buf)) == 0)
					GLOBAL(dbenv)->errx(GLOBAL(dbenv),
					    "logclean: remove %s", buf);
				else {
					GLOBAL(dbenv)->err(GLOBAL(dbenv), ret,
					    "logclean: remove %s", buf);
					GLOBAL(dbenv)->errx(GLOBAL(dbenv),
					    "logclean: Error remove %s", buf);
					ret = 1;
					goto out;
				}
			}
			free(begin);
		}
		/* 
		 * Don't have the support threads convoy on each other.
		 * Have them randomly sleep some period rather than
		 * a fixed amount of time.
		 */
		reptest_yield(random_int(5, 30));
	}

out:
	GLOBAL(shutdown) = 1;
	return ((void *)(uintptr_t)ret);
}

int
trickle_init(pct)
	long pct;
{
	if ((percent = pct) == 0)
		return (0);

	return ((trickle_threads =
	    spawn_kids("trickle threads", 1, trickle_thread)) == NULL ? 1 : 0);
}

int
trickle_shutdown()
{
	return (wait_kids("trickle_threads", trickle_threads));
}

void *
trickle_thread(arg)
	void *arg;
{
	int nwrote, ret;

	arg = 0;				/* UNUSED. */
	GLOBAL(dbenv)->errx(GLOBAL(dbenv),
	    "Trickle output thread: %lu",
	    (u_long)MY_ID);

	for (;;) {
		if (GLOBAL(shutdown))
			return (NULL);

		if ((ret = GLOBAL(dbenv)->
		    memp_trickle(GLOBAL(dbenv), (int)percent, &nwrote)) != 0) {
			GLOBAL(dbenv)->err(GLOBAL(dbenv), ret,
			    "trickle thread: %s", db_strerror(ret));
			GLOBAL(shutdown) = 1;
			return (NULL);
		}

		if (GLOBAL(verbose))
			GLOBAL(dbenv)->errx(GLOBAL(dbenv),
			    "trickle thread: wrote %d buffers", nwrote);

		/* 
		 * Don't have the support threads convoy on each other.
		 * Have them randomly sleep some period rather than
		 * a fixed amount of time.
		 */
		reptest_yield(random_int(1, 10));
	}
	/* NOTREACHED */
}

#define MAXWAIT	100000	/* .1 second */

/*
 * Sleep for some number of seconds.  However, we sleep in 1/10 second
 * intervals because we want to break out early if the shutdown flag
 * gets triggered.
 */
static void
reptest_yield(sec)
	u_long sec;
{
	u_long sleeptotal, total_usecs;

	total_usecs = sec * US_PER_SEC;
	sleeptotal = 0;
	while (sleeptotal < total_usecs) {
		__os_yield(NULL, 0, MAXWAIT);
		sleeptotal += MAXWAIT;
		if (GLOBAL(shutdown))
			return;
	}
	return;
}
