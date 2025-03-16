#ifndef WEBSERVER_HTTP_CONN_H
#define WEBSERVER_HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <atomic>
#include <stdarg.h>
#include "buffer.h"

class HttpConn {
public:
    enum HttpCode {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    
    enum Method {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT
    };
    
    enum CheckState {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

public:
    HttpConn();
    ~HttpConn();

    void init(int sockfd, const sockaddr_in& addr);
    void close_conn();
    bool read();
    bool write();
    bool process();
    
    // 获取客户端地址信息
    const char* get_ip() const;
    int get_port() const;
    int get_fd() const { return sockfd_; }
    bool is_keep_alive() const { return linger_; }

    static std::atomic<int> user_count;

private:
    HttpCode parse_request();
    HttpCode parse_request_line(const std::string& text);
    HttpCode parse_headers(const std::string& text);
    HttpCode parse_content(const std::string& text);
    HttpCode do_request();
    
    void unmap();
    bool add_response(const char* format, ...);
    bool add_content(const std::string& content);
    bool add_content_type();
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    
private:
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    
    int sockfd_;
    sockaddr_in addr_;
    bool is_close_;
    
    Buffer read_buffer_;
    Buffer write_buffer_;
    
    CheckState check_state_;
    Method method_;
    
    // 解析请求相关的变量
    std::string url_;
    std::string version_;
    std::string host_;
    bool linger_;
    int content_length_;
    
    // 响应相关的变量
    char* file_address_;
    struct stat file_stat_;
    struct iovec iv_[2];
    int iv_count_;
};

#endif // WEBSERVER_HTTP_CONN_H 