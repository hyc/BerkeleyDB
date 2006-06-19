# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2006
#	Sleepycat Software.  All rights reserved.
#
# $Id: test081.tcl,v 12.2 2006/01/02 22:03:28 bostic Exp $
#
# TEST	test081
# TEST	Test off-page duplicates and overflow pages together with
# TEST	very large keys (key/data as file contents).
proc test081 { method {ndups 13} {tnum "081"} args} {
	source ./include.tcl

	eval {test017 $method 1 $ndups $tnum} $args
}
