#include <cstddef>
#include <assert.h>
#include <iostream>
#include <cstring> //perror
#include <strings.h>
#include <sys/uio.h> //readv
#include <unistd.h> // write
#include "../log/log.h"
#include <errno.h>

void Buffer::Retrieve(size_t len){
    assert(len <= ReadableBytes());
    read_pos_ += len;
}

void Buffer::RetrieveUntil(const char* end){
    assert(Peek() <= end);
    Retrieve(end-Peek());
}

void Buffer::RetrieveAll(){
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

void Buffer::Append(const std::string& str){
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len){
    //assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const char* str, size_t len){
    //assert(str);
    EnsureWritable(len);
    std::copy(str, str+len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const Buffer& buff){
    Append(buff.Peek(), buff.ReadableBytes());
}

void Buffer::EnsureWritable(size_t len){
    if(WritableBytes() <len){
        // Creating new spaces when usable<len to accommendate data in the the buff
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

// The data from the TCP buffer will firstly be put into buffer_ and when the buffer_ is full, the remaining
// will be put into buff
ssize_t Buffer::ReadFd(int fd, int* save_errno){
    char buff[65535]; // about 65k
    struct iovec iov[2]; 
    const size_t writable = WritableBytes();
    iov[0].iov_base = BeginPtr_() + write_pos_; // iov[0].iov_base is the reading position
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    const ssize_t len = readv(fd, iov, 2); 
    if(len<0){
        *save_errno = errno;
    }else if(static_cast<size_t>(len) <= writable){
        write_pos_ += len;
    }else{
        write_pos_ = buffer_.size();
        Append(buff, len-writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* save_errno){
   size_t read_size = ReadableBytes();
   ssize_t len = write(fd, Peek(), read_size);
   if(len<0){
       *save_errno = errno;
       return len;
   }
   read_pos_ += len;
   return len;
}

void Buffer::MakeSpace_(size_t len){
    // AppendableBytes: reusable spaces in the buffer_ array
    if(WritableBytes() + AppendableBytes() < len){
        buffer_.resize(write_pos_ + len + 1);
    }else{
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_()+read_pos_, BeginPtr_()+write_pos_, BeginPtr_());
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable;
        assert(readable == ReadableBytes());
    }
}