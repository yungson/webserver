#ifndef SQLCONNECTION_RAII_H
#define SQLCONNECTION_RAII_H

#include "sql_connection_pool.h"
#include <assert.h>

class SqlConnectionRaii {

public:
    SqlConnectionRaii(MYSQL** connection, SqlConnectionPool* sql_connection_pool){
        assert(sql_connection_pool);
        *connection = sql_connection_pool->GetConnection();
        connection_ = *connection;
        sql_connection_pool_ = sql_connection_pool;

    }
    ~SqlConnectionRaii(){
        if(connection_){
            sql_connection_pool_->FreeConnection(connection_);
        }
    }

private:
    MYSQL* connection_;
    SqlConnectionPool* sql_connection_pool_;
};

#endif