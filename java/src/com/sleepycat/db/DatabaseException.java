/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: DatabaseException.java,v 12.3 2006/07/14 07:45:57 mjc Exp $
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

public class DatabaseException extends Exception {
    private Environment dbenv;
    private int errno;

    public DatabaseException(final String s) {
        this(s, 0, (Environment)null);
    }

    public DatabaseException(final String s, final int errno) {
        this(s, errno, (Environment)null);
    }

    public DatabaseException(final String s,
                             final int errno,
                             final Environment dbenv) {
        super(s);
        this.errno = errno;
        this.dbenv = dbenv;
    }

    /* package */ DatabaseException(final String s,
                                final int errno,
                                final DbEnv dbenv) {
        this(s, errno, (dbenv == null) ? null : dbenv.wrapper);
    }

    public Environment getEnvironment() {
        return dbenv;
    }

    public int getErrno() {
        return errno;
    }

    public String toString() {
        String s = super.toString();
        if (errno != 0)
            s += ": " + DbEnv.strerror(errno);
        return s;
    }
}
