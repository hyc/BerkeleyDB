using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
	[TestFixture]
	public class BTreeCursorTest
	{
		private string testFixtureName;
		private string testFixtureHome;
		private string testName;
		private string testHome;

		private DatabaseEnvironment paramEnv;
		private BTreeDatabase paramDB;
		private EventWaitHandle signal;

		private delegate void BTCursorMoveFuncDelegate(
                    BTreeCursor cursor, LockingInfo lockingInfo);
		private BTCursorMoveFuncDelegate btCursorFunc;

		[TestFixtureSetUp]
		public void RunBeforeTests()
		{
			testFixtureName = "BTreeCursorTest";
			testFixtureHome = "./TestOut/" + testFixtureName;
		}

		[Test]
		public void TestAddKeyFirst()
		{
			BTreeDatabase db;
			BTreeCursor cursor;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestAddKeyFirst";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			// Add record("key", "data") into database.
			CursorTest.GetCursorInBtreeDBWithoutEnv(
                            testHome, testName, out db, out cursor);
			CursorTest.AddOneByCursor(db, cursor);

			// Add record("key","data1") as the first of the data item of "key".
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1")));
			cursor.Add(pair, Cursor.InsertLocation.FIRST);

			// Confirm the record is added as the first of the data item of "key".
			cursor.Move(new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")), true);
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data1"),
                            cursor.Current.Value.Data);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestAddKeyLast()
		{
			BTreeDatabase db;
			BTreeCursor cursor;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestAddKeyLast";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			// Add record("key", "data") into database.
			CursorTest.GetCursorInBtreeDBWithoutEnv(testHome, testName,
                            out db, out cursor);
			CursorTest.AddOneByCursor(db, cursor);

			// Add new record("key","data1") as the last of the data item of "key".
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1")));
			cursor.Add(pair, Cursor.InsertLocation.LAST);

			// Confirm the record is added as the first of the data item of "key".
			cursor.Move(new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")), true);
			Assert.AreNotEqual(ASCIIEncoding.ASCII.GetBytes("data1"), 
                            cursor.Current.Value.Data);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestAddUnique()
		{
			BTreeDatabase db;
			BTreeCursor cursor;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestAddUnique";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			// Open a database and cursor.
			BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			
			// To put no duplicate data, the database should be set to be sorted.
			dbConfig.Duplicates = DuplicatesPolicy.SORTED;
			db = BTreeDatabase.Open(testHome + "/" + testName + ".db", dbConfig);
			cursor = db.Cursor();

			// Add record("key", "data") into database.
			CursorTest.AddOneByCursor(db, cursor);

			// Fail to add duplicate record("key","data").
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
			try
			{
				cursor.AddUnique(pair);
			}
			catch (KeyExistException)
			{
			}
			finally
			{
				cursor.Close();
				db.Close();
			}
		}

		[Test]
		public void TestDuplicateWithSamePos()
		{
			BTreeDatabase db;
			BTreeDatabaseConfig dbConfig;
			BTreeCursor cursor, dupCursor;
			DatabaseEnvironment env;
			DatabaseEnvironmentConfig envConfig;
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			Transaction txn;

			testName = "TestDuplicateWithSamePos";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			envConfig = new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			envConfig.NoMMap = false;
			env = DatabaseEnvironment.Open(testHome, envConfig);

			txn = env.BeginTransaction();
			dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Env = env;
			db = BTreeDatabase.Open(testName + ".db", dbConfig, txn);
			key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key"));
			data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data"));
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
			db.Put(key, data, txn);
			txn.Commit();

			txn = env.BeginTransaction();
			cursor = db.Cursor(txn);
			cursor.Move(key, true);

			//Duplicate a new cursor to the same position. 
			dupCursor = cursor.Duplicate(true);

			// Overwrite the record.
			dupCursor.Overwrite(new DatabaseEntry(
                            ASCIIEncoding.ASCII.GetBytes("newdata")));

			// Confirm that the original data doesn't exist.
			Assert.IsFalse(dupCursor.Move(pair, true));

			dupCursor.Close();
			cursor.Close();
			txn.Commit();
			db.Close();
			env.Close();
		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestDuplicateToDifferentPos()
		{
			BTreeDatabase db;
			BTreeDatabaseConfig dbConfig;
			BTreeCursor cursor, dupCursor;
			DatabaseEnvironment env;
			DatabaseEnvironmentConfig envConfig;
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			Transaction txn;

			testName = "TestDuplicateToDifferentPos";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			envConfig = new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			envConfig.NoMMap = false;
			env = DatabaseEnvironment.Open(testHome, envConfig);

			txn = env.BeginTransaction();
			dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Env = env;
			db = BTreeDatabase.Open(testName + ".db", dbConfig, txn);
			key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key"));
			data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data"));
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
			db.Put(key, data, txn);
			txn.Commit();

			txn = env.BeginTransaction();
			cursor = db.Cursor(txn);
			cursor.Move(key, true);

			//Duplicate a new cursor to the same position. 
			dupCursor = cursor.Duplicate(false);

			/*
			 * The duplicate cursor points to nothing so overwriting the 
			 * record is not allowed.
			 */
			try
			{
				dupCursor.Overwrite(new DatabaseEntry(
                                    ASCIIEncoding.ASCII.GetBytes("newdata")));
			}
			catch (DatabaseException)
			{
				throw new ExpectedTestException();
			}
			finally
			{
				dupCursor.Close();
				cursor.Close();
				txn.Commit();
				db.Close();
				env.Close();
			}
		}

		[Test]
		public void TestInsertAfter()
		{
			BTreeDatabase db;
			BTreeCursor cursor;
			DatabaseEntry data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestInsertAfter";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			// Add record("key", "data") into database.
			CursorTest.GetCursorInBtreeDBWithoutEnv(testHome, testName, 
                            out db, out cursor);
			CursorTest.AddOneByCursor(db, cursor);

			// Insert the new record("key","data1") after the record("key", "data").
			data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1"));
			cursor.Insert(data, Cursor.InsertLocation.AFTER);

			/*
			 * Move the cursor to the record("key", "data") and confirm that 
			 * the next record is the one just inserted. 
			 */
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
			Assert.IsTrue(cursor.Move(pair, true));
			Assert.IsTrue(cursor.MoveNext());
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("key"), cursor.Current.Key.Data);
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data1"), cursor.Current.Value.Data);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestInsertBefore()
		{
			BTreeDatabase db;
			BTreeCursor cursor;
			DatabaseEntry data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestInsertBefore";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			// Add record("key", "data") into database.
			CursorTest.GetCursorInBtreeDBWithoutEnv(
			    testHome, testName, out db, out cursor);
			CursorTest.AddOneByCursor(db, cursor);

			// Insert the new record("key","data1") before the record("key", "data").
			data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1"));
			cursor.Insert(data, Cursor.InsertLocation.BEFORE);

			/*
			 * Move the cursor to the record("key", "data") and confirm 
			 * that the previous record is the one just inserted.
			 */
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
			Assert.IsTrue(cursor.Move(pair, true));
			Assert.IsTrue(cursor.MovePrev());
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("key"),cursor.Current.Key.Data);
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data1"), cursor.Current.Value.Data);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMoveToRecno()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMoveToRecno";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			GetCursorInBtreeDBUsingRecno(testHome, testName,
			    out db, out cursor);
			for (int i = 0; i < 10; i++)
				db.Put(
				    new DatabaseEntry(BitConverter.GetBytes(i)),
				    new DatabaseEntry(BitConverter.GetBytes(i)));

			MoveCursorToRecno(cursor, null);
			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMoveToRecnoWithRMW()
		{
			testName = "TestMoveToRecnoWithRMW";
			testHome = testFixtureHome + "/" + testName;
			Configuration.ClearDir(testHome);

			btCursorFunc = new BTCursorMoveFuncDelegate(
			    MoveCursorToRecno);

			// Move to a specified key and a key/data pair.
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestRecno()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestRecno";
			testHome = testFixtureHome + "/" + testName;

			Configuration.ClearDir(testHome);

			GetCursorInBtreeDBUsingRecno(testHome, testName,
			    out db, out cursor);
			for (int i = 0; i < 10; i++)
				db.Put(
				    new DatabaseEntry(BitConverter.GetBytes(i)),
				    new DatabaseEntry(BitConverter.GetBytes(i)));

			ReturnRecno(cursor, null);
			cursor.Close();
			db.Close();
		}


		[Test]
		public void TestRecnoWithRMW()
		{
			testName = "TestRecnoWithRMW";
			testHome = testFixtureHome + "/" + testName;
			Configuration.ClearDir(testHome);

			// Use MoveCursorToRecno() as its move function.
			btCursorFunc = new BTCursorMoveFuncDelegate(
			    ReturnRecno);

			// Move to a specified key and a key/data pair.
			MoveWithRMW(testHome, testName);
		}

		/*
		 * Move the cursor according to recno. The recno 
		 * starts from 1 and is increased by 1.
		 */
		public void MoveCursorToRecno(BTreeCursor cursor,
		    LockingInfo lck)
		{
			for (uint i = 1; i <= 5; i++)
				if (lck == null)
					Assert.IsTrue(cursor.Move(i));
				else
					Assert.IsTrue(cursor.Move(i, lck));
		}

		/*l
		 * Move the cursor according to a given recno and 
		 * return the current record's recno. The given recno
		 * and the one got from the cursor should be the same.
		 */
		public void ReturnRecno(BTreeCursor cursor,
		    LockingInfo lck)
		{
			for (uint i = 1; i <= 5; i++)
				if (lck == null)
				{
					if (cursor.Move(i) == true)
						Assert.AreEqual(i, cursor.Recno());
				}
				else
				{
					if (cursor.Move(i, lck) == true)
						Assert.AreEqual(i, cursor.Recno(lck));
				}
		}

		public void GetCursorInBtreeDBUsingRecno(string home,
		    string name, out BTreeDatabase db,
		    out BTreeCursor cursor)
		{
			string dbFileName = home + "/" + name + ".db";
			BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
			dbConfig.UseRecordNumbers = true;
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			db = BTreeDatabase.Open(dbFileName, dbConfig);
			cursor = db.Cursor();
		}

		public void RdMfWt()
		{
			Transaction txn = paramEnv.BeginTransaction();
			BTreeCursor dbc = paramDB.Cursor(txn);

			try
			{
				LockingInfo lck = new LockingInfo();
				lck.ReadModifyWrite = true;

				// Read record.
				btCursorFunc(dbc, lck);

				// Block the current thread until event is set.
				signal.WaitOne();

				// Write new records into database.
				DatabaseEntry key = new DatabaseEntry(BitConverter.GetBytes(55));
				DatabaseEntry data = new DatabaseEntry(BitConverter.GetBytes(55));
				dbc.Add(new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data));

				dbc.Close();
				txn.Commit();
			}
			catch (DeadlockException)
			{
				dbc.Close();
				txn.Abort();
			}
		}


		public void MoveWithRMW(string home, string name)
		{
			paramEnv = null;
			paramDB = null;

			// Open the environment.
			DatabaseEnvironmentConfig envCfg =
			    new DatabaseEnvironmentConfig();
			envCfg.Create = true;
			envCfg.FreeThreaded = true;
			envCfg.UseLocking = true;
			envCfg.UseLogging = true;
			envCfg.UseMPool = true;
			envCfg.UseTxns = true;
			paramEnv = DatabaseEnvironment.Open(home, envCfg);

			// Open database in transaction.
			Transaction openTxn = paramEnv.BeginTransaction();
			BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
			cfg.Creation = CreatePolicy.ALWAYS;
			cfg.Env = paramEnv;
			cfg.FreeThreaded = true;
			cfg.PageSize = 4096;
			// Use record number.
			cfg.UseRecordNumbers = true;
			paramDB = BTreeDatabase.Open(name + ".db", cfg, openTxn);
			openTxn.Commit();

			/*
			 * Put 10 different, 2 duplicate and another different 
			 * records into database.
			 */
			Transaction txn = paramEnv.BeginTransaction();
			for (int i = 0; i < 13; i++)
			{
				DatabaseEntry key, data;
				if (i == 10 || i == 11)
				{
					key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key"));
					data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data"));
				}
				else
				{
					key = new DatabaseEntry(BitConverter.GetBytes(i));
					data = new DatabaseEntry(BitConverter.GetBytes(i));
				}
				paramDB.Put(key, data, txn);
			}

			txn.Commit();

			// Get a event wait handle.
			signal = new EventWaitHandle(false,
			    EventResetMode.ManualReset);

			/*
			 * Start RdMfWt() in two threads. RdMfWt() reads 
			 * and writes data into database.
			 */
			Thread t1 = new Thread(new ThreadStart(RdMfWt));
			Thread t2 = new Thread(new ThreadStart(RdMfWt));
			t1.Start();
			t2.Start();

			// Give both threads time to read before signalling them to write. 
			Thread.Sleep(1000);

			// Invoke the write operation in both threads.
			signal.Set();

			// Return the number of deadlocks.
			while (t1.IsAlive || t2.IsAlive)
			{
				/*
				 * Give both threads time to write before 
				 * counting the number of deadlocks.
				 */
				Thread.Sleep(1000);
				uint deadlocks = paramEnv.DetectDeadlocks(DeadlockPolicy.DEFAULT);
				
				// Confirm that there won't be any deadlock. 
				Assert.AreEqual(0, deadlocks);
			}

			t1.Join();
			t2.Join();
			paramDB.Close();
			paramEnv.Close();
		}

	}
}

