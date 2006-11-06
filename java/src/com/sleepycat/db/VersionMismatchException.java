/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2006 Oracle.  All rights reserved.
 *
 * $Id: VersionMismatchException.java,v 1.6 2006/11/01 00:53:30 bostic Exp $
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

public class VersionMismatchException extends DatabaseException {
    /* package */ VersionMismatchException(final String s,
                                   final int errno,
                                   final DbEnv dbenv) {
        super(s, errno, dbenv);
    }
}
