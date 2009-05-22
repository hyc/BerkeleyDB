using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// Statistical information about the memory pool subsystem
    /// </summary>
    public class MPoolStats {
        private Internal.MPoolStatStruct st;
        private CacheInfo ci;
        private List<MPoolFileStats> mempfiles;

        internal MPoolStats(Internal.MempStatStruct stats) {
            st = stats.st;
            ci = new CacheInfo(st.st_gbytes, st.st_bytes, (int)st.st_max_ncache);
            mempfiles = new List<MPoolFileStats>();
            foreach (Internal.MPoolFileStatStruct file in stats.files)
                mempfiles.Add(new MPoolFileStats(file));
        }
        /// <summary>
        /// Total cache size and number of regions
        /// </summary>
        public CacheInfo CacheSettings { get { return ci; } }
        /// <summary>
        /// Maximum number of regions. 
        /// </summary>
        public uint CacheRegions { get { return st.st_ncache; } }
        /// <summary>
        /// Maximum file size for mmap. 
        /// </summary>
        public uint MaxMMapSize { get { return st.st_mmapsize; } }
        /// <summary>
        /// Maximum number of open fd's. 
        /// </summary>
        public int MaxOpenFileDescriptors { get { return st.st_maxopenfd; } }
        /// <summary>
        /// Maximum buffers to write. 
        /// </summary>
        public int MaxBufferWrites { get { return st.st_maxwrite; } }
        /// <summary>
        /// Sleep after writing max buffers. 
        /// </summary>
        public uint MaxBufferWritesSleep { get { return st.st_maxwrite_sleep; } }
        /// <summary>
        /// Total number of pages. 
        /// </summary>
        public uint Pages { get { return st.st_pages; } }
        /// <summary>
        /// Pages from mapped files. 
        /// </summary>
        public uint MappedPages { get { return st.st_map; } }
        /// <summary>
        /// Pages found in the cache. 
        /// </summary>
        public long PagesInCache { get { return st.st_cache_hit; } }
        /// <summary>
        /// Pages not found in the cache. 
        /// </summary>
        public long PagesNotInCache { get { return st.st_cache_miss; } }
        /// <summary>
        /// Pages created in the cache. 
        /// </summary>
        public long PagesCreatedInCache { get { return st.st_page_create; } }
        /// <summary>
        /// Pages read in. 
        /// </summary>
        public long PagesRead { get { return st.st_page_in; } }
        /// <summary>
        /// Pages written out. 
        /// </summary>
        public long PagesWritten { get { return st.st_page_out; } }
        /// <summary>
        /// Clean pages forced from the cache. 
        /// </summary>
        public long CleanPagesEvicted { get { return st.st_ro_evict; } }
        /// <summary>
        /// Dirty pages forced from the cache. 
        /// </summary>
        public long DirtyPagesEvicted { get { return st.st_rw_evict; } }
        /// <summary>
        /// Pages written by memp_trickle. 
        /// </summary>
        public long PagesTrickled { get { return st.st_page_trickle; } }
        /// <summary>
        /// Clean pages. 
        /// </summary>
        public uint CleanPages { get { return st.st_page_clean; } }
        /// <summary>
        /// Dirty pages. 
        /// </summary>
        public uint DirtyPages { get { return st.st_page_dirty; } }
        /// <summary>
        /// Number of hash buckets. 
        /// </summary>
        public uint HashBuckets { get { return st.st_hash_buckets; } }
        /// <summary>
        /// Total hash chain searches. 
        /// </summary>
        public uint HashChainSearches { get { return st.st_hash_searches; } }
        /// <summary>
        /// Longest hash chain searched. 
        /// </summary>
        public uint LongestHashChainSearch { get { return st.st_hash_longest; } }
        /// <summary>
        /// Total hash entries searched. 
        /// </summary>
        public long HashEntriesSearched { get { return st.st_hash_examined; } }
        /// <summary>
        /// Hash lock granted with nowait. 
        /// </summary>
        public long HashLockNoWait { get { return st.st_hash_nowait; } }
        /// <summary>
        /// Hash lock granted after wait. 
        /// </summary>
        public long HashLockWait { get { return st.st_hash_wait; } }
        /// <summary>
        /// Max hash lock granted with nowait. 
        /// </summary>
        public long MaxHashLockNoWait { get { return st.st_hash_max_nowait; } }
        /// <summary>
        /// Max hash lock granted after wait. 
        /// </summary>
        public long MaxHashLockWait { get { return st.st_hash_max_wait; } }
        /// <summary>
        /// Region lock granted with nowait. 
        /// </summary>
        public long RegionLockNoWait { get { return st.st_region_nowait; } }
        /// <summary>
        /// Region lock granted after wait. 
        /// </summary>
        public long RegionLockWait { get { return st.st_region_wait; } }
        /// <summary>
        /// Buffers frozen. 
        /// </summary>
        public long FrozenBuffers { get { return st.st_mvcc_frozen; } }
        /// <summary>
        /// Buffers thawed. 
        /// </summary>
        public long ThawedBuffers { get { return st.st_mvcc_thawed; } }
        /// <summary>
        /// Frozen buffers freed. 
        /// </summary>
        public long FrozenBuffersFreed { get { return st.st_mvcc_freed; } }
        /// <summary>
        /// Number of page allocations. 
        /// </summary>
        public long PageAllocations { get { return st.st_alloc; } }
        /// <summary>
        /// Buckets checked during allocation. 
        /// </summary>
        public long BucketsCheckedDuringAlloc { get { return st.st_alloc_buckets; } }
        /// <summary>
        /// Max checked during allocation. 
        /// </summary>
        public long MaxBucketsCheckedDuringAlloc { get { return st.st_alloc_max_buckets; } }
        /// <summary>
        /// Pages checked during allocation. 
        /// </summary>
        public long PagesCheckedDuringAlloc { get { return st.st_alloc_pages; } }
        /// <summary>
        /// Max checked during allocation. 
        /// </summary>
        public long MaxPagesCheckedDuringAlloc { get { return st.st_alloc_max_pages; } }
        /// <summary>
        /// Thread waited on buffer I/O. 
        /// </summary>
        public long BlockedOperations { get { return st.st_io_wait; } }
        /// <summary>
        /// Region size. 
        /// </summary>
        public uint RegionSize { get { return st.st_regsize; } }
        /// <summary>
        /// Stats for files open in the memory pool
        /// </summary>
        public List<MPoolFileStats> Files { get { return mempfiles; } }
    }
}