#include "sql_connection_pool.h"
#include <assert.h>
#include <iostream>
#include "../log/log.h"

using namespace std;
SqlConnectionPool::SqlConnectionPool(){
    curr_user_count_ = 0;
    free_user_count_ = 0;
}

SqlConnectionPool* SqlConnectionPool::Instance(){
    static SqlConnectionPool sql_connection_pool;
    return &sql_connection_pool;
}

void SqlConnectionPool::Init(const char* user,   const char* host, int port, 
              const char* passwd, const char* db_name, int connection_size=10)
{
    assert(connection_size>0);
    for(int i=0;i<connection_size;i++){
        MYSQL* connection = nullptr;
        connection = mysql_init(connection);
        if(!connection){
            LOG_ERROR("mysql initialization error");
            assert(connection);
        }
        connection = mysql_real_connect(connection, host, user, passwd, db_name, port, nullptr, 0);
        if(!connection){
            LOG_ERROR("mysql connection error");
        }
        connection_queue_.push(connection);
    }
    max_connection_num_  = connection_size;
    sem_init(&connection_sem_, 0, max_connection_num_);
}

MYSQL* SqlConnectionPool::GetConnection(){
    MYSQL* connection = nullptr;
    if(connection_queue_.empty()){
        LOG_WARN("SqlConnectionPool busy!");
        return nullptr;
    }
    sem_wait(&connection_sem_);
    lock_guard<mutex> locker(connection_mutex_);
    connection = connection_queue_.front();
    connection_queue_.pop();
    return connection;
}

void SqlConnectionPool::FreeConnection(MYSQL* connection){
    assert(connection);
    lock_guard<mutex> locker(connection_mutex_);
    connection_queue_.push(connection);
    sem_post(&connection_sem_);
}

int SqlConnectionPool::GetFreeConnectionCount(){
    lock_guard<mutex> locker(connection_mutex_);
    return connection_queue_.size();
}

void SqlConnectionPool::ClosePool(){
    lock_guard<mutex> locker(connection_mutex_);
    while(!connection_queue_.empty()){
        auto connection = connection_queue_.front();
        connection_queue_.pop();
        mysql_close(connection);
    }
    mysql_library_end();
}

SqlConnectionPool::~SqlConnectionPool(){
    ClosePool();
}
