/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2006 Oracle.  All rights reserved.
 *
 * $Id: SecondaryKeyCreator.java,v 12.4 2006/11/01 00:53:30 bostic Exp $
 */

package com.sleepycat.db;

public interface SecondaryKeyCreator {
    boolean createSecondaryKey(SecondaryDatabase secondary,
                                      DatabaseEntry key,
                                      DatabaseEntry data,
                                      DatabaseEntry result)
        throws DatabaseException;
}
