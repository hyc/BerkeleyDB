# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2006
#	Sleepycat Software.  All rights reserved.
#
# $Id: dead006.tcl,v 12.2 2006/01/02 22:03:12 bostic Exp $
#
# TEST	dead006
# TEST	use timeouts rather than the normal dd algorithm.
proc dead006 { { procs "2 4 10" } {tests "ring clump" } \
    {timeout 1000} {tnum 006} } {
	source ./include.tcl

	dead001 $procs $tests $timeout $tnum
	dead002 $procs $tests $timeout $tnum
}
