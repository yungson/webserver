#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <atomic>
#include <vector>
#include <string>
#include <sys/types.h>

class Buffer {

public:
    Buffer(int buffer_size=1024) {
        buffer_.reserve(buffer_size);
        read_pos_ = 0;
        write_pos_ = 0;
    }
    ~Buffer() = default;

    size_t WritableBytes() const { return buffer_.size() - write_pos_; }  
    size_t ReadableBytes() const { return write_pos_ - read_pos_; }
    size_t AppendableBytes() const { return read_pos_; }

    void EnsureWritable(size_t len);
    void HasWritten(size_t len) { write_pos_ += len; }

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);

    void RetrieveAll();

    const char* Peek() const { return BeginPtr_()+read_pos_;}
    const char* BeginWriteConst() const { return BeginPtr_()+write_pos_;}
    char* BeginWrite() { return BeginPtr_()+write_pos_; }
    
    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* save_errno);
    ssize_t WriteFd(int fd, int* save_errno);

    std::string RetrieveAllToStr();

private:
    char* BeginPtr_() { return &*buffer_.begin();} 
    const char* BeginPtr_() const { return &*buffer_.begin(); }
    void MakeSpace_(size_t len); // make more space to auto-increase buffer size
    std::vector<char> buffer_; // the array to store the data
    std::atomic<std::size_t> read_pos_; // reading position
    std::atomic<std::size_t> write_pos_; // writing position
};

#endif