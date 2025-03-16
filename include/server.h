#ifndef WEBSERVER_SERVER_H
#define WEBSERVER_SERVER_H

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <functional>
#include "threadpool.h"
#include "http_conn.h"
#include "timer.h"

class WebServer {
public:
    WebServer(int port = 8080, int thread_num = 8);
    ~WebServer();
    
    void start();

private:
    bool init_socket();
    void add_client(int fd, struct sockaddr_in addr);
    void deal_listen();
    void deal_read(int fd);
    void deal_write(int fd);
    void on_read(int fd);
    void on_write(int fd);
    void on_process(int fd);
    void close_conn(HttpConn* client);
    void set_nonblocking(int fd);

private:
    int port_;
    int listen_fd_;
    int epoll_fd_;
    bool is_close_;
    
    uint32_t listen_event_;
    uint32_t conn_event_;
    
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<Timer> timer_;
    std::unique_ptr<HttpConn[]> users_;
    
    static const int MAX_FD = 65536;
    static const int MAX_EVENT_NUMBER = 10000;
    epoll_event events_[MAX_EVENT_NUMBER];
};

#endif // WEBSERVER_SERVER_H 