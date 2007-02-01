# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004,2006 Oracle.  All rights reserved.
#
# $Id: rep070.tcl,v 1.2 2006/12/07 20:04:54 carol Exp $
#
# TEST	rep070
# TEST	Test of internal initialization and startup_done event.
# TEST
# TEST	Get a client into internal init. 
# TEST	Do not do any additional updates on the master and make
# TEST	sure that the client gets the startup_done event.
#
proc rep070 { method { niter 200 } { tnum "070" } args } {

	source ./include.tcl

	# Run for btree and queue only.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || [is_queue $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_btree $method] != 1 && [is_queue $method] != 1 } {
		puts "Skipping rep070 for method $method."
		return
	}

	set args [convert_args $method $args]
	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery,
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	#
	# 'user' means that the "app" (the test in this case) has
	# its own handle open to the database.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r $args): Test of\
			    internal initialization and startup_done."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep070_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep070_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
	global startp_done
	global rep_verbose
 
	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {rep on} "
	}
 
	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 4]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -log_max $log_max $verbargs -errpfx MASTER \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Set a low limit so that there are lots of reps between
	# master and client.  This allows greater control over
	# the test.
	error_check_good thr [$masterenv rep_limit 0 [expr 10 * 1024]] 0

	# Put some data into the database
	puts "\tRep$tnum.a: Run rep_test in master env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 0 $largs
	incr start $niter

	set stop 0
	set endlog 10
	while { $stop == 0 } {
		# Run test in the master (don't update client).
		eval rep_test $method \
		    $masterenv NULL $niter $start $start 0 0 $largs
		incr start $niter

		if { $m_logtype != "in-memory" } {
			set res \
			    [eval exec $util_path/db_archive -l -h $masterdir]
		}
		# Make sure the master has gone as far as we requested.
		set last_master_log [get_logfile $masterenv last]
		if { $last_master_log > $endlog } {
			set stop 1
		}
	}

	# Open a client
	puts "\tRep$tnum.c: Open client."
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -log_max $log_max $verbargs -errpfx CLIENT \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	set stop 0
	set client_endlog 5
	set last_client_log 0
	set nproced 0
	incr nproced [proc_msgs_once $envlist NONE err]
	incr nproced [proc_msgs_once $envlist NONE err]

	puts "\tRep$tnum.d: Client catches up partway."
	error_check_good ckp [$masterenv txn_checkpoint] 0

	# We have checkpointed on the master, but we want to get the
	# client a healthy way through the logs before archiving on
	# the master.
	while { $stop == 0 } {
		set nproced 0
		incr nproced [proc_msgs_once $envlist NONE err]
		if { $nproced == 0 } {
			error_check_good \
			    ckp [$masterenv txn_checkpoint -force] 0
		}

		# Stop processing when the client is partway through.
		if { $c_logtype != "in-memory" } {
			set res \
			    [eval exec $util_path/db_archive -l -h $clientdir]
		}
		set last_client_log [get_logfile $clientenv last]
		set first_client_log [get_logfile $clientenv first]
		if { $last_client_log > $client_endlog } {
			set stop 1
		}
	}

	# Now that the client is well on its way of normal processing,
	# simply fairly far behind the master, archive on the master,
	# removing the log files the client needs, sending it into
	# internal init with the database pages reflecting the client's
	# current LSN.
	#
	puts "\tRep$tnum.e: Force internal initialization."
	if { $m_logtype != "in-memory" } {
		puts "\tRep$tnum.e1: Archive on master."
		set res [eval exec $util_path/db_archive -d -h $masterdir]
	} else {
		# Master is in-memory, and we'll need a different
		# technique to create the gap forcing internal init.
		puts "\tRep$tnum.e1: Run rep_test until gap is created."
		set stop 0
		while { $stop == 0 } {
			eval rep_test $method $masterenv \
			    NULL $niter $start $start 0 0 $largs
			incr start $niter
			set first_master_log [get_logfile $masterenv first]
			if { $first_master_log > $last_client_log } {
				set stop 1
			}
		}
	}

	puts "\tRep$tnum.f: Process messages."
	#
	# Process messages once per loop.  Wait until we are
	# in startup (startup == 0) because the client currently
	# is not in startup, and won't be until it finds the master,
	# which may take a few messaging iterations.
	#
	while { 1 } {
		set nproced 0
		incr nproced [proc_msgs_once $envlist NONE err]
		set start [stat_field $clientenv rep_stat "Startup complete"]
		if { $nproced == 0 || $start == 0 } {
			break
		}
	}
	#
	# We should be in startup now.  Check that we didn't
	# just get through all messages without seeing
	# the client in startup.
	#
	error_check_good in_startup $start 0
	process_msgs $envlist

	#
	# Now that we're done with all messages, we should
	# see that startup is complete.
	#
	set start [stat_field $clientenv rep_stat "Startup complete"]
	error_check_good startup_over $start 1

	# We have now forced an internal initialization.  Verify it is correct.
	rep_verify $masterdir $masterenv $clientdir $clientenv 1

	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
