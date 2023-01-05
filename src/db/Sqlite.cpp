/*************************************************************************
	> File Name: Sqlite.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年1月5日 星期四 10时38分01秒
 ************************************************************************/


#include "Sqlite.h"
#include "../Log.h"
#include "../Config.h"
#include "../Env.h"

static Routn::ConfigVar<std::map<std::string, std::map<std::string, std::string>>>::ptr g_sqlite3_dbs 
    = Routn::Config::Lookup("sqlite3.dbs", std::map<std::string, std::map<std::string, std::string>>(), "sqlite3 dbs");

namespace Routn{
    SQLite3::SQLite3(sqlite3* db)
        : _db(db){

    }

    SQLite3::~SQLite3(){
        close();
    }

    SQLite3::ptr SQLite3::Create(sqlite3* db){
        SQLite3::ptr rt(new SQLite3(db));
        return rt;
    }

    SQLite3::ptr SQLite3::Create(const std::string& dbname, int flags){
        sqlite3* db;
        if(sqlite3_open_v2(dbname.c_str(), &db, flags, nullptr) == SQLITE_OK){
            return SQLite3::ptr(new SQLite3(db));
        }
        return nullptr;
    }

    ITranscation::ptr SQLite3::openTransaction(bool auto_commit){
        ITranscation::ptr trans(new SQLite3Transcation(shared_from_this(), auto_commit));
        if(trans->begin()){
            return trans;
        }
        return nullptr;
    }

    int SQLite3::getErrno(){
        return sqlite3_errcode(_db);
    }

    std::string SQLite3::getErrStr(){
        return sqlite3_errmsg(_db);
    }

    int SQLite3::close(){
        int rc = SQLITE_OK;
        if(_db){
            rc = sqlite3_close(_db);
            if(rc == SQLITE_OK){
                _db = nullptr;
            }
        }
        return rc;
    }

    IStmt::ptr SQLite3::prepare(const std::string& stmt){
        return SQLite3Stmt::Create(shared_from_this(), stmt.c_str());
    }

    int SQLite3::execute(const char* format, ...){
        va_list ap;
        va_start(ap, format);
        int rt = execute(format, ap);
        va_end(ap);
        return rt;
    }

    int SQLite3::execute(const char* format, va_list ap){
       std::shared_ptr<char> sql(sqlite3_vmprintf(format, ap), sqlite3_free);
        return sqlite3_exec(_db, sql.get(), 0, 0, 0);
    }

    ISQLData::ptr SQLite3::query(const char* format, ...){
        va_list ap;
        va_start(ap, format);
        std::shared_ptr<char> sql(sqlite3_vmprintf(format, ap), sqlite3_free);
        va_end(ap);
        auto stmt = SQLite3Stmt::Create(shared_from_this(), sql.get());
        if(!stmt){
            return nullptr;
        }
        return stmt->query();
    }

    ISQLData::ptr SQLite3::query(const std::string& sql){
        auto stmt = SQLite3Stmt::Create(shared_from_this(), sql.c_str());
        if(!stmt){
            return nullptr;
        }
        return stmt->query();
    }

    int SQLite3::execute(const std::string& sql){
        return sqlite3_exec(_db, sql.c_str(), 0, 0, 0);
    }

    int64_t SQLite3::getLastInsertId(){
        return sqlite3_last_insert_rowid(_db);
    }

    SQLite3Stmt::ptr SQLite3Stmt::Create(SQLite3::ptr db, const char* stmt){
        SQLite3Stmt::ptr rt(new SQLite3Stmt(db));
        if(rt->prepare(stmt) != SQLITE_OK){
            return nullptr;
        }
        return rt;
    }

    int64_t SQLite3Stmt::getLastInsertId(){
        return _db->getLastInsertId();
    }

    int SQLite3Stmt::bindInt8(int idx, const int8_t& value) {
    return bind(idx, (int32_t)value);
}

    int SQLite3Stmt::bindUint8(int idx, const uint8_t& value) {
        return bind(idx, (int32_t)value);
    }

    int SQLite3Stmt::bindInt16(int idx, const int16_t& value) {
        return bind(idx, (int32_t)value);
    }

