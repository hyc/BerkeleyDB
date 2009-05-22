using System;
using System.IO;
using System.Collections.Generic;
using System.Collections;
using System.Diagnostics;
using System.Text;
using System.Runtime.Serialization.Formatters.Binary;
using BerkeleyDB;

namespace ex_access {
    class Program {
        static void Main(string[] args) {
            BTreeDatabase btreeDB;
            BTreeDatabaseConfig btreeConfig;
            Cursor dbc;
            DatabaseEntry key;
            DatabaseEntry data;
            string buff, dbFileName, keyString;

            try {
                dbFileName = "access.db";
                if (args.Length > 0)
                    dbFileName = args[0];
            } catch {
                usage();
                return;
            }

            /* Optiionally remove the existing database file. */
            if (File.Exists(dbFileName)) {
                while (true) {
                    Console.Write
                        ("{0} already exists.  Delete it? (y/n) ", dbFileName);
                    buff = Console.ReadLine().ToLower();
                    if (buff == "y" || buff == "n")
                        break;
                }

                if (buff == "y") {
                    try {
                        File.Delete(dbFileName);
                    } catch {
                        Console.WriteLine("Unable to delete {0}.", dbFileName);
                        return;
                    }
                }
            }

            /* Configure the database. */
            btreeConfig = new BTreeDatabaseConfig();
            btreeConfig.Duplicates = DuplicatesPolicy.SORTED;
            btreeConfig.ErrorPrefix = "ex_csharp_access";
            btreeConfig.Creation = CreatePolicy.IF_NEEDED;
            btreeConfig.CacheSize = new CacheInfo(0, 64 * 1024, 1);
            btreeConfig.PageSize = 8 * 1024;

            /* Create and open a new database in the file. */
            try {
                btreeDB = BTreeDatabase.Open(dbFileName, btreeConfig);
            } catch (Exception e) {
                Console.WriteLine("Error opening {0}.", dbFileName);
                Console.WriteLine(e.Message);
                return;
            }

            /* Input key/data pairs and insert them into the database.*/
            key = new DatabaseEntry();
            data = new DatabaseEntry();

            while (true) {
                Console.Write("key [blank line to quit] > ");
                keyString = Console.ReadLine();
                if (keyString == "")
                    break;

                dbtFromString(key, keyString);
                dbtFromString(data, reverse(keyString));

                try {
                    btreeDB.Put(key, data);
                } catch {
                    return;
                }

            }

            /* Acquire a cursor for the database. */
            using (dbc = btreeDB.Cursor()) {
                /* Walk through the database and print out key/data pairs. */
                Console.WriteLine("All key : data pairs:");
                foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> p in dbc)
                    Console.WriteLine("{0}::{1}",
                        strFromDBT(p.Key), strFromDBT(p.Value));
            }
            
            Console.Write("Press any key to exit >");
            Console.ReadKey(true);

            /* Close the cursor and database. */
            btreeDB.Close();
        }

        #region Utilities
        public static void usage() {
            Console.WriteLine(
                "Usage: ex_access [database]");
        }

        static void dbtFromString(DatabaseEntry dbt, string s) {
            dbt.Data = System.Text.Encoding.ASCII.GetBytes(s);
        }

        public static string strFromDBT(DatabaseEntry dbt) {

            System.Text.ASCIIEncoding decode =
                new ASCIIEncoding();
            return decode.GetString(dbt.Data);
        }

        public static string reverse(string s) {
            StringBuilder tmp = new StringBuilder(s.Length);
            for (int i = s.Length - 1; i >= 0; i--)
                tmp.Append(s[i]);
            return tmp.ToString();
        }
        #endregion Utilities

    }
}