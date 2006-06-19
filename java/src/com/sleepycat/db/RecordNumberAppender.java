/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: RecordNumberAppender.java,v 12.2 2006/01/02 22:02:37 bostic Exp $
 */
package com.sleepycat.db;

public interface RecordNumberAppender {
    void appendRecordNumber(Database db, DatabaseEntry data, int recno)
        throws DatabaseException;
}