    int SQLite3Stmt::bindUint16(int idx, const uint16_t& value) {
        return bind(idx, (int32_t)value);
    }

    int SQLite3Stmt::bindInt32(int idx, const int32_t& value) {
        return bind(idx, (int32_t)value);
    }

    int SQLite3Stmt::bindUint32(int idx, const uint32_t& value) {
        return bind(idx, (int32_t)value);
    }

    int SQLite3Stmt::bindInt64(int idx, const int64_t& value) {
        return bind(idx, (int64_t)value);
    }

    int SQLite3Stmt::bindUint64(int idx, const uint64_t& value) {
        return bind(idx, (int64_t)value);
    }

    int SQLite3Stmt::bindFloat(int idx, const float& value) {
        return bind(idx, (double)value);
    }

    int SQLite3Stmt::bindDouble(int idx, const double& value) {
        return bind(idx, (double)value);
    }

    int SQLite3Stmt::bindString(int idx, const char* value) {
        return bind(idx, value);
    }

    int SQLite3Stmt::bindString(int idx, const std::string& value) {
        return bind(idx, value);
    }

    int SQLite3Stmt::bindBlob(int idx, const void* value, int64_t size) {
        return bind(idx, value, size);
    }

    int SQLite3Stmt::bindBlob(int idx, const std::string& value) {
        return bind(idx, (void*)&value[0], value.size());
    }

    int SQLite3Stmt::bindTime(int idx, const time_t& value) {
        return bind(idx, Routn::TimerToString(value));
    }

    int SQLite3Stmt::bindNull(int idx) {
        return bind(idx);
    }

    int SQLite3Stmt::getErrno(){
        return _db->getErrno();
    }    

    std::string SQLite3Stmt::getErrStr(){
        return _db->getErrStr();
    }

    int SQLite3Stmt::bind(int idx, int32_t value){
        return sqlite3_bind_int(_stmt, idx, value);
    }

    int SQLite3Stmt::bind(int idx, uint32_t value){
        return sqlite3_bind_int(_stmt, idx, value);
    }

    int SQLite3Stmt::bind(int idx, double value){
        return sqlite3_bind_double(_stmt, idx, value);
    }

    int SQLite3Stmt::bind(int idx, int64_t value){
        return sqlite3_bind_int64(_stmt, idx, value);
    }

    int SQLite3Stmt::bind(int idx, uint64_t value){
        return sqlite3_bind_int64(_stmt, idx, value);
    }

