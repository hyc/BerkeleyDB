# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2006
#	Sleepycat Software.  All rights reserved.
#
# $Id: test009.tcl,v 12.2 2006/01/02 22:03:25 bostic Exp $
#
# TEST	test009
# TEST	Small keys/large data
# TEST		Same as test008; close and reopen database
# TEST
# TEST	Check that we reuse overflow pages.  Create database with lots of
# TEST	big key/data pairs.  Go through and delete and add keys back
# TEST	randomly.  Then close the DB and make sure that we have everything
# TEST	we think we should.
proc test009 { method args} {
	eval {test008 $method "009" 0} $args
}
