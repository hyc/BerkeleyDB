/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001,2006 Oracle.  All rights reserved.
 *
 * $Id: Lock.java,v 12.5 2006/11/01 00:53:30 bostic Exp $
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbLock;

public final class Lock {
    private DbLock dbLock;

    private Lock(final DbLock inLock) {
        this.dbLock = inLock;
        inLock.wrapper = this;
    }

    /* package */
    static Lock wrap(final DbLock dblock) {
        return (dblock == null) ? null : new Lock(dblock);
    }

    /* package */
    DbLock unwrap() {
        return dbLock;
    }
}
