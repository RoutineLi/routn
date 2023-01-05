/*************************************************************************
	> File Name: Sqlite.h
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2023年1月5日 星期四 10时38分01秒
 ************************************************************************/

#ifndef __DB_SQLITE_H_
#define __DB_SQLITE_H_

#include <sqlite3.h>
#include <memory>
#include <string>
#include <list>
#include <map>
#include "DateBase.h"
#include "../Noncopyable.h"
#include "../Thread.h"
#include "../Singleton.h"


namespace Routn{

    class SQLite3Stmt;
namespace{
    template<size_t N, typename... Args>
    struct SQLite3Binder{
        static int Bind(std::shared_ptr<SQLite3Stmt> stmt) { return SQLITE_OK;}
    };
}

    class SQLite3Manager;
    class SQLite3 : public IDB, public std::enable_shared_from_this<SQLite3>{
    friend class SQLite3Manager;
    public:
        enum Flags{
            READONLY = SQLITE_OPEN_READONLY,
            READWRITE = SQLITE_OPEN_READWRITE,
            CREATE = SQLITE_OPEN_CREATE
        };
        using ptr = std::shared_ptr<SQLite3>;
        static SQLite3::ptr Create(sqlite3* db);
        static SQLite3::ptr Create(const std::string& dbname, int flags = READWRITE | CREATE);
        ~SQLite3();

        IStmt::ptr prepare(const std::string& stmt) override;

        int getErrno() override;
        std::string getErrStr() override;

        int execute(const char* format, ...) override;
        int execute(const char* format, va_list ap);
        int execute(const std::string& sql) override;
        int64_t getLastInsertId() override;
        ISQLData::ptr query(const char* format, ...) override;
        ISQLData::ptr query(const std::string& sql) override;

        ITranscation::ptr openTransaction(bool auto_commit = false) override;

        template<typename... Args>
        int execStmt(const char* stmt, Args&&... args);

        template<class... Args>
        ISQLData::ptr queryStmt(const char* stmt, const Args&... args);

        int close();

        sqlite3* getDB() const { return _db;}
    private:
        SQLite3(sqlite3* db);
    private:
        sqlite3* _db;
        uint64_t _lastUsedTime = 0;
    };

    class SQLite3Stmt;
    class SQLite3Data : public ISQLData{
    public:
        using ptr = std::shared_ptr<SQLite3Data>;
        SQLite3Data(std::shared_ptr<SQLite3Stmt> stmt, int err, const char* errstr);
        int getErrno() const override { return _errno;}
        const std::string& getErrStr() const override { return _errstr;}

        int getDataCount() override;
        int getColumnCount() override;
        int getColumnBytes(int idx);
        int getColumnType(int idx);

        std::string getColumnName(int idx);

        bool isNull(int idx) override;
        int8_t getInt8(int idx) override;
        uint8_t getUint8(int idx) override;
        int16_t getInt16(int idx) override;
        uint16_t getUint16(int idx) override;
        int32_t getInt32(int idx) override;
        uint32_t getUint32(int idx) override;
        int64_t getInt64(int idx) override;
        uint64_t getUint64(int idx) override;
        float getFloat(int idx) override;
        double getDouble(int idx) override;
        std::string getString(int idx) override;
        std::string getBlob(int idx) override;
        time_t getTime(int idx) override;

        bool next();
    private:
        int _errno;
        bool _first;
        std::string _errstr;
        std::shared_ptr<SQLite3Stmt> _stmt;
    };

    class SQLite3Stmt : public IStmt, public std::enable_shared_from_this<SQLite3Stmt>{
        friend class SQLite3Data;
    public:
        using ptr = std::shared_ptr<SQLite3Stmt>;
        enum Type{
            COPY = 1,
            REF = 2
        };
        static SQLite3Stmt::ptr Create(SQLite3::ptr db, const char* stmt);

        int prepare(const char* stmt);
        ~SQLite3Stmt();
        int finish();

        int bind(int idx, int32_t value);
        int bind(int idx, uint32_t value);
        int bind(int idx, double value);
        int bind(int idx, int64_t value);
        int bind(int idx, uint64_t value);
        int bind(int idx, const char* value, Type type = COPY);
        int bind(int idx, const void* value, int len, Type type = COPY);
        int bind(int idx, const std::string& value, Type type = COPY);

        // for null type
        int bind(int idx);

        int bindInt8(int idx, const int8_t& value) override;
        int bindUint8(int idx, const uint8_t& value) override;
        int bindInt16(int idx, const int16_t& value) override;
        int bindUint16(int idx, const uint16_t& value) override;
        int bindInt32(int idx, const int32_t& value) override;
        int bindUint32(int idx, const uint32_t& value) override;
        int bindInt64(int idx, const int64_t& value) override;
        int bindUint64(int idx, const uint64_t& value) override;
        int bindFloat(int idx, const float& value) override;
        int bindDouble(int idx, const double& value) override;
        int bindString(int idx, const char* value) override;
        int bindString(int idx, const std::string& value) override;
        int bindBlob(int idx, const void* value, int64_t size) override;
        int bindBlob(int idx, const std::string& value) override;
        int bindTime(int idx, const time_t& value) override;
        int bindNull(int idx) override;

