/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: DeadlockException.java,v 12.3 2006/07/14 07:45:57 mjc Exp $
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

public class DeadlockException extends DatabaseException {
    /* package */ DeadlockException(final String s,
                                final int errno,
                                final DbEnv dbenv) {
        super(s, errno, dbenv);
    }
}
