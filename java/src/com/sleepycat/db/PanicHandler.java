/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2006 Oracle.  All rights reserved.
 *
 * $Id: PanicHandler.java,v 12.4 2006/11/01 00:53:30 bostic Exp $
 */
package com.sleepycat.db;

public interface PanicHandler {
    void panic(Environment dbenv, DatabaseException e);
}
