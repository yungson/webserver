#include "heaptimer.h"


void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    time_node_2id_[heap_[i].id] = i;
    time_node_2id_[heap_[j].id] = j;
}


void HeapTimer::ShiftUp_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i-1)/2;
    while(j>=0){
        if(heap_[j]<heap_[i]){break;}
        SwapNode_(i, j);
        i = j;
        j = (i-1)/2;
    }
}

bool HeapTimer::ShiftDown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size()); 
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i*2+1;
    while(j<n){
        if(j+1<n && heap_[j+1]<heap_[j]) j++;
        if(heap_[i]<heap_[j]) break;
        SwapNode_(i,j);
        i = j;
        j = i*2+1;
    }
    return i>index;
}

void HeapTimer::Add(int id, int time_out, const TimeOutCallBack &cb) {
    size_t i;
    if(time_node_2id_.count(id)==0){
        // new node
        i = heap_.size();
        time_node_2id_[id] = i;
        heap_.push_back({id, Clock::now()+Ms(time_out), cb}); 
        ShiftUp_(i);
    }else{
        // existing nodes
        i = time_node_2id_[id];
        heap_[i].expire = Clock::now()+Ms(time_out);
        heap_[i].cb = cb;
        if(!ShiftDown_(i, heap_.size())){
            ShiftUp_(i);
        }
    }
}

void HeapTimer::DoWork(int id) {
    if(heap_.empty() || time_node_2id_.count(id)==0){ 
        return;
    }
    size_t i = time_node_2id_[id];
    TimerNode node = heap_[i];
    node.cb();
    Del_(i);
}

void HeapTimer::Del_(size_t index) {
    size_t i = index;
    size_t n = heap_.size() -1;
    if(i<n){
        SwapNode_(i,n); 
        if(!ShiftDown_(i,n)){
            ShiftUp_(i);
        }
    }
    time_node_2id_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::Adjust(int id, int time_out) {
    assert(!heap_.empty() && time_node_2id_.count(id) > 0);
    heap_[time_node_2id_[id]].expire = Clock::now()+Ms(time_out); 
    ShiftDown_(time_node_2id_[id], heap_.size());
}

void HeapTimer::tick(){
    while(!heap_.empty()){
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<Ms>(node.expire-Clock::now()).count()>0) break;
        node.cb();
        Pop();
    }
}

void HeapTimer::Pop() {
    Del_(0);
}

void HeapTimer::Clear() {
    heap_.clear();
    time_node_2id_.clear();
}

int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()){
        res = std::chrono::duration_cast<Ms>(heap_.front().expire-Clock::now()).count();
        if(res < 0) res = 0;
    }
    return res;
}