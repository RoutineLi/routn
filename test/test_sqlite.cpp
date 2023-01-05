#include "../src/Log.h"
#include "../src/Util.h"
#include "../src/db/Sqlite.h"

using namespace Routn;

static Logger::ptr g_logger = ROUTN_LOG_ROOT();

void test(SQLite3::ptr db){
    auto ts = GetCurrentMs();
    int n = 10000000;
    SQLite3Transcation trans(db);
    trans.begin();
    SQLite3Stmt::ptr stmt = SQLite3Stmt::Create(db
            , "insert into user(name, age) values(?, ?)");
    for(int i = 0; i < n; ++i){
        stmt->reset();
        stmt->bind(1, "batch_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->step();
    }
    trans.commit();
    auto ts2 = GetCurrentMs();

    ROUTN_LOG_INFO(g_logger) << "used: " << (ts2 - ts) / 1000.0 << "s batch insert n = " << n;
}

int main(int argc, char** argv){
    const std::string dbname = "/home/test/test.db";
    auto db = SQLite3::Create(dbname, SQLite3::READWRITE);
    if(!db){
        ROUTN_LOG_INFO(g_logger) << "dbname = " << dbname << " not exists";
        db = SQLite3::Create(dbname, SQLite3::READWRITE | SQLite3::CREATE);
        if(!db){
            ROUTN_LOG_INFO(g_logger) << "db create fail";
            return 0;
        }
    }
#define XX(...) #__VA_ARGS__
    int rt = db->execute(
XX(create table user(
        id integer primary key autoincrement,
        name varchar(50) not null default "",
        age int not null default 0,
        create_time datetime
        )));
#undef XX

    if(rt != SQLITE_OK){
        ROUTN_LOG_INFO(g_logger) << "create table error, errno = "
            << db->getErrno() << ", reason: " << db->getErrStr();
        return 0;
    }

    for(int i = 0; i < 10; ++i){
        if(db->execute("insert into user(name, age) values(\"name_%d\", %d)", i, i) != SQLITE_OK){
            ROUTN_LOG_INFO(g_logger) << "insert into error from [name_" << i << "], errno = " 
                << db->getErrno() << ", reason: " << db->getErrStr(); 
        }
    }

    SQLite3Stmt::ptr stmt = SQLite3Stmt::Create(db, "insert into user(name, age, create_time) values(?,?,?)");
    if(!stmt){
        ROUTN_LOG_ERROR(g_logger) << "create statement error ";
        return 0;
    }

    int64_t now = time(0);
    for(int i = 0; i < 10; ++i){
        stmt->bind(1, "stmt_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->bind(3, now + rand() % 100);

        if(stmt->execute() != SQLITE_OK){
            ROUTN_LOG_ERROR(g_logger) << "execute statment error ";
        }
        stmt->reset();
    }

    SQLite3Stmt::ptr query = SQLite3Stmt::Create(db, "select * from user");
    if(!query){
        ROUTN_LOG_ERROR(g_logger) << "create statement error";
        return 0;
    }
    auto ds = query->query();
    if(!ds){
        ROUTN_LOG_ERROR(g_logger) << "query error";
        return 0;
    }
    while(ds->next()){

    };

    const std::string v = "hello ' world";
    db->execStmt("insert into user(name) values (?)", v);
    auto dd = (db->queryStmt("select * from user"));
    while(dd->next()){
        ROUTN_LOG_INFO(g_logger) << "ds.data_count = " << dd->getDataCount()
            << " ds.column_count = " << dd->getColumnCount()
            << " 0=" << dd->getInt32(0) << " 1=" << dd->getString(1)
            << " 2=" << dd->getString(2)
            << " 3=" << dd->getString(3);
    }

    test(db);
    return 0;
}