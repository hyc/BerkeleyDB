/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: fop.h,v 12.5 2006/07/05 05:37:09 mjc Exp $
 */

#ifndef	_DB_FOP_H_
#define	_DB_FOP_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define	MAKE_INMEM(D) do {					\
	F_SET((D), DB_AM_INMEM);				\
	(void)__memp_set_flags((D)->mpf, DB_MPOOL_NOFILE, 1);	\
} while (0)

#include "dbinc_auto/fileops_auto.h"
#include "dbinc_auto/fileops_ext.h"

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_FOP_H_ */
