using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// Statistical information about the Replication Manager
    /// </summary>
    public class RepMgrStats {
        private Internal.RepMgrStatStruct st;
        internal RepMgrStats(Internal.RepMgrStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Existing connections dropped. 
        /// </summary>
        public long DroppedConnections { get { return st.st_connection_drop; } }
        /// <summary>
        /// # msgs discarded due to excessive queue length.
        /// </summary>
        public long DroppedMessages { get { return st.st_msgs_dropped; } }
        /// <summary>
        /// Failed new connection attempts. 
        /// </summary>
        public long FailedConnections { get { return st.st_connect_fail; } }
        /// <summary>
        /// # of insufficiently ack'ed msgs. 
        /// </summary>
        public long FailedMessages { get { return st.st_perm_failed; } }
        /// <summary>
        /// # msgs queued for network delay. 
        /// </summary>
        public long QueuedMessages { get { return st.st_msgs_queued; } }
    }
}
