Build BFILE extension
========================================================================
BFILE directory location: <db>/lang/sql/sqlite/ext/bfile

BFile has two interface:
1) SQL expressions
2) C-functions API
The default interface is SQL expression. If you want to use C-functions API,
set ENABLE_BFILE_CAPI in bfile/build/Makefile to zero.

$ cd <db>/build_unix
$ export DBSQL_DIR=$PWD/../install
$ ../dist/configure --enable-sql --enable-load-extension --prefix=$DBSQL_DIR && make && make install
$ cd ../lang/sql/sqlite/ext/bfile/build
$ make

Run example
========================================================================
$ cd lang/sql/ext/bfile/build
$ export LD_LIBRARY_PATH=$PWD:$DBSQL_DIR/lib
$ ./bfile_example_sql     # for SQL expressions interface
$ ./bfile_example_capi    # for C-functions API

Load BFILE extension
========================================================================
- in dbsql shell:
	dbsql> .load libbfile_ext.so
- by API:
	i.  sqlite3_enable_load_extension(db, 1);
	ii. rc = sqlite3_load_extension(db, "libbfile_ext.so", NULL, &zErrMsg);

Run Tests
=========================================================================
i. Tcl-based tests
  $ env TEXTFIXTURE=/path/of/textfixture TEST_DIR=/path/of/test/dir make tcl_test
ii. API test
  $ ./bfile_test_sql     # for SQL expressions interface
  $ ./bfile_test_capi    # for C-functions API

Note: Build for windriver-ovia should set BUILD_OVIA=1. For more detail, please
refer to ./build/Makefile.
