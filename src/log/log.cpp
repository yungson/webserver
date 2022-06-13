#include "log.h"
#include <iostream>

void Log::Init(int level = 1, const char *path, int max_queue_capacity) {
    level_ = level;
    path_ = path;
    line_count_ = 0;
    is_async_ = false;
    is_open_ = true;
    time_t timer = time(nullptr);
    struct tm *t = localtime(&timer);
    today_ = t->tm_mday;
    if(max_queue_capacity>0){
        is_async_ = true;
        std::unique_ptr<BlockDeque<std::string> > new_deque(new BlockDeque<std::string>);
        log_deque_ = std::move(new_deque);
        std::unique_ptr<std::thread> new_threads(new std::thread(FlushLogThread)); // only creating a single thread for log
        write_threads_ = move(new_threads);
    }
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN-1, "%s/%04d_%02d_%02d.log", path_, t->tm_year+1900, t->tm_mon+1, t->tm_mday);
    std::lock_guard<std::mutex> locker(mutex_);
    buff_.RetrieveAll();
    CreateLogFile(file_name);
}

void Log::CreateLogFile(char* file_name){
    log_fp_ = fopen(file_name, "a");
    if(log_fp_ == nullptr){
        mkdir(path_, 0777);
        log_fp_ = fopen(file_name, "a");
    }
    assert(log_fp_!=nullptr);
}


void Log::CreateNewLogFile(struct tm* t){
    std::unique_lock<std::mutex> locker(mutex_);
    locker.unlock(); 
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN-1, "%s/%04d_%02d_%02d.log", path_, t->tm_year+1900, t->tm_mon+1, t->tm_mday);
    if(today_!=t->tm_mday){
        snprintf(file_name, LOG_NAME_LEN-1, "%s/%04d_%02d_%02d.log", path_, t->tm_year+1900, t->tm_mon+1, t->tm_mday);
        today_ = t->tm_mday;
        line_count_ = 0;
    }else{
        snprintf(file_name, LOG_NAME_LEN-1, "%s/%04d_%02d_%02d-%d.log", path_, t->tm_year+1900, t->tm_mon+1, t->tm_mday, line_count_/MAX_LINES);
    }
    locker.lock(); 
    Flush();
    fclose(log_fp_);
    CreateLogFile(file_name);
}


void Log::Write(int level, const char *format, ...) {
    time_t timer = time(nullptr);
    struct tm *t = localtime(&timer);

    if(today_!=t->tm_mday ||(line_count_ && line_count_%MAX_LINES ==0)){
        CreateNewLogFile(t);
    }

    //write the title
    std::unique_lock<std::mutex> locker(mutex_);
    line_count_++;
    int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d ", t->tm_year+1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    buff_.HasWritten(n);
    AppendLogLevelTitle_(level);

    //write the message
    va_list var_args_list;
    va_start(var_args_list, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, var_args_list);
    va_end(var_args_list);
    buff_.HasWritten(m);
    buff_.Append("\n\0",2);

    // add to the log_deque_ if asynchronously or fputs to the file directly.
    if(is_async_ && log_deque_ && !log_deque_->Full()){
        log_deque_->PushBack(buff_.RetrieveAllToStr());
    }else{
        fputs(buff_.Peek(), log_fp_); 
    }
    buff_.RetrieveAll();
} 


void Log::AppendLogLevelTitle_(int level) {
    switch(level) {
        case 0:
            buff_.Append("[debug]: ", 9);
            break;
        case 1:
            buff_.Append("[info] : ", 9);
            break;
        case 2:
            buff_.Append("[warn] : ", 9);
            break;
        case 3:
            buff_.Append("[error]: ", 9);
            break;
        default:
            buff_.Append("[info] : ", 9);
            break;
    }
}

void Log::Flush(){
    if(is_async_){
        log_deque_->Flush();
    }
    fflush(log_fp_); 
}

void Log::AsyncWrite_() {
    std::string str = "";
    while(log_deque_->Pop(str)){
        std::lock_guard<std::mutex> locker(mutex_);
        fputs(str.c_str(), log_fp_);
    }
}

Log::~Log(){
    if(write_threads_ && write_threads_->joinable()){
        while(!log_deque_->Empty()){
            log_deque_->Flush();
        }
        log_deque_->Close();
        write_threads_->join();
    }
    if(log_fp_){
        std::lock_guard<std::mutex> locker(mutex_);
        Flush();
        fclose(log_fp_);
    }
}