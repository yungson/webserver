#include "http_connection.h"
#include "../log/log.h"
#include <sys/uio.h>

const char* HttpConnection::resource_root;
bool HttpConnection::is_et_mode;
std::atomic<int> HttpConnection::user_count;

HttpConnection::HttpConnection(){
    // sock_fd_ = -1;
    // addr_ = {0};
    // is_closed_ = true;
}

void HttpConnection::Init(int sock_fd, const sockaddr_in& addr){
    // assert(sock_fd>0);
    user_count++;
    addr_ = addr;
    sock_fd_ = sock_fd;
    write_buffer_.RetrieveAll(); 
    read_buffer_.RetrieveAll();
    is_closed_ = false;
    LOG_INFO("Client[%d](%s:%d) Added, userCount:%d", sock_fd_, GetIp(), GetPort(), (int)user_count);
}


void HttpConnection::Close(){
    response_.UnMapFile();
    if(is_closed_ == false){
        is_closed_ = true;
        user_count--;
        close(sock_fd_); 
    }
}

int HttpConnection::GetSockFd() const {
    return sock_fd_;
}

struct sockaddr_in HttpConnection::GetAddr() const{
    return addr_;
} 

const char* HttpConnection::GetIp() const{
    return inet_ntoa(addr_.sin_addr);
}

int HttpConnection::GetPort() const{
    return addr_.sin_port;
}


ssize_t HttpConnection::Read(int* save_error){
    ssize_t len = -1;
    do{
        len = read_buffer_.ReadFd(sock_fd_, save_error); 
        if(len<=0) {
            break; 
        }
    }while(is_et_mode); // If ET mode, read all data in this run; or will just read one times.
    return len;
}

ssize_t HttpConnection::Write(int* save_error){
    ssize_t len = -1;
    do {
        len = writev(sock_fd_, iov_, iov_cnt_);
        if(len <= 0) {
            *save_error = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { // write completed
            break;
        }else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                write_buffer_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            write_buffer_.Retrieve(len);
        }
    } while(is_et_mode || ToWriteBytes() > 10240);
    return len;
}

bool HttpConnection::ProcessConnection(){
    request_.Init();
    if(read_buffer_.ReadableBytes()<=0) {
        return false;
    }else if(request_.parse(read_buffer_)){
        response_.Init(resource_root, request_.Path(), request_.IsKeepAlive(), 200);
    }else{
        response_.Init(resource_root, request_.Path(), false, 400);
    }
    response_.MakeResponse(write_buffer_);
    // Response Headers
    iov_[0].iov_base = const_cast<char*>(write_buffer_.Peek()); 
    iov_[0].iov_len = write_buffer_.ReadableBytes();
    iov_cnt_ = 1;
    // Response File
    if(response_.FileLen()>0 && response_.MappedFile()){
        iov_[1].iov_base = response_.MappedFile();
        iov_[1].iov_len = response_.FileLen();
        iov_cnt_ = 2;
    }
    return true;
}


HttpConnection::~HttpConnection(){
    Close();
}

int HttpConnection::IsKeepAlive() {
    return request_.IsKeepAlive();
}

int HttpConnection::ToWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}
