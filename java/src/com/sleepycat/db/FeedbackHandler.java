/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2006 Oracle.  All rights reserved.
 *
 * $Id: FeedbackHandler.java,v 12.4 2006/11/01 00:53:30 bostic Exp $
 */
package com.sleepycat.db;

public interface FeedbackHandler {
    void recoveryFeedback(Environment dbenv, int percent);
    void upgradeFeedback(Database db, int percent);
    void verifyFeedback(Database db, int percent);
}
