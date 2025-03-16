#ifndef WEBSERVER_BUFFER_H
#define WEBSERVER_BUFFER_H

#include <vector>
#include <string>
#include <algorithm>
#include <sys/uio.h>
#include <unistd.h>

class Buffer {
public:
    static const size_t kInitialSize = 1024;
    
    explicit Buffer(size_t initial_size = kInitialSize)
        : buffer_(initial_size), read_pos_(0), write_pos_(0) {}
    
    size_t readable_bytes() const { return write_pos_ - read_pos_; }
    size_t writable_bytes() const { return buffer_.size() - write_pos_; }
    size_t prependable_bytes() const { return read_pos_; }
    
    const char* peek() const { return begin() + read_pos_; }
    
    void retrieve(size_t len) {
        if(len < readable_bytes()) {
            read_pos_ += len;
        }
        else {
            retrieve_all();
        }
    }
    
    void retrieve_all() {
        read_pos_ = 0;
        write_pos_ = 0;
    }
    
    std::string retrieve_as_string(size_t len) {
        std::string str(peek(), len);
        retrieve(len);
        return str;
    }
    
    std::string retrieve_all_as_string() {
        return retrieve_as_string(readable_bytes());
    }
    
    void append(const char* data, size_t len) {
        ensure_writable_bytes(len);
        std::copy(data, data + len, begin_write());
        has_written(len);
    }
    
    void append(const std::string& str) {
        append(str.data(), str.length());
    }
    
    ssize_t read_fd(int fd, int* saved_errno);
    ssize_t write_fd(int fd, int* saved_errno);
    
private:
    char* begin() { return &*buffer_.begin(); }
    const char* begin() const { return &*buffer_.begin(); }
    
    void make_space(size_t len) {
        if(writable_bytes() + prependable_bytes() < len) {
            buffer_.resize(write_pos_ + len);
        }
        else {
            size_t readable = readable_bytes();
            std::copy(begin() + read_pos_, begin() + write_pos_, begin());
            read_pos_ = 0;
            write_pos_ = read_pos_ + readable;
        }
    }
    
    char* begin_write() { return begin() + write_pos_; }
    const char* begin_write() const { return begin() + write_pos_; }
    
    void has_written(size_t len) { write_pos_ += len; }
    
    void ensure_writable_bytes(size_t len) {
        if(writable_bytes() < len) {
            make_space(len);
        }
    }
    
private:
    std::vector<char> buffer_;
    size_t read_pos_;
    size_t write_pos_;
};

#endif // WEBSERVER_BUFFER_H 