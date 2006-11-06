/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2006 Oracle.  All rights reserved.
 *
 * $Id: MultipleEntry.java,v 12.7 2006/11/01 00:53:30 bostic Exp $
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

import java.nio.ByteBuffer;

public abstract class MultipleEntry extends DatabaseEntry {
    /* package */ int pos;

    /* package */ MultipleEntry(final byte[] data, final int offset, final int size) {
        super(data, offset, size);
        setUserBuffer((data != null) ? (data.length - offset) : 0, true);
    }

    /* package */ MultipleEntry(final ByteBuffer data) {
        super(data);
    }

    public void setUserBuffer(final int length, final boolean usermem) {
        if (!usermem)
            throw new IllegalArgumentException("User buffer required");
        super.setUserBuffer(length, usermem);
    }
}
