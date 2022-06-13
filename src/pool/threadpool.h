#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <thread> 
#include <condition_variable>
#include <queue>
#include <functional> 
#include <assert.h>
#include "../log/log.h"
class ThreadPool{

public:
    explicit ThreadPool(size_t num_thread = 8): pool_(std::make_shared<Pool>()){
        assert(num_thread>0);
        for(size_t i=0; i<num_thread;i++){
            std::thread([pool = pool_]{ // thread is a template class, accepts a function for thread execution
                std::unique_lock<std::mutex> locker(pool->mutex_lock); 
                while(true){
                    if(!pool->tasks.empty()){
                        auto task = move(pool->tasks.front()); 
                        pool->tasks.pop();
                        LOG_DEBUG("dealing with a task!");
                        locker.unlock(); 
                        task();
                        locker.lock(); 
                        LOG_DEBUG("dealing with a task success!");
                    }else if(pool->is_closed){
                        break;
                    }else{
                        pool->cond.wait(locker);// block wait
                    }
                }
            }).detach();
        }
    }

    ~ThreadPool() {
        if(static_cast<bool>(pool_)){
            std::lock_guard<std::mutex> locker(pool_->mutex_lock);
            pool_->is_closed = true;
        }
        pool_->cond.notify_all();
    }

    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;

    template<class F>
    void AddTask(F&& task){
        std::lock_guard<std::mutex> locker(pool_->mutex_lock);
        pool_->tasks.emplace(std::forward<F>(task)); // Forward：right reference is actually a left value
        pool_->cond.notify_one(); 
    }

private:

    struct Pool {
        std::mutex mutex_lock;
        std::condition_variable cond;
        bool is_closed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};


#endif //THREADPOOL_H