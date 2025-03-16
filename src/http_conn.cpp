#include "http_conn.h"
#include "log.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

std::atomic<int> HttpConn::user_count(0);
const char* doc_root = "./resources";

HttpConn::HttpConn() {
    sockfd_ = -1;
    memset(&addr_, 0, sizeof(addr_));
    is_close_ = true;
    file_address_ = nullptr;
    iv_count_ = 0;
}

HttpConn::~HttpConn() {
    close_conn();
    unmap();
}

void HttpConn::init(int sockfd, const sockaddr_in& addr) {
    sockfd_ = sockfd;
    addr_ = addr;
    
    // 端口复用
    int reuse = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // 初始化其他信息
    read_buffer_.retrieve_all();
    write_buffer_.retrieve_all();
    is_close_ = false;
    file_address_ = nullptr;
    iv_count_ = 0;
    
    // 初始化HTTP相关信息
    check_state_ = CHECK_STATE_REQUESTLINE;
    method_ = GET;
    url_ = "";
    version_ = "";
    host_ = "";
    content_length_ = 0;
    linger_ = false;
    
    LOG_INFO("Client[%d] connected", sockfd_);
    ++user_count;
}

void HttpConn::close_conn() {
    if(!is_close_) {
        is_close_ = true;
        --user_count;
        close(sockfd_);
        LOG_INFO("Client[%d] quit, UserCount:%d", sockfd_, (int)user_count);
    }
}

bool HttpConn::read() {
    int saved_errno = 0;
    ssize_t len = -1;
    do {
        len = read_buffer_.read_fd(sockfd_, &saved_errno);
        if(len <= 0) {
            break;
        }
    } while(true);
    
    return len > 0;
}

bool HttpConn::write() {
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = write_buffer_.readable_bytes();
    
    if(bytes_to_send == 0) {
        // 将要发送的字节为0，这一次响应结束
        return true;
    }
    
    while(1) {
        temp = writev(sockfd_, iv_, iv_count_);
        if(temp <= -1) {
            // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return true;
            }
            unmap();
            return false;
        }
        
        bytes_have_send += temp;
        bytes_to_send -= temp;
        
        if(bytes_have_send >= iv_[0].iov_len) {
            iv_[0].iov_len = 0;
            iv_[1].iov_base = file_address_ + (bytes_have_send - write_buffer_.readable_bytes());
            iv_[1].iov_len = bytes_to_send;
        }
        else {
            iv_[0].iov_base = (uint8_t*)iv_[0].iov_base + temp;
            iv_[0].iov_len -= temp;
        }
        
        if(bytes_to_send <= 0) {
            // 没有数据要发送了
            unmap();
            return true;
        }
    }
}

bool HttpConn::process() {
    HttpCode ret = parse_request();
    if(ret == NO_REQUEST) {
        return false;
    }
    
    ret = do_request();
    if(ret == FILE_REQUEST) {
        add_status_line(200, "OK");
        add_headers(file_stat_.st_size);
        
        iv_[0].iov_base = const_cast<char*>(write_buffer_.peek());
        iv_[0].iov_len = write_buffer_.readable_bytes();
        iv_[1].iov_base = file_address_;
        iv_[1].iov_len = file_stat_.st_size;
        iv_count_ = 2;
        
        return true;
    }
    else {
        const char* error_info;
        switch(ret) {
            case BAD_REQUEST:
                error_info = "Bad Request";
                break;
            case NO_RESOURCE:
                error_info = "No Resource";
                break;
            case FORBIDDEN_REQUEST:
                error_info = "Forbidden";
                break;
            case INTERNAL_ERROR:
                error_info = "Internal Error";
                break;
            default:
                error_info = "Unknown Error";
                break;
        }
        add_status_line(400, error_info);
        add_headers(0);
        
        iv_[0].iov_base = const_cast<char*>(write_buffer_.peek());
        iv_[0].iov_len = write_buffer_.readable_bytes();
        iv_count_ = 1;
    }
    return true;
}

const char* HttpConn::get_ip() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::get_port() const {
    return ntohs(addr_.sin_port);
}

