# See the file LICENSE for redistribution information.
#
# Copyright (c) 1998-2006
#	Sleepycat Software.  All rights reserved.
#
# $Id: test034.tcl,v 12.2 2006/01/02 22:03:26 bostic Exp $
#
# TEST	test034
# TEST	test032 with off-page duplicates
# TEST	DB_GET_BOTH, DB_GET_BOTH_RANGE functionality with off-page duplicates.
proc test034 { method {nentries 10000} args} {
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test034: Skipping for specific pagesizes"
		return
	}
	# Test with off-page duplicates
	eval {test032 $method $nentries 20 "034" -pagesize 512} $args

	# Test with multiple pages of off-page duplicates
	eval {test032 $method [expr $nentries / 10] 100 "034" -pagesize 512} \
	    $args
}
