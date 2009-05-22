#include <db_config.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::getline;
using std::ofstream;
using std::unitbuf;
using std::string;
using std::ostream;
using std::ostringstream;
using std::istringstream;

#include <db_int.h>
#include <db_cxx.h>

u_int32_t role = 0;

template <class T>
static T
token_() {
    T x;

    if (cin >> x)
        return x;
    throw "no token";
}

static int
itoken() {
    return token_<int>();
}

static string
token() {
    return token_<string>();
}

static u_int32_t
start_type(string flag_name) {
    if (flag_name == "master")
        return DB_REP_MASTER;
    else if (flag_name == "client")
        return DB_REP_CLIENT;
    else if (flag_name == "election")
        return DB_REP_ELECTION;
    else
        throw "unknown start type";
}

static void
event_catcher(DbEnv *env, u_int32_t event, void *info) {
    COMPQUIET(env, 0);
    COMPQUIET(info, 0);
    
    if (event == DB_EVENT_REP_CLIENT)
        role = DB_REP_CLIENT;
    else if (event == DB_EVENT_REP_MASTER)
        role = DB_REP_MASTER;
}

int
main() {
    DbEnv *env = new DbEnv(0);
    Db *dbp = 0;
    u_int32_t flags;
    string home = "TESTDIR";
    ofstream *out = 0;

    env->set_event_notify(event_catcher);
    flags = DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_REP |
        DB_INIT_TXN | DB_CREATE | DB_THREAD;

    env->set_error_stream(&cerr);
    env->set_message_stream(&cerr);
    string cmd;
    while (cin >> cmd) {
        if (cmd == "exit")
            break;
        else if (cmd == "home") {
            home = token();
        } else if (cmd == "die") {
            return (0);
        } else if (cmd == "remote") { // remote [-p] host port
            u_int32_t flags = 0;
            string host;
            string t1 = token();
            if (t1 == "-p") {
                flags = DB_REPMGR_PEER;
                host = token();
            } else
                host = t1;
            int p = itoken();
            env->repmgr_add_remote_site(host.c_str(), p, NULL, flags);
        } else if (cmd == "local") {
            int p = itoken();
            env->repmgr_set_local_site("localhost", p, 0);
        } else if (cmd == "open_env") {
            env->open(home.c_str(), flags, 0);
        } else if (cmd == "open_db") {
            dbp = new Db(env, 0);
            u_int32_t flags = DB_AUTO_COMMIT;

            DB_REP_STAT *statp;
            env->rep_stat(&statp, 0);
            if (statp->st_status == DB_REP_MASTER)
                flags |= DB_CREATE;
            free(statp);
            
            string name = token();
            dbp->open(0, name.c_str(), 0, DB_BTREE, flags, 0);
        } else if (cmd == "start") {
            int ret = env->repmgr_start(3, start_type(token()));

            /*
             * repmgr_start can now return either 0 or DB_REP_IGNORE,
             * either of which are legal, non-error values.  So, echo
             * back onto 'cout' the information as to which value we
             * got, because some tests will care.
             */
            ostream *temp = env->get_error_stream();
            ostringstream buf;
            env->set_error_stream(&buf);
            env->err(ret, "start");
            env->set_error_stream(temp);
            istringstream in(buf.str());
            string result;
            getline(in, result);

            cout << result << endl;
            if (out)
                env->errx("%s", result.c_str());
        } else if (cmd == "put") {
            string key = token();
            string data = token();
            char *key_in_c = const_cast<char*>(key.c_str());
            char *value_in_c = const_cast<char*>(data.c_str());
            Dbt k(key_in_c, (int)key.length());
            Dbt v(value_in_c, (int)data.length());
            dbp->put(0, &k, &v, 0);
        } else if (cmd == "output") {
            env->set_error_stream(0);
            env->set_message_stream(0);
            if (out) {
                out->close();
                delete out;
                out = 0;
            }
            string fname = token();
            out = new ofstream(fname.c_str());
            (*out) << unitbuf;
            env->set_error_stream(out);
            env->set_message_stream(out);
        } else if (cmd == "get") {
            string key = token();
            char *key_in_c = const_cast<char*>(key.c_str());
            Dbt k(key_in_c, (int)key.length());
            Dbc *dbc;
            dbp->cursor(0, &dbc, 0);
            Dbt d;
            if (dbc->get(&k, &d, DB_SET) == DB_NOTFOUND)
                cout << "***NOTFOUND***" << endl;
            else {
                char *data = (char*)d.get_data();
                string dat(data, d.get_size());
                cout << dat << endl;
            }
            dbc->close();
        } else if (cmd == "echo") {
            // This is useful in tests, to synchronize the workflow.
            // A test sends an "echo" command, and then reads output
            // waiting for the resulting text.  When we get the
            // output, we know that any commands that were sent before
            // the "echo" have now been executed.  (Otherwise it's
            // like "type-ahead": we send a barrage of commands, but
            // we don't know when they've been completely done.)
            // 
            string w = token();
            cout << w << endl;
            if (out)
                env->errx("echo: %s", w.c_str());
        } else
            throw "unknown command";
    }

    if (dbp) {
        dbp->close(0);
        delete dbp;
    }
    env->close(0);
    delete env;
    if (out) {
        out->close();
        delete out;
    }
    return 0;
}
        
