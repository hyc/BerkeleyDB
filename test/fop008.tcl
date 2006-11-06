# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005,2006 Oracle.  All rights reserved.
#
# $Id: fop008.tcl,v 12.4 2006/11/01 00:53:53 bostic Exp $
#
# TEST	fop008
# TEST	Test file system operations on named in-memory databases.
# TEST	Combine two ops in one transaction.
proc fop008 { method args } {
	eval {fop006 $method 1} $args
}



