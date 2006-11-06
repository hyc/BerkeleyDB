/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000,2006 Oracle.  All rights reserved.
 *
 * $Id: EventHandler.java,v 1.3 2006/11/01 00:53:30 bostic Exp $
 */
package com.sleepycat.db;
import com.sleepycat.db.EventType;

public interface EventHandler {
    int handleEvent(EventType event);
}
