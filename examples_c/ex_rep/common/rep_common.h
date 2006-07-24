/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: rep_common.h,v 12.1 2006/07/07 23:36:10 alanb Exp $
 */

typedef struct {
	int is_master;
	void *comm_infrastructure;
} APP_DATA;

int create_env __P((const char *progname, DB_ENV **));
int doloop __P((DB_ENV *, APP_DATA *));
int env_init __P((DB_ENV *, const char *));
void usage __P((const char *));
