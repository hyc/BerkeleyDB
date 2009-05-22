using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    internal class LockRequest {
        private DB_LOCKREQ lockreq;

        internal Lock Lck {
            get { return new Lock(lockreq.lck); }
            set { lockreq.lck = Lock.GetDB_LOCK(value); }
        }

        internal LockOperation Op {
            get { return LockOperation.GetLockOperation(lockreq.op); }
            set { lockreq.op = LockOperation.GetOperation(value); }
        }

        internal LockMode Mode {
            get { return LockMode.GetLockMode(lockreq.mode); }
            set { lockreq.mode = LockMode.GetMode(value); }
        }

        internal DatabaseEntry Obj {
            get { return DatabaseEntry.fromDBT(lockreq.obj); }
            set { lockreq.obj = DatabaseEntry.getDBT(value); }
        }

        internal uint timeout {
            get { return lockreq.timeout; }
            set { lockreq.timeout = value; }
        }

        internal LockRequest() {
            lockreq = new DB_LOCKREQ();
        }

        internal static DB_LOCKREQ get_DB_LOCKREQ(LockRequest req) {
            if (req != null)
                return req.lockreq;
            return null;
        }
    }
}
