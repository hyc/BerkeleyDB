# DO NOT EDIT: automatically built by dist/s_android.
# Makefile for building a drop-in replacement of SQLite using
# Berkeley DB 11g Release 2, library version 11.2.5.0.21: (March 30, 2010)
###################################################################
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

###################################################################
# build libsqlite replacement
LOCAL_MODULE := libsqlite

# BDB_TOP will change with release numbers
BDB_TOP := db-5.0.21
BDB_PATH := $(LOCAL_PATH)/$(BDB_TOP)

# This directive results in arm (vs thumb) code.  It's necessary to
# allow some BDB assembler code (for mutexes) to compile.
LOCAL_ARM_MODE := arm

# basic includes for BDB 11gR2
LOCAL_C_INCLUDES := $(BDB_PATH) $(LOCAL_PATH)/$(BDB_TOP)/build_android \
	$(LOCAL_PATH)/$(BDB_TOP)/sql/generated

# this is needed for sqlite3.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(BDB_TOP)/build_android/sql

# Source files
LOCAL_SRC_FILES := \
	$(BDB_TOP)/btree/bt_compact.c \
	$(BDB_TOP)/btree/bt_compare.c \
	$(BDB_TOP)/btree/bt_compress.c \
	$(BDB_TOP)/btree/bt_conv.c \
	$(BDB_TOP)/btree/bt_curadj.c \
	$(BDB_TOP)/btree/bt_cursor.c \
	$(BDB_TOP)/btree/bt_delete.c \
	$(BDB_TOP)/btree/bt_method.c \
	$(BDB_TOP)/btree/bt_open.c \
	$(BDB_TOP)/btree/bt_put.c \
	$(BDB_TOP)/btree/bt_rec.c \
	$(BDB_TOP)/btree/bt_reclaim.c \
	$(BDB_TOP)/btree/bt_recno.c \
	$(BDB_TOP)/btree/bt_rsearch.c \
	$(BDB_TOP)/btree/bt_search.c \
	$(BDB_TOP)/btree/bt_split.c \
	$(BDB_TOP)/btree/bt_stat.c \
	$(BDB_TOP)/btree/bt_upgrade.c \
	$(BDB_TOP)/btree/btree_auto.c \
	$(BDB_TOP)/clib/rand.c \
	$(BDB_TOP)/clib/snprintf.c \
	$(BDB_TOP)/common/clock.c \
	$(BDB_TOP)/common/crypto_stub.c \
	$(BDB_TOP)/common/db_byteorder.c \
	$(BDB_TOP)/common/db_compint.c \
	$(BDB_TOP)/common/db_err.c \
	$(BDB_TOP)/common/db_getlong.c \
	$(BDB_TOP)/common/db_idspace.c \
	$(BDB_TOP)/common/db_log2.c \
	$(BDB_TOP)/common/db_shash.c \
	$(BDB_TOP)/common/dbt.c \
	$(BDB_TOP)/common/mkpath.c \
	$(BDB_TOP)/common/os_method.c \
	$(BDB_TOP)/common/zerofill.c \
	$(BDB_TOP)/db/crdel_auto.c \
	$(BDB_TOP)/db/crdel_rec.c \
	$(BDB_TOP)/db/db.c \
	$(BDB_TOP)/db/db_am.c \
	$(BDB_TOP)/db/db_auto.c \
	$(BDB_TOP)/db/db_cam.c \
	$(BDB_TOP)/db/db_cds.c \
	$(BDB_TOP)/db/db_compact.c \
	$(BDB_TOP)/db/db_conv.c \
	$(BDB_TOP)/db/db_dispatch.c \
	$(BDB_TOP)/db/db_dup.c \
	$(BDB_TOP)/db/db_iface.c \
	$(BDB_TOP)/db/db_join.c \
	$(BDB_TOP)/db/db_meta.c \
	$(BDB_TOP)/db/db_method.c \
	$(BDB_TOP)/db/db_open.c \
	$(BDB_TOP)/db/db_overflow.c \
	$(BDB_TOP)/db/db_pr.c \
	$(BDB_TOP)/db/db_rec.c \
	$(BDB_TOP)/db/db_reclaim.c \
	$(BDB_TOP)/db/db_remove.c \
	$(BDB_TOP)/db/db_rename.c \
	$(BDB_TOP)/db/db_ret.c \
	$(BDB_TOP)/db/db_setid.c \
	$(BDB_TOP)/db/db_setlsn.c \
	$(BDB_TOP)/db/db_sort_multiple.c \
	$(BDB_TOP)/db/db_stati.c \
	$(BDB_TOP)/db/db_truncate.c \
	$(BDB_TOP)/db/db_upg.c \
	$(BDB_TOP)/db/db_upg_opd.c \
	$(BDB_TOP)/db/db_vrfy_stub.c \
	$(BDB_TOP)/db/partition.c \
	$(BDB_TOP)/dbreg/dbreg.c \
	$(BDB_TOP)/dbreg/dbreg_auto.c \
	$(BDB_TOP)/dbreg/dbreg_rec.c \
	$(BDB_TOP)/dbreg/dbreg_stat.c \
	$(BDB_TOP)/dbreg/dbreg_util.c \
	$(BDB_TOP)/env/env_alloc.c \
	$(BDB_TOP)/env/env_config.c \
	$(BDB_TOP)/env/env_failchk.c \
	$(BDB_TOP)/env/env_file.c \
	$(BDB_TOP)/env/env_globals.c \
	$(BDB_TOP)/env/env_method.c \
	$(BDB_TOP)/env/env_name.c \
	$(BDB_TOP)/env/env_open.c \
	$(BDB_TOP)/env/env_recover.c \
	$(BDB_TOP)/env/env_region.c \
	$(BDB_TOP)/env/env_register.c \
	$(BDB_TOP)/env/env_sig.c \
	$(BDB_TOP)/env/env_stat.c \
	$(BDB_TOP)/fileops/fileops_auto.c \
	$(BDB_TOP)/fileops/fop_basic.c \
	$(BDB_TOP)/fileops/fop_rec.c \
	$(BDB_TOP)/fileops/fop_util.c \
	$(BDB_TOP)/hash/hash_func.c \
	$(BDB_TOP)/hash/hash_stub.c \
	$(BDB_TOP)/hmac/hmac.c \
	$(BDB_TOP)/hmac/sha1.c \
	$(BDB_TOP)/lock/lock.c \
	$(BDB_TOP)/lock/lock_deadlock.c \
	$(BDB_TOP)/lock/lock_failchk.c \
	$(BDB_TOP)/lock/lock_id.c \
	$(BDB_TOP)/lock/lock_list.c \
	$(BDB_TOP)/lock/lock_method.c \
	$(BDB_TOP)/lock/lock_region.c \
	$(BDB_TOP)/lock/lock_stat.c \
	$(BDB_TOP)/lock/lock_timer.c \
	$(BDB_TOP)/lock/lock_util.c \
	$(BDB_TOP)/log/log.c \
	$(BDB_TOP)/log/log_archive.c \
	$(BDB_TOP)/log/log_compare.c \
	$(BDB_TOP)/log/log_debug.c \
	$(BDB_TOP)/log/log_get.c \
	$(BDB_TOP)/log/log_method.c \
	$(BDB_TOP)/log/log_print.c \
	$(BDB_TOP)/log/log_put.c \
	$(BDB_TOP)/log/log_stat.c \
	$(BDB_TOP)/log/log_verify_stub.c \
	$(BDB_TOP)/mp/mp_alloc.c \
	$(BDB_TOP)/mp/mp_bh.c \
	$(BDB_TOP)/mp/mp_fget.c \
	$(BDB_TOP)/mp/mp_fmethod.c \
	$(BDB_TOP)/mp/mp_fopen.c \
	$(BDB_TOP)/mp/mp_fput.c \
	$(BDB_TOP)/mp/mp_fset.c \
	$(BDB_TOP)/mp/mp_method.c \
	$(BDB_TOP)/mp/mp_mvcc.c \
	$(BDB_TOP)/mp/mp_region.c \
	$(BDB_TOP)/mp/mp_register.c \
	$(BDB_TOP)/mp/mp_resize.c \
	$(BDB_TOP)/mp/mp_stat.c \
	$(BDB_TOP)/mp/mp_sync.c \
	$(BDB_TOP)/mp/mp_trickle.c \
	$(BDB_TOP)/mutex/mut_alloc.c \
	$(BDB_TOP)/mutex/mut_failchk.c \
	$(BDB_TOP)/mutex/mut_method.c \
	$(BDB_TOP)/mutex/mut_region.c \
	$(BDB_TOP)/mutex/mut_stat.c \
	$(BDB_TOP)/mutex/mut_tas.c \
	$(BDB_TOP)/os/os_abort.c \
	$(BDB_TOP)/os/os_abs.c \
	$(BDB_TOP)/os/os_alloc.c \
	$(BDB_TOP)/os/os_clock.c \
	$(BDB_TOP)/os/os_config.c \
	$(BDB_TOP)/os/os_cpu.c \
	$(BDB_TOP)/os/os_ctime.c \
	$(BDB_TOP)/os/os_dir.c \
	$(BDB_TOP)/os/os_errno.c \
	$(BDB_TOP)/os/os_fid.c \
	$(BDB_TOP)/os/os_flock.c \
	$(BDB_TOP)/os/os_fsync.c \
	$(BDB_TOP)/os/os_getenv.c \
	$(BDB_TOP)/os/os_handle.c \
	$(BDB_TOP)/os/os_map.c \
	$(BDB_TOP)/os/os_mkdir.c \
	$(BDB_TOP)/os/os_open.c \
	$(BDB_TOP)/os/os_pid.c \
	$(BDB_TOP)/os/os_rename.c \
	$(BDB_TOP)/os/os_root.c \
	$(BDB_TOP)/os/os_rpath.c \
	$(BDB_TOP)/os/os_rw.c \
	$(BDB_TOP)/os/os_seek.c \
	$(BDB_TOP)/os/os_stack.c \
	$(BDB_TOP)/os/os_stat.c \
	$(BDB_TOP)/os/os_tmpdir.c \
	$(BDB_TOP)/os/os_truncate.c \
	$(BDB_TOP)/os/os_uid.c \
	$(BDB_TOP)/os/os_unlink.c \
	$(BDB_TOP)/os/os_yield.c \
	$(BDB_TOP)/qam/qam_stub.c \
	$(BDB_TOP)/rep/rep_stub.c \
	$(BDB_TOP)/repmgr/repmgr_stub.c \
	$(BDB_TOP)/sequence/seq_stat.c \
	$(BDB_TOP)/sequence/sequence.c \
	$(BDB_TOP)/txn/txn.c \
	$(BDB_TOP)/txn/txn_auto.c \
	$(BDB_TOP)/txn/txn_chkpt.c \
	$(BDB_TOP)/txn/txn_failchk.c \
	$(BDB_TOP)/txn/txn_method.c \
	$(BDB_TOP)/txn/txn_rec.c \
	$(BDB_TOP)/txn/txn_recover.c \
	$(BDB_TOP)/txn/txn_region.c \
	$(BDB_TOP)/txn/txn_stat.c \
	$(BDB_TOP)/txn/txn_util.c \
	$(BDB_TOP)/sql/generated/sqlite3.c

