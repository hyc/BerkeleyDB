/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: ReplicationHostAddress.java,v 12.1 2006/05/11 11:27:37 alexg Exp $
 */

package com.sleepycat.db;

public class ReplicationHostAddress
{
    public int port;
    public String host;
    public boolean isPeer;

    public ReplicationHostAddress()
    {
        this("localhost", 0, false);
    }

    public ReplicationHostAddress(String host, int port)
    {
        this(host, port, false);
    }

    public ReplicationHostAddress(String host, int port, boolean isPeer)
    {
        this.host = host;
        this.port = port;
        this.isPeer = isPeer;
    }
}