HttpConn::HttpCode HttpConn::parse_request() {
    bool has_more = true;
    bool ok = true;
    HttpCode ret = NO_REQUEST;
    
    while(has_more) {
        switch(check_state_) {
            case CHECK_STATE_REQUESTLINE: {
                const char* crlf = strstr(read_buffer_.peek(), "\r\n");
                if(crlf == nullptr) {
                    return NO_REQUEST;
                }
                ret = parse_request_line(std::string(read_buffer_.peek(), crlf));
                if(ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                read_buffer_.retrieve(crlf + 2 - read_buffer_.peek());
                break;
            }
            case CHECK_STATE_HEADER: {
                const char* crlf = strstr(read_buffer_.peek(), "\r\n");
                if(crlf == nullptr) {
                    return NO_REQUEST;
                }
                if(read_buffer_.peek() == crlf) {
                    read_buffer_.retrieve(2);
                    if(content_length_ > 0) {
                        check_state_ = CHECK_STATE_CONTENT;
                    }
                    else {
                        return GET_REQUEST;
                    }
                    break;
                }
                ret = parse_headers(std::string(read_buffer_.peek(), crlf));
                if(ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                read_buffer_.retrieve(crlf + 2 - read_buffer_.peek());
                break;
            }
            case CHECK_STATE_CONTENT: {
                if(read_buffer_.readable_bytes() < static_cast<size_t>(content_length_)) {
                    return NO_REQUEST;
                }
                ret = parse_content(std::string(read_buffer_.peek(), content_length_));
                if(ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                read_buffer_.retrieve(content_length_);
                return GET_REQUEST;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::parse_request_line(const std::string& text) {
    // GET /index.html HTTP/1.1
    size_t pos = text.find(' ');
    if(pos == std::string::npos) {
        return BAD_REQUEST;
    }
    
    // 解析请求方法
    std::string method = text.substr(0, pos);
    if(method == "GET") {
        method_ = GET;
    }
    else if(method == "POST") {
        method_ = POST;
    }
    else {
        return BAD_REQUEST;
    }
    
    // 跳过空格
    pos = text.find_first_not_of(' ', pos);
    if(pos == std::string::npos) {
        return BAD_REQUEST;
    }
    
    // 解析URL
    size_t end = text.find(' ', pos);
    if(end == std::string::npos) {
        return BAD_REQUEST;
    }
    url_ = text.substr(pos, end - pos);
    
    // 解析HTTP版本
    pos = text.find_last_not_of(' ');
    if(pos == std::string::npos) {
        return BAD_REQUEST;
    }
    version_ = text.substr(end + 1, pos - end);
    
    check_state_ = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::parse_headers(const std::string& text) {
    if(text.empty()) {
        if(content_length_ > 0) {
            check_state_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    
    if(strncasecmp(text.c_str(), "Host:", 5) == 0) {
        host_ = text.substr(5);
        host_ = host_.substr(host_.find_first_not_of(' '));
    }
    else if(strncasecmp(text.c_str(), "Connection:", 11) == 0) {
        std::string connection = text.substr(11);
        connection = connection.substr(connection.find_first_not_of(' '));
        linger_ = (strcasecmp(connection.c_str(), "keep-alive") == 0);
    }
    else if(strncasecmp(text.c_str(), "Content-Length:", 15) == 0) {
        std::string length = text.substr(15);
        length = length.substr(length.find_first_not_of(' '));
        content_length_ = std::stoi(length);
    }
    
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::parse_content(const std::string& text) {
    if(read_buffer_.readable_bytes() >= static_cast<size_t>(content_length_)) {
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::do_request() {
    // 将网站根目录和url拼接
    std::string path = doc_root + url_;
    if(url_ == "/") {
        path += "index.html";
    }
    
    // 获取文件信息
    if(stat(path.c_str(), &file_stat_) < 0) {
        return NO_RESOURCE;
    }
    
    // 判断访问权限
    if(!(file_stat_.st_mode & S_IROTH)) {
        return FORBIDDEN_REQUEST;
    }
    
    // 判断是否是目录
    if(S_ISDIR(file_stat_.st_mode)) {
        return BAD_REQUEST;
    }
    
    // 以只读方式打开文件
    int fd = open(path.c_str(), O_RDONLY);
    if(fd < 0) {
        return NO_RESOURCE;
    }
    
    // 创建内存映射
    file_address_ = (char*)mmap(0, file_stat_.st_size, PROT_READ,
                               MAP_PRIVATE, fd, 0);
    close(fd);
    if(file_address_ == MAP_FAILED) {
        return INTERNAL_ERROR;
    }
    
    return FILE_REQUEST;
}

void HttpConn::unmap() {
    if(file_address_) {
        munmap(file_address_, file_stat_.st_size);
        file_address_ = nullptr;
    }
}

bool HttpConn::add_response(const char* format, ...) {
    va_list arg_list;
    va_start(arg_list, format);
    char buf[1024];
    int len = vsnprintf(buf, sizeof(buf) - 1, format, arg_list);
    if(len >= 0) {
        write_buffer_.append(buf, len);
        return true;
    }
    va_end(arg_list);
    return false;
}

bool HttpConn::add_status_line(int status, const char* title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpConn::add_headers(int content_len) {
    bool ret = add_response("Content-Length: %d\r\n", content_len);
    ret = ret && add_response("Connection: %s\r\n", linger_ ? "keep-alive" : "close");
    ret = ret && add_response("Content-Type: %s\r\n", "text/html");
    ret = ret && add_response("\r\n");
    return ret;
}

bool HttpConn::add_content(const std::string& content) {
    return add_response("%s", content.c_str());
}

bool HttpConn::add_content_type() {
    std::string suffix;
    size_t pos = url_.find_last_of('.');
    if(pos != std::string::npos) {
        suffix = url_.substr(pos);
    }
    
    if(suffix == ".html" || suffix == ".htm") {
        return add_response("Content-Type: text/html\r\n");
    }
    else if(suffix == ".jpg" || suffix == ".jpeg") {
        return add_response("Content-Type: image/jpeg\r\n");
    }
    else if(suffix == ".png") {
        return add_response("Content-Type: image/png\r\n");
    }
    else if(suffix == ".css") {
        return add_response("Content-Type: text/css\r\n");
    }
    else if(suffix == ".js") {
        return add_response("Content-Type: application/javascript\r\n");
    }
    
    return add_response("Content-Type: text/plain\r\n");
} 