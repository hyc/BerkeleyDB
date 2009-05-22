# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	repmgr002
# TEST	Basic repmgr election test. 
# TEST
# TEST	Open three clients of different priorities and make sure repmgr 
# TEST	elects expected master. Shut master down, make sure repmgr elects 
# TEST	expected remaining client master, make sure former master can join 
# TEST	as client.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr002 { method { niter 100 } { tnum "002" } args } {

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

	puts "Repmgr$tnum ($method): Basic repmgr election test."
	repmgr002_sub $method $niter $tnum $args
}

proc repmgr002_sub { method niter tnum largs } {
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

	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2
	set clientdir3 $testdir/CLIENTDIR3

	file mkdir $clientdir
	file mkdir $clientdir2
	file mkdir $clientdir3

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	puts "\tRepmgr$tnum.a: Start three clients."

	# Open first client
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites -pri 100 \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start elect

	# Open second client
	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all -nsites $nsites -pri 30 \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start elect

	# Open third client
	set cl3_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT3 -home $clientdir3 -txn -rep -thread"
	set clientenv3 [eval $cl3_envcmd]
	$clientenv3 repmgr -ack all -nsites $nsites -pri 20 \
	    -timeout {conn_retry 5000000} \
	    -local [list localhost [lindex $ports 2]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -start elect

	puts "\tRepmgr$tnum.b: Elect first client master."
	await_expected_master $clientenv
	set masterenv $clientenv
	set masterdir $clientdir
	await_startup_done $clientenv2
	await_startup_done $clientenv3

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.c: Run some transactions at master."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	puts "\tRepmgr$tnum.d: Verify client database contents."
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1
	rep_verify $masterdir $masterenv $clientdir3 $clientenv3 1 1 1

	puts "\tRepmgr$tnum.e: Shut down master, elect second client master."
	error_check_good client_close [$clientenv close] 0
	await_expected_master $clientenv2
	set masterenv $clientenv2
	await_startup_done $clientenv3

	puts "\tRepmgr$tnum.f: Restart former master as client."
	# Open -recover to clear env region, including startup_done value.
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all -nsites $nsites -pri 100 \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.g: Run some transactions at new master."
	eval rep_test $method $masterenv NULL $niter $niter 0 0 $largs

	puts "\tRepmgr$tnum.h: Verify client database contents."
	set masterdir $clientdir2
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir3 $clientenv3 1 1 1

	error_check_good client3_close [$clientenv3 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good client2_close [$clientenv2 close] 0
}
