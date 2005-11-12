/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: Timer.java,v 12.1 2005/06/16 20:23:43 bostic Exp $
 */

package com.sleepycat.db.rpcserver;

/**
 * Class to keep track of access times. This is slightly devious by having
 * both the access_time and a reference to another Timer that can be
 * used to group/share access times. This is done to keep the Java code
 * close to the canonical C implementation of the RPC server.
 */
public class Timer {
    Timer timer = this;
    long last_access;
}
