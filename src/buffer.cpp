#include "buffer.h"
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

ssize_t Buffer::read_fd(int fd, int* saved_errno) {
    char extra_buf[65536];
    struct iovec vec[2];
    const size_t writable = writable_bytes();
    
    vec[0].iov_base = begin() + write_pos_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extra_buf;
    vec[1].iov_len = sizeof(extra_buf);
    
    const ssize_t n = readv(fd, vec, 2);
    if(n < 0) {
        *saved_errno = errno;
    }
    else if(static_cast<size_t>(n) <= writable) {
        write_pos_ += n;
    }
    else {
        write_pos_ = buffer_.size();
        append(extra_buf, n - writable);
    }
    return n;
}

ssize_t Buffer::write_fd(int fd, int* saved_errno) {
    size_t readable = readable_bytes();
    ssize_t n = write(fd, peek(), readable);
    if(n < 0) {
        *saved_errno = errno;
    }
    else {
        retrieve(n);
    }
    return n;
} 