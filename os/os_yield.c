/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: os_yield.c,v 12.9 2006/05/19 19:25:18 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

#ifndef NO_SYSTEM_INCLUDES
#if defined(HAVE_SCHED_YIELD)
#include <sched.h>
#endif
#endif

/*
 * __os_yield --
 *	Yield the processor.
 *
 * PUBLIC: void __os_yield __P((DB_ENV *));
 */
void
__os_yield(dbenv)
	DB_ENV *dbenv;
{
	if (DB_GLOBAL(j_yield) != NULL) {
		(void)DB_GLOBAL(j_yield)();
		return;
	}

#if defined(HAVE_VXWORKS)
	taskDelay(1);
#elif defined(HAVE_MUTEX_UI_THREADS)
	thr_yield();
#elif defined(HAVE_PTHREAD_YIELD) &&					\
    (defined(HAVE_MUTEX_PTHREADS) || defined(HAVE_PTHREAD_API))
	pthread_yield();
#elif defined(HAVE_SCHED_YIELD)
	(void)sched_yield();
#elif defined(HAVE_YIELD)
	yield();
#else
	__os_sleep(dbenv, 0, 1);
#endif
	COMPQUIET(dbenv, NULL);
}
