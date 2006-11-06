/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000,2006 Oracle.  All rights reserved.
 *
 * $Id: Hasher.java,v 12.4 2006/11/01 00:53:30 bostic Exp $
 */
package com.sleepycat.db;

public interface Hasher {
    int hash(Database db, byte[] data, int len);
}
