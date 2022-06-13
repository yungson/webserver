#ifndef LOG_H
#define LOG_H


#include <thread>
#include <mutex>
#include "../buffer/buffer.h"
#include "blockqueue.h"
#include <assert.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdarg.h>

class Log{
public:
    void Init(int level, const char* path = "./log", int max_queue_capacity  = 1024);
    void Write(int level, const char* format, ...);
    void Flush();
    static Log* Instance() {
        static Log inst;
        return &inst;
    }
    static void FlushLogThread(){
        Log::Instance()->AsyncWrite_();
    }
    int GetLevel(){
        std::lock_guard<std::mutex> locker(mutex_);
        return level_;
    }
    bool IsOpen () {
        return is_open_;
    }

private:
    Log(){}
    virtual ~Log();
    void AppendLogLevelTitle_(int level);
    void AsyncWrite_();
    void CreateLogFile(char* file_name);
    void CreateNewLogFile(struct tm* t);
    static const int LOG_PATH_PEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;
    const char* path_;
    // const char* suffix_;
    int line_count_;
    int today_;
    bool is_open_;
    Buffer buff_;
    int level_;
    bool is_async_;
    FILE* log_fp_;
    std::unique_ptr<BlockDeque<std::string>> log_deque_;
    std::unique_ptr<std::thread> write_threads_;
    std::mutex mutex_;
};

#define LOG_BASE(level, format, ...){ \
    Log* log = Log::Instance();              \
    if(log->IsOpen() && log->GetLevel()<=level){ \
        log->Write(level, format, ##__VA_ARGS__); \
        log->Flush();\
    }                                 \
}
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0); 
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);
#endif
