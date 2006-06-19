# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2006
#	Sleepycat Software.  All rights reserved.
#
# $Id: fop008.tcl,v 12.2 2006/01/02 22:03:14 bostic Exp $
#
# TEST	fop008
# TEST	Test file system operations on named in-memory databases.
# TEST	Combine two ops in one transaction.
proc fop008 { method args } {
	eval {fop006 $method 1} $args
}



