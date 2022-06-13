#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <arpa/inet.h> //sockaddr_in
#include <sys/types.h>
#include <atomic>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "http_request.h"
#include "http_response.h"
#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpConnection {

public:
    HttpConnection();
    ~HttpConnection();

    void Init(int sock_fd, const sockaddr_in& addr);
    ssize_t Read(int* save_error);
    ssize_t Write(int* save_error);
    void Close();
    bool ProcessConnection();
    static const char* resource_root;
    static bool is_et_mode;
    static std::atomic<int> user_count;

    int GetSockFd() const;
    int GetPort() const;
    const char* GetIp() const;
    sockaddr_in GetAddr() const;
    int IsKeepAlive();
    int ToWriteBytes();

private:

    int sock_fd_;
    struct sockaddr_in addr_;
    bool is_closed_;

    int iov_cnt_;
    struct iovec iov_[2];

    Buffer read_buffer_;
    Buffer write_buffer_;

    HttpRequest request_;
    HttpResponse response_;
};

#endif