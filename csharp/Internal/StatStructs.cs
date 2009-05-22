using System;
using System.Runtime.InteropServices;

namespace BerkeleyDB.Internal {
    /* 
     * These structures are laid out identically to their C library
     * counterparts.  They will need to be kept in sync with the C structures,
     * but should not be re-organized on their own, correct marshalling depends
     * upon them matching byte for byte.
     */
    [StructLayout(LayoutKind.Sequential)]
    internal struct BTreeStatStruct {
        internal uint bt_magic;
        internal uint bt_version;
        internal uint bt_metaflags;
        internal uint bt_nkeys;
        internal uint bt_ndata;
        internal uint bt_pagecnt;
        internal uint bt_pagesize;
        internal uint bt_minkey;
        internal uint bt_re_len;
        internal uint bt_re_pad;
        internal uint bt_levels;
        internal uint bt_int_pg;
        internal uint bt_leaf_pg;
        internal uint bt_dup_pg;
        internal uint bt_over_pg;
        internal uint bt_empty_pg;
        internal uint bt_free;
        internal long bt_int_pgfree;
        internal long bt_leaf_pgfree;
        internal long bt_dup_pgfree;
        internal long bt_over_pgfree;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct HashStatStruct {
        internal uint hash_magic;
        internal uint hash_version;
        internal uint hash_metaflags;
        internal uint hash_nkeys;
        internal uint hash_ndata;
        internal uint hash_pagecnt;
        internal uint hash_pagesize;
        internal uint hash_ffactor;
        internal uint hash_buckets;
        internal uint hash_free;
        internal long hash_bfree;
        internal uint hash_bigpages;
        internal long hash_big_bfree;
        internal uint hash_overflows;
        internal long hash_ovfl_free;
        internal uint hash_dup;
        internal long hash_dup_free;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LockStatStruct {
        internal uint st_id;
        internal uint st_cur_maxid;
        internal uint st_maxlocks;
        internal uint st_maxlockers;
        internal uint st_maxobjects;
        internal uint st_partitions;
        internal int st_nmodes;
        internal uint st_nlockers;
        internal uint st_nlocks;
        internal uint st_maxnlocks;
        internal uint st_maxhlocks;
        internal long st_locksteals;
        internal long st_maxlsteals;
        internal uint st_maxnlockers;
        internal uint st_nobjects;
        internal uint st_maxnobjects;
        internal uint st_maxhobjects;
        internal long st_objectsteals;
        internal long st_maxosteals;
        internal long st_nrequests;
        internal long st_nreleases;
        internal long st_nupgrade;
        internal long st_ndowngrade;
        internal long st_lock_wait;
        internal long st_lock_nowait;
        internal long st_ndeadlocks;
        internal uint st_locktimeout;
        internal long st_nlocktimeouts;
        internal uint st_txntimeout;
        internal long st_ntxntimeouts;
        internal long st_part_wait;
        internal long st_part_nowait;
        internal long st_part_max_wait;
        internal long st_part_max_nowait;
        internal long st_objs_wait;
        internal long st_objs_nowait;
        internal long st_lockers_wait;
        internal long st_lockers_nowait;
        internal long st_region_wait;
        internal long st_region_nowait;
        internal uint st_hash_len;
        internal uint st_regsize;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LogStatStruct {
        internal uint st_magic;
        internal uint st_version;
        internal int st_mode;
        internal uint st_lg_bsize;
        internal uint st_lg_size;
        internal uint st_wc_bytes;
        internal uint st_wc_mbytes;
        internal long st_record;
        internal uint st_w_bytes;
        internal uint st_w_mbytes;
        internal long st_wcount;
        internal long st_wcount_fill;
        internal long st_rcount;
        internal long st_scount;
        internal long st_region_wait;
        internal long st_region_nowait;
        internal uint st_cur_file;
        internal uint st_cur_offset;
        internal uint st_disk_file;
        internal uint st_disk_offset;
        internal uint st_maxcommitperflush;
        internal uint st_mincommitperflush;
        internal uint st_regsize;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct MPoolFileStatStruct {
        internal string file_name;
        internal uint st_pagesize;
        internal uint st_map;
        internal long st_cache_hit;
        internal long st_cache_miss;
        internal long st_page_create;
        internal long st_page_in;
        internal long st_page_out;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct MPoolStatStruct {
        internal uint st_gbytes;
        internal uint st_bytes;
        internal uint st_ncache;
        internal uint st_max_ncache;
        internal uint st_mmapsize;
        internal int st_maxopenfd;
        internal int st_maxwrite;
        internal uint st_maxwrite_sleep;
        internal uint st_pages;
        internal uint st_map;
        internal long st_cache_hit;
        internal long st_cache_miss;
        internal long st_page_create;
        internal long st_page_in;
        internal long st_page_out;
        internal long st_ro_evict;
        internal long st_rw_evict;
        internal long st_page_trickle;
        internal uint st_page_clean;
        internal uint st_page_dirty;
        internal uint st_hash_buckets;
        internal uint st_hash_searches;
        internal uint st_hash_longest;
        internal long st_hash_examined;
        internal long st_hash_nowait;
        internal long st_hash_wait;
        internal long st_hash_max_nowait;
        internal long st_hash_max_wait;
        internal long st_region_nowait;
        internal long st_region_wait;
        internal long st_mvcc_frozen;
        internal long st_mvcc_thawed;
        internal long st_mvcc_freed;
        internal long st_alloc;
        internal long st_alloc_buckets;
        internal long st_alloc_max_buckets;
        internal long st_alloc_pages;
        internal long st_alloc_max_pages;
        internal long st_io_wait;
        internal uint st_regsize;
    }

    internal struct MempStatStruct {
        internal MPoolStatStruct st;
        internal MPoolFileStatStruct[] files;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct MutexStatStruct {
        internal uint st_mutex_align;
        internal uint st_mutex_tas_spins;
        internal uint st_mutex_cnt;
        internal uint st_mutex_free;
        internal uint st_mutex_inuse;
        internal uint st_mutex_inuse_max;
        internal long st_region_wait;
        internal long st_region_nowait;
        internal uint st_regsize;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct QueueStatStruct {
        internal uint qs_magic;
        internal uint qs_version;
        internal uint qs_metaflags;
        internal uint qs_nkeys;
        internal uint qs_ndata;
        internal uint qs_pagesize;
        internal uint qs_extentsize;
        internal uint qs_pages;
        internal uint qs_re_len;
        internal uint qs_re_pad;
        internal uint qs_pgfree;
        internal uint qs_first_recno;
        internal uint qs_cur_recno;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct RecnoStatStruct {
        internal uint bt_magic;
        internal uint bt_version;
        internal uint bt_metaflags;
        internal uint bt_nkeys;
        internal uint bt_ndata;
        internal uint bt_pagecnt;
        internal uint bt_pagesize;
        internal uint bt_minkey;
        internal uint bt_re_len;
        internal uint bt_re_pad;
        internal uint bt_levels;
        internal uint bt_int_pg;
        internal uint bt_leaf_pg;
        internal uint bt_dup_pg;
        internal uint bt_over_pg;
        internal uint bt_empty_pg;
        internal uint bt_free;
        internal uint bt_int_pgfree;
        internal uint bt_leaf_pgfree;
        internal uint bt_dup_pgfree;
        internal uint bt_over_pgfree;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct RepMgrStatStruct {
        internal long st_perm_failed;
        internal long st_msgs_queued;
        internal long st_msgs_dropped;
        internal long st_connection_drop;
        internal long st_connect_fail;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ReplicationStatStruct {
        internal long st_log_queued;
       internal uint st_startup_complete;
        internal uint st_status;
        internal DB_LSN_STRUCT st_next_lsn;
        internal DB_LSN_STRUCT st_waiting_lsn;
        internal DB_LSN_STRUCT st_max_perm_lsn;
        internal uint st_next_pg;
        internal uint st_waiting_pg;
        internal uint st_dupmasters;
        internal int st_env_id;
        internal uint st_env_priority;
        internal long st_bulk_fills;
        internal long st_bulk_overflows;
        internal long st_bulk_records;
        internal long st_bulk_transfers;
        internal long st_client_rerequests;
        internal long st_client_svc_req;
        internal long st_client_svc_miss;
        internal uint st_gen;
        internal uint st_egen;
        internal long st_log_duplicated;
        internal long st_log_queued_max;
        internal long st_log_queued_total;
        internal long st_log_records;
        internal long st_log_requested;
        internal int st_master;
        internal uint st_master_changes;
        internal uint st_msgs_badgen;
        internal uint st_msgs_processed;
        internal uint st_msgs_recover;
        internal long st_msgs_send_failures;
        internal long st_msgs_sent;
        internal long st_newsites;
        internal uint st_nsites;
        internal long st_nthrottles;
        internal long st_outdated;
        internal long st_pg_duplicated;
        internal long st_pg_records;
        internal long st_pg_requested;
        internal long st_txns_applied;
        internal long st_startsync_delayed;
        /* Elections generally. */
        internal long st_elections;
        internal long st_elections_won;
        /* Statistics about an in-progress election. */
        internal int st_election_cur_winner;
        internal uint st_election_gen;
        internal DB_LSN_STRUCT st_election_lsn;
        internal uint st_election_nsites;
        internal uint st_election_nvotes;
        internal uint st_election_priority;
        internal int st_election_status;
        internal uint st_election_tiebreaker;
        internal uint st_election_votes;
        internal uint st_election_sec;
        internal uint st_election_usec;
        internal uint st_max_lease_sec;
        internal uint st_max_lease_usec;
        /* Statistics only used by the test system. */
        internal uint st_filefail_cleanups;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct SequenceStatStruct {
        internal long st_wait;
        internal long st_nowait;
        internal long st_current;
        internal long st_value;
        internal long st_last_value;
        internal long st_min;
        internal long st_max;
        internal int st_cache_size;
        internal uint st_flags;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TransactionStatStruct {
        internal uint st_nrestores;
        internal DB_LSN_STRUCT st_last_ckp;
        internal long st_time_ckp;
        internal uint st_last_txnid;
        internal uint st_maxtxns;
        internal long st_naborts;
        internal long st_nbegins;
        internal long st_ncommits;
        internal uint st_nactive;
        internal uint st_nsnapshot;
        internal uint st_maxnactive;
        internal uint st_maxnsnapshot;
        internal IntPtr st_txnarray;
        internal long st_region_wait;
        internal long st_region_nowait;
        internal uint st_regsize;
    }

    internal struct DB_LSN_STRUCT {
        internal uint file;
        internal uint offset;
    }

    internal enum DB_TXN_ACTIVE_STATUS {
        TXN_ABORTED = 3,
        TXN_COMMITTED = 1,
        TXN_PREPARED = 2,
        TXN_RUNNING = 4
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct DB_TXN_ACTIVE {
        internal uint txnid;
        internal uint parentid;
        internal int pid;
        internal uint tid;
        internal DB_LSN_STRUCT lsn;
        internal DB_LSN_STRUCT read_lsn;
        internal uint mvcc_ref;
        internal DB_TXN_ACTIVE_STATUS status;
    }

    internal struct TxnStatStruct {
        internal TransactionStatStruct st;
        internal DB_TXN_ACTIVE[] st_txnarray;
        internal byte[][] st_txngids;
        internal string[] st_txnnames;
    }
}