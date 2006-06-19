/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: Hasher.java,v 12.2 2006/01/02 22:02:34 bostic Exp $
 */
package com.sleepycat.db;

public interface Hasher {
    int hash(Database db, byte[] data, int len);
}
