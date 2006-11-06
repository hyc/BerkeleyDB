/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999,2006 Oracle.  All rights reserved.
 *
 * $Id: DeadlockException.java,v 12.5 2006/11/01 00:53:29 bostic Exp $
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
