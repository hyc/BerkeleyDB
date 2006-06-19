/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: EventHandler.java,v 1.1 2006/05/04 05:24:05 alexg Exp $
 */
package com.sleepycat.db;
import com.sleepycat.db.EventType;

public interface EventHandler {
    int handleEvent(EventType event);
}
