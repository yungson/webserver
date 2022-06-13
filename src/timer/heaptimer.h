#ifndef HEAPTIMER_H
#define HEAPTIMER_H


#include <chrono>
#include <vector>
#include <unordered_map>
#include <functional>
#include <assert.h>

typedef std::function<void()> TimeOutCallBack; 
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds Ms;
typedef Clock::time_point TimeStamp;


struct TimerNode{
    int id;
    TimeStamp expire;
    TimeOutCallBack cb;
    bool operator<(const TimerNode& t){
        return expire < t.expire;
    }
};

class HeapTimer {

public:
    HeapTimer(){heap_.reserve(64);}
    ~HeapTimer(){Clear();}
    void Adjust(int id, int new_expires);
    void Add(int id, int time_out, const TimeOutCallBack& cb);
    void DoWork(int id);
    void Clear();
    void tick();
    void Pop();
    int GetNextTick();

private:
    void Del_(size_t i);
    void ShiftUp_(size_t i);
    bool ShiftDown_(size_t index, size_t n);
    void SwapNode_(size_t i, size_t j);
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> time_node_2id_;
};

#endif