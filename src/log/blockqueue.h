#ifndef BLOCKQUEUE_H

#define BLOCKQUEUE_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <assert.h>

template<class T>
class BlockDeque{

public:
    explicit BlockDeque(size_t max_capacity = 1000); 
    ~BlockDeque();
    void Clear();
    bool Empty();
    bool Full();
    void Close();
    size_t Size();
    size_t Capacity();

    T Front();
    T Back();

    void PushBack(const T& item);
    void PushFront(const T& item);
    bool Pop(T &item);
    bool Pop(T &item, int time_out);
    void Flush();

private:
    std::mutex mtx_;
    std::condition_variable cond_consumer_;
    std::condition_variable cond_producer_;
    std::deque<T> deq_;
    size_t capacity_;
    bool is_close_;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t max_capacity): capacity_(max_capacity){
    assert(max_capacity>0);
    is_close_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();
}

template<class T>
void BlockDeque<T>::Close() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
    is_close_ = true;
    cond_producer_.notify_all();
    cond_consumer_.notify_all();
}

template<class T>
void BlockDeque<T>::Flush() {
    cond_consumer_.notify_one();
}

template<class T>
void BlockDeque<T>::Clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template<class T>
T BlockDeque<T>::Front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<class T>
T BlockDeque<T>::Back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}


template<class T>
size_t BlockDeque<T>::Size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template<class T>
size_t BlockDeque<T>::Capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<class T>
void BlockDeque<T>::PushFront(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_){
        cond_producer_.wait(locker);
    }
    deq_.push_front(item);
    cond_consumer_.notify_one();
}


template<class T>
void BlockDeque<T>::PushBack(const T &item){
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_){
        cond_producer_.wait(locker);
    }
    deq_.push_back(item);
    cond_consumer_.notify_one();
}


template<class T>
bool BlockDeque<T>::Full() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template<class T>
bool BlockDeque<T>::Empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<class T>
bool BlockDeque<T>::Pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        cond_consumer_.wait(locker);
        if(is_close_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer_.notify_one();
    return true;

}

template<class T>
bool BlockDeque<T>::Pop(T &item, int time_out) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        if(cond_consumer_.wait_for(locker,std::chrono::seconds(time_out)) == std::cv_status::timeout) return false;
        if(is_close_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer_.notify_one();
    return true;
}


#endif