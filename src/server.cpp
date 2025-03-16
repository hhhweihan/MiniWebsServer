#include "server.h"
#include "log.h"
#include <functional>
#include <string.h>

WebServer::WebServer(int port, int thread_num)
    : port_(port),
      is_close_(false),
      thread_pool_(new ThreadPool(thread_num)),
      timer_(new Timer()),
      users_(new HttpConn[MAX_FD]) {
    
    // 初始化日志
    Log::instance()->init("./log/WebServer.log", 0, 2000, 800000, 800);
    
    // 初始化事件模式
    listen_event_ = EPOLLRDHUP;
    conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
    
    if(!init_socket()) {
        is_close_ = true;
    }
}

WebServer::~WebServer() {
    close(listen_fd_);
    is_close_ = true;
}

void WebServer::start() {
    if(!is_close_) {
        LOG_INFO("========== Server start ==========");
        LOG_INFO("Port:%d", port_);
    }
    
    while(!is_close_) {
        int event_count = epoll_wait(epoll_fd_, events_, MAX_EVENT_NUMBER, -1);
        if(event_count < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll failure");
            break;
        }
        
        for(int i = 0; i < event_count; i++) {
            int sockfd = events_[i].data.fd;
            
            if(sockfd == listen_fd_) {
                deal_listen();
            }
            else if(events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(sockfd >= 0 && sockfd < MAX_FD);
                close_conn(&users_[sockfd]);
            }
            else if(events_[i].events & EPOLLIN) {
                assert(sockfd >= 0 && sockfd < MAX_FD);
                deal_read(sockfd);
            }
            else if(events_[i].events & EPOLLOUT) {
                assert(sockfd >= 0 && sockfd < MAX_FD);
                deal_write(sockfd);
            }
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

bool WebServer::init_socket() {
    int ret;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd_ < 0) {
        LOG_ERROR("Create socket error");
        return false;
    }
    
    ret = bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind error");
        close(listen_fd_);
        return false;
    }
    
    ret = listen(listen_fd_, 5);
    if(ret < 0) {
        LOG_ERROR("Listen error");
        close(listen_fd_);
        return false;
    }
    
    ret = epoll_create(5);
    if(ret < 0) {
        LOG_ERROR("Create epoll error");
        close(listen_fd_);
        return false;
    }
    epoll_fd_ = ret;
    
    epoll_event event;
    event.data.fd = listen_fd_;
    event.events = EPOLLIN | listen_event_;
    ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &event);
    if(ret < 0) {
        LOG_ERROR("Add listen error");
        close(listen_fd_);
        return false;
    }
    
    set_nonblocking(listen_fd_);
    LOG_INFO("Server init success");
    return true;
}

void WebServer::add_client(int fd, struct sockaddr_in addr) {
    assert(fd >= 0 && fd < MAX_FD);
    users_[fd].init(fd, addr);
    timer_->add_timer(fd, 60000, std::bind(&WebServer::close_conn, this, &users_[fd]));
    
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | conn_event_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
    set_nonblocking(fd);
    LOG_INFO("Client[%d] in!", fd);
}

void WebServer::deal_listen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    
    do {
        int fd = accept(listen_fd_, (struct sockaddr*)&addr, &len);
        if(fd <= 0) {
            return;
        }
        else if(HttpConn::user_count >= MAX_FD) {
            LOG_WARN("Clients is full!");
            close(fd);
            return;
        }
        add_client(fd, addr);
    } while(listen_event_ & EPOLLET);
}

void WebServer::deal_read(int sockfd) {
    assert(sockfd >= 0 && sockfd < MAX_FD);
    thread_pool_->add_task(std::bind(&WebServer::on_read, this, sockfd));
}

void WebServer::deal_write(int sockfd) {
    assert(sockfd >= 0 && sockfd < MAX_FD);
    thread_pool_->add_task(std::bind(&WebServer::on_write, this, sockfd));
}

void WebServer::on_read(int sockfd) {
    assert(sockfd >= 0 && sockfd < MAX_FD);
    HttpConn* client = &users_[sockfd];
    if(client->read()) {
        thread_pool_->add_task(std::bind(&WebServer::on_process, this, sockfd));
    }
    else {
        close_conn(client);
    }
}

void WebServer::on_write(int sockfd) {
    assert(sockfd >= 0 && sockfd < MAX_FD);
    HttpConn* client = &users_[sockfd];
    if(!client->write()) {
        close_conn(client);
    }
}

void WebServer::on_process(int sockfd) {
    assert(sockfd >= 0 && sockfd < MAX_FD);
    HttpConn* client = &users_[sockfd];
    if(client->process()) {
        // 处理完成，注册写事件
        epoll_event ev;
        ev.data.fd = sockfd;
        ev.events = EPOLLOUT | conn_event_;
        epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, sockfd, &ev);
    }
    else {
        close_conn(client);
    }
}

void WebServer::close_conn(HttpConn* client) {
    assert(client);
    timer_->del_timer(client->get_fd());
    client->close_conn();
}

void WebServer::set_nonblocking(int fd) {
    int flag = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}