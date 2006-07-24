# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2006
#	Sleepycat Software.  All rights reserved.
#
# $Id: rep056.tcl,v 1.5 2006/07/19 17:45:35 carol Exp $
#
# TEST  rep056
# TEST	Replication test with in-memory named databases.
# TEST
# TEST	Rep056 is just a driver to run rep001 with in-memory
# TEST	named databases.

proc rep056 { method args } {
	source ./include.tcl

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	eval { rep001 $method 1000 "056" } $args
}
