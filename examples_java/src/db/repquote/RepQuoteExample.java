/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: RepQuoteExample.java,v 1.2 2006/05/11 22:28:15 alexg Exp $
 */

package db.repquote;

import java.io.FileNotFoundException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.lang.Thread;
import java.lang.InterruptedException;

import com.sleepycat.db.*;
import db.repquote.RepConfig;

public class RepQuoteExample implements EventHandler
{
    private RepConfig appConfig;
    private RepQuoteEnvironment dbenv;

    public static void usage()
    {
        System.err.println("usage: " + RepConfig.progname); 
        System.err.println("[-h home][-o host:port][-m host:port]" + 
            "[-f host:port][-n nsites][-p priority]");
    
        System.err.println("\t -m host:port (required; m stands for me)\n" +
             "\t -o host:port (optional; o stands for other; any " +
             "number of these may bespecified)\n" +
             "\t -h home directory\n" +
             "\t -n nsites (optional; number of sites in replication " +
             "group; defaults to 0\n" +
             "\t    in which case we try to dynamically compute the " +
             "number of sites in\n" +
             "\t    the replication group)\n" +
             "\t -p priority (optional: defaults to 100)\n"); 

        System.exit(1);
    }

    public static void main(String[] argv)
        throws Exception
    {
        RepConfig config = new RepConfig();
        boolean isPeer;
        String tmpHost;
        int tmpPort = 0;
        // Extract the command line parameters
        for (int i = 0; i < argv.length; i++)
        {
            isPeer = false;
            if (argv[i] == "-C") {
                config.startPolicy = ReplicationManagerStartPolicy.REP_CLIENT;
            } else if (argv[i].compareTo("-F") == 0) {
                config.startPolicy = 
                    ReplicationManagerStartPolicy.REP_FULL_ELECTION;
            } else if (argv[i].compareTo("-h") == 0) {
                // home - a string arg.
                i++;
                config.home = argv[i];
            } else if (argv[i].compareTo("-M") == 0) {
                config.startPolicy = ReplicationManagerStartPolicy.REP_MASTER;
            } else if (argv[i].compareTo("-m") == 0) {
                // "me" should be host:port
                i++;
                String[] words = argv[i].split(":");
                if (words.length != 2) {
                    System.err.println(
                        "Invalid host specification host:port needed.");
                    usage();
                }
                try {
                    tmpPort = Integer.parseInt(words[1]);
                } catch (NumberFormatException nfe) {
                    System.err.println("Invalid host specification, " +
                        "could not parse port number.");
                    usage();
                }
                config.setThisHost(words[0], tmpPort);
            } else if (argv[i].compareTo("-n") == 0) {
                i++;
                config.totalSites = Integer.parseInt(argv[i]);
            } else if (argv[i].compareTo("-f") == 0 || 
                       argv[i].compareTo("-o") == 0) {
                if (argv[i] == "-f")
                    isPeer = true;
                i++;
                String[] words = argv[i].split(":");
                if (words.length != 2) {
                    System.err.println(
                        "Invalid host specification host:port needed.");
                    usage();
                }
                try {
                    tmpPort = Integer.parseInt(words[1]);
                } catch (NumberFormatException nfe) {
                    System.err.println("Invalid host specification, " +
                        "could not parse port number.");
                    usage();
                }
                config.addOtherHost(words[0], tmpPort, isPeer);
            } else if (argv[i].compareTo("-p") == 0) {
                i++;
                config.priority = Integer.parseInt(argv[i]);
            } else if (argv[i].compareTo("-v") == 0) {
                config.verbose = true;
            } else {
                System.err.println("Unrecognized option: " + argv[i]);
                usage();
            }

        }

        // Error check command line. 
        if ((!config.gotListenAddress()) || config.home.length() == 0)
            usage();

        RepQuoteExample runner = null;
        try {
            runner = new RepQuoteExample();
            runner.init(config);
            
            // Sleep to give ourselves time to find a master. 
            try {
                Thread.sleep(5000);
            } catch (InterruptedException e) {}

            runner.doloop();
            runner.terminate();
        } catch (DatabaseException dbe) {
            System.err.println("Caught an exception during " +
                "initialization or processing: " + dbe.toString());
            if (runner != null)
                runner.terminate();
        }
    } // end main

    public RepQuoteExample()
        throws DatabaseException
    {
        appConfig = null;
        dbenv = null;
    }