    int SQLite3Stmt::bind(int idx, const char* value, Type type){
        return sqlite3_bind_text(_stmt, idx, value, strlen(value), type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }

    int SQLite3Stmt::bind(int idx, const void* value, int len, Type type){
        return sqlite3_bind_blob(_stmt, idx, value, len, type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }

    int SQLite3Stmt::bind(int idx, const std::string& value, Type type){
        return sqlite3_bind_text(_stmt, idx, value.c_str(), value.size(), type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }

    int SQLite3Stmt::bind(int idx){
        return sqlite3_bind_null(_stmt, idx);
    }

    int SQLite3Stmt::bind(const char* name, int32_t value){
        auto idx = sqlite3_bind_parameter_index(_stmt, name);
        return bind(idx, value);
    }

    int SQLite3Stmt::bind(const char* name, uint32_t value){
        auto idx = sqlite3_bind_parameter_index(_stmt, name);
        return bind(idx, value);
    }

    int SQLite3Stmt::bind(const char* name, double value){
        auto idx = sqlite3_bind_parameter_index(_stmt, name);
        return bind(idx, value);
    }

    int SQLite3Stmt::bind(const char* name, int64_t value){
        auto idx = sqlite3_bind_parameter_index(_stmt, name);
        return bind(idx, value);
    }

    int SQLite3Stmt::bind(const char* name, const char* value, Type type){
        auto idx = sqlite3_bind_parameter_index(_stmt, name);
        return bind(idx, value, type);
    }

    int SQLite3Stmt::bind(const char* name, const void* value, int len, Type type){
        auto idx = sqlite3_bind_parameter_index(_stmt, name);
        return bind(idx, value, len, type);
    }

    int SQLite3Stmt::bind(const char* name , const std::string& value, Type type){
        auto idx = sqlite3_bind_parameter_index(_stmt, name);
        return bind(idx, value, type);
    }

    int SQLite3Stmt::bind(const char* name){
        auto idx = sqlite3_bind_parameter_index(_stmt, name);
        return bind(idx);
    }

    int SQLite3Stmt::step(){
        return sqlite3_step(_stmt);
    }

    int SQLite3Stmt::reset(){
        return sqlite3_reset(_stmt);
    }

    int SQLite3Stmt::prepare(const char* stmt){
        auto rt = finish();
        if(rt != SQLITE_OK){
            return rt;
        }

        return sqlite3_prepare_v2(_db->getDB(), stmt, strlen(stmt), &_stmt, nullptr);
    }

    int SQLite3Stmt::finish(){
        auto rc = SQLITE_OK;
        if(_stmt){
            rc = sqlite3_finalize(_stmt);
            _stmt = nullptr;
        }
        return rc;
    }

    SQLite3Stmt::SQLite3Stmt(SQLite3::ptr db)
        : _db(db)
        , _stmt(nullptr){

    }

    SQLite3Stmt::~SQLite3Stmt(){
        finish();
    }

    ISQLData::ptr SQLite3Stmt::query() {
        return SQLite3Data::ptr(new SQLite3Data(shared_from_this(), 0, ""));
    }

    int SQLite3Stmt::execute(){
        int rt = step();
        if(rt == SQLITE_DONE){
            rt = SQLITE_OK;
        }
        return rt;
    }

    SQLite3Data::SQLite3Data(std::shared_ptr<SQLite3Stmt> stmt, int err, const char* errstr)
        : _errno(err)
        , _first(true)
        , _errstr(errstr)
        , _stmt(stmt){

    }

    int SQLite3Data::getDataCount(){
        return sqlite3_data_count(_stmt->_stmt);
    }

    int SQLite3Data::getColumnCount(){
        return sqlite3_column_count(_stmt->_stmt);
    }

    int SQLite3Data::getColumnBytes(int idx){
        return sqlite3_column_bytes(_stmt->_stmt, idx);
    }

    int SQLite3Data::getColumnType(int idx){
        return sqlite3_column_type(_stmt->_stmt, idx);
    }

    std::string SQLite3Data::getColumnName(int idx){
        const char* name = sqlite3_column_name(_stmt->_stmt, idx);
        if(name){
            return name;
        }
        return "";
    }

    bool SQLite3Data::isNull(int idx){
        return false;
    }

    int8_t SQLite3Data::getInt8(int idx){
        return getInt32(idx);
    }
    
    uint8_t SQLite3Data::getUint8(int idx) {
        return getInt32(idx);
    }

    int16_t SQLite3Data::getInt16(int idx) {
        return getInt32(idx);
    }

    uint16_t SQLite3Data::getUint16(int idx) {
        return getInt32(idx);
    }

    int32_t SQLite3Data::getInt32(int idx){
        return sqlite3_column_int(_stmt->_stmt, idx);
    }

    uint32_t SQLite3Data::getUint32(int idx){
        return getInt32(idx);
    }

    int64_t SQLite3Data::getInt64(int idx){
        return sqlite3_column_int64(_stmt->_stmt, idx);
    }

    uint64_t SQLite3Data::getUint64(int idx) {
        return getInt64(idx);
    }

    float SQLite3Data::getFloat(int idx) {
        return getDouble(idx);
    }

    double SQLite3Data::getDouble(int idx){
        return sqlite3_column_double(_stmt->_stmt, idx);
    }

    std::string SQLite3Data::getString(int idx){
        const char* v = (const char*)sqlite3_column_text(_stmt->_stmt, idx);
        if(v){
            return std::string(v, getColumnBytes(idx));
        }
        return "";
    }

    std::string SQLite3Data::getBlob(int idx){
        const char* v = (const char*)sqlite3_column_blob(_stmt->_stmt, idx);
        if(v){
            return std::string(v, getColumnType(idx));
        }
        return "";
    }

    time_t SQLite3Data::getTime(int idx){
        auto str = getString(idx);
        struct tm t;
        memset(&t, 0, sizeof(t));
        if(!strptime(str.c_str(), "%Y-%m-%d %H:%M:%S", &t)) {
            return 0;
        }
        return mktime(&t);
    }

    bool SQLite3Data::next(){
        int rt = _stmt->step();
        if(_first){
            _errno = _stmt->getErrno();
            _errstr = _stmt->getErrStr();
            _first = false;
        }
        return rt == SQLITE_ROW;
    } 

    SQLite3Transcation::SQLite3Transcation(SQLite3::ptr db, bool auto_commit, Type type)
        : _db(db)
        , _type(type)
        , _status(0)
        , _autoCommit(auto_commit){

    }

    SQLite3Transcation::~SQLite3Transcation(){
        if(_status == 1){
            if(_autoCommit){
                commit();
            }else{
                rollback();
            }

            if(_status == 1){
                ROUTN_LOG_ERROR(g_logger) << _db << "auto_commit = " << _autoCommit << "fail";
            }
        }
    }

    int SQLite3Transcation::execute(const char* format, ...){
        va_list ap;
        va_start(ap, format);
        int rt = _db->execute(format, ap);
        va_end(ap);
        return rt;
    }

    int SQLite3Transcation::execute(const std::string& sql){
        return _db->execute(sql);
    }

    int64_t SQLite3Transcation::getLastInsertId(){
        return _db->getLastInsertId();
    }

    bool SQLite3Transcation::begin(){
        if(_status == 0){
            const char* sql = "BEGIN";
            if(_type == IMMEDIATE){
                sql = "BEGIN IMMEDIATE";
            }else if(_type == EXCLUSIVE){
                sql = "BEGIN EXCLUSIVE";
            }
            int rt = _db->execute(sql);
            if(rt == SQLITE_OK){
                _status = 1;
            }
            return rt == SQLITE_OK;
        }
        return false;
    }

    bool SQLite3Transcation::commit(){
        if(_status == 1){
            int rc = _db->execute("COMMIT");
            if(rc == SQLITE_OK){
                _status = 2;
            }
            return rc == SQLITE_OK;
        }
        return false;
    }

    bool SQLite3Transcation::rollback(){
        if(_status == 1){
            int rc = _db->execute("ROLLBACK");
            if(rc == SQLITE_OK){
                _status = 3;
            }
            return rc == SQLITE_OK;
        }
        return false;
    }

    SQLite3Manager::SQLite3Manager()
        : _maxConn(10){

    }

    SQLite3Manager::~SQLite3Manager(){
        for(auto& i : _conns){
            for(auto& n : i.second)
                delete n;
        }
    }

    SQLite3::ptr SQLite3Manager::get(const std::string& name){
        MutexType::Lock lock(_mutex);
        auto it = _conns.find(name);
        if(it != _conns.end()){
            if(!it->second.empty()){
                SQLite3* rt = it->second.front();
                it->second.pop_front();
                lock.unlock();
                return SQLite3::ptr(rt, std::bind(&SQLite3Manager::freeSQLite3, this, name, std::placeholders::_1));
            }
        }
        auto config = g_sqlite3_dbs->getValue();
        auto sit = config.find(name);
        std::map<std::string, std::string> args;
        if(sit != config.end()){
            args = sit->second;
        }else{
            return nullptr;
        }
        lock.unlock();
        std::string path = Routn::GetParamValue<std::string>(args, "path");
        if(path.empty()){
            ROUTN_LOG_ERROR(g_logger) << "open db name = " << name << " path is null";
            return nullptr;
        }
        if(path.find(":") == std::string::npos){
            static Routn::ConfigVar<std::string>::ptr g_server_work_path = 
                Routn::Config::Lookup<std::string>("server.work_path");
            path = g_server_work_path->getValue() + "/" + path;
        }
        sqlite3* db;
        if(sqlite3_open_v2(path.c_str(), &db, SQLite3::CREATE | SQLite3::READWRITE, nullptr)){
            ROUTN_LOG_ERROR(g_logger) << "open db name = " << name << " path is null";
            return nullptr;
        }

        SQLite3* rt = new SQLite3(db);
        std::string sql = Routn::GetParamValue<std::string>(args, "sql");
        if(!sql.empty()){
            ROUTN_LOG_ERROR(g_logger) << "execute sql = " << sql << " errno = " 
                << rt->getErrno() << " reason = " << rt->getErrStr();
            delete rt;
            return nullptr;
        }   
        rt->_lastUsedTime = time(0);
        return SQLite3::ptr(rt, std::bind(&SQLite3Manager::freeSQLite3, this, name, std::placeholders::_1));
    }

    void SQLite3Manager::registerSQLite3(const std::string& name, const std::map<std::string, std::string>& params){
        MutexType::Lock lock(_mutex);
        _dbDefines[name] = params;
    }

    void SQLite3Manager::checkConnection(int sec){
        time_t now = time(0);
        std::vector<SQLite3*> conns;
        MutexType::Lock lock(_mutex);
        for(auto& i : _conns){
            for(auto it = i.second.begin(); it != i.second.end();){
                if((int)(now - (*it)->_lastUsedTime) >= sec){
                    auto tmp = *it;
                    i.second.erase(it++);
                    conns.push_back(tmp);
                }else{
                    ++it;
                }
            }
        }
        lock.unlock();
        for(auto& i : conns){
            delete i;
        }
    }

    int SQLite3Manager::execute(const std::string& name, const char* format, ...){
        va_list ap;
        va_start(ap, format);
        int rt = execute(name, format, ap);
        va_end(ap);
        return rt;
    }

    int SQLite3Manager::execute(const std::string& name, const char* format, va_list ap){
        auto conn = get(name);
        if(!conn){
            ROUTN_LOG_ERROR(g_logger) << "SQLite3Manager::execute, get(" << name 
                << ") fail, format = " << format;
            return -1;
        }
        return conn->execute(format, ap);
    }

    int SQLite3Manager::execute(const std::string& name, const std::string& sql){
        auto conn = get(name);
        if(!conn){
            ROUTN_LOG_ERROR(g_logger) << "SQLite3Manager::execute, get(" << name 
                << ") fail, sql = " << sql;
            return -1;            
        }
        return conn->execute(sql);
    }

    ISQLData::ptr SQLite3Manager::query(const std::string& name, const char* format, ...){
        va_list ap;
        va_start(ap, format);
        auto res = query(name, format, ap);
        va_end(ap);
        return res;
    }

    ISQLData::ptr SQLite3Manager::query(const std::string& name, const char* format, va_list ap){
        auto conn = get(name);
        if(!conn){
            ROUTN_LOG_ERROR(g_logger) << "SQLite3Manager::execute, query(" << name 
                << ") fail, format = " << format;
            return nullptr;            
        }
        return conn->query(format, ap);
    } 

    ISQLData::ptr SQLite3Manager::query(const std::string& name, const std::string& sql){
        auto conn = get(name);
        if(!conn) {
            ROUTN_LOG_ERROR(g_logger) << "SQLite3Manager::query, get(" << name
                << ") fail, sql=" << sql;
            return nullptr;
        }     
        return conn->query(sql);   
    }
    
    SQLite3Transcation::ptr SQLite3Manager::openTransaction(const std::string& name, bool auto_commit){
        auto conn = get(name);
        if(!conn){
            ROUTN_LOG_ERROR(g_logger) << "SQLite3Manager::openTransaction, get(" << name
            << ") fail";
            return nullptr;
        }
        SQLite3Transcation::ptr trans(new SQLite3Transcation(conn, auto_commit));
        return trans;
    }

    void SQLite3Manager::freeSQLite3(const std::string& name, SQLite3* m){
        if(m->_db){
            MutexType::Lock lock(_mutex);
            if(_conns[name].size() < _maxConn){
                _conns[name].push_back(m);
                return ;
            }
        }
        delete m;
    }

}