ifneq ($(TARGET_ARCH),arm)
LOCAL_LDLIBS += -lpthread -ldl
endif

# flags -- most of these are from the SQLite build, some are not.
# SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT controls the default size of
# the Berkeley DB log file (set to 1MB here).
LOCAL_CFLAGS += -Wall -DHAVE_USLEEP=1 \
	-DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 \
	-DSQLITE_THREADSAFE=1 -DNDEBUG=1 -DSQLITE_TEMP_STORE=3 \
	-DSQLITE_OMIT_TRUNCATE_OPTIMIZATION -DSQLITE_OS_UNIX=1 \
	-D_HAVE_SQLITE_CONFIG_H -DSQLITE_THREAD_OVERRIDE_LOCK=-1 \
	-DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS3_BACKWARDS -Dfdatasync=fsync

# LOCAL_CFLAGS that are not used at this time that should be examined
# -DSQLITE_ENABLE_POISON
# -DSQLITE_ENABLE_MEMORY_MANAGEMENT

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES := libdl
endif

LOCAL_C_INCLUDES += $(call include-path-for, system-core)/cutils
LOCAL_SHARED_LIBRARIES += liblog libicuuc libicui18n libutils

# This links in some static symbols from Android
LOCAL_WHOLE_STATIC_LIBRARIES := libsqlite3_android

include $(BUILD_SHARED_LIBRARY)

################################################################################
##device commande line tool:sqlite3
################################################################################
ifneq ($(SDK_ONLY),true)  # SDK doesn't need device version of sqlite3
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := $(BDB_TOP)/sql/sqlite/src/shell.c
LOCAL_SHARED_LIBRARIES := libsqlite
LOCAL_C_INCLUDES := $(BDB_PATH) $(LOCAL_PATH)/$(BDB_TOP)/build_android\
	 $(LOCAL_PATH)/$(BDB_TOP)/sql/generated $(LOCAL_PATH)/../android

ifneq ($(TARGET_ARCH),arm)
LOCAL_LDLIBS += -lpthread -ldl
endif

LOCAL_CFLAGS += -DHAVE_USLEEP=1 -DTHREADSAFE=1 -DNDEBUG=1
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := sqlite3
include $(BUILD_EXECUTABLE)
endif # !SDK_ONLY