    public int init(RepConfig config)
        throws DatabaseException
    {
        int ret = 0;
        appConfig = config;
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setErrorStream(System.err);
        envConfig.setErrorPrefix(RepConfig.progname);

        envConfig.setReplicationManagerLocalSite(appConfig.getThisHost());
        for (ReplicationHostAddress host = appConfig.getFirstOtherHost();
            host != null; host = appConfig.getNextOtherHost())
        {
            envConfig.replicationManagerAddRemoteSite(host);
        }

        if (appConfig.totalSites > 0)
            envConfig.setReplicationNumSites(appConfig.totalSites);
        envConfig.setReplicationPriority(appConfig.priority);

         envConfig.setCacheSize(RepConfig.CACHESIZE);
        envConfig.setTxnNoSync(true);

        envConfig.setEventHandler(this);
        envConfig.setReplicationManagerAckPolicy(ReplicationManagerAckPolicy.ALL);

        envConfig.setAllowCreate(true);
        envConfig.setRunRecovery(true);
        envConfig.setThreaded(true);
        envConfig.setInitializeReplication(true);
        envConfig.setInitializeLocking(true);
        envConfig.setInitializeLogging(true);
        envConfig.setInitializeCache(true);
        envConfig.setTransactional(true);
        envConfig.setVerboseReplication(appConfig.verbose);
        try {
            dbenv = new RepQuoteEnvironment(appConfig.getHome(), envConfig);
        } catch(FileNotFoundException e) {
            System.err.println("FileNotFound exception: " + e.toString());
            System.err.println(
                "Ensure that the environment directory is pre-created.");
            ret = 1;
        }

        // start replication manager
        dbenv.replicationManagerStart(3, appConfig.startPolicy);
        return ret;
    }

    public int doloop()
        throws DatabaseException
    {
        boolean wasMaster = false;
        Database db = null;

        for (;;)
        {
            /*
             * Check whether I've changed from client to master or the
             * other way around.
             */
            if (dbenv.getIsMaster() != wasMaster) {
                if (db != null)
                    db.close(true); // close with noSync.
                db = null;
                wasMaster = dbenv.getIsMaster();
            }
            if (db == null) {
                DatabaseConfig dbconf = new DatabaseConfig();
                // Set page size small so page allocation is cheap. 
                dbconf.setPageSize(512);
                dbconf.setType(DatabaseType.BTREE);
                if (dbenv.getIsMaster()) {
                    dbconf.setAllowCreate(true);
                    dbconf.setTransactional(true);
                } else {
                    dbconf.setReadOnly(true);
                }

                try {
                    db = dbenv.openDatabase
                        (null, RepConfig.progname, null, dbconf);
                } catch (java.io.FileNotFoundException e) {
                    System.err.println("no stock database available yet.");
                    db.close(true);
                    db = null;
                    try {
                        Thread.sleep(RepConfig.SLEEPTIME);
                    } catch (InterruptedException ie) {}
                    continue;
                }
            }

            BufferedReader stdin = 
                new BufferedReader(new InputStreamReader(System.in));
            if (dbenv.getIsMaster()) {
                // listen for input, and add it to the database.
                System.out.print("QUOTESERVER> ");
                System.out.flush();
                String nextline = null;
                try {
                    nextline = stdin.readLine();
                } catch (IOException ioe) {
                    System.err.println("Unable to get data from stdin");
                    break;
                }
                String[] words = nextline.split("\\s");

                if (words.length == 1 && 
                    (words[0].compareToIgnoreCase("quit") == 0 || 
                    words[0].compareToIgnoreCase("exit") == 0)) {
                    break;
                } else if (words.length != 2) {
                    System.err.println("Format: TICKER VALUE");
                    continue;
                }

                DatabaseEntry key = new DatabaseEntry(words[0].getBytes());
                DatabaseEntry data = new DatabaseEntry(words[1].getBytes());

                db.put(null, key, data);
            } else {
                try {
                    Thread.sleep(RepConfig.SLEEPTIME);
                } catch (InterruptedException ie) {}

                Cursor dbc = db.openCursor(null, null);
                try {
                    OperationStatus ret = printStocks(dbc);
                } catch (DeadlockException de) {
                    dbc.close();
                    continue;
                } catch (DatabaseException e) {
                    // this could be DB_REP_HANDLE_DEAD
                    // should close the database and continue
                    System.err.println("Got db exception reading replication" +
                        "DB: " + e.toString());
                    System.err.println("Expected if it was due to a dead " +
                        "replication handle, otherwise an unexpected error.");
                    dbc.close();
                    db.close(true); // close no sync.
                    db = null;
                    continue;
                }
                dbc.close();
            }
        }
        if (db != null)
            db.close(true);
        return 0;
    }

    public void terminate()
        throws DatabaseException
    {
    }

    public int handleEvent(EventType event)
    {
        int ret = 0;
        if (event == EventType.REP_MASTER)
            dbenv.setIsMaster(true);
        else if (event == EventType.REP_CLIENT)
            dbenv.setIsMaster(false);
        else if (event == EventType.REP_NEW_MASTER) {
            // ignored for now.
        } else {
            System.err.println("Unknown event callback received.\n");
            ret = 1;
        }
        return ret;
    }

    private OperationStatus printStocks(Cursor dbc)
        throws DeadlockException, DatabaseException
    {
        System.out.println("\tSymbol\tPrice");
        System.out.println("\t======\t=====");

        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry data = new DatabaseEntry();
        OperationStatus ret;
        for (ret = dbc.getFirst(key, data, LockMode.DEFAULT);
            ret == OperationStatus.SUCCESS;
            ret = dbc.getNext(key, data, LockMode.DEFAULT)) {
            String keystr = new String
                (key.getData(), key.getOffset(), key.getSize());
            String datastr = new String
                (data.getData(), data.getOffset(), data.getSize());
            System.out.println("\t"+keystr+"\t"+datastr);

        }
        // other return types are propogated by exception
        return OperationStatus.SUCCESS;
    }
} // end class

