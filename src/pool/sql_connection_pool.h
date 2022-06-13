#ifndef SQLCONNECTIONPOOL_H
#define SQLCONNECTIONPOOL_H

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>

using namespace std;

class SqlConnectionPool {

public:
    static SqlConnectionPool* Instance();
    MYSQL *GetConnection();
    void FreeConnection(MYSQL* connection);
    int GetFreeConnectionCount();
    void Init(const char* user,   const char* host, int port, 
              const char* passwd, const char* db_name, int connection_size);
    void ClosePool();

private:
    SqlConnectionPool();
    ~SqlConnectionPool();

    int max_connection_num_;
    int curr_user_count_;
    int free_user_count_;

    queue<MYSQL*> connection_queue_;
    mutex connection_mutex_;
    sem_t connection_sem_;
};

#endif