/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: ReplicationManagerAckPolicy.java,v 12.1 2006/05/11 11:27:38 alexg Exp $
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

public final class ReplicationManagerAckPolicy {

    public static final ReplicationManagerAckPolicy ALL =
        new ReplicationManagerAckPolicy("ALL", DbConstants.DB_REPMGR_ACKS_ALL);

    public static final ReplicationManagerAckPolicy ALL_PEERS =
        new ReplicationManagerAckPolicy(
        "ALL_PEERS", DbConstants.DB_REPMGR_ACKS_ALL_PEERS);

    public static final ReplicationManagerAckPolicy NONE =
        new ReplicationManagerAckPolicy(
        "NONE", DbConstants.DB_REPMGR_ACKS_NONE);

    public static final ReplicationManagerAckPolicy ONE =
        new ReplicationManagerAckPolicy("ONE", DbConstants.DB_REPMGR_ACKS_ONE);

    public static final ReplicationManagerAckPolicy ONE_PEER =
        new ReplicationManagerAckPolicy(
        "ONE_PEER", DbConstants.DB_REPMGR_ACKS_ONE_PEER);

    /* package */
    public static ReplicationManagerAckPolicy fromInt(int type) {
        switch(type) {
        case DbConstants.DB_REPMGR_ACKS_ALL:
            return ALL;
        case DbConstants.DB_REPMGR_ACKS_ALL_PEERS:
            return ALL_PEERS;
        case DbConstants.DB_REPMGR_ACKS_NONE:
            return NONE;
        case DbConstants.DB_REPMGR_ACKS_ONE:
            return ONE;
        case DbConstants.DB_REPMGR_ACKS_ONE_PEER:
            return ONE_PEER;
        default:
            throw new IllegalArgumentException(
                "Unknown ACK policy: " + type);
        }
    }

    private String statusName;
    private int id;

    private ReplicationManagerAckPolicy(final String statusName, final int id) {
        this.statusName = statusName;
        this.id = id;
    }

    /* package */
    int getId() {
        return id;
    }

    public String toString() {
        return "ReplicationManagerAckPolicy." + statusName;
    }
}

