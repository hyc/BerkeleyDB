/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "reptest_extern.h"

/*
 * spawn_kids --
 *	Create an array of N threads.
 */
#define	KTYPE		pthread_t
pthread_t *
spawn_kids(msg, nthreads, func)
	char *msg;
	long nthreads;
	void *(*func)__P((void *));
{
	pthread_t *kidsp;
	long i;

	/* Spawn off threads. */
	if ((kidsp =
	    (KTYPE *)calloc(sizeof(KTYPE), nthreads + 1)) == NULL) {
		fprintf(OUTFP, "%s: %s\n", msg, strerror(errno));
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < nthreads; i++) {
		if ((errno =
		    pthread_create(&kidsp[i], NULL, func, (void *)i)) != 0) {
			fprintf(OUTFP,
			    "%s: failed spawning thread %ld %s\n",
			    msg, i, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	kidsp[nthreads] = (KTYPE)NULL;

	return (kidsp);
}

/*
 * wait_kids --
 *	Wait for an array of N threads.
 */
int
wait_kids(msg, kidsp)
	char *msg;
	KTYPE *kidsp;
{
	int i, status;
	void *retp;

	status = 0;
	for (i = 0; kidsp[i] != (pthread_t)NULL; i++) {
		pthread_join(kidsp[i], &retp);
		if (retp != NULL) {
			status = 1;
			fprintf(OUTFP,
			    "%s: child %ld exited with error\n", msg, (long)i);
		}
	}

	free(kidsp);
	return (status);
}
