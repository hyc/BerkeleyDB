# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	repmgr003
# TEST	Basic repmgr internal init test. 
# TEST
# TEST	Start an appointed master site and two clients, processing 
# TEST	transactions between each additional site. Verify all expected 
# TEST	transactions are replicated.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr003 { method { niter 100 } { tnum "003" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		return btree
	}
	if { [is_btree $method] == 0 } {
		puts "Repmgr$tnum: skipping for non-btree method $method."
		return
	}

	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): Basic repmgr internal init test."
	repmgr003_sub $method $niter $tnum $args
}

proc repmgr003_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 3

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	# Open a master.
	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master

	puts "\tRepmgr$tnum.b: Run some transactions at master."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	# Open first client
	puts "\tRepmgr$tnum.c: Start first client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.d: Run some more transactions at master."
	eval rep_test $method $masterenv NULL $niter $niter 0 0 $largs

	# Open second client
	puts "\tRepmgr$tnum.e: Start second client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl_envcmd]
	$clientenv2 repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 5000000} \
	    -local [list localhost [lindex $ports 2]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -start client
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.f: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}