        int bind(const char* name, int32_t value);
        int bind(const char* name, uint32_t value);
        int bind(const char* name, double value);
        int bind(const char* name, int64_t value);
        int bind(const char* name, uint64_t value);
        int bind(const char* name, const char* value, Type type = COPY);
        int bind(const char* name, const void* value, int len, Type type = COPY);
        int bind(const char* name, const std::string& value, Type type = COPY);
        // for null type
        int bind(const char* name);

        int step();
        int reset();

        ISQLData::ptr query() override;
        int execute() override;
        int64_t getLastInsertId() override;

        int getErrno() override;
        std::string getErrStr() override;
    protected:
        SQLite3Stmt(SQLite3::ptr db);
    private:
        SQLite3::ptr _db;
        sqlite3_stmt* _stmt;
    };

    class SQLite3Transcation : public ITranscation{
    public:
        enum Type {
            DEFERRED = 0,
            IMMEDIATE = 1,
            EXCLUSIVE = 2
        };
        SQLite3Transcation(SQLite3::ptr db, bool auto_commit = false, Type type = DEFERRED);
        ~SQLite3Transcation();
        bool begin() override;
        bool commit() override;
        bool rollback() override;

        int execute(const char* format, ...) override;
        int execute(const std::string& sql) override;
        int64_t getLastInsertId() override;
    private:
        SQLite3::ptr _db;
        Type _type;
        int8_t _status;
        bool _autoCommit;
    };

    class SQLite3Manager{
    public:
        using MutexType = Routn::Mutex;
        SQLite3Manager();
        ~SQLite3Manager();

        SQLite3::ptr get(const std::string& name);
        void registerSQLite3(const std::string& name, const std::map<std::string, std::string>& params);

        void checkConnection(int sec = 30);

        uint32_t getMaxConn() const { return _maxConn;}
        void setMaxConn(uint32_t v) { _maxConn = v;}

        
        int execute(const std::string& name, const char* format, ...);
        int execute(const std::string& name, const char* format, va_list ap);
        int execute(const std::string& name, const std::string& sql);

        ISQLData::ptr query(const std::string& name, const char* format, ...);
        ISQLData::ptr query(const std::string& name, const char* format, va_list ap); 
        ISQLData::ptr query(const std::string& name, const std::string& sql);
        
        SQLite3Transcation::ptr openTransaction(const std::string& name, bool auto_commit);
    private:
        void freeSQLite3(const std::string& name, SQLite3* m);
    private:
        uint32_t _maxConn;
        MutexType _mutex;
        std::map<std::string, std::list<SQLite3*>> _conns;
        std::map<std::string, std::map<std::string, std::string>> _dbDefines;
    };

    using SQLite3Mgr = Routn::Singleton<SQLite3Manager>;

namespace{
    template<typename... Args>
    int bindX(SQLite3Stmt::ptr stmt, const Args&... args){
        return SQLite3Binder<1, Args...>::Bind(stmt, args...);
    }
}
    template<typename... Args>
    int SQLite3::execStmt(const char* stmt, Args&&... args){
        auto st = SQLite3Stmt::Create(shared_from_this(), stmt);
        if(!st){
            return -1;
        }
        int rt = bindX(st, args...);
        if(rt != SQLITE_OK){
            return rt;
        }
        return st->execute();
    }

    template<class ...Args>
    ISQLData::ptr SQLite3::queryStmt(const char* stmt, const Args&... args){
        auto st = SQLite3Stmt::Create(shared_from_this(), stmt);
        if(!st){
            return nullptr;
        }
        int rt = bindX(st, args...);
        if(rt != SQLITE_OK){
            return nullptr;
        }
        return st->query();
    }

namespace{

    template<size_t N, typename Head, typename... Tail>
    struct SQLite3Binder<N, Head, Tail...>{
        static int Bind(SQLite3Stmt::ptr stmt, const Head&, const Tail&...){
            static_assert(sizeof...(Tail) < 0, "invalid type");
            return SQLITE_OK;
        }
    };
}
#define XX(type, type2) \
    template<size_t N, typename... Tail> \
    struct SQLite3Binder<N, type, Tail...> {  \
        static int Bind(SQLite3Stmt::ptr stmt, const type2& value, const Tail&... tail){ \
            int rt = stmt->bind(N, value); \
            if(rt != SQLITE_OK){ \
                return rt; \
            }\
            return SQLite3Binder<N + 1, Tail...>::Bind(stmt, tail...);\
        } \
    } 

    XX(char*, char* const);
    XX(const char*, char* const);
    XX(std::string, std::string);
    XX(int8_t, int32_t);
    XX(uint8_t, int32_t);
    XX(int16_t, int32_t);
    XX(uint16_t, int32_t);
    XX(int32_t, int32_t);
    XX(uint32_t, int32_t);
    XX(int64_t, int64_t);
    XX(uint64_t, int64_t);
    XX(float, double);
    XX(double, double);
#undef XX

}

#endif