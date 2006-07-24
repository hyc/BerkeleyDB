/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: MultipleEntry.java,v 12.5 2006/07/14 07:45:58 mjc Exp $
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
