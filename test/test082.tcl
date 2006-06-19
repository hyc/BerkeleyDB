# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2006
#	Sleepycat Software.  All rights reserved.
#
# $Id: test082.tcl,v 12.2 2006/01/02 22:03:28 bostic Exp $
#
# TEST	test082
# TEST	Test of DB_PREV_NODUP (uses test074).
proc test082 { method {dir -prevnodup} {nitems 100} {tnum "082"} args} {
	source ./include.tcl

	eval {test074 $method $dir $nitems $tnum} $args
}
