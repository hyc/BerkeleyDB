/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: env_globals.c,v 1.1 2006/11/09 14:15:30 bostic Exp $
 */

#include "db_config.h"

/*
 * This is the file that initializes the global array.  Do it this way because
 * people keep changing the structure without changing the initialization code.
 * Having declaration and initialization in one file will hopefully fix that.
 */
#define	DB_INITIALIZE_DB_GLOBALS	1

#include "db_int.h"
