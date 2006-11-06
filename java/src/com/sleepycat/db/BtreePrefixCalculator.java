/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001,2006 Oracle.  All rights reserved.
 *
 * $Id: BtreePrefixCalculator.java,v 12.4 2006/11/01 00:53:28 bostic Exp $
 */

package com.sleepycat.db;

public interface BtreePrefixCalculator {
    int prefix(Database db, DatabaseEntry dbt1, DatabaseEntry dbt2);
}
